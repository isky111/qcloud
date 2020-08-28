#include "light.h"
#include "stdio.h"
#include "esp_log.h"
#include "led_5050.h"
#include "led_ws2812.h"

#define LED5050
#define LEDWS2812


light_t *g_light = NULL;

static const char *TAG = "light";

esp_err_t light_driver_init(void)
{   
    g_light = (light_t *)calloc(1, sizeof(light_t));
    if (g_light == NULL) {
        ESP_LOGE(TAG, "light calloc mem fail");
        return ESP_FAIL;
    }
#ifdef LED5050
    ESP_LOGI(TAG, "init LED5050");
    led5050_init();
    led5050_set_status(g_light->status, g_light->hsl.h, g_light->hsl.s, g_light->hsl.l);
#endif

#ifdef LEDWS2812
    ESP_LOGI(TAG, "init LEDWS2812");
    ledws2812_init();
    ledws2812_set_status(g_light->status, g_light->hsl.h, g_light->hsl.s, g_light->hsl.l);
#endif
    return ESP_OK;
}

esp_err_t light_driver_deinit(void)
{
    if (g_light == NULL) {
        ESP_LOGE(TAG, "g_light is null, deinit fail");
        return ESP_FAIL;
    }
    free(g_light);
#ifdef LED5050
    led5050_deinit();
#endif

#ifdef LEDWS2812
    ledws2812_deinit();
#endif
    return ESP_OK;
}

esp_err_t light_set_status(bool value, uint16_t hue, uint16_t saturation, uint16_t lightness)
{
    if (g_light == NULL) {
        ESP_LOGE(TAG, "g_light is null, set fail");
        return ESP_FAIL;
    }
    g_light->status = value;
    g_light->hsl.h = hue;
    g_light->hsl.s = saturation;
    g_light->hsl.l = lightness;
#ifdef LED5050
    led5050_set_status(g_light->status, g_light->hsl.h, g_light->hsl.s, g_light->hsl.l);
#endif

#ifdef LEDWS2812
    ledws2812_set_status(g_light->status, g_light->hsl.h, g_light->hsl.s, g_light->hsl.l);
#endif
    return ESP_OK;
}

esp_err_t light_set_hue(uint16_t hue)
{
    if (g_light == NULL) {
        ESP_LOGE(TAG, "g_light is null, set fail");
        return ESP_FAIL;
    }
    g_light->hsl.h = hue;
    return ESP_OK;
}

esp_err_t light_set_lightness(uint16_t lightness )
{    
    if (g_light == NULL) {
        ESP_LOGE(TAG, "g_light is null, set fail");
        return ESP_FAIL;
    }
    g_light->hsl.l = lightness;
    return ESP_OK;
}

esp_err_t light_set_saturation(uint16_t saturation)
{
    if (g_light == NULL) {
        ESP_LOGE(TAG, "g_light is null, set fail");
        return ESP_FAIL;
    }
    g_light->hsl.s = saturation;
    return ESP_OK;
}

esp_err_t light_get_hue(uint16_t *hue)
{
     if (g_light == NULL) {
        ESP_LOGE(TAG, "g_light is null, get fail");
        return ESP_FAIL;
    }
    *hue = g_light->hsl.h;
    return ESP_OK;     
}

esp_err_t light_get_lightness(uint16_t *lightness )
{
    if (g_light == NULL) {
        ESP_LOGE(TAG, "g_light is null, get fail");
        return ESP_FAIL;
    }
    *lightness = g_light->hsl.l;
    return ESP_OK;
}

esp_err_t light_get_saturation(uint16_t *saturation)
{
    if (g_light == NULL) {
        ESP_LOGE(TAG, "g_light is null, get fail");
        return ESP_FAIL;
    }
    *saturation = g_light->hsl.s;
    return ESP_OK;
}

esp_err_t light_get_status(bool *value, uint16_t *hue, uint16_t *saturation, uint16_t *lightness)
{
    if (g_light == NULL) {
        ESP_LOGE(TAG, "g_light is null, get fail");
        return ESP_FAIL;
    }
    *value = g_light->status;
    *hue = g_light->hsl.h;
    *saturation = g_light->hsl.s;
    *lightness = g_light->hsl.l;
    return ESP_OK;
}

esp_err_t light_update(bool status)
{
    if (g_light == NULL) {
        ESP_LOGE(TAG, "g_light is null, update fail");
        return ESP_FAIL;
    }
    g_light->status = status;

#ifdef LED5050
    led5050_set_status(g_light->status, g_light->hsl.h, g_light->hsl.s, g_light->hsl.l);
#endif

#ifdef LEDWS2812
    ledws2812_set_status(g_light->status, g_light->hsl.h, g_light->hsl.s, g_light->hsl.l);
#endif
    return ESP_OK;
}