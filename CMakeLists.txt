idf_component_register(SRC_DIRS "qcloud_iot_c_sdk/platform" "qcloud_iot_c_sdk/sdk_src"
                        INCLUDE_DIRS "qcloud_iot_c_sdk/include" "qcloud_iot_c_sdk/include/exports" "qcloud_iot_c_sdk/sdk_src/internal_inc"
                        REQUIRES mbedtls 
                        )

# set(COMPONENT_REQUIRES "nvs_flash" "app_update" "esp-tls")

component_compile_options(-DAUTH_MODE_CERT)