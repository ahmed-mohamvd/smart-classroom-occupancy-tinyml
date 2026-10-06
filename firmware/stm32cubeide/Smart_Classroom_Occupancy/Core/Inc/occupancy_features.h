#ifndef OCCUPANCY_FEATURES_H
#define OCCUPANCY_FEATURES_H

#include <stdbool.h>
#include <stdint.h>

#define OCCUPANCY_WINDOW_SAMPLES 30U
#define OCCUPANCY_STEP_SAMPLES    5U
#define OCCUPANCY_FEATURE_COUNT   6U
#define OCCUPANCY_CLASS_COUNT     3U

typedef enum
{
  OCCUPANCY_EMPTY = 0,
  OCCUPANCY_LOW   = 1,
  OCCUPANCY_HIGH  = 2
} OccupancyClass;

typedef struct
{
  uint8_t pir1[OCCUPANCY_WINDOW_SAMPLES];
  uint8_t pir2[OCCUPANCY_WINDOW_SAMPLES];
  uint8_t write_index;
  uint8_t sample_count;
  uint8_t samples_since_inference;
} OccupancyFeatureBuffer;

void OccupancyFeatures_Init(OccupancyFeatureBuffer *buffer);
bool OccupancyFeatures_AddSample(OccupancyFeatureBuffer *buffer,
                                 uint8_t pir1,
                                 uint8_t pir2);
void OccupancyFeatures_Compute(const OccupancyFeatureBuffer *buffer,
                               float features[OCCUPANCY_FEATURE_COUNT]);
void OccupancyFeatures_Normalize(
    const float features[OCCUPANCY_FEATURE_COUNT],
    float normalized[OCCUPANCY_FEATURE_COUNT]);
void OccupancyFeatures_QuantizeInt8(
    const float normalized[OCCUPANCY_FEATURE_COUNT],
    int8_t quantized[OCCUPANCY_FEATURE_COUNT]);
OccupancyClass Occupancy_SelectClassInt8(
    const int8_t output[OCCUPANCY_CLASS_COUNT]);
float Occupancy_OutputProbability(int8_t quantized_output);
const char *Occupancy_ClassName(OccupancyClass occupancy_class);

#endif /* OCCUPANCY_FEATURES_H */
