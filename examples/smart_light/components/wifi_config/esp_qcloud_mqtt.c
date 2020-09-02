#include "esp_qcloud_prov.h"
#include "esp_log.h"
#include "factory_restore.h"
#include "qcloud_iot_import.h"
#include "qcloud_wifi_config.h"
#include "wifi_config_internal.h"
#include "qcloud_iot_export_log.h"
#include "qcloud_iot_export.h"

#include "utils_hmac.h"
#include "utils_base64.h"
#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

static const char *TAG = "esp_qcloud_mqtt";
static void *g_mqtt_client = NULL;

#define MAX_TOKEN_LENGTH 32
extern char sg_token_str[MAX_TOKEN_LENGTH + 4];
extern bool sg_token_received;

typedef struct TokenHandleData {
    bool sub_ready;
    bool send_ready;
    int  reply_code;
} TokenHandleData;

static void mqtt_event_handler(void *pclient, void *handle_context, MQTTEventMsg *msg)
{
    MQTTMessage *    mqtt_messge = (MQTTMessage *)msg->msg;
    uintptr_t        packet_id   = (uintptr_t)msg->msg;
    TokenHandleData *app_data    = (TokenHandleData *)handle_context;
    switch (msg->event_type) {
        case MQTT_EVENT_DISCONNECT:
            ESP_LOGW(TAG, "MQTT disconnect");
            break;

        case MQTT_EVENT_RECONNECT:
            ESP_LOGI(TAG, "MQTT reconnected");
            break;

        case MQTT_EVENT_PUBLISH_RECVEIVED:
            ESP_LOGI(TAG, "unhandled msg arrived: topic=%s", mqtt_messge->ptopic );
            break;

        case MQTT_EVENT_SUBCRIBE_SUCCESS:
            Log_d("mqtt topic subscribe success");
            ESP_LOGI(TAG, "mqtt topic subscribe success" );
            app_data->sub_ready = true;
            break;

        case MQTT_EVENT_SUBCRIBE_TIMEOUT:
            Log_i("mqtt topic subscribe timeout");
            app_data->sub_ready = false;
            break;

        case MQTT_EVENT_SUBCRIBE_NACK:
            Log_i("mqtt topic subscribe NACK");
            app_data->sub_ready = false;
            break;

        case MQTT_EVENT_PUBLISH_SUCCESS:
            Log_i("publish success, packet-id=%u", (unsigned int)packet_id);
            app_data->send_ready = true;
            break;

        case MQTT_EVENT_PUBLISH_TIMEOUT:
            Log_i("publish timeout, packet-id=%u", (unsigned int)packet_id);
            app_data->send_ready = false;
            break;

        case MQTT_EVENT_PUBLISH_NACK:
            Log_i("publish nack, packet-id=%u", (unsigned int)packet_id);
            app_data->send_ready = false;
            break;

        default:
            Log_i("unknown event type: %d", msg->event_type);
            break;
    }
}

static int get_reg_dev_info(DeviceInfo *dev_info)
{
    int ret = HAL_GetDevInfo(dev_info);
    if (ret) {
        Log_e("HAL_GetDevInfo error: %d", ret);
        PUSH_LOG("HAL_GetDevInfo error: %d", ret);
        return ret;
    }

    // 简单演示进入动态注册的条件，用户可根据自己情况调整
    // 如果 dev_info->device_secret == "YOUR_IOT_PSK", 表示设备没有有效的PSK
    // 并且 dev_info->product_secret != "YOUR_PRODUCT_SECRET", 表示具备产品密钥，可以进行动态注册
    if (!strncmp(dev_info->device_secret, "YOUR_IOT_PSK", MAX_SIZE_OF_DEVICE_SECRET) &&
        strncmp(dev_info->product_secret, "YOUR_PRODUCT_SECRET", MAX_SIZE_OF_PRODUCT_SECRET)) {
        ret = IOT_DynReg_Device(dev_info);
        if (ret) {
            Log_e("dynamic register device failed: %d", ret);
            PUSH_LOG("dynamic register device failed: %d", ret);
            return ret;
        }

        // save the dev info
        ret = HAL_SetDevInfo(dev_info);
        if (ret) {
            Log_e("HAL_SetDevInfo failed: %d", ret);
            return ret;
        }

        // delay a while
        HAL_SleepMs(500);
    }

    return QCLOUD_RET_SUCCESS;
}

void *esp_qcloud_mqtt_init(void *app_data)
{
    DeviceInfo dev_info;
    MQTTInitParams init_params;
    int        ret = get_reg_dev_info(&dev_info);
    if (ret) {
        ESP_LOGE(TAG, "Get device info failed: %d", ret);
        return NULL;
    }

    init_params.region                 = "china";
    init_params.product_id             = dev_info.product_id;
    init_params.device_name            = dev_info.device_name;
    init_params.device_secret          = dev_info.device_secret;
    init_params.command_timeout        = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
    init_params.keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;
    init_params.auto_connect_enable    = 1;
    init_params.clean_session          = 1;
    init_params.event_handle.h_fp      = mqtt_event_handler;
    init_params.event_handle.context   = app_data;
    init_params.err_code               = 0;

    g_mqtt_client = IOT_MQTT_Construct(&init_params);
    if (g_mqtt_client != NULL) {
        ESP_LOGI(TAG, "Cloud Device Construct Success");
        return g_mqtt_client;
    }
    ESP_LOGE(TAG, "Device %s /%s connect failed: %d",  init_params.product_id, init_params.device_name, init_params.err_code);
    return NULL;
}

static void _on_message_callback(void *pClient, MQTTMessage *message, void *userData)
{
    if (message == NULL) {
        Log_e("msg null");
        return;
    }

    if (message->topic_len == 0 && message->payload_len == 0) {
        Log_e("length zero");
        return;
    }

    Log_i("recv msg topic: %s", message->ptopic);

    uint32_t msg_topic_len = message->payload_len + 4;
    char *   buf           = (char *)HAL_Malloc(msg_topic_len);
    if (buf == NULL) {
        Log_e("malloc %u bytes failed", msg_topic_len);
        return;
    }

    memset(buf, 0, msg_topic_len);
    memcpy(buf, message->payload, message->payload_len);
    Log_i("msg payload: %s", buf);

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        Log_e("Parsing JSON Error");
        push_error_log(ERR_TOKEN_REPLY, ERR_APP_CMD_JSON_FORMAT);
    } else {
        cJSON *code_json = cJSON_GetObjectItem(root, "code");
        if (code_json) {
            TokenHandleData *app_data = (TokenHandleData *)userData;
            app_data->reply_code      = code_json->valueint;
            Log_d("token reply code = %d", code_json->valueint);

            if (app_data->reply_code) {
                Log_e("token reply error: %d", code_json->valueint);
                PUSH_LOG("token reply error: %d", code_json->valueint);
                push_error_log(ERR_TOKEN_REPLY, app_data->reply_code);
            }
        } else {
            Log_e("Parsing reply code Error");
            push_error_log(ERR_TOKEN_REPLY, ERR_APP_CMD_JSON_FORMAT);
        }

        cJSON_Delete(root);
    }

    HAL_Free(buf);
}

static int subscribe_topic_wait_result(void *client, DeviceInfo *dev_info, TokenHandleData *app_data)
{
    char topic_name[128] = {0};
    // int size = HAL_Snprintf(topic_name, sizeof(topic_name), "%s/%s/data", dev_info->product_id,
    // dev_info->device_name);
    int size = HAL_Snprintf(topic_name, sizeof(topic_name), "$thing/down/service/%s/%s", dev_info->product_id,
                            dev_info->device_name);
    if (size < 0 || size > sizeof(topic_name) - 1) {
        Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_name));
        return QCLOUD_ERR_FAILURE;
    }
    SubscribeParams sub_params    = DEFAULT_SUB_PARAMS;
    sub_params.qos                = QOS0;
    sub_params.on_message_handler = _on_message_callback;
    sub_params.user_data          = (void *)app_data;
    int rc                        = IOT_MQTT_Subscribe(client, topic_name, &sub_params);
    if (rc < 0) {
        Log_e("MQTT subscribe failed: %d", rc);
        return rc;
    }

    int wait_cnt = 2;
    while (!app_data->sub_ready && (wait_cnt-- > 0)) {
        // wait for subscription result
        rc = IOT_MQTT_Yield(client, 1000);
        if (rc) {
            Log_e("MQTT error: %d", rc);
            return rc;
        }
    }

    if (wait_cnt > 0) {
        return QCLOUD_RET_SUCCESS;
    } else {
        Log_w("wait for subscribe result timeout!");
        return QCLOUD_ERR_FAILURE;
    }
}

static int _publish_token_msg(void *client, DeviceInfo *dev_info, char *token_str)
{
    char topic_name[128] = {0};
    // int size = HAL_Snprintf(topic_name, sizeof(topic_name), "%s/%s/data", dev_info->product_id,
    // dev_info->device_name);
    int size = HAL_Snprintf(topic_name, sizeof(topic_name), "$thing/up/service/%s/%s", dev_info->product_id,
                            dev_info->device_name);
    if (size < 0 || size > sizeof(topic_name) - 1) {
        Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_name));
        return QCLOUD_ERR_FAILURE;
    }

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos           = QOS1;

    char topic_content[256] = {0};
    size                    = HAL_Snprintf(topic_content, sizeof(topic_content),
                        "{\"method\":\"app_bind_token\",\"clientToken\":\"%s-%u\",\"params\": {\"token\":\"%s\"}}",
                        dev_info->device_name, HAL_GetTimeMs(), token_str);
    if (size < 0 || size > sizeof(topic_content) - 1) {
        Log_e("payload content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_content));
        return QCLOUD_ERR_MALLOC;
    }

    pub_params.payload     = topic_content;
    pub_params.payload_len = strlen(topic_content);

    return IOT_MQTT_Publish(client, topic_name, &pub_params);
}

static int send_token_wait_reply(void *client, DeviceInfo *dev_info, TokenHandleData *app_data)
{
    int ret      = 0;
    int wait_cnt = 20;

   // for smartconfig, we need to wait for the token data from app
    while (!sg_token_received && (wait_cnt-- > 0)) {
        IOT_MQTT_Yield(client, 1000);
        Log_i("wait for token data...");
    }

    if (!sg_token_received) {
        Log_e("Wait for token data timeout");
        PUSH_LOG("Wait for token data timeout");
        return QCLOUD_ERR_INVAL;
    }

    wait_cnt = 3;
publish_token:
    ret = _publish_token_msg(client, dev_info, sg_token_str);
    if (ret < 0 && (wait_cnt-- > 0)) {
        Log_e("Client publish token failed: %d", ret);
        if (is_wifi_sta_connected() && IOT_MQTT_IsConnected(client)) {
            IOT_MQTT_Yield(client, 500);
        } else {
            Log_e("Wifi or MQTT lost connection! Wait and retry!");
            IOT_MQTT_Yield(client, 2000);
        }
        goto publish_token;
    }

    if (ret < 0) {
        Log_e("pub token failed: %d", ret);
        PUSH_LOG("pub token failed: %d", ret);
        return ret;
    }

    wait_cnt = 5;
    while (!app_data->send_ready && (wait_cnt-- > 0)) {
        IOT_MQTT_Yield(client, 1000);
        Log_i("wait for token sending result...");
    }

    ret = 0;
    if (!app_data->send_ready) {
        Log_e("pub token timeout");
        PUSH_LOG("pub token timeout");
        ret = QCLOUD_ERR_FAILURE;
    }

    return ret;
}

static void *setup_mqtt_connect(DeviceInfo *dev_info, TokenHandleData *app_data)
{
    MQTTInitParams init_params       = DEFAULT_MQTTINIT_PARAMS;
    init_params.device_name          = dev_info->device_name;
    init_params.product_id           = dev_info->product_id;
    init_params.device_secret        = dev_info->device_secret;
    init_params.event_handle.h_fp    = mqtt_event_handler;
    init_params.event_handle.context = (void *)app_data;

    void *client = IOT_MQTT_Construct(&init_params);
    if (client != NULL) {
        Log_i("Cloud Device Construct Success");
        return client;
    }

    // construct failed, give it another try
    if (is_wifi_sta_connected()) {
        Log_e("Cloud Device Construct Failed: %d! Try one more time!", init_params.err_code);
        HAL_SleepMs(1000);
    } else {
        Log_e("Wifi lost connection! Wait and retry!");
        HAL_SleepMs(2000);
    }

    client = IOT_MQTT_Construct(&init_params);
    if (client != NULL) {
        Log_i("Cloud Device Construct Success");
        return client;
    }

    Log_e("Device %s/%s connect failed: %d", 
        dev_info->product_id, dev_info->device_name, init_params.err_code);
    PUSH_LOG("Device %s/%s connect failed: %d", 
        dev_info->product_id, dev_info->device_name, init_params.err_code);
    push_error_log(ERR_MQTT_CONNECT, init_params.err_code);
    return NULL;
}

esp_err_t esp_qcloud_bind(const char *token)
{
       // get the device info or do device dynamic register
    DeviceInfo dev_info;
    int        ret = get_reg_dev_info(&dev_info);
    if (ret) {
        Log_e("Get device info failed: %d", ret);
        PUSH_LOG("Get device info failed: %d", ret);
        return ret;
    }

    // token handle data
    TokenHandleData app_data;
    app_data.sub_ready  = false;
    app_data.send_ready = false;
    app_data.reply_code = 404;

    // mqtt connection
    // void *client = setup_mqtt_connect(&dev_info, &app_data);
    void *client = esp_qcloud_mqtt_init(&app_data);
    if (client == NULL) {
        return QCLOUD_ERR_MQTT_NO_CONN;
    } else {
        Log_i("Device %s/%s connect success", dev_info.product_id, dev_info.device_name);
        PUSH_LOG("Device %s/%s connect success", dev_info.product_id, dev_info.device_name);
    }

    // subscribe token reply topic
    ret = subscribe_topic_wait_result(client, &dev_info, &app_data);
    if (ret < 0) {
        Log_w("Subscribe topic failed: %d", ret);
        PUSH_LOG("Subscribe topic failed: %d", ret);
    }

    // publish token msg and wait for reply
    int retry_cnt = 2;
    do {
        ret = send_token_wait_reply(client, &dev_info, &app_data);

        IOT_MQTT_Yield(client, 1000);
    } while (ret && retry_cnt--);

    if (ret)
        push_error_log(ERR_TOKEN_SEND, ret);

    //IOT_MQTT_Destroy(&client);

    // sleep 5 seconds to avoid frequent MQTT connection
    if (ret == 0)
        HAL_SleepMs(500);

    return ret;

}

