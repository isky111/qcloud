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
#include "qcloud_wifi_config.h"
#include "factory_restore.h"
#include "light.h"
#include "esp_qcloud_prov.h"

#define STA_SSID_KEY             "stassid"
#define STA_PASSWORD_KEY         "pswd"

bool wifi_connected = false;
bool provisioned = false;

static const char *TAG = "main";

/* normal WiFi STA mode init and connection ops */
//#ifndef CONFIG_WIFI_CONFIG_ENABLED

/* WiFi router SSID  */
#define TEST_WIFI_SSID                 CONFIG_DEMO_WIFI_SSID
/* WiFi router password */
#define TEST_WIFI_PASSWORD             CONFIG_DEMO_WIFI_PASSWORD

static const int CONNECTED_BIT = BIT0;
static EventGroupHandle_t wifi_event_group;

bool wait_for_wifi_ready(int event_bits, uint32_t wait_cnt, uint32_t BlinkTime)
{
    EventBits_t uxBits;
    uint32_t cnt = 0;
    uint8_t blueValue = 0;

    while (cnt++ < wait_cnt) {
        uxBits = xEventGroupWaitBits(wifi_event_group, event_bits, true, false, BlinkTime / portTICK_RATE_MS);

        if (uxBits & CONNECTED_BIT) {
            Log_d("WiFi Connected to AP");
            return true;
        }
    }

    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);

    return false;
}


static void wifi_connection(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = TEST_WIFI_SSID,
            .password = TEST_WIFI_PASSWORD,
        },
    };
    int ssid_len = 32;
    int password_len = 64;
        if (provisioned) {
        memset(&wifi_config, 0, sizeof(wifi_config_t));
        int ret = qcloud_nvs_kv_get(STA_SSID_KEY, wifi_config.sta.ssid, &ssid_len);

        ret = qcloud_nvs_kv_get(STA_PASSWORD_KEY, wifi_config.sta.password, &password_len);
    }
    Log_i("Setting WiFi configuration SSID:%s,  PSW:%s", wifi_config.sta.ssid, wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    esp_wifi_connect();
}

static void _esp_event_handler_new(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    Log_i("event = %d", event_id);
    switch (event_id) {
        case SYSTEM_EVENT_STA_START:
            Log_i("SYSTEM_EVENT_STA_START");
            wifi_connection();
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            Log_i("SYSTEM_EVENT_STA_DISCONNECTED");
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            esp_wifi_connect();
            break;

        default:
            break;
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    Log_i("Got IPv4[%s]", ip4addr_ntoa((const ip4_addr_t *)&event->ip_info.ip));
    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
}

static void esp_wifi_initialise(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_esp_event_handler_new, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_FLASH) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

//#endif //#ifnef CONFIG_DEMO_WIFI_BOARDING

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
        Log_d("Waiting for system time to be set... (%d/%d)", retry, retry_count);
        sleep(1);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
    vTaskDelete(NULL);
}

static bool app_prov_is_provisioned(bool * provisioned)
{
    *provisioned = false;
    esp_err_t err;
    int len = 32;
    uint8_t ssid[32] = { 0 };

    int ret = qcloud_nvs_kv_get(STA_SSID_KEY, ssid, &len);
    ESP_LOGI("main", "ssid : %s",ssid );

    if (strlen((const char*) ssid)) {
        *provisioned = true;
    }
    return ESP_OK;
}

void smart_light_task(void* parm)
{
    ESP_LOGI(TAG, "smart_light_task start");

#if CONFIG_WIFI_CONFIG_ENABLED
    ESP_LOGI(TAG, "use provision mode");
    if ( app_prov_is_provisioned(&provisioned) != ESP_OK) {
        ESP_LOGE(TAG, "getting device provisioning state fail");
    }
    ESP_LOGI("main", "provisioned : %d", provisioned );
    if (!provisioned) {
        int wifi_config_state;
        int ret = start_softAP("ESP8266-SAP", "12345678", 0);
        //int ret = start_smartconfig();
        if (ret) {
            ESP_LOGE(TAG, "start wifi config failed: %d", ret);
        }else{
            int wait_cnt = 1500;
            do{
                ESP_LOGI(TAG, "waiting for wifi config result...");
                HAL_SleepMs(200);
                wifi_config_state = query_wifi_config_state();
            } while (wifi_config_state == WIFI_CONFIG_GOING_ON && wait_cnt--);
        }

        wifi_connected = is_wifi_config_successful();
        if (!wifi_connected) {
            ESP_LOGE(TAG, "wifi config failed!");
            //start_log_softAP();
        }
    } else {
         esp_wifi_initialise();
         wifi_connected = wait_for_wifi_ready(CONNECTED_BIT, 20, 1000);
    }
    
#else
    ESP_LOGI(TAG, "use user configuration");
    esp_wifi_initialise();
    wifi_connected = wait_for_wifi_ready(CONNECTED_BIT, 20, 1000);
#endif
    if (wifi_connected) {
        xTaskCreate(setup_sntp, "setup_sntp", 8196, NULL, 4, NULL); 
        ESP_LOGI(TAG,"timestamp now:%d", (int)HAL_Timer_current_sec());
        smart_light_demo();
    } else {
        ESP_LOGE(TAG, "WiFi is not ready, please check configuration");
    }
    ESP_LOGW(TAG, "smart_light_task quit");

    esp_restart();
    vTaskDelete(NULL);
}

#if 0
void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(factory_restore_init());

    ESP_ERROR_CHECK(light_driver_init());
    light_set_status(true, 300, 100, 100);

    //init log level
    IOT_Log_Set_Level(eLOG_DEBUG);

    xTaskCreate((void (*)(void *))smart_light_task, "smart_light_task", 3072, NULL, 4, NULL);

}
#endif

void app_main()
{
    bool provisioned = false;
    IOT_Log_Set_Level(eLOG_DEBUG);
    ESP_ERROR_CHECK( nvs_flash_init() );

    ESP_ERROR_CHECK( factory_restore_init() );

    ESP_ERROR_CHECK( light_driver_init() );

    ESP_ERROR_CHECK( light_set_status(true, 300, 100, 100) );

    ESP_ERROR_CHECK( esp_qcloud_wifi_init() );

#if 0
     // ESP_ERROR_CHECK( app_prov_is_provisioned(&provisioned) );
    if (!provisioned) {
        //todo 
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
    xTaskCreate(setup_sntp, "setup_sntp", 8196, NULL, 4, NULL); 
    smart_light_demo();

}


