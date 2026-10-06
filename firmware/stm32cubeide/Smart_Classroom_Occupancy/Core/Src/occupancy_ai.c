#include "occupancy_ai.h"

#include <string.h>

#include "occupancy.h"
#include "occupancy_data.h"
#include "occupancy_data_params.h"

AI_ALIGNED(AI_OCCUPANCY_ACTIVATIONS_ALIGNMENT)
static ai_u8 occupancy_activations[AI_OCCUPANCY_DATA_ACTIVATIONS_SIZE];

static ai_handle occupancy_network = AI_HANDLE_NULL;
static ai_buffer *occupancy_inputs = NULL;
static ai_buffer *occupancy_outputs = NULL;

bool OccupancyAI_Init(void)
{
  const ai_handle activations[AI_OCCUPANCY_DATA_ACTIVATIONS_COUNT] = {
      AI_HANDLE_PTR(occupancy_activations),
  };
  const ai_handle weights[AI_OCCUPANCY_DATA_WEIGHTS_COUNT] = {
      AI_OCCUPANCY_DATA_WEIGHTS_TABLE_GET()[0],
  };
  ai_error error;

  error = ai_occupancy_create_and_init(
      &occupancy_network, activations, weights);
  if (error.type != AI_ERROR_NONE)
  {
    occupancy_network = AI_HANDLE_NULL;
    return false;
  }

  occupancy_inputs = ai_occupancy_inputs_get(occupancy_network, NULL);
  occupancy_outputs = ai_occupancy_outputs_get(occupancy_network, NULL);
  if ((occupancy_inputs == NULL) || (occupancy_outputs == NULL) ||
      (occupancy_inputs[0].data == AI_HANDLE_NULL) ||
      (occupancy_outputs[0].data == AI_HANDLE_NULL)) return false;

  return true;
}

bool OccupancyAI_Run(
    const int8_t input[OCCUPANCY_FEATURE_COUNT],
    int8_t output[OCCUPANCY_CLASS_COUNT])
{
  ai_i32 batches;
  if ((occupancy_network == AI_HANDLE_NULL) ||
      (occupancy_inputs == NULL) || (occupancy_outputs == NULL) ||
      (input == NULL) || (output == NULL)) return false;

  memcpy(occupancy_inputs[0].data, input, AI_OCCUPANCY_IN_1_SIZE_BYTES);
  batches = ai_occupancy_run(
      occupancy_network, occupancy_inputs, occupancy_outputs);
  if (batches != 1) return false;

  memcpy(output, occupancy_outputs[0].data, AI_OCCUPANCY_OUT_1_SIZE_BYTES);
  return true;
}
