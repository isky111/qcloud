/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t esp_err_t;

typedef struct hsp {
    uint16_t h;  // 0-360  hue
    uint16_t s;  // 0-100  saturation
    uint16_t l;  // 0-100  lightness
} light_hsp_t;

typedef struct light {
    bool status;
    light_hsp_t hsl;
} light_t;

esp_err_t light_driver_init(void);

esp_err_t light_driver_deinit(void);

esp_err_t light_set_status(bool value, uint16_t hue, uint16_t saturation, uint16_t lightness);

esp_err_t light_set_hue(uint16_t hue);

esp_err_t light_set_lightness(uint16_t lightness);

esp_err_t light_set_saturation(uint16_t saturation);

esp_err_t light_get_status(bool *value, uint16_t *hue, uint16_t *saturation, uint16_t *lightness);

esp_err_t light_get_hue(uint16_t *hue);

esp_err_t light_get_lightness(uint16_t *lightness );

esp_err_t light_get_saturation(uint16_t *saturation);

esp_err_t light_update(bool status);


#ifdef __cplusplus
}
#endif


