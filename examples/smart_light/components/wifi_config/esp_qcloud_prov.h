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
#include "stdio.h"
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_event.h>

#ifndef __ESP_QCLOUD_PROV_H__
#define __ESP_QCLOUD_PROV_H__

typedef int32_t esp_err_t;

esp_err_t esp_qcloud_wifi_init(void);

esp_err_t esp_qcloud_wifi_start(wifi_config_t *conf);

esp_err_t esp_qcloud_prov_is_provisioned(bool *provisioned, wifi_config_t *sta_cfg);

esp_err_t esp_qcloud_get_wifi_config(wifi_config_t *conf);

esp_err_t esp_qcloud_wifi_start(wifi_config_t *conf);

esp_err_t esp_qcloud_prov_softap_start(const char *ssid, const char *password, const char *pop);

esp_err_t esp_qcloud_prov_wait(wifi_config_t *sta_cfg, char *token, TickType_t ticks_wait);

esp_err_t esp_qcloud_prov_softap_stop();

#endif