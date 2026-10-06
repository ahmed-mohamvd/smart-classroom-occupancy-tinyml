#include "occupancy_features.h"

#include <stddef.h>
#include <string.h>

/* Values exported with the trained PIR-v2 model. */
static const float feature_mean[OCCUPANCY_FEATURE_COUNT] = {
    0.1832574606f, 0.1527061164f, 0.2979767323f,
    0.0379868522f, 3.1775417328f, 2.2109255791f,
};

static const float feature_std[OCCUPANCY_FEATURE_COUNT] = {
    0.1592362225f, 0.1534320861f, 0.2113679200f,
    0.0624778681f, 2.7039642334f, 2.1993520259f,
};

/* INT8 tensor quantization from occupancy_model_int8.tflite. */
static const float input_scale = 0.0139727779f;
static const int32_t input_zero_point = -27;
static const float output_scale = 0.00390625f;
static const int32_t output_zero_point = -128;

static int8_t clamp_int8(int32_t value)
{
  if (value > 127) value = 127;
  else if (value < -128) value = -128;
  return (int8_t)value;
}

static int32_t round_to_int32(float value)
{
  return (value >= 0.0f) ? (int32_t)(value + 0.5f)
                         : (int32_t)(value - 0.5f);
}

void OccupancyFeatures_Init(OccupancyFeatureBuffer *buffer)
{
  if (buffer != NULL) memset(buffer, 0, sizeof(*buffer));
}

bool OccupancyFeatures_AddSample(OccupancyFeatureBuffer *buffer,
                                 uint8_t pir1,
                                 uint8_t pir2)
{
  if (buffer == NULL) return false;

  buffer->pir1[buffer->write_index] = (pir1 != 0U) ? 1U : 0U;
  buffer->pir2[buffer->write_index] = (pir2 != 0U) ? 1U : 0U;
  buffer->write_index =
      (uint8_t)((buffer->write_index + 1U) % OCCUPANCY_WINDOW_SAMPLES);

  if (buffer->sample_count < OCCUPANCY_WINDOW_SAMPLES)
  {
    buffer->sample_count++;
    if (buffer->sample_count == OCCUPANCY_WINDOW_SAMPLES)
    {
      buffer->samples_since_inference = 0U;
      return true;
    }
    return false;
  }

  buffer->samples_since_inference++;
  if (buffer->samples_since_inference >= OCCUPANCY_STEP_SAMPLES)
  {
    buffer->samples_since_inference = 0U;
    return true;
  }
  return false;
}

void OccupancyFeatures_Compute(const OccupancyFeatureBuffer *buffer,
                               float features[OCCUPANCY_FEATURE_COUNT])
{
  uint32_t pir1_count = 0U;
  uint32_t pir2_count = 0U;
  uint32_t pir_any_count = 0U;
  uint32_t pir_both_count = 0U;
  uint32_t pir1_rising_edges = 0U;
  uint32_t pir2_rising_edges = 0U;
  uint8_t previous_pir1 = 0U;
  uint8_t previous_pir2 = 0U;
  uint32_t sample;

  if ((buffer == NULL) || (features == NULL) ||
      (buffer->sample_count < OCCUPANCY_WINDOW_SAMPLES)) return;

  for (sample = 0U; sample < OCCUPANCY_WINDOW_SAMPLES; sample++)
  {
    uint32_t index =
        ((uint32_t)buffer->write_index + sample) % OCCUPANCY_WINDOW_SAMPLES;
    uint8_t pir1 = buffer->pir1[index];
    uint8_t pir2 = buffer->pir2[index];

    pir1_count += pir1;
    pir2_count += pir2;
    pir_any_count += ((pir1 != 0U) || (pir2 != 0U)) ? 1U : 0U;
    pir_both_count += ((pir1 != 0U) && (pir2 != 0U)) ? 1U : 0U;

    if (sample > 0U)
    {
      if ((previous_pir1 == 0U) && (pir1 != 0U)) pir1_rising_edges++;
      if ((previous_pir2 == 0U) && (pir2 != 0U)) pir2_rising_edges++;
    }
    previous_pir1 = pir1;
    previous_pir2 = pir2;
  }

  features[0] = (float)pir1_count / (float)OCCUPANCY_WINDOW_SAMPLES;
  features[1] = (float)pir2_count / (float)OCCUPANCY_WINDOW_SAMPLES;
  features[2] = (float)pir_any_count / (float)OCCUPANCY_WINDOW_SAMPLES;
  features[3] = (float)pir_both_count / (float)OCCUPANCY_WINDOW_SAMPLES;
  features[4] = (float)pir1_rising_edges;
  features[5] = (float)pir2_rising_edges;
}

void OccupancyFeatures_Normalize(
    const float features[OCCUPANCY_FEATURE_COUNT],
    float normalized[OCCUPANCY_FEATURE_COUNT])
{
  uint32_t index;
  if ((features == NULL) || (normalized == NULL)) return;
  for (index = 0U; index < OCCUPANCY_FEATURE_COUNT; index++)
  {
    normalized[index] =
        (features[index] - feature_mean[index]) / feature_std[index];
  }
}

void OccupancyFeatures_QuantizeInt8(
    const float normalized[OCCUPANCY_FEATURE_COUNT],
    int8_t quantized[OCCUPANCY_FEATURE_COUNT])
{
  uint32_t index;
  if ((normalized == NULL) || (quantized == NULL)) return;
  for (index = 0U; index < OCCUPANCY_FEATURE_COUNT; index++)
  {
    int32_t value =
        round_to_int32(normalized[index] / input_scale) + input_zero_point;
    quantized[index] = clamp_int8(value);
  }
}

OccupancyClass Occupancy_SelectClassInt8(
    const int8_t output[OCCUPANCY_CLASS_COUNT])
{
  uint32_t best_index = 0U;
  uint32_t index;
  if (output == NULL) return OCCUPANCY_EMPTY;
  for (index = 1U; index < OCCUPANCY_CLASS_COUNT; index++)
  {
    if (output[index] > output[best_index]) best_index = index;
  }
  return (OccupancyClass)best_index;
}

float Occupancy_OutputProbability(int8_t quantized_output)
{
  return ((float)((int32_t)quantized_output - output_zero_point)) * output_scale;
}

const char *Occupancy_ClassName(OccupancyClass occupancy_class)
{
  switch (occupancy_class)
  {
    case OCCUPANCY_LOW:  return "LOW";
    case OCCUPANCY_HIGH: return "HIGH";
    case OCCUPANCY_EMPTY:
    default:             return "EMPTY";
  }
}
