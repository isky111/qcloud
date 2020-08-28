set(COMPONENT_ADD_INCLUDEDIRS "led_driver/inc" ".")

set(COMPONENT_SRCDIRS "led_driver/src" ".") 

set(COMPONENT_REQUIRES "qcloud" "wifi_config" "soc" "driver" "esp_common")

register_component()
