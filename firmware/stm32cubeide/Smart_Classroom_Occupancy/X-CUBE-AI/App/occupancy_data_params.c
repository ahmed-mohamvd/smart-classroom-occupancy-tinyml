/**
  ******************************************************************************
  * @file    occupancy_data_params.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-09-16T11:25:45+0800
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */

#include "occupancy_data_params.h"


/**  Activations Section  ****************************************************/
ai_handle g_occupancy_activations_table[1 + 2] = {
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
  AI_HANDLE_PTR(NULL),
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
};




/**  Weights Section  ********************************************************/
AI_ALIGNED(32)
const ai_u64 s_occupancy_weights_array_u64[45] = {
  0xbdbd128d81342981U, 0xba9dac81b493a081U, 0xcd6581477c65aeb8U, 0x81eec97fc9d4c053U,
  0x9234b44d5d73964dU, 0x229ebd91810a7f9eU, 0xa83377f5187f98e9U, 0x7fb7592e8117aedcU,
  0x84f7a772907fb957U, 0x53426836894eee81U, 0xadaeecc57fbd5c13U, 0x7f54585f0fd5b47fU,
  0x7f000012f6U, 0x1295000002fdU, 0x44700000884U, 0xfffff2e00000093aU,
  0x39f00000324U, 0x1d50000052dU, 0xe5e00000fd2U, 0xd6d0000089fU,
  0x87f0fec67d51f4aU, 0x30d47af7127fbe52U, 0x57813bab083351efU, 0x9d05a7c094aa61e8U,
  0x44fdceb6163fcdfcU, 0xa5efd084f57fa86fU, 0x872a4e6160fbdf1cU, 0xbe7f265160c2304aU,
  0xdd4dc5051645653aU, 0xe41b057fe7faaeb7U, 0x6ef0002ecf300dfU, 0xd9004000e7a3fa81U,
  0x72ed9a8446177f7aU, 0xe33a31eadeae1f0eU, 0xce476d766925d151U, 0x7fc6506a2af1004cU,
  0xfffffb7b00000e33U, 0x68ffffff935U, 0xfffffd0800000763U, 0xfcafffffba2U,
  0xf8511a168b295c81U, 0x4556027f45429e87U, 0x721ca381b8374fcbU, 0xfffffce3fffffb82U,
  0x6ffU,
};


ai_handle g_occupancy_weights_table[1 + 2] = {
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
  AI_HANDLE_PTR(s_occupancy_weights_array_u64),
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
};

