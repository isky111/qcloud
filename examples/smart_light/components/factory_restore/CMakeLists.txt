set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_SRCS "factory_restore.c")

set(COMPONENT_REQUIRES "nvs_flash" "esp32" "driver" "qcloud" "wifi_config")

register_component()
