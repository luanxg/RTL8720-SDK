/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define TAG "lightduer_os"

#include "lightduer_coap_defs.h"
#include "lightduer_jni.h"
#include "lightduer_jni_controlpoint.h"
#include "lightduer_jni_dcs.h"
#include "lightduer_jni_event.h"
#include "lightduer_jni_log.h"
#include "lightduer_jni_utils.h"
#include "lightduer_jni_voice.h"
#include "lightduer_jni_vad.h"
#include "lightduer_memory.h"
#include "lightduer_lib.h"
#include "lightduer_voice.h"
#include "lightduer_system_info.h"

duer_system_static_info_t g_system_static_info = {
    .os_version       = "Android x.x.x",
    .sw_version       = "unknown",
    .brand            = "unknown",
    .hardware_version = "unknown",
    .equipment_type   = "unknown",
    .ram_size         = 0,
    .rom_size         = 0,
};

static void lightduer_os_jni_initialize(JNIEnv *env, jclass clazz, jobject *jinfo)
{
    if (jinfo) {
        jclass info_clazz = NULL;
        jfieldID id = NULL;
        jstring jstr = NULL;
        const char *str = NULL;

        do {
            info_clazz = (*env)->FindClass(env,
                "com/baidu/lightduer/lib/jni/utils/LightduerSystemStaticInfo");
            if (info_clazz == NULL) {
                LOGE("Can't find LightduerSystemStaticInfo class.");
                break;
            }

            id = (*env)->GetFieldID(env, info_clazz, "mOsVersion", "Ljava/lang/String;");
            if (id == NULL) {
                LOGE("Can't find mOsVersion field.");
                break;
            }

            jstr = (jstring)(*env)->GetObjectField(env, jinfo, id);
            str = (*env)->GetStringUTFChars(env, jstr, NULL);
            DUER_SNPRINTF(g_system_static_info.os_version, OS_VERSION_LEN, "%s", str);

            (*env)->ReleaseStringUTFChars(env, jstr, str);
            (*env)->DeleteLocalRef(env, jstr);

            id = (*env)->GetFieldID(env, info_clazz, "mSwVersion", "Ljava/lang/String;");
            if (id == NULL) {
                LOGE("Can't find mSwVersion field.");
                break;
            }

            jstr = (jstring)(*env)->GetObjectField(env, jinfo, id);
            str = (*env)->GetStringUTFChars(env, jstr, NULL);
            DUER_SNPRINTF(g_system_static_info.sw_version, SW_VERSION_LEN, "%s", str);

            (*env)->ReleaseStringUTFChars(env, jstr, str);
            (*env)->DeleteLocalRef(env, jstr);

            id = (*env)->GetFieldID(env, info_clazz, "mBrand", "Ljava/lang/String;");
            if (id == NULL) {
                LOGE("Can't find mBrand field.");
                break;
            }

            jstr = (jstring)(*env)->GetObjectField(env, jinfo, id);
            str = (*env)->GetStringUTFChars(env, jstr, NULL);
            DUER_SNPRINTF(g_system_static_info.brand, BRAND_LEN, "%s", str);

            (*env)->ReleaseStringUTFChars(env, jstr, str);
            (*env)->DeleteLocalRef(env, jstr);

            id = (*env)->GetFieldID(env, info_clazz, "mEquipmentType", "Ljava/lang/String;");
            if (id == NULL) {
                LOGE("Can't find mEquipmentType field.");
                break;
            }

            jstr = (jstring)(*env)->GetObjectField(env, jinfo, id);
            str = (*env)->GetStringUTFChars(env, jstr, NULL);
            DUER_SNPRINTF(g_system_static_info.equipment_type, EQUIPMENT_TYPE_LEN, "%s", str);

            (*env)->ReleaseStringUTFChars(env, jstr, str);
            (*env)->DeleteLocalRef(env, jstr);

            id = (*env)->GetFieldID(env, info_clazz, "mHardwareVersion", "Ljava/lang/String;");
            if (id == NULL) {
                LOGE("Can't find mHardwareVersion field.");
                break;
            }

            jstr = (jstring)(*env)->GetObjectField(env, jinfo, id);
            str = (*env)->GetStringUTFChars(env, jstr, NULL);
            DUER_SNPRINTF(g_system_static_info.hardware_version, HARDWARE_VERSION_LEN, "%s", str);

            (*env)->ReleaseStringUTFChars(env, jstr, str);
            (*env)->DeleteLocalRef(env, jstr);

            id = (*env)->GetFieldID(env, info_clazz, "mRamSize", "I");
            if (id == NULL) {
                LOGE("Can't find mRamSize field.");
                break;
            }
            g_system_static_info.ram_size = (uint32_t)(*env)->GetIntField(env, jinfo, id);

            id = (*env)->GetFieldID(env, info_clazz, "mRomSize", "I");
            if (id == NULL) {
                LOGE("Can't find mRomSize field.");
                break;
            }
            g_system_static_info.rom_size = (uint32_t)(*env)->GetIntField(env, jinfo, id);
        } while (0);

        if (info_clazz) {
            (*env)->DeleteLocalRef(env, info_clazz);
        }
    }
    duer_initialize();
}

static void duer_report_package_info()
{
    baidu_json *data = NULL;
    baidu_json *os_info = NULL;
    baidu_json *package_info = NULL;

    do {
        data = baidu_json_CreateObject();
        if (data == NULL) {
            LOGE("Create json object failed");
            break;
        }

        package_info = baidu_json_CreateObject();
        if (package_info == NULL) {
            LOGE("Create json object failed");
            break;
        }
        baidu_json_AddItemToObject(data, "package_info", package_info);

        os_info = baidu_json_CreateObject();
        if (os_info == NULL) {
            LOGE("Create json object failed");
            break;
        }

        // only version item of os is useful
        baidu_json_AddStringToObject(os_info, "name", "Android");
        baidu_json_AddStringToObject(os_info, "developer", "lishun");
        baidu_json_AddStringToObject(os_info, "staged_version", "unknown");
        baidu_json_AddStringToObject(os_info, "version", g_system_static_info.sw_version);
        baidu_json_AddItemToObject(package_info, "os", os_info);

        baidu_json_AddStringToObject(package_info, "version", "unknown");
        baidu_json_AddStringToObject(package_info, "product", "unknown");
        baidu_json_AddStringToObject(package_info, "batch", "unknown");

        duer_data_report(data);
    } while (0);

    if (data) {
        baidu_json_Delete(data);
    }
}

static void duer_event_hook(duer_event_t *event)
{
    if (!event) {
        LOGE("NULL event!!!\n");
        return;
    }

    LOGD("event: %d\n", event->_event);
    LOGD("event code: %d\n", event->_code);
    int needDetach = 0;
    JNIEnv *jenv = duer_jni_get_JNIEnv(&needDetach);
    if (jenv == NULL) {
        LOGE("can't get jenv\n");
        return;
    }

    duer_jni_event_call_callback(jenv, event);

    if (event->_event == DUER_EVENT_STARTED) {
        duer_report_package_info();
    }

    if (needDetach) {
        duer_jni_detach_current_thread();
    }
}

static void lightduer_os_jni_set_event_listener(JNIEnv *jenv, jclass cls, jobject obj)
{
    int init_result = duer_jni_event_init(jenv, obj);

    if (init_result != DUER_OK) {
        LOGE("init event failed!\n");
        return;
    }

    duer_set_event_callback(duer_event_hook);
}

static void duer_voice_result(struct baidu_json *result)
{
    int needDetach = 0;
    JNIEnv *jenv = duer_jni_get_JNIEnv(&needDetach);

    char *str_result = baidu_json_PrintUnformatted(result);
    LOGD("duer_voice_result:%s \n", str_result);
    jstring jstr_result = (*jenv)->NewStringUTF(jenv, str_result);

    LOGD("before call voice callback \n");
    duer_jni_voice_call_callback(jenv, jstr_result);
    LOGD("after call voice callback \n");

    (*jenv)->DeleteLocalRef(jenv, jstr_result);
    baidu_json_release(str_result);

    if (needDetach) {
        duer_jni_detach_current_thread();
    }
}

static void lightduer_os_jni_set_voice_result_listener(JNIEnv *jenv, jclass clazz, jobject obj)
{
    LOGW("@deprecated dcs3.0 does not to setVoiceResultListener");
    // @deprecated dcs3.0 does not to set the callback
    //
    // int init_result = duer_jni_voice_init(jenv, obj);

    // if (init_result != DUER_OK) {
    //     LOGE("voice init failed\n");
    //     return;
    // }

    // duer_voice_set_result_callback(duer_voice_result);
}

static jint lightduer_os_jni_start_server_by_profile(JNIEnv *jenv, jclass clazz, jbyteArray byteArray)
{
    jbyte *data = (*jenv)->GetByteArrayElements(jenv, byteArray, 0);
    jsize size = (*jenv)->GetArrayLength(jenv, byteArray);
    LOGI("data:%s \n", data);
    LOGI("size:%d \n", size);

    int result = duer_start(data, size);

    (*jenv)->ReleaseByteArrayElements(jenv, byteArray, data, 0);
    return result;
}

static jint lightduer_os_jni_stop_server(JNIEnv *env, jclass clazz)
{
    return duer_stop();
}

static void lightduer_os_jni_set_voice_mode(JNIEnv *env, jclass clazz, jint mode)
{
    LOGI("set voice mode:%d\n", mode);
    switch (mode) {
    case 1:
        duer_voice_set_mode(DUER_VOICE_MODE_CHINESE_TO_ENGLISH);
        break;
    case 2:
        duer_voice_set_mode(DUER_VOICE_MODE_ENGLISH_TO_CHINESE);
        break;
    case 3:
        duer_voice_set_mode(DUER_VOICE_MODE_WCHAT);
        break;
    default:
        duer_voice_set_mode(DUER_VOICE_MODE_DEFAULT);
        break;
    }
}

static jint lightduer_os_jni_get_voice_mode(JNIEnv *env, jclass clazz)
{
    return (jint)duer_voice_get_mode();
}

static jint lightduer_os_jni_start_voice(JNIEnv *env, jclass clazz, jint samplerate)
{
    return duer_voice_start(samplerate);
}

static jint lightduer_os_jni_send_voice_data(JNIEnv *jenv, jclass clazz, jbyteArray voice_data)
{
    jbyte *data = (*jenv)->GetByteArrayElements(jenv, voice_data, 0);
    jsize size = (*jenv)->GetArrayLength(jenv, voice_data);

    jint result = duer_voice_send(data, size);

    (*jenv)->ReleaseByteArrayElements(jenv, voice_data, data, 0);

    return result;
}

static jint lightduer_os_jni_stop_voice(JNIEnv *env, jclass clazz)
{
    return duer_voice_stop();
}

static jint lightduer_os_jni_report_data(JNIEnv *jenv, jclass clazz, jstring data)
{
    int result = DUER_OK;
    baidu_json *json_data = NULL;
    const char *c_data = (*jenv)->GetStringUTFChars(jenv, data, NULL);

    do {
        if (c_data == NULL) {
            LOGE("get c char failed\n");
            result = DUER_ERR_FAILED;
            break;
        }
        json_data = baidu_json_Parse(c_data);
        if (json_data == NULL) {
            LOGE("get json failed\n");
            result = DUER_ERR_FAILED;
            break;
        }
        result = duer_data_report(json_data);
    } while(0);

    if (c_data) {
        (*jenv)->ReleaseStringUTFChars(jenv, data, c_data);
        c_data = NULL;
    }
    if (json_data) {
        baidu_json_Delete(json_data);
        json_data = NULL;
    }

    return result;
}

static jint lightduer_os_jni_add_resource(JNIEnv *jenv, jclass clazz, jobjectArray objArray)
{
    int result = DUER_ERR_FAILED;
    duer_res_t *resources = NULL;

    jsize arraylength = (*jenv)->GetArrayLength(jenv, objArray);
    if (arraylength <= 0) {
        LOGE("no element in array");
        return result;
    }

    do {
        resources = (duer_res_t*)DUER_MALLOC(sizeof(duer_res_t) * arraylength);
        if (!resources) {
            LOGE("resources alloc fail!!\n");
            break;
        }

        DUER_MEMSET(resources, 0, sizeof(duer_res_t) * arraylength);

        result = duer_jni_utils_init(jenv);
        if (result != DUER_OK) {
            LOGE("utils init failed");
            break;
        }

        result = duer_jni_get_native_duer_resources(jenv, objArray, resources, arraylength);

        if (result == DUER_OK) {
            result = duer_add_resources(resources, arraylength);
        }

    } while(0);

    // release the memory allocate in duer_jni_get_native_duer_resources
    // more info see duer_jni_get_native_duer_resources
    if (resources) {
        for (int i = 0; i < arraylength; ++i) {
            if (resources[i].path) {
                DUER_FREE(resources[i].path);
                resources[i].path = NULL;
            }
            if (resources[i].mode == DUER_RES_MODE_STATIC) {
                if (resources[i].res.s_res.data) {
                    DUER_FREE(resources[i].res.s_res.data);
                    resources[i].res.s_res.data = NULL;
                }
            }
        }
        DUER_FREE(resources);
        resources = NULL;
    }

    return result;
}

static jint lightduer_os_jni_send_response(JNIEnv *jenv, jclass clazz, jobject message, jshort msgCode, jbyteArray payload_)
{
    duer_msg_t duer_message;
    DUER_MEMSET(&duer_message, 0, sizeof(duer_msg_t));

    int result = duer_jni_get_native_duer_message(jenv, message, &duer_message);
    do {
        if (result != DUER_OK) {
            LOGE("create native duer_message fail\n");
            break;
        }

        LOGI("msgCode: %d\n", msgCode);
        if (payload_) {
            jbyte *payload = (*jenv)->GetByteArrayElements(jenv, payload_, NULL);
            jsize payload_len = (*jenv)->GetArrayLength(jenv, payload_);
            LOGD("payload_len: %d\n", payload_len);

            duer_response(&duer_message, msgCode, payload, payload_len);

            (*jenv)->ReleaseByteArrayElements(jenv, payload_, payload, 0);
        } else {
            duer_response(&duer_message, msgCode, NULL, 0);
        }
    } while(0);

    // release the buffer allocate in duer_jni_get_native_duer_message
    // see duer_jni_get_native_duer_message for more info
    if (duer_message.path) {
        DUER_FREE(duer_message.path);
        duer_message.path = NULL;
    }
    if (duer_message.query) {
        DUER_FREE(duer_message.query);
        duer_message.query = NULL;
    }
    if(duer_message.token) {
        DUER_FREE(duer_message.token);
        duer_message.token = NULL;
    }
    if (duer_message.payload) {
        DUER_FREE(duer_message.payload);
        duer_message.payload = NULL;
    }

    return result;
}

static jint lightduer_os_jni_set_speex_quality(JNIEnv *env, jclass clazz, jint quality)
{
    return duer_voice_set_speex_quality(quality);
}

static JNINativeMethod g_lightduer_os_methods[] = {
    /* name, signature, funcPtr */
    { "initialize", "(Lcom/baidu/lightduer/lib/jni/utils/LightduerSystemStaticInfo;)V",
            (void*) lightduer_os_jni_initialize },
    { "setEventListener", "(Lcom/baidu/lightduer/lib/jni/event/LightduerEventListener;)V",
            (void*) lightduer_os_jni_set_event_listener },
    { "setVoiceResultListener",
            "(Lcom/baidu/lightduer/lib/jni/voice/LightduerVoiceResultListener;)V",
            (void*) lightduer_os_jni_set_voice_result_listener },
    { "startServerByProfile", "([B)I",
            (void*) lightduer_os_jni_start_server_by_profile },
    { "stopServer", "()I",
            (void*) lightduer_os_jni_stop_server },
    { "setVoiceMode", "(I)V",
            (void*) lightduer_os_jni_set_voice_mode },
    { "getVoiceMode", "()I",
            (void*) lightduer_os_jni_get_voice_mode },
    { "startVoice", "(I)I",
            (void*) lightduer_os_jni_start_voice },
    { "sendVoiceData", "([B)I",
            (void*) lightduer_os_jni_send_voice_data },
    { "stopVoice", "()I",
            (void*) lightduer_os_jni_stop_voice },
    { "reportData", "(Ljava/lang/String;)I",
            (void*) lightduer_os_jni_report_data },
    { "addResource", "([Lcom/baidu/lightduer/lib/jni/controlpoint/LightduerResource;)I",
            (void*) lightduer_os_jni_add_resource },
    { "sendResponse", "(Lcom/baidu/lightduer/lib/jni/utils/LightduerMessage;S[B)I",
            (void*) lightduer_os_jni_send_response },
    { "initFramework", "()I",
            (void*) lightduer_os_jni_init_framework },
    { "initVoiceInput", "(Lcom/baidu/lightduer/lib/jni/dcs/VoiceInputListener;)I",
            (void*) lightduer_os_jni_init_voice_input },
    { "onListenStarted", "()I",
            (void*) lightduer_os_jni_on_listen_started },
    { "onListenStopped", "()I",
            (void*) lightduer_os_jni_on_listen_stopped },
    { "initVoiceOutput", "(Lcom/baidu/lightduer/lib/jni/dcs/VoiceOutputListener;)I",
            (void*) lightduer_os_jni_init_voice_output },
    { "onSpeakFinished", "()I",
            (void*) lightduer_os_jni_on_speak_finished },
    { "initSpeakerControl", "(Lcom/baidu/lightduer/lib/jni/dcs/SpeakerControlListener;)I",
            (void*) lightduer_os_jni_init_speaker_control },
    { "onVolumeChange", "()I",
            (void*) lightduer_os_jni_on_volume_change },
    { "onMute", "()I",
            (void*) lightduer_os_jni_on_mute },
    { "initAudioPlayer", "(Lcom/baidu/lightduer/lib/jni/dcs/AudioPlayerListener;)I",
            (void*) lightduer_os_jni_init_audio_player },
    { "sendPlayControlCommand", "(I)I",
            (void*) lightduer_os_jni_send_play_control_command },
    { "onAudioFinished", "()I",
            (void*) lightduer_os_jni_on_audio_finished },
    { "audioPlayFailed", "(ILjava/lang/String;)I",
            (void*) lightduer_os_jni_audio_play_failed },
    { "audioReportMetadata", "(Ljava/lang/String;)I",
            (void*) lightduer_os_jni_audio_report_metadata },
    { "onAudioStuttered", "(Z)I",
            (void*) lightduer_os_jni_on_audio_stuttered },
    { "syncState", "()I",
            (void*) lightduer_os_jni_sync_state },
    { "closeMultiDialog", "()I",
            (void*) lightduer_os_jni_close_multi_dialog },
    { "initScreenDisplay", "(Lcom/baidu/lightduer/lib/jni/dcs/ScreenDisplayListener;)I",
            (void*) lightduer_os_jni_init_screen_display },
    { "initDeviceControl", "(Lcom/baidu/lightduer/lib/jni/dcs/DeviceControlListener;)I",
            (void*) lightduer_os_jni_init_device_control },
    { "nativeAddDcsDirective", "([Lcom/baidu/lightduer/lib/jni/dcs/DcsDirective;"
            "Lcom/baidu/lightduer/lib/jni/dcs/DcsDirective$IDirectiveHandler;Ljava/lang/String;)I",
            (void*) lightduer_os_jni_add_dcs_directive },
    { "registerClientContextProvider",
            "(Lcom/baidu/lightduer/lib/jni/dcs/DcsClientContextProvider;)I",
            (void*) lightduer_os_jni_register_client_context_provider },
    { "createDcsEvent", "(Ljava/lang/String;Ljava/lang/String;Z)Ljava/lang/String;",
            (void*) lightduer_os_jni_create_dcs_event },
    { "dcsDialogCancel", "()I",
            (void*) lightduer_os_jni_dcs_dialog_cancel },
    { "dcsOnLinkClicked", "(Ljava/lang/String;)I",
            (void*) lightduer_os_jni_dcs_on_link_clicked },
    { "initDcsAlert", "(Lcom/baidu/lightduer/lib/jni/dcs/DcsAlertListener;)I",
            (void*) lightduer_os_jni_init_dcs_alert },
    { "reportDcsAlertEvent", "(Ljava/lang/String;I)I",
            (void*) lightduer_os_jni_report_dcs_alert_event },
    { "dcsCapabilityDeclare", "(I)I",
            (void*) lightduer_os_jni_dcs_capability_declare },
    { "initDeviceVad", "(Lcom/baidu/lightduer/lib/jni/vad/DeviceVadListener;)I",
            (void*) lightduer_os_jni_init_device_vad },
    { "setVadStatus", "(I)I",
            (void*) lightduer_os_jni_set_vad_status },
    { "deviceVad", "([B)I",
            (void*) lightduer_os_jni_device_vad },
    { "setSpeexQuality", "(I)I",
        (void*) lightduer_os_jni_set_speex_quality },
};

static int jniRegisterNativeMethods(JNIEnv *env, const char *className,
    const JNINativeMethod *gMethods, int numMethods)
{
    LOGI("Registering %s's %d native methods...", className, numMethods);

    jclass c = (*env)->FindClass(env, className);
    if (c == NULL) {
        char *msg;
        asprintf(&msg, "Native registration unable to find class '%s'; aborting...", className);
        (*env)->FatalError(env, msg);
    }

    if ((*env)->RegisterNatives(env, c, gMethods, numMethods) < 0) {
        char *msg;
        asprintf(&msg, "RegisterNatives failed for '%s'; aborting...", className);
        (*env)->FatalError(env, msg);
    }

    return 0;
}

int register_java_com_baidu_lightduer_lib_jni_LightduerOsJni(JNIEnv *env)
{
    return jniRegisterNativeMethods(env, "com/baidu/lightduer/lib/jni/LightduerOsJni",
            g_lightduer_os_methods,
            (int) sizeof(g_lightduer_os_methods) / sizeof(g_lightduer_os_methods[0]));
}
