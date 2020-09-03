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

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define CONFIG_EXAMPLE_RMT_TX_GPIO 18
#define CONFIG_EXAMPLE_STRIP_LED_NUMBER 24

static led_strip_t *strip;
static const char *TAG = "ledWS2812";

static void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i)
    {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void ledws2812_init(void)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_EXAMPLE_RMT_TX_GPIO, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(CONFIG_EXAMPLE_STRIP_LED_NUMBER, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
}

esp_err_t ledws2812_deinit(void)
{
	if (strip == NULL) {
		ESP_LOGE(TAG, "not found ws2812 ");
		return ESP_FAIL;
	}
	return (esp_err_t)(strip->del) ;
}

void ledws2812_set_status(bool value, uint16_t hue, uint16_t saturation, uint16_t lightness)
{
	//ESP_LOGI(TAG, "ledws2812_set_status : %s", value == true ? "true" : "false");
    if (value == true) {
        uint32_t red = 0;
        uint32_t green = 0;
        uint32_t blue = 0;
        led_strip_hsv2rgb(hue, saturation, lightness, &red, &green, &blue);

        for (int i = 0; i < 3; i++) {
            for (int j = i; j < CONFIG_EXAMPLE_STRIP_LED_NUMBER; j += 3) {
                ESP_ERROR_CHECK(strip->set_pixel(strip, j, red, green, blue));
            }
            ESP_ERROR_CHECK(strip->refresh(strip, 10));
        }
    } else {
        ESP_ERROR_CHECK(strip->clear(strip, 10));
    }
}



