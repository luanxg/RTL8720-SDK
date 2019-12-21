[TOC]

#how to compile
##a.  init the environment
    source build/envsetup.sh
##b.  choose the setting
    lightduerconfig

    1. freertos_esp32-sdkconfig
    2. freertos_mw300-sdkconfig
    3. freertos_rtk-xxxconfig
    4. lightduer_storydroid-x86linuxdemoconfig
    5. linux_mips-democonfig
    6. linux_mips-sdkconfig
    7. linux_mtk-democonfig
    8. linux_x86-democonfig
    .........
##c. after choose the setting then run make
    make

## Demo: compile the linux-demo on ubuntu

  1.  choose 4(lightduer_storydroid-x86linuxdemoconfig) or 8(linux_x86-democonfig) in the above 2
  2. run the demo(suppose choose 8)
 ```
   out/linux/x86/democonfig/modules/linux-demo/linux-demo -h
   out/linux/x86/democonfig/modules/linux-demo/linux-demo -p profile -r record_file
   ```

##How to compile for android platform
  1. install android ndk first(suggested version: android-ndk-r15c)
  2. make sure ndk-build work, then run below command in the root dir of this project
   ```
   ./android_ndk.sh
   ```
  3. find the build results in: out/android/
jni : used for andriod

#compile system introduction
## all the config file under build/device folder
structure of build/device/ [just a example, maybe different with real world]
```
├── freertos #platform
│   ├── esp32 #compile environment for esp32
│   │   ├── configsetup.sh
│   │   └── sdkconfig.mk
│   ├── mw300 #compile environment for mw300
│   │   ├── configsetup.sh
│   │   └── sdkconfig.mk
│   └── rtk #compile environment for rtk
│       ├── configsetup.sh
│       └── xxxconfig.mk
├── lightduer #vendor
│   └── storydroid #product
│       ├── configsetup.sh
│       └── x86linuxdemoconfig.mk
│── linux #platform
│   ├── mips #compile environment for mips
│   │   ├── configsetup.sh
│   │   ├── democonfig.mk
│   │   └── sdkconfig.mk
│   ├── mtk #compile enviroment for mtk
|   │   ├── configsetup.sh
│   │   └── democonfig.mk
│   └── x86 #compile enviroment for x86
│       ├── configsetup.sh
|       └── democonfig.mk
├── moduleconfig.mk
```
as demonstrated above

 - configsetup.sh will be invoked in "source build/envsetup.sh" to add the support setting
    configsetup.sh use the command add_config_combo to add an setting, as below:
    ```
    add_config_combo linux_mtk-democonfig
    ```
 - **rules** for the setting name, e.g. linux_mtk-democonfig
```
    <dir_name1>_<dir_name1>-<config_file>
    a. Don't use  '_'/'-' in dir_name1/dir_name2/config_file
    b. <dir_name1> is the directory name under build/device,
       the name represent the platform or vendor
    c. <dir_name2> is the directory name under <dir_name1>,
       the name represent the compile-enviroment or product
    d. <config_file> is prefix of the file under <dir_name2>
       which set the compile variables.
       the full config file name is <config_file>.mk
```
- moduleconfig.mk
     this file will be used to config the dependency and exclusive relationship between modules
     it also can have default settings for all compile
```
#=====relationship between modules =======#
#dependency for CA
ifeq ($(strip $(modules_module_connagent)),y)
ifeq ($(origin $(modules_module_device_status)), undefined)
modules_module_device_status=y
endif
endif

#exclusive relationship for platform
platform_select=0
ifeq ($(strip $(platform_module_port-freertos)),y)
$(eval platform_select=$(shell echo $$(($(platform_select)+1))))
endif
ifeq ($(strip $(platform_module_port-linux)),y)
$(eval platform_select=$(shell echo $$(($(platform_select)+1))))
endif
ifeq ($(strip $(platform_module_port-lpb100)),y)
$(eval platform_select=$(shell echo $$(($(platform_select)+1))))
endif
ifeq ($(strip $(platform_module_port-lpt230)),y)
$(eval platform_select=$(shell echo $$(($(platform_select)+1))))
endif
ifneq ($(platform_select), 1)
$(warning platform selected, $(platform_select))
$(error exactly one platform should be selected)
endif

#=====relationship between modules=======#
```
- what's in the  < config_file>.mk
    1. the compile environment, such as compiler
    2. the modules needed in the compile-target, as below
```
    #=====start modules select=======#
        modules_module_Device_Info=n
        modules_module_HTTP=n
        modules_module_OTA=n
        modules_module_coap=y
        modules_module_connagent=y
        modules_module_dcs=n
        modules_module_ntp=y
        modules_module_voice_engine=y

        platform_module_platform=n
        platform_module_port-freertos=n
        platform_module_port-linux=y
        platform_module_port-lpb100=n

        module_framework=y

        external_module_mbedtls=y
        external_module_nsdl=y
        external_module_cjson=y
        external_module_speex=y
        #=====end modules select=======#
```
#module introduction
##external:
    mbedtls : framework
    nsdl    :
    cjson   :
    speex   : framework

##examples:
    linux-demo : connagent coap voice_engine ntp port-linux framework nsdl speex mbedtls cjson
    tracker    : connagent coap port-linux framework nsdl mbedtls cjson

##framework:
    framework : cjson mbedtls
##modules:
    Device_Info  : framework coap
    HTTP         : framework coap connagent cjson voice_engine
    OTA          : framework coap
    coap         : framework nsdl
    connagent    : cjson framework coap platform
    dcs          : framework coap connagent cjson platform
    ntp          : framework
    voice_engine : framework cjson coap connagent speex mbedtls platform
##platform:
    platform      :
    port-freertos : framework cjson connagent coap voice_engine platform
    port-linux    : framework cjson connagent coap voice_engine platform
    port-lpb100   : framework cjson connagent coap platform

##duer-device[default compile target] :
    framework cjson nsdl coap connagent ntp voice_engine
    HTTP OTA Device_Info dcs
        port-freertos(mw300)/port-freertos(esp32)/port-linux(linux)
        mbedtls(esp32)
        speex(mw300)[DUER_COMPRESS_QUALITY=8]

##below command will show the modules selected
```
make modules
```

#FAQ
## 1. when compile with esp32 config
    use the IDF commit: ff6a3b1a11032798565d7b78319444aa81881542
    ask Espressif System for more info.
## 2. when add a new module, which files need update or add
    if a common used module, remember to update the file, moduleconfig.mk
    add the new module to variable lightduer_supported_modules in Makefile
