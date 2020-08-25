set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_SRCS "qcloud_iot_ota_esp.c")

set(COMPONENT_REQUIRES "app_update" "qcloud" "wifi_config")

register_component()
