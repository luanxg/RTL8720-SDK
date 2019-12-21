// Copyright (2017) Baidu Inc. All rights reserved.
/**
 * File: duerapp.c
 * Auth: Su Hao(suhao@baidu.com)
 * Desc: Duer Application Main.
 */

//#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

#include "lightduer_connagent.h"
#include "lightduer_voice.h"
#include "lightduer_timers.h"
#include "lightduer_key.h"
#include "lightduer_voice.h"
#include "lightduer_net_ntp.h"

#include "duerapp.h"
#include "duerapp_config.h"
#include "lightduer_memory.h"
#include "section_config.h"
#include "duerapp_audio.h"

//for DCS
#include "lightduer_dcs.h"
#include "lightduer_system_info.h"


//for OTA
#include "duerapp_ota.h"

//for alarm
#include "lightduer_dcs_alert.h"

#define OPEN_DUER_AUTO_RESTART
#define PROFILE_PATH    SDCARD_PATH"/profile"

#define VOICE_FROM_AUDIO_CODEC      0
#define VOICE_FROM_SDCARD           0
#define PROFILE_FROM_SDCARD         0
#define DUER_VOICE_DELAY			5*1000

static TaskHandle_t xHandle = NULL;
duer_timer_handler g_restart_timer = NULL;

T_APP_DUER DuerAppInfo = {0};
volatile T_APP_DUER *pDuerAppInfo = &DuerAppInfo;

//SystemInfo
const duer_system_static_info_t g_system_static_info = {
    .os_version         = "FreeRTOS V8.2.0",
    .sw_version         = "dcs3.0",
    .brand              = "RTK",
    .equipment_type     = "8195AM",
    .hardware_version   = "8195",
    .ram_size           = 4431,
    .rom_size           = 4096,
};

#define DUER_PROFILE_UUID   11  //Range: 5,6,7,8,9
#if (PROFILE_FROM_SDCARD == 0)
#if (DUER_PROFILE_UUID == 5)
SDRAM_DATA_SECTION static const char duer_profile[] =
    "{\"configures\":\"{}\",\"bindToken\":\"babfd6add70bcfc83cbd9341bbbc52f3\",\"coapPort\":443,\
\"token\":\"jRJG498X\",\"serverAddr\":\"device.iot.baidu.com\",\"lwm2mPort\":443,\
\"uuid\":\"087c0000000005\",\"rsaCaCrt\":\"-----BEGIN CERTIFICATE-----\\n\
MIIDUDCCAjgCCQCmVPUErMYmCjANBgkqhkiG9w0BAQUFADBqMQswCQYDVQQGEwJD\\n\
TjETMBEGA1UECAwKU29tZS1TdGF0ZTEOMAwGA1UECgwFYmFpZHUxGDAWBgNVBAMM\\n\
DyouaW90LmJhaWR1LmNvbTEcMBoGCSqGSIb3DQEJARYNaW90QGJhaWR1LmNvbTAe\\n\
Fw0xNjAzMTEwMzMwNDlaFw0yNjAzMDkwMzMwNDlaMGoxCzAJBgNVBAYTAkNOMRMw\\n\
EQYDVQQIDApTb21lLVN0YXRlMQ4wDAYDVQQKDAViYWlkdTEYMBYGA1UEAwwPKi5p\\n\
b3QuYmFpZHUuY29tMRwwGgYJKoZIhvcNAQkBFg1pb3RAYmFpZHUuY29tMIIBIjAN\\n\
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtbhIeiN7pznzuMwsLKQj2xB02+51\\n\
OvCJ5d116ZFLjecp9qtllqOfN7bm+AJa5N2aAHJtsetcTHMitY4dtGmOpw4dlGqx\\n\
luoz50kWJWQjVR+z6DLPnGE4uELOS8vbKHUoYPPQTT80eNVnl9S9h/l7DcjEAJYC\\n\
IYJbf6+K9x+Ti9VRChvWcvgZQHMRym9j1g/7CKGMCIwkC+6ihkGD/XG40r7KRCyH\\n\
bD53KnBjBO9FH4IL3rGlZWKWzMw3zC6RTS2ekfEsgAtYDvROKd4rNs+uDU9xaBLO\\n\
dXTl5uxgudH2VnVzWtj09OUbBtXcQFD2IhmOl20BrckYul+HEIMR0oDibwIDAQAB\\n\
MA0GCSqGSIb3DQEBBQUAA4IBAQCzTTH91jNh/uYBEFekSVNg1h1kPSujlwEDDf/W\\n\
pjqPJPqrZvW0w0cmYsYibNDy985JB87MJMfJVESG/v0Y/YbvcnRoi5gAenWXQNL4\\n\
h2hf08A5wEQfLO/EaD1GTH3OIierKYZ6GItGrz4uFKHV5fTMiflABCdu37ALGjrA\\n\
rIjwjxQG6WwLr9468hkKrWNG3dMBHKvmqO8x42sZOFRJMkqBbKzaBd1uW4xY5XwM\\n\
S1QX56tVrgO0A3S+4dEg5uiLVN4YVP/Vqh4SMtYkL7ZZiZAxD9GtNnhRyFsWlC2r\\n\
OVSdXs1ttZxEaEBGUl7tgsBte556BIvufZX+BXGyycVJdBu3\\n\
-----END CERTIFICATE-----\\n\",\"macId\":\"\",\"version\":3048}";

#elif (DUER_PROFILE_UUID == 10)
SDRAM_DATA_SECTION static const char duer_profile[] =
    "{\"configures\":\"{}\",\"bindToken\":\"387a9af9d35fc29382b2795a8d15340c\",\"coapPort\":443,\
\"token\":\"58kx4zPrWiVaKT1XVKwEypZt9r8hFQyi\",\"serverAddr\":\"device.iot.baidu.com\",\"lwm2mPort\":443,\
\"uuid\":\"1876000000000a\",\"rsaCaCrt\":\"-----BEGIN CERTIFICATE-----\\n\
MIIDUDCCAjgCCQCmVPUErMYmCjANBgkqhkiG9w0BAQUFADBqMQswCQYDVQQGEwJD\\n\
TjETMBEGA1UECAwKU29tZS1TdGF0ZTEOMAwGA1UECgwFYmFpZHUxGDAWBgNVBAMM\\n\
DyouaW90LmJhaWR1LmNvbTEcMBoGCSqGSIb3DQEJARYNaW90QGJhaWR1LmNvbTAe\\n\
Fw0xNjAzMTEwMzMwNDlaFw0yNjAzMDkwMzMwNDlaMGoxCzAJBgNVBAYTAkNOMRMw\\n\
EQYDVQQIDApTb21lLVN0YXRlMQ4wDAYDVQQKDAViYWlkdTEYMBYGA1UEAwwPKi5p\\n\
b3QuYmFpZHUuY29tMRwwGgYJKoZIhvcNAQkBFg1pb3RAYmFpZHUuY29tMIIBIjAN\\n\
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtbhIeiN7pznzuMwsLKQj2xB02+51\\n\
OvCJ5d116ZFLjecp9qtllqOfN7bm+AJa5N2aAHJtsetcTHMitY4dtGmOpw4dlGqx\\n\
luoz50kWJWQjVR+z6DLPnGE4uELOS8vbKHUoYPPQTT80eNVnl9S9h/l7DcjEAJYC\\n\
IYJbf6+K9x+Ti9VRChvWcvgZQHMRym9j1g/7CKGMCIwkC+6ihkGD/XG40r7KRCyH\\n\
bD53KnBjBO9FH4IL3rGlZWKWzMw3zC6RTS2ekfEsgAtYDvROKd4rNs+uDU9xaBLO\\n\
dXTl5uxgudH2VnVzWtj09OUbBtXcQFD2IhmOl20BrckYul+HEIMR0oDibwIDAQAB\\n\
MA0GCSqGSIb3DQEBBQUAA4IBAQCzTTH91jNh/uYBEFekSVNg1h1kPSujlwEDDf/W\\n\
pjqPJPqrZvW0w0cmYsYibNDy985JB87MJMfJVESG/v0Y/YbvcnRoi5gAenWXQNL4\\n\
h2hf08A5wEQfLO/EaD1GTH3OIierKYZ6GItGrz4uFKHV5fTMiflABCdu37ALGjrA\\n\
rIjwjxQG6WwLr9468hkKrWNG3dMBHKvmqO8x42sZOFRJMkqBbKzaBd1uW4xY5XwM\\n\
S1QX56tVrgO0A3S+4dEg5uiLVN4YVP/Vqh4SMtYkL7ZZiZAxD9GtNnhRyFsWlC2r\\n\
OVSdXs1ttZxEaEBGUl7tgsBte556BIvufZX+BXGyycVJdBu3\\n\
-----END CERTIFICATE-----\\n\",\"macId\":\"\",\"version\":11402}";
#elif (DUER_PROFILE_UUID == 6)
SDRAM_DATA_SECTION static const char duer_profile[] =
    "{\"configures\":\"{}\",\"bindToken\":\"379979429fad5d2504c27b8fcf533dca\",\"coapPort\":443,\
\"token\":\"AWb8nsaC\",\"serverAddr\":\"device.iot.baidu.com\",\"lwm2mPort\":443,\
\"uuid\":\"087c0000000006\",\"rsaCaCrt\":\"-----BEGIN CERTIFICATE-----\\n\
MIIDUDCCAjgCCQCmVPUErMYmCjANBgkqhkiG9w0BAQUFADBqMQswCQYDVQQGEwJD\\n\
TjETMBEGA1UECAwKU29tZS1TdGF0ZTEOMAwGA1UECgwFYmFpZHUxGDAWBgNVBAMM\\n\
DyouaW90LmJhaWR1LmNvbTEcMBoGCSqGSIb3DQEJARYNaW90QGJhaWR1LmNvbTAe\\n\
Fw0xNjAzMTEwMzMwNDlaFw0yNjAzMDkwMzMwNDlaMGoxCzAJBgNVBAYTAkNOMRMw\\n\
EQYDVQQIDApTb21lLVN0YXRlMQ4wDAYDVQQKDAViYWlkdTEYMBYGA1UEAwwPKi5p\\n\
b3QuYmFpZHUuY29tMRwwGgYJKoZIhvcNAQkBFg1pb3RAYmFpZHUuY29tMIIBIjAN\\n\
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtbhIeiN7pznzuMwsLKQj2xB02+51\\n\
OvCJ5d116ZFLjecp9qtllqOfN7bm+AJa5N2aAHJtsetcTHMitY4dtGmOpw4dlGqx\\n\
luoz50kWJWQjVR+z6DLPnGE4uELOS8vbKHUoYPPQTT80eNVnl9S9h/l7DcjEAJYC\\n\
IYJbf6+K9x+Ti9VRChvWcvgZQHMRym9j1g/7CKGMCIwkC+6ihkGD/XG40r7KRCyH\\n\
bD53KnBjBO9FH4IL3rGlZWKWzMw3zC6RTS2ekfEsgAtYDvROKd4rNs+uDU9xaBLO\\n\
dXTl5uxgudH2VnVzWtj09OUbBtXcQFD2IhmOl20BrckYul+HEIMR0oDibwIDAQAB\\n\
MA0GCSqGSIb3DQEBBQUAA4IBAQCzTTH91jNh/uYBEFekSVNg1h1kPSujlwEDDf/W\\n\
pjqPJPqrZvW0w0cmYsYibNDy985JB87MJMfJVESG/v0Y/YbvcnRoi5gAenWXQNL4\\n\
h2hf08A5wEQfLO/EaD1GTH3OIierKYZ6GItGrz4uFKHV5fTMiflABCdu37ALGjrA\\n\
rIjwjxQG6WwLr9468hkKrWNG3dMBHKvmqO8x42sZOFRJMkqBbKzaBd1uW4xY5XwM\\n\
S1QX56tVrgO0A3S+4dEg5uiLVN4YVP/Vqh4SMtYkL7ZZiZAxD9GtNnhRyFsWlC2r\\n\
OVSdXs1ttZxEaEBGUl7tgsBte556BIvufZX+BXGyycVJdBu3\\n\
-----END CERTIFICATE-----\\n\",\"macId\":\"\",\"version\":4827}";


#elif (DUER_PROFILE_UUID == 7)
SDRAM_DATA_SECTION static const char duer_profile[] =
    "{\"configures\":\"{}\",\"bindToken\":\"844bb5ec1f1846cf4fa90ccc1053485e\",\"coapPort\":443,\
\"token\":\"NNbUfh6d\",\"serverAddr\":\"device.iot.baidu.com\",\"lwm2mPort\":443,\
\"uuid\":\"087c0000000007\",\"rsaCaCrt\":\"-----BEGIN CERTIFICATE-----\\n\
MIIDUDCCAjgCCQCmVPUErMYmCjANBgkqhkiG9w0BAQUFADBqMQswCQYDVQQGEwJD\\n\
TjETMBEGA1UECAwKU29tZS1TdGF0ZTEOMAwGA1UECgwFYmFpZHUxGDAWBgNVBAMM\\n\
DyouaW90LmJhaWR1LmNvbTEcMBoGCSqGSIb3DQEJARYNaW90QGJhaWR1LmNvbTAe\\n\
Fw0xNjAzMTEwMzMwNDlaFw0yNjAzMDkwMzMwNDlaMGoxCzAJBgNVBAYTAkNOMRMw\\n\
EQYDVQQIDApTb21lLVN0YXRlMQ4wDAYDVQQKDAViYWlkdTEYMBYGA1UEAwwPKi5p\\n\
b3QuYmFpZHUuY29tMRwwGgYJKoZIhvcNAQkBFg1pb3RAYmFpZHUuY29tMIIBIjAN\\n\
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtbhIeiN7pznzuMwsLKQj2xB02+51\\n\
OvCJ5d116ZFLjecp9qtllqOfN7bm+AJa5N2aAHJtsetcTHMitY4dtGmOpw4dlGqx\\n\
luoz50kWJWQjVR+z6DLPnGE4uELOS8vbKHUoYPPQTT80eNVnl9S9h/l7DcjEAJYC\\n\
IYJbf6+K9x+Ti9VRChvWcvgZQHMRym9j1g/7CKGMCIwkC+6ihkGD/XG40r7KRCyH\\n\
bD53KnBjBO9FH4IL3rGlZWKWzMw3zC6RTS2ekfEsgAtYDvROKd4rNs+uDU9xaBLO\\n\
dXTl5uxgudH2VnVzWtj09OUbBtXcQFD2IhmOl20BrckYul+HEIMR0oDibwIDAQAB\\n\
MA0GCSqGSIb3DQEBBQUAA4IBAQCzTTH91jNh/uYBEFekSVNg1h1kPSujlwEDDf/W\\n\
pjqPJPqrZvW0w0cmYsYibNDy985JB87MJMfJVESG/v0Y/YbvcnRoi5gAenWXQNL4\\n\
h2hf08A5wEQfLO/EaD1GTH3OIierKYZ6GItGrz4uFKHV5fTMiflABCdu37ALGjrA\\n\
rIjwjxQG6WwLr9468hkKrWNG3dMBHKvmqO8x42sZOFRJMkqBbKzaBd1uW4xY5XwM\\n\
S1QX56tVrgO0A3S+4dEg5uiLVN4YVP/Vqh4SMtYkL7ZZiZAxD9GtNnhRyFsWlC2r\\n\
OVSdXs1ttZxEaEBGUl7tgsBte556BIvufZX+BXGyycVJdBu3\\n\
-----END CERTIFICATE-----\\n\",\"macId\":\"\",\"version\":3048}";

#elif (DUER_PROFILE_UUID == 8)
SDRAM_DATA_SECTION static const char duer_profile[] =
    "{\"configures\":\"{}\",\"bindToken\":\"3e300a435db22aed92c995ff4b0235c4\",\"coapPort\":443,\
\"token\":\"1cRdRNzc\",\"serverAddr\":\"device.iot.baidu.com\",\"lwm2mPort\":443,\
\"uuid\":\"087c0000000008\",\"rsaCaCrt\":\"-----BEGIN CERTIFICATE-----\\n\
MIIDUDCCAjgCCQCmVPUErMYmCjANBgkqhkiG9w0BAQUFADBqMQswCQYDVQQGEwJD\\n\
TjETMBEGA1UECAwKU29tZS1TdGF0ZTEOMAwGA1UECgwFYmFpZHUxGDAWBgNVBAMM\\n\
DyouaW90LmJhaWR1LmNvbTEcMBoGCSqGSIb3DQEJARYNaW90QGJhaWR1LmNvbTAe\\n\
Fw0xNjAzMTEwMzMwNDlaFw0yNjAzMDkwMzMwNDlaMGoxCzAJBgNVBAYTAkNOMRMw\\n\
EQYDVQQIDApTb21lLVN0YXRlMQ4wDAYDVQQKDAViYWlkdTEYMBYGA1UEAwwPKi5p\\n\
b3QuYmFpZHUuY29tMRwwGgYJKoZIhvcNAQkBFg1pb3RAYmFpZHUuY29tMIIBIjAN\\n\
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtbhIeiN7pznzuMwsLKQj2xB02+51\\n\
OvCJ5d116ZFLjecp9qtllqOfN7bm+AJa5N2aAHJtsetcTHMitY4dtGmOpw4dlGqx\\n\
luoz50kWJWQjVR+z6DLPnGE4uELOS8vbKHUoYPPQTT80eNVnl9S9h/l7DcjEAJYC\\n\
IYJbf6+K9x+Ti9VRChvWcvgZQHMRym9j1g/7CKGMCIwkC+6ihkGD/XG40r7KRCyH\\n\
bD53KnBjBO9FH4IL3rGlZWKWzMw3zC6RTS2ekfEsgAtYDvROKd4rNs+uDU9xaBLO\\n\
dXTl5uxgudH2VnVzWtj09OUbBtXcQFD2IhmOl20BrckYul+HEIMR0oDibwIDAQAB\\n\
MA0GCSqGSIb3DQEBBQUAA4IBAQCzTTH91jNh/uYBEFekSVNg1h1kPSujlwEDDf/W\\n\
pjqPJPqrZvW0w0cmYsYibNDy985JB87MJMfJVESG/v0Y/YbvcnRoi5gAenWXQNL4\\n\
h2hf08A5wEQfLO/EaD1GTH3OIierKYZ6GItGrz4uFKHV5fTMiflABCdu37ALGjrA\\n\
rIjwjxQG6WwLr9468hkKrWNG3dMBHKvmqO8x42sZOFRJMkqBbKzaBd1uW4xY5XwM\\n\
S1QX56tVrgO0A3S+4dEg5uiLVN4YVP/Vqh4SMtYkL7ZZiZAxD9GtNnhRyFsWlC2r\\n\
OVSdXs1ttZxEaEBGUl7tgsBte556BIvufZX+BXGyycVJdBu3\\n\
-----END CERTIFICATE-----\\n\",\"macId\":\"\",\"version\":3048}";

#elif (DUER_PROFILE_UUID == 9)
SDRAM_DATA_SECTION static const char duer_profile[] =
    "{\"configures\":\"{}\",\"bindToken\":\"6567f68c186b703a33eeace4d63236a4\",\"coapPort\":443,\
\"token\":\"2AQ5XG0T\",\"serverAddr\":\"device.iot.baidu.com\",\"lwm2mPort\":443,\
\"uuid\":\"087c0000000009\",\"rsaCaCrt\":\"-----BEGIN CERTIFICATE-----\\n\
MIIDUDCCAjgCCQCmVPUErMYmCjANBgkqhkiG9w0BAQUFADBqMQswCQYDVQQGEwJD\\n\
TjETMBEGA1UECAwKU29tZS1TdGF0ZTEOMAwGA1UECgwFYmFpZHUxGDAWBgNVBAMM\\n\
DyouaW90LmJhaWR1LmNvbTEcMBoGCSqGSIb3DQEJARYNaW90QGJhaWR1LmNvbTAe\\n\
Fw0xNjAzMTEwMzMwNDlaFw0yNjAzMDkwMzMwNDlaMGoxCzAJBgNVBAYTAkNOMRMw\\n\
EQYDVQQIDApTb21lLVN0YXRlMQ4wDAYDVQQKDAViYWlkdTEYMBYGA1UEAwwPKi5p\\n\
b3QuYmFpZHUuY29tMRwwGgYJKoZIhvcNAQkBFg1pb3RAYmFpZHUuY29tMIIBIjAN\\n\
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtbhIeiN7pznzuMwsLKQj2xB02+51\\n\
OvCJ5d116ZFLjecp9qtllqOfN7bm+AJa5N2aAHJtsetcTHMitY4dtGmOpw4dlGqx\\n\
luoz50kWJWQjVR+z6DLPnGE4uELOS8vbKHUoYPPQTT80eNVnl9S9h/l7DcjEAJYC\\n\
IYJbf6+K9x+Ti9VRChvWcvgZQHMRym9j1g/7CKGMCIwkC+6ihkGD/XG40r7KRCyH\\n\
bD53KnBjBO9FH4IL3rGlZWKWzMw3zC6RTS2ekfEsgAtYDvROKd4rNs+uDU9xaBLO\\n\
dXTl5uxgudH2VnVzWtj09OUbBtXcQFD2IhmOl20BrckYul+HEIMR0oDibwIDAQAB\\n\
MA0GCSqGSIb3DQEBBQUAA4IBAQCzTTH91jNh/uYBEFekSVNg1h1kPSujlwEDDf/W\\n\
pjqPJPqrZvW0w0cmYsYibNDy985JB87MJMfJVESG/v0Y/YbvcnRoi5gAenWXQNL4\\n\
h2hf08A5wEQfLO/EaD1GTH3OIierKYZ6GItGrz4uFKHV5fTMiflABCdu37ALGjrA\\n\
rIjwjxQG6WwLr9468hkKrWNG3dMBHKvmqO8x42sZOFRJMkqBbKzaBd1uW4xY5XwM\\n\
S1QX56tVrgO0A3S+4dEg5uiLVN4YVP/Vqh4SMtYkL7ZZiZAxD9GtNnhRyFsWlC2r\\n\
OVSdXs1ttZxEaEBGUl7tgsBte556BIvufZX+BXGyycVJdBu3\\n\
-----END CERTIFICATE-----\\n\",\"macId\":\"\",\"version\":3048}";
#elif (DUER_PROFILE_UUID == 11)
SDRAM_DATA_SECTION static const char duer_profile[] =
    "{\"configures\":\"{}\",\"bindToken\":\"bbe16892e6cee7b064ecd2a89b9fa4b5\",\"coapPort\":443,\
\"token\":\"3Eve5A9Uyinsd7YtWcQns9DFpvUctCWY\",\"serverAddr\":\"device.iot.baidu.com\",\"lwm2mPort\":443,\
\"uuid\":\"1a1c000000000a\",\"rsaCaCrt\":\"-----BEGIN CERTIFICATE-----\\n\
MIIDUDCCAjgCCQCmVPUErMYmCjANBgkqhkiG9w0BAQUFADBqMQswCQYDVQQGEwJD\\n\
TjETMBEGA1UECAwKU29tZS1TdGF0ZTEOMAwGA1UECgwFYmFpZHUxGDAWBgNVBAMM\\n\
DyouaW90LmJhaWR1LmNvbTEcMBoGCSqGSIb3DQEJARYNaW90QGJhaWR1LmNvbTAe\\n\
Fw0xNjAzMTEwMzMwNDlaFw0yNjAzMDkwMzMwNDlaMGoxCzAJBgNVBAYTAkNOMRMw\\n\
EQYDVQQIDApTb21lLVN0YXRlMQ4wDAYDVQQKDAViYWlkdTEYMBYGA1UEAwwPKi5p\\n\
b3QuYmFpZHUuY29tMRwwGgYJKoZIhvcNAQkBFg1pb3RAYmFpZHUuY29tMIIBIjAN\\n\
BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtbhIeiN7pznzuMwsLKQj2xB02+51\\n\
OvCJ5d116ZFLjecp9qtllqOfN7bm+AJa5N2aAHJtsetcTHMitY4dtGmOpw4dlGqx\\n\
luoz50kWJWQjVR+z6DLPnGE4uELOS8vbKHUoYPPQTT80eNVnl9S9h/l7DcjEAJYC\\n\
IYJbf6+K9x+Ti9VRChvWcvgZQHMRym9j1g/7CKGMCIwkC+6ihkGD/XG40r7KRCyH\\n\
bD53KnBjBO9FH4IL3rGlZWKWzMw3zC6RTS2ekfEsgAtYDvROKd4rNs+uDU9xaBLO\\n\
dXTl5uxgudH2VnVzWtj09OUbBtXcQFD2IhmOl20BrckYul+HEIMR0oDibwIDAQAB\\n\
MA0GCSqGSIb3DQEBBQUAA4IBAQCzTTH91jNh/uYBEFekSVNg1h1kPSujlwEDDf/W\\n\
pjqPJPqrZvW0w0cmYsYibNDy985JB87MJMfJVESG/v0Y/YbvcnRoi5gAenWXQNL4\\n\
h2hf08A5wEQfLO/EaD1GTH3OIierKYZ6GItGrz4uFKHV5fTMiflABCdu37ALGjrA\\n\
rIjwjxQG6WwLr9468hkKrWNG3dMBHKvmqO8x42sZOFRJMkqBbKzaBd1uW4xY5XwM\\n\
S1QX56tVrgO0A3S+4dEg5uiLVN4YVP/Vqh4SMtYkL7ZZiZAxD9GtNnhRyFsWlC2r\\n\
OVSdXs1ttZxEaEBGUl7tgsBte556BIvufZX+BXGyycVJdBu3\\n\
-----END CERTIFICATE-----\\n\",\"macId\":\"\",\"version\":12527}";
#endif
#endif

#if VOICE_FROM_SDCARD
static void duer_test_task(void *pvParameters);
#endif
void duer_test_start();



static void duer_dcs_init()
{
    duer_dcs_framework_init();
    duer_dcs_voice_input_init();
    duer_dcs_voice_output_init();
    duer_dcs_speaker_control_init();
    duer_dcs_audio_player_init();
    //duer_dcs_screen_init();
    duer_alert_init();
    duer_dcs_sync_state();
}

static void duer_record_event_callback(int event, const void *data, size_t size)
{
    //DUER_LOGD("record callback : %d, size %d", event,size);
    char *null_voice = NULL;
    switch (event) {
    case REC_START:
    {
        DUER_LOGI("start send voice: %s", (const char *)data);
        duer_dcs_on_listen_started();
        duer_voice_start(16000);
        break;
    }
    case REC_DATA:
        duer_voice_send(data, size);
        break;
    case REC_STOP:
        DUER_LOGD("stop send voice: %s", (const char *)data);
        duer_voice_stop();
        break;
    default:
        DUER_LOGE("event not supported: %d!", event);
        break;
    }
}

static void duer_voice_delay_func_callback(duer_u32_t delay){
	DUER_LOGI("voice delay %d",delay);
	duer_voice_terminate();
	duer_dcs_stop_listen_handler();
}

static void duer_event_hook(duer_event_t *event)
{
    int ret = DUER_OK;

    if (!event) {
        DUER_LOGE("NULL event!!!");
    }

    DUER_LOGI("event: %d", event->_event);

    switch (event->_event) {
    case DUER_EVENT_STARTED:

#if VOICE_FROM_SDCARD
        xTaskCreate(&duer_test_task, "duer_test_task", 4096, NULL, 5, &xHandle);
#endif
#if VOICE_FROM_AUDIO_CODEC
        duer_record_set_event_callback(duer_record_event_callback);
#endif
		duer_voice_set_delay_threshold(DUER_VOICE_DELAY, duer_voice_delay_func_callback);

		duerapp_gpio_led_mode(DUER_LED_OFF);
        ret = duer_report_device_info();
        if (ret != DUER_OK) {
            DUER_LOGE("Report device info failed ret:%d", ret);
        }
        
        duer_dcs_init();
        duer_initialize_ota();
	ret = duer_ota_notify_package_info();
        if (ret != DUER_OK) {
            DUER_LOGE("Report package info failed ret:%d", ret);
        }

	duer_voice_terminate();
	//duer_control_point_init();
        pDuerAppInfo->started = true;

        break;
    case DUER_EVENT_STOPPED:
        MediaURLFree();
        duer_voice_terminate();
        pDuerAppInfo->started = false;
        if (xHandle) {
            vTaskDelete(xHandle);
            xHandle = NULL;
        }
#ifdef OPEN_DUER_AUTO_RESTART
        duer_test_start();
#endif
    }
}


extern QueueHandle_t voice_result_queue;
extern int duer_init_play(void);

#if VOICE_FROM_SDCARD
void duer_test_task(void *pvParameters)
{
    (void)pvParameters;

    duer_record_set_event_callback(duer_record_event_callback);

    while (1) {
        duer_record_all();
        vTaskDelay(20000 / portTICK_PERIOD_MS);
    }
}
#endif

// It called in duerapp_wifi_config.c, when the Wi-Fi is connected.
void duer_test_start()
{

    duer_voice_terminate();

#if PROFILE_FROM_SDCARD
    const char *data = duer_load_profile(PROFILE_PATH);
    if (data == NULL) {
        DUER_LOGE("load profile failed!");
        return;
    }

    DUER_LOGD("profile: \n%s", data);

    // We start the duer with duer_start(), when network connected.
    duer_start(data, strlen(data));

    free((void *)data);
#else
    // We start the duer with duer_start(), when network connected.
    duer_start(duer_profile, strlen(duer_profile));

#endif
}

static void duer_restart_timer_expired(void *param)
{
    sys_reset();
}

static TaskHandle_t xNtpHandle = NULL;
void ntp_task() {
    DuerTime time;
    int ret = -1;

    vTaskDelay(5000 / portTICK_PERIOD_MS); // wait network ready

    ret = duer_ntp_client(NULL, 0, &time, NULL);

    while (ret < 0) {
        ret = duer_ntp_client(NULL, 0, &time, NULL);
    }

    if (xNtpHandle) {
        vTaskDelete(xNtpHandle);
        xNtpHandle = NULL;
    }
}
static void get_ntp_time() {
    xTaskCreate(&ntp_task, "ntp_task", 4096, NULL, 5, &xNtpHandle);
}

void initialize_duerapp(){	
	
	pDuerAppInfo->mute = false;
	pDuerAppInfo->started = false;
	pDuerAppInfo->audio_has_inited = 0;
	pDuerAppInfo->duer_rec_state = RECORDER_STOP;
	
	if(!pDuerAppInfo->xSemaphoreHTTPAudioThread){
		pDuerAppInfo->xSemaphoreHTTPAudioThread = xSemaphoreCreateMutex();
	}
	
	if(!pDuerAppInfo->xSemaphoreMediaPlayDone)
		pDuerAppInfo->xSemaphoreMediaPlayDone = xSemaphoreCreateBinary();
	
	if(!pDuerAppInfo->xSemaphoreHttpDownloadDone)
		pDuerAppInfo->xSemaphoreHttpDownloadDone = xSemaphoreCreateBinary();
}

void example_duer_thread()
{

    initialize_wifi();

#if (PROFILE_FROM_SDCARD || VOICE_FROM_SDCARD)
    initialize_sdcard();
#endif

	initialize_duerapp();

    initialize_audio();

    initialize_sdcard_record();

    // init CA
    duer_initialize();

    // Set the Duer Event Callback
    duer_set_event_callback(duer_event_hook);

    initialize_deviceInfo();

    g_restart_timer = duer_timer_acquire(duer_restart_timer_expired, NULL, DUER_TIMER_ONCE);

    duer_test_start();
    vTaskDelete(NULL);
}

void example_duer()
{
   if (xTaskCreate(example_duer_thread, ((const char*)"example_duer_thread"), 1024, NULL, tskIDLE_PRIORITY + 3, NULL) != pdPASS)
        DUER_LOGE("Create example duer thread failed!");  
}
