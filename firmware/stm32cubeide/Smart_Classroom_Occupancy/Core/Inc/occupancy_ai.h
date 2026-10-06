#ifndef OCCUPANCY_AI_H
#define OCCUPANCY_AI_H

#include <stdbool.h>
#include <stdint.h>

#include "occupancy_features.h"

bool OccupancyAI_Init(void);
bool OccupancyAI_Run(
    const int8_t input[OCCUPANCY_FEATURE_COUNT],
    int8_t output[OCCUPANCY_CLASS_COUNT]);

#endif /* OCCUPANCY_AI_H */
