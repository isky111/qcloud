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

#include <string.h>
#include <time.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_smartconfig.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/apps/sntp.h"

#include "qcloud_iot_export.h"
#include "smart_light.h"
#include "factory_restore.h"
#include "light.h"
#include "esp_qcloud_prov.h"

static const char *TAG = "main";

/* normal WiFi STA mode init and connection ops */
//#ifndef CONFIG_WIFI_CONFIG_ENABLED

/* WiFi router SSID  */
#define TEST_WIFI_SSID                 CONFIG_DEMO_WIFI_SSID
/* WiFi router password */
#define TEST_WIFI_PASSWORD             CONFIG_DEMO_WIFI_PASSWORD


void setup_sntp(void )
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    // to set more sntp server, plz modify macro SNTP_MAX_SERVERS in sntp_opts.h file
    // set sntp server after got ip address, you'd better adjust the sntp server to your area
    sntp_setservername(0, "time1.cloud.tencent.com");
    sntp_setservername(1, "cn.pool.ntp.org");
    sntp_setservername(2, "time-a.nist.gov");
    sntp_setservername(3, "cn.ntp.org.cn");

    sntp_init();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2019 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        sleep(1);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
    vTaskDelete(NULL);
}

void light_provision_task (void)
{
    uint16_t hue = 0;
    while (1) {
        light_set_status(true, (hue++)%360, 100, 100); 
        vTaskDelay(5);
     }
}

void app_main()
{
    bool provisioned = false;
    TaskHandle_t light_task = NULL;
    int rc;

    IOT_Log_Set_Level(eLOG_DEBUG);
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK( factory_restore_init() );
    ESP_ERROR_CHECK( light_driver_init() );
    ESP_ERROR_CHECK( light_set_status(true, 300, 100, 100) );
    ESP_ERROR_CHECK( esp_qcloud_wifi_init() );

#if CONFIG_WIFI_CONFIG_ENABLED
    wifi_config_t wifi_config = { 0 };
    ESP_ERROR_CHECK( esp_qcloud_prov_is_provisioned(&provisioned, &wifi_config) );
    if (!provisioned) {
        //prompt in the softap mode
        xTaskCreate((void (*)(void *))light_provision_task, "light_breathing", 2048, NULL, 4, &light_task); 
        
        //start softap
        esp_qcloud_prov_softap_start("qcloud_123", NULL, NULL);
        
        //block max time : 5min 12s = 312s
        esp_qcloud_prov_wait(&wifi_config, NULL, ( 312*1000 ) / portTICK_PERIOD_MS);
        esp_qcloud_send_token() ;

        //off task,light up
        if(light_task != NULL)    
            vTaskDelete(light_task);
        ESP_ERROR_CHECK( light_set_status(true, 300, 100, 100) );
    }
#else
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = TEST_WIFI_SSID,
            .password = TEST_WIFI_PASSWORD,
        },
    };
#endif

    ESP_ERROR_CHECK(esp_qcloud_wifi_start(&wifi_config));
    xTaskCreate((void (*)(void *))setup_sntp, "setup_sntp", 8196, NULL, 4, NULL); 
    rc = smart_light_demo();

    //if run here,have an unhandled error
    ESP_LOGE(TAG, "smart light demo quit, resaon: %d", rc);
    light_driver_deinit();
    vTaskDelay(1000);
    esp_restart();
}


