#include "esp_qcloud_prov.h"
#include "esp_log.h"
#include "factory_restore.h"

static const char *TAG = "esp_qcloud_prov";
static EventGroupHandle_t g_wifi_event_group;

/* WiFi router SSID  */
#define TEST_WIFI_SSID                 CONFIG_DEMO_WIFI_SSID
/* WiFi router password */
#define TEST_WIFI_PASSWORD             CONFIG_DEMO_WIFI_PASSWORD

static const int CONNECTED_BIT = BIT0;
static EventGroupHandle_t g_wifi_event_group;

static void esp_wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "event = %d", event_id);

    switch (event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
            esp_wifi_connect();
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            //xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
           esp_wifi_connect();
            break;

        default:
            ESP_LOGW(TAG, "unkown wifi_event");
            break;
    }
}

static void esp_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    ESP_LOGI(TAG, "Got IPv4[%s]", ip4addr_ntoa((const ip4_addr_t *)&event->ip_info.ip));
    xEventGroupSetBits(g_wifi_event_group, CONNECTED_BIT);
}

esp_err_t esp_qcloud_prov_is_provisioned(bool *provisioned)
{
    uint8_t len = 32;
    uint8_t ssid[32] = { 0 };

    *provisioned = false;
    ESP_ERROR_CHECK(qcloud_nvs_kv_get("stassid", ssid, (int *)&len));
    //ESP_LOGI(TAG, "ssid : %s", ssid );

    if (strlen((const char*) ssid)) {
        *provisioned = true;
    }
    return ESP_OK;
}

esp_err_t esp_qcloud_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    g_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &esp_wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &esp_ip_event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_FLASH) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    return ESP_OK;
}

esp_err_t esp_qcloud_get_wifi_config(wifi_config_t *conf)
{
    int ssid_len = 32;
    int password_len = 64;
    memset(conf, 0, sizeof(wifi_config_t));
    ESP_ERROR_CHECK(qcloud_nvs_kv_get("stassid", conf->sta.ssid, &ssid_len));
    ESP_ERROR_CHECK(qcloud_nvs_kv_get("pswd", conf->sta.password, &password_len));
    ESP_LOGI(TAG, "Setting WiFi configuration SSID:%s,  PSW:%s", conf->sta.ssid, conf->sta.password);
    return ESP_OK;
}

esp_err_t esp_qcloud_wifi_start(wifi_config_t *conf)
{
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, conf));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());

    xEventGroupWaitBits(g_wifi_event_group, CONNECTED_BIT, true, true, portMAX_DELAY);

    return ESP_OK;
}