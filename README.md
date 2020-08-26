# ESP32 设备对接腾讯云指南
# 目录

- [0.介绍](#Introduction)
- [1.目的](#aim)
- [2.硬件准备](#hardwareprepare)  
- [3.腾讯云平台准备](#qcloudprepare)  
- [4.IDF环境搭建](#compileprepare)  
- [5.腾讯云SDK准备](#sdkprepare)  
- [6.编译&烧写&运行](#makeflash)  

# <span id = "Introduction">0.介绍</span>
[乐鑫](https://www.espressif.com/zh-hans)是高集成度芯片的设计专家，专注于设计简单灵活、易于制造和部署的解决方案。乐鑫研发和设计 IoT 业内集成度高、性能稳定、功耗低的无线系统级芯片，乐鑫的模组产品集成了自主研发的系统级芯片，因此具备强大的 Wi-Fi 和蓝牙功能，以及出色的射频性能。

[腾讯物联网开发平台](https://console.cloud.tencent.com/iotexplorer)为各行业的设备制造商、方案商及应用开发商提供一站式设备智能化服务。平台提供海量设备连接与管理能力及小程序应用开发能力，并打通腾讯云基础产品及 AI 能力，提升传统行业设备智能化的效率，降低用户的开发运维成本，助力用户业务发展。

# <span id = "aim">1.目的</span>
本文基于 linux 环境，介绍 ESP 设备对接腾讯云平台的具体流程，供读者参考。

# <span id = "hardwareprepare">2.硬件准备</span>
- **linux 环境**  
用来编译 & 烧写 & 运行等操作的必须环境。 
> windows 用户可安装虚拟机，在虚拟机中安装 linux。

- **ESP 设备**  
本文采用 [ESP32-S2-Saola-1开发板](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s2/hw-reference/esp32s2/user-guide-saola-1-v1.2.html)

- **USB 线**  
连接 PC 和 ESP 设备，用来烧写/下载程序，查看 log 等。

# <span id = "qcloudprepare">3.腾讯云平台准备</span>
根据[腾讯官方文档](https://cloud.tencent.com/document/product/1081/34739)，在腾讯云平台创建产品，创建设备，注意查看自动产生的 `product id`, `device name`, `device secret`。每一个设备都需要使用不同的一组 `product id`, `device name`, `device secret` 完成配置。
> 在`examples - smart_light` 有具体关于数据模板的说明，供您参考。

# <span id = "compileprepare">4.IDF环境搭建</span>
您可以参考[ESP-IDF编程指南-快速入门](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html#get-started-setup-toolchain) 快速完成工具链与环境的搭建。
> Espressif SDK 搭建完成后：    
>  ESP-IDF: 请切换到 release/v4.2 分支： `git checkout release/v4.2`   

> 如果您之前已完成搭建，可能需要使用 ：`git submodule update --init --recursive `更新子仓库

# <span id = "sdkprepare">5.腾讯云SDK准备</span> 
[qcloud-iot-explorer-sdk-embedded-c](https://github.com/tencentyun/qcloud-iot-explorer-sdk-embedded-c), 通过该 SDK 可实现使用 MQTT 协议，连接 ESP 设备到腾讯云。
> 该目录已预制 v3.12 的SDK，如不需要更新可以跳过

1. 获取腾讯云SDK
```
git clone https://github.com/tencentyun/qcloud-iot-explorer-sdk-embedded-c.git
```
2. 配置CMakeLists.txt 文件
> 根据您的需求开启相应功能，您也可以参考[当前SDK配置](./qcloud_iot_c_sdk/include/config.h)
```
set(BUILD_TYPE                  "release")
set(PLATFORM 	                "freertos")
set(EXTRACT_SRC ON)
```

3. 抽取SDK
> 在 output/qcloud_iot_c_sdk 中即可找到相关代码文件。将生成的文件移动到[qcloud_iot_c_sdk](./qcloud_iot_c_sdk)
```
mkdir build
cd build
cmake ..
```

# <span id = "makeflash">6.编译&烧写&运行</span>
您可以在 examples 下选择您需要的工程编译，具体说明请到对应工程查看。
> 不过，无论编译哪个 examples 您都需要将 ` 3.腾讯云平台准备`中生产的三元组信息填写到 [HAL_Device_freertos.c](./qcloud_iot_c_sdk/platform/HAL_Device_freertos.c)

```
/* Product Id */
static char sg_product_id[MAX_SIZE_OF_PRODUCT_ID + 1]    = "PRODUCT_ID";
/* Device Name */
static char sg_device_name[MAX_SIZE_OF_DEVICE_NAME + 1]  = "YOUR_DEV_NAME";
/* Device Secret */
static char sg_device_secret[MAX_SIZE_OF_DEVICE_SECRET + 1] = "YOUR_IOT_PSK";
```





