/*
 * Copyright (c) 2020 Tencent Cloud. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "esp_err.h"

#pragma once
#ifndef _LED_WS2812_H__
#define _LED_WS2812_H__

/**
 * @brief initialize the ledws2812 module
 *
 * @param none
 *
 * @return none
 */
void ledws2812_init(void);

/**
 * @brief deinitialize the ledws2812 module
 *
 * @param none
 *
 * @return none
 */
void ledws2812_deinit(void);

/**
 * @brief turn on/off the ledws2812 and set the saturation、hue、lightness of the ledws2812
 *        If value is true, the parameter will be invalid
 *
 * @param value The "On" value
 *
 * @return none
 */
void ledws2812_set_status(bool value, uint16_t hue, uint16_t saturation, uint16_t lightness);

#endif
