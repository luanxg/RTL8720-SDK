/**
 * copyright (2017) Baidu Inc. All rights reserved.
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

#include "lightduer_jni_dcs.h"
#include "lightduer_jni_voice.h"
#include "lightduer_jni_log.h"
#include "lightduer_types.h"
#include "lightduer_dcs.h"
#include "lightduer_dcs_alert.h"
#include "lightduer_dcs_router.h"
#include "lightduer_memory.h"
#include "lightduer_lib.h"

#define TAG "lightduer_jni_dcs"

static jobject s_audio_player_listener = NULL;
static jobject s_device_control_listener = NULL;
static jobject s_screen_display_listener = NULL;
static jobject s_speaker_control_listener = NULL;
static jobject s_voice_input_listener = NULL;
static jobject s_voice_output_listener = NULL;
static jobject s_directives_handler = NULL;
static jobject s_dcs_alert_listener = NULL;
static jclass s_dcs_alert_info_class = NULL;
static jobject s_client_context_provider = NULL;

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initFramework
 * Signature: ()I
 */
jint lightduer_os_jni_init_framework(JNIEnv *env, jclass clazz)
{
    duer_dcs_framework_init();

    return DUER_OK;
}

void duer_dcs_listen_handler(void)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jmethodID listen = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_voice_input_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_voice_input_listener");
            break;
        }

        listen = (*env)->GetMethodID(env, clazz, "listen", "()I");
        if (listen == NULL) {
            LOGE("method listen is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_voice_input_listener, listen);
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

void duer_dcs_stop_listen_handler(void)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jmethodID stop_listen = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_voice_input_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_voice_input_listener");
            break;
        }

        stop_listen = (*env)->GetMethodID(env, clazz, "stopListen", "()I");
        if (stop_listen == NULL) {
            LOGE("method stopListen is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_voice_input_listener, stop_listen);
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initVoiceInput
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/VoiceInputListener;)I
 */
jint lightduer_os_jni_init_voice_input(JNIEnv *env, jclass clazz, jobject jlistener)
{
    if (s_voice_input_listener != NULL) {
        (*env)->DeleteGlobalRef(env, s_voice_input_listener);
    }

    s_voice_input_listener = (*env)->NewGlobalRef(env, jlistener);
    if (s_voice_input_listener == NULL) {
        LOGE("initVoiceInput failed");
        return DUER_ERR_FAILED;
    }

    duer_dcs_voice_input_init();

    return DUER_OK;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onListenStarted
 * Signature: ()I
 */
jint lightduer_os_jni_on_listen_started(JNIEnv *env, jclass clazz)
{
    return duer_dcs_on_listen_started();
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onListenStarted
 * Signature: ()I
 */
jint lightduer_os_jni_on_listen_stopped(JNIEnv *env, jclass clazz)
{
    return duer_dcs_on_listen_stopped();
}

void duer_dcs_speak_handler(const char *url)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jstring jurl = NULL;
    jclass clazz = NULL;
    jmethodID speak = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    jurl = (*env)->NewStringUTF(env, url);

    do {
        clazz = (*env)->GetObjectClass(env, s_voice_output_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_voice_output_listener");
            break;
        }

        speak = (*env)->GetMethodID(env, clazz, "speak", "(Ljava/lang/String;)I");
        if (speak == NULL) {
            LOGE("method speak is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_voice_output_listener, speak, jurl);
    } while (0);

    if (jurl != NULL) {
        (*env)->DeleteLocalRef(env, jurl);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

void duer_dcs_stop_speak_handler()
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jmethodID stop_speak = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_voice_output_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_voice_output_listener");
            break;
        }

        stop_speak = (*env)->GetMethodID(env, clazz, "stopSpeak", "()I");
        if (stop_speak == NULL) {
            LOGE("method stopSpeak is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_voice_output_listener, stop_speak);
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initVoiceOutput
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/VoiceOutputListener;)I
 */
jint lightduer_os_jni_init_voice_output(JNIEnv *env, jclass clazz, jobject jlistener)
{
    if (s_voice_output_listener != NULL) {
        (*env)->DeleteGlobalRef(env, s_voice_output_listener);
    }

    s_voice_output_listener = (*env)->NewGlobalRef(env, jlistener);
    if (s_voice_output_listener == NULL) {
        LOGE("initVoiceOutput failed");
        return DUER_ERR_FAILED;
    }

    duer_dcs_voice_output_init();

    return DUER_OK;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onSpeakFinished
 * Signature: ()I
 */
jint lightduer_os_jni_on_speak_finished(JNIEnv *env, jclass clazz)
{
    duer_dcs_speech_on_finished();

    return DUER_OK;
}

void duer_dcs_get_speaker_state(int *volume, duer_bool *is_mute)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jmethodID get_state = NULL;
    jobject jstate = NULL;
    jclass ss_clazz = NULL;
    jfieldID volume_id = 0;
    jfieldID is_mute_id = 0;

    if (volume == NULL || is_mute == NULL) {
        LOGE("invalid arguments");
        return;
    }

    *volume = 0;
    *is_mute = DUER_FALSE;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_speaker_control_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_speaker_control_listener");
            break;
        }

        get_state = (*env)->GetMethodID(env, clazz, "getSpeakerState",
                                        "()L"PACKAGE"/dcs/SpeakerState;");
        if (get_state == NULL) {
            LOGE("method getSpeakerState is not found!!\n");
            break;
        }

        jstate = (*env)->CallObjectMethod(env, s_speaker_control_listener, get_state);

        if (jstate == NULL) {
            LOGE("Speakerstate object is NULL");
            break;
        }

        ss_clazz = (*env)->GetObjectClass(env, jstate);
        if (ss_clazz == NULL) {
            LOGE("cant't get class Speakerstate");
            break;
        }

        volume_id = (*env)->GetFieldID(env, ss_clazz, "volume", "I");
        if (volume_id == 0) {
            LOGE("cant't get field volume");
            break;
        }

        *volume = (*env)->GetIntField(env, jstate, volume_id);

        is_mute_id = (*env)->GetFieldID(env, ss_clazz, "isMute", "Z");
        if (is_mute_id == 0) {
            LOGE("cant't get field is_mute");
            break;
        }

        *is_mute = (duer_bool)(*env)->GetBooleanField(env, jstate, is_mute_id);
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (jstate != NULL) {
        (*env)->DeleteLocalRef(env, jstate);
    }

    if (ss_clazz != NULL) {
        (*env)->DeleteLocalRef(env, ss_clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

void duer_dcs_volume_set_handler(int volume)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jmethodID set_volume = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_speaker_control_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_speaker_control_listener");
            break;
        }

        set_volume = (*env)->GetMethodID(env, clazz, "setVolume", "(I)I");
        if (set_volume == NULL) {
            LOGE("method setVolume is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_speaker_control_listener, set_volume, volume);
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

void duer_dcs_volume_adjust_handler(int volume)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jmethodID adjust_volume = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_speaker_control_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_speaker_control_listener");
            break;
        }

        adjust_volume = (*env)->GetMethodID(env, clazz, "adjustVolume", "(I)I");
        if (adjust_volume == NULL) {
            LOGE("method adjustVolume is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_speaker_control_listener, adjust_volume, volume);
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

void duer_dcs_mute_handler(duer_bool is_mute)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jmethodID set_mute = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_speaker_control_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_speaker_control_listener");
            break;
        }

        set_mute = (*env)->GetMethodID(env, clazz, "setMute", "(Z)I");
        if (set_mute == NULL) {
            LOGE("method setMute is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_speaker_control_listener, set_mute, (jboolean)is_mute);
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initSpeakerControl
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/SpeakControlListener;)I
 */
jint lightduer_os_jni_init_speaker_control(JNIEnv *env, jclass clazz, jobject jlistener)
{
    if (s_speaker_control_listener != NULL) {
        (*env)->DeleteGlobalRef(env, s_speaker_control_listener);
    }

    s_speaker_control_listener = (*env)->NewGlobalRef(env, jlistener);
    if (s_speaker_control_listener == NULL) {
        LOGE("initSpeakControl failed");
        return DUER_ERR_FAILED;
    }

    duer_dcs_speaker_control_init();

    return DUER_OK;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onVolumeChange
 * Signature: ()I
 */
jint lightduer_os_jni_on_volume_change(JNIEnv *env, jclass clazz)
{
    return duer_dcs_on_volume_changed();
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onMute
 * Signature: ()I
 */
jint lightduer_os_jni_on_mute(JNIEnv *env, jclass clazz)
{
    return duer_dcs_on_mute();
}

void duer_dcs_audio_play_handler(const duer_dcs_audio_info_t *audio_info)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jstring jurl = NULL;
    jstring jaudio_item_id = NULL;
    jclass clazz = NULL;
    jmethodID play = NULL;

    if (audio_info == NULL) {
        LOGE("audio_info is NULL");
        return;
    }

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    jurl = (*env)->NewStringUTF(env, audio_info->url);
    jaudio_item_id = (*env)->NewStringUTF(env, audio_info->audio_item_id);

    do {
        clazz = (*env)->GetObjectClass(env, s_audio_player_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_audio_player_listener");
            break;
        }

        play = (*env)->GetMethodID(env, clazz, "play", "(Ljava/lang/String;ILjava/lang/String;)I");
        if (play == NULL) {
            LOGE("method play is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_audio_player_listener, play, jurl,
                audio_info->offset, jaudio_item_id);
    } while (0);

    if (jurl != NULL) {
        (*env)->DeleteLocalRef(env, jurl);
    }

    if (jaudio_item_id != NULL) {
        (*env)->DeleteLocalRef(env, jaudio_item_id);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

void duer_dcs_audio_stop_handler(void)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jmethodID stop = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_audio_player_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_audio_player_listener");
            break;
        }

        stop = (*env)->GetMethodID(env, clazz, "stop", "()I");
        if (stop == NULL) {
            LOGE("method stop is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_audio_player_listener, stop);
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

void duer_dcs_audio_resume_handler(const duer_dcs_audio_info_t *audio_info)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jstring jurl = NULL;
    jstring jaudio_item_id = NULL;
    jclass clazz = NULL;
    jmethodID resume = NULL;

    if (audio_info == NULL) {
        LOGE("audio_info is NULL");
        return;
    }

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    jurl = (*env)->NewStringUTF(env, audio_info->url);
    jaudio_item_id = (*env)->NewStringUTF(env, audio_info->audio_item_id);

    do {
        clazz = (*env)->GetObjectClass(env, s_audio_player_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_audio_player_listener");
            break;
        }

        resume = (*env)->GetMethodID(env, clazz, "resume",
                "(Ljava/lang/String;ILjava/lang/String;)I");
        if (resume == NULL) {
            LOGE("method resume is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_audio_player_listener, resume, jurl,
                audio_info->offset, jaudio_item_id);
    } while (0);

    if (jurl != NULL) {
        (*env)->DeleteLocalRef(env, jurl);
    }

    if (jaudio_item_id != NULL) {
        (*env)->DeleteLocalRef(env, jaudio_item_id);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

void duer_dcs_audio_pause_handler(void)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jmethodID pause = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_audio_player_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_audio_player_listener");
            break;
        }

        pause = (*env)->GetMethodID(env, clazz, "pause", "()I");
        if (pause == NULL) {
            LOGE("method pause is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_audio_player_listener, pause);
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

int duer_dcs_audio_get_play_progress(void)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jmethodID get_progress = NULL;
    jint progress = 0;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return 0;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_audio_player_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_audio_player_listener");
            break;
        }

        get_progress = (*env)->GetMethodID(env, clazz, "getAudioPlayProgress", "()I");
        if (get_progress == NULL) {
            LOGE("method getAudioPlayProgress is not found!\n");
            break;
        }

        progress = (*env)->CallIntMethod(env, s_audio_player_listener, get_progress);
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }

    return progress;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initAudioPlayer
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/AudioPlayerListener;)I
 */
jint lightduer_os_jni_init_audio_player(JNIEnv *env, jclass clazz, jobject jlistener)
{
    if (s_audio_player_listener != NULL) {
        (*env)->DeleteGlobalRef(env, s_audio_player_listener);
    }

    s_audio_player_listener = (*env)->NewGlobalRef(env, jlistener);
    if (s_audio_player_listener == NULL) {
        LOGE("initSpeakControl failed");
        return DUER_ERR_FAILED;
    }

    duer_dcs_audio_player_init();

    return DUER_OK;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    sendPlayControlCommand
 * Signature: (I)I
 */
jint lightduer_os_jni_send_play_control_command(JNIEnv *env, jclass clazz, jint jcommand)
{
    return duer_dcs_send_play_control_cmd(jcommand);
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onAudioFinished
 * Signature: ()I
 */
jint lightduer_os_jni_on_audio_finished(JNIEnv *env, jclass clazz)
{
    duer_dcs_audio_on_finished();

    return DUER_OK;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    audioPlayFailed
 * Signature: (ILjava/lang/String;)I
 */
jint lightduer_os_jni_audio_play_failed(JNIEnv *env, jclass clazz, jint jtype, jstring jmsg)
{
    int result = DUER_OK;
    const char *msg = (*env)->GetStringUTFChars(env, jmsg, NULL);

    result = duer_dcs_audio_play_failed((duer_dcs_audio_error_t)jtype, msg);

    if (msg) {
        (*env)->ReleaseStringUTFChars(env, jmsg, msg);
        msg = NULL;
    }

    return result;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    audioReportMetadata
 * Signature: (Ljava/lang/String;)I
 */
jint lightduer_os_jni_audio_report_metadata(JNIEnv *env, jclass clazz, jstring jmetadata)
{
    int result = DUER_OK;
    baidu_json *json_data = NULL;
    const char *metadata = (*env)->GetStringUTFChars(env, jmetadata, NULL);

    do {
        if (metadata == NULL) {
            LOGE("get metadata failed\n");
            result = DUER_ERR_FAILED;
            break;
        }

        json_data = baidu_json_Parse(metadata);
        if (json_data == NULL) {
            LOGE("get json failed\n");
            result = DUER_ERR_FAILED;
            break;
        }

        result = duer_dcs_audio_report_metadata(json_data);
    } while(0);

    if (metadata) {
        (*env)->ReleaseStringUTFChars(env, jmetadata, metadata);
        metadata = NULL;
    }

    if (json_data) {
        baidu_json_Delete(json_data);
        json_data = NULL;
    }

    return DUER_OK;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onAudioStuttered
 * Signature: (Z)I
 */
jint lightduer_os_jni_on_audio_stuttered(JNIEnv *env, jclass clazz, jboolean jis_stuttered)
{
    return duer_dcs_audio_on_stuttered(jis_stuttered);
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    syncState
 * Signature: ()I
 */
jint lightduer_os_jni_sync_state(JNIEnv *env, jclass clazz)
{
    duer_dcs_sync_state();

    return DUER_OK;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    syncState
 * Signature: ()I
 */
jint lightduer_os_jni_close_multi_dialog(JNIEnv *env, jclass clazz)
{
    return duer_dcs_close_multi_dialog();
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initScreenDisplay
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/ScreenDisplayListener;)I
 */
jint lightduer_os_jni_init_screen_display(JNIEnv *env, jclass clazz, jobject jlistener)
{
    if (s_screen_display_listener != NULL) {
        (*env)->DeleteGlobalRef(env, s_screen_display_listener);
    }

    s_screen_display_listener = (*env)->NewGlobalRef(env, jlistener);
    if (s_screen_display_listener == NULL) {
        LOGE("initScreenDisplay failed");
        return DUER_ERR_FAILED;
    }

    duer_dcs_screen_init();

    return DUER_OK;
}

duer_status_t duer_dcs_input_text_handler(const char *text, const char *type)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jstring jtext = NULL;
    jstring jtype = NULL;
    jclass clazz = NULL;
    jmethodID input_text = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return DUER_ERR_FAILED;
    }

    do {
        jtext = (*env)->NewStringUTF(env, text);
        if (jtext == NULL) {
            LOGE("new String jtext failed");
            break;
        }

        jtype = (*env)->NewStringUTF(env, type);
        if (jtype == NULL) {
            LOGE("new String jtype failed");
            break;
        }

        clazz = (*env)->GetObjectClass(env, s_screen_display_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_screen_display_listener");
            break;
        }

        input_text = (*env)->GetMethodID(env, clazz, "setInputText",
                    "(Ljava/lang/String;Ljava/lang/String;)I");
        if (input_text == NULL) {
            LOGE("method setInputText is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_screen_display_listener, input_text, jtext, jtype);
    } while (0);

    if (jtext != NULL) {
        (*env)->DeleteLocalRef(env, jtext);
    }

    if (jtype != NULL) {
        (*env)->DeleteLocalRef(env, jtype);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }

    return DUER_OK;
}

duer_status_t duer_dcs_render_card_handler(baidu_json *payload)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jstring jpayload = NULL;
    jclass clazz = NULL;
    jmethodID render_card = NULL;
    char *str_payload = NULL;

    if (payload == NULL) {
        LOGE("duer_dcs_render_card_handler payload is NULL!");
        return DUER_ERR_FAILED;
    }

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return DUER_ERR_FAILED;
    }

    do {
        str_payload = baidu_json_PrintUnformatted(payload);
        jpayload = (*env)->NewStringUTF(env, str_payload);
        if (jpayload == NULL) {
            LOGE("new String jpayload failed");
            break;
        }

        clazz = (*env)->GetObjectClass(env, s_screen_display_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_screen_display_listener");
            break;
        }

        render_card = (*env)->GetMethodID(env, clazz, "renderCard", "(Ljava/lang/String;)I");
        if (render_card == NULL) {
            LOGE("method renderCard is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_screen_display_listener, render_card, jpayload);
    } while (0);

    if (str_payload != NULL) {
        DUER_FREE(str_payload);
    }

    if (jpayload != NULL) {
        (*env)->DeleteLocalRef(env, jpayload);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }

    return DUER_OK;
}

jint lightduer_os_jni_init_device_control(JNIEnv *env, jclass clazz, jobject jlistener)
{
    if (s_device_control_listener != NULL) {
        (*env)->DeleteGlobalRef(env, s_device_control_listener);
    }

    s_device_control_listener = (*env)->NewGlobalRef(env, jlistener);
    if (s_device_control_listener == NULL) {
        LOGE("initDeviceControl failed");
        return DUER_ERR_FAILED;
    }

    duer_dcs_device_control_init();

    return DUER_OK;
}

void duer_dcs_bluetooth_set_handler(duer_bool is_switch, const char *target)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jstring jtarget = NULL;
    jclass clazz = NULL;
    jmethodID set_bt = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        if (target != NULL) {
            jtarget = (*env)->NewStringUTF(env, target);
            if (jtarget == NULL) {
                LOGE("new String jtarget failed");
                break;
            }
        }

        clazz = (*env)->GetObjectClass(env, s_device_control_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_device_control_listener");
            break;
        }

        set_bt = (*env)->GetMethodID(env, clazz, "setBluetooth", "(ZLjava/lang/String;)I");
        if (set_bt == NULL) {
            LOGE("method setBluetooth is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_device_control_listener, set_bt,
                (jboolean)is_switch, jtarget);
    } while (0);

    if (jtarget != NULL) {
        (*env)->DeleteLocalRef(env, jtarget);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

void duer_dcs_bluetooth_connect_handler(duer_bool is_connect, const char *target)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jstring jtarget = NULL;
    jclass clazz = NULL;
    jmethodID connect_bt = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        if (target != NULL) {
            jtarget = (*env)->NewStringUTF(env, target);
            if (jtarget == NULL) {
                LOGE("new String jtarget failed");
                break;
            }
        }

        clazz = (*env)->GetObjectClass(env, s_device_control_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_device_control_listener");
            break;
        }

        connect_bt = (*env)->GetMethodID(env, clazz, "connectBluetooth", "(ZLjava/lang/String;)I");
        if (connect_bt == NULL) {
            LOGE("method connectBluetooth is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_device_control_listener, connect_bt,
                (jboolean)is_connect, jtarget);
    } while (0);

    if (jtarget != NULL) {
        (*env)->DeleteLocalRef(env, jtarget);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

static duer_status_t dcs_directive_handler_for_java(const baidu_json *directive)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jstring jdirective = NULL;
    jclass clazz = NULL;
    jmethodID handle_directive = NULL;
    char *str_directive = NULL;

    if (s_directives_handler == NULL) {
        LOGE("dcs_directive_handler_for_java s_directives_handler is NULL");
        return DUER_ERR_FAILED;
    }

    if (directive == NULL) {
        LOGE("dcs_directive_handler_for_java directive is NULL!");
        return DUER_ERR_FAILED;
    }

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return DUER_ERR_FAILED;
    }

    do {
        str_directive = baidu_json_PrintUnformatted(directive);
        jdirective = (*env)->NewStringUTF(env, str_directive);
        if (jdirective == NULL) {
            LOGE("new String jdirective failed");
            break;
        }

        clazz = (*env)->GetObjectClass(env, s_directives_handler);
        if (clazz == NULL) {
            LOGE("cant't get class of s_directives_handler");
            break;
        }

        handle_directive = (*env)->GetMethodID(env, clazz,
                "handleDirective", "(Ljava/lang/String;)I");
        if (handle_directive == NULL) {
            LOGE("method handleDirective is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_directives_handler, handle_directive, jdirective);
    } while (0);

    if (str_directive != NULL) {
        DUER_FREE(str_directive);
    }

    if (jdirective != NULL) {
        (*env)->DeleteLocalRef(env, jdirective);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }

    return DUER_OK;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    nativeAddDcsDirective
 * Signature: ([Lcom/baidu/lightduer/lib/jni/dcs/DcsDirective; \
                Lcom/baidu/lightduer/lib/jni/dcs/DcsDirective$IDirectiveHandler;)I
 */
jint lightduer_os_jni_add_dcs_directive(JNIEnv *env, jclass clazz,
        jobjectArray jdirectives, jobject jhandler, jstring jnamespace)
{
    int ret = DUER_OK;
    jobject jdirective = NULL;
    jclass jdirective_class = NULL;
    jfieldID name_id = NULL;
    jstring jname = NULL;
    duer_directive_list *directives = NULL;
    const char *namespace = NULL;

    jsize arraylength = (*env)->GetArrayLength(env, jdirectives);
    if (arraylength <= 0) {
        LOGE("no element in array");
        return DUER_ERR_FAILED;
    }

    if (s_directives_handler == NULL) {
        s_directives_handler = (*env)->NewGlobalRef(env, jhandler);
        if (s_directives_handler == NULL) {
            LOGE("s_directives_handler is NULL");
            return DUER_ERR_FAILED;
        }
    }

    do {
        directives = (duer_directive_list*)DUER_MALLOC(sizeof(duer_directive_list) * arraylength);
        if (!directives) {
            LOGE("directives alloc fail!!");
            break;
        }

        DUER_MEMSET(directives, 0, sizeof(duer_directive_list) * arraylength);

        jdirective = (*env)->GetObjectArrayElement(env, jdirectives, 0);
        if (jdirective == NULL) {
            LOGE("get element 0 fail!\n");
            break;
        }

        jdirective_class = (*env)->GetObjectClass(env, jdirective);
        if (jdirective_class == NULL) {
            LOGE("get class for DcsDirective fail!\n");
            break;
        }

        name_id = (*env)->GetFieldID(env, jdirective_class, "name", "Ljava/lang/String;");
        if (name_id == NULL) {
            LOGE("get field name failed!\n");
            break;
        }

        (*env)->DeleteLocalRef(env, jdirective);
        jdirective = NULL;

        for (int i = 0; i < arraylength; ++i) {
            jdirective = (*env)->GetObjectArrayElement(env, jdirectives, i);
            jname = (jstring)(*env)->GetObjectField(env, jdirective, name_id);
            jsize len = (*env)->GetStringUTFLength(env, jname);
            if (len == 0) {
                LOGE("the length of name is 0, i = %d", i);
                ret = DUER_ERR_FAILED;
                break;
            }
            char *name = (char *)DUER_MALLOC(len + 1);
            if (name == NULL) {
                LOGE("alloc fail! %d\n", i);
                ret = DUER_ERR_FAILED;
                break;
            }

            (*env)->GetStringUTFRegion(env, jname, 0, len, name);
            name[len] = 0;
            directives[i].directive_name = name;
            directives[i].cb = dcs_directive_handler_for_java;

            (*env)->DeleteLocalRef(env, jdirective);
            (*env)->DeleteLocalRef(env, jname);
            jdirective = NULL;
            jname = NULL;
        }

        namespace = (*env)->GetStringUTFChars(env, jnamespace, NULL);
        if (namespace == NULL) {
            LOGE("namespace is NULL");
            ret = DUER_ERR_FAILED;
            break;
        }

        if (ret == DUER_OK) {
            ret = duer_add_dcs_directive(directives, arraylength, namespace);
        }
    } while(0);

    if (jdirective != NULL) {
        (*env)->DeleteLocalRef(env, jdirective);
        jdirective = NULL;
    }

    if (jdirective_class != NULL) {
        (*env)->DeleteLocalRef(env, jdirective_class);
        jdirective_class = NULL;
    }

    if (jname != NULL) {
        (*env)->DeleteLocalRef(env, jname);
        jname = NULL;
    }

    if (directives) {
        for (int i = 0; i < arraylength; ++i) {
            if (directives[i].directive_name) {
                DUER_FREE((char *)directives[i].directive_name);
                directives[i].directive_name = NULL;
            }
        }
        DUER_FREE(directives);
        directives = NULL;
    }

    if (namespace) {
        (*env)->ReleaseStringUTFChars(env, jnamespace, namespace);
        namespace = NULL;
    }

    return ret;
}

static baidu_json *get_dcs_client_context()
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jclass clazz = NULL;
    jmethodID method = NULL;
    jstring jctx = NULL;
    const char *ctx_str = NULL;
    baidu_json *ctx = NULL;

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return NULL;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_client_context_provider);
        if (clazz == NULL) {
            LOGE("cant't get class of s_client_context_provider");
            break;
        }

        method = (*env)->GetMethodID(env, clazz, "getClientContext", "()Ljava/lang/String;");
        if (method == NULL) {
            LOGE("method getClientContext is not found!!\n");
            break;
        }

        jctx = (jstring)(*env)->CallObjectMethod(env, s_client_context_provider, method);
        ctx_str = (*env)->GetStringUTFChars(env, jctx, NULL);
        if (ctx_str == NULL) {
            LOGE("getClientContext failed");
            break;
        }

        ctx = baidu_json_Parse(ctx_str);
        if (ctx == NULL) {
            LOGE("get json failed\n");
        }
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (ctx_str) {
        (*env)->ReleaseStringUTFChars(env, jctx, ctx_str);
        ctx_str = NULL;
    }

    if (jctx != NULL) {
        (*env)->DeleteLocalRef(env, jctx);
        jctx = NULL;
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }

    return ctx;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    registerClientContextProvider
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/DcsClientContextProvider;)I
 */
jint lightduer_os_jni_register_client_context_provider(
        JNIEnv *env, jclass clazz, jobject jprovider)
{
    if (s_client_context_provider != NULL) {
        (*env)->DeleteGlobalRef(env, s_client_context_provider);
    }

    s_client_context_provider = (*env)->NewGlobalRef(env, jprovider);
    if (s_client_context_provider == NULL) {
        LOGE("registerClientContextProvider failed");
        return DUER_ERR_FAILED;
    }

    return duer_reg_client_context_cb(get_dcs_client_context);
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    createDcsEvent
 * Signature: (Ljava/lang/String;Ljava/lang/String;Z)Ljava/lang/String;
 */
jstring lightduer_os_jni_create_dcs_event(JNIEnv *env, jclass clazz, jstring jname,
        jstring jnamespace, jboolean need_msg_id)
{
    const char *name = NULL;
    const char *namespace = NULL;
    char *str_event = NULL;
    baidu_json* json_data = NULL;
    jstring result = NULL;

    do {
        name = (*env)->GetStringUTFChars(env, jname, NULL);
        if (name == NULL) {
            LOGE("createDcsEvent name is NULL");
            break;
        }

        namespace = (*env)->GetStringUTFChars(env, jnamespace, NULL);
        if (namespace == NULL) {
            LOGE("createDcsEvent namespace is NULL");
            break;
        }

        json_data = duer_create_dcs_event(name, namespace, need_msg_id);
        if (json_data == NULL) {
            LOGE("duer_create_dcs_event failed");
            break;
        }

        str_event = baidu_json_PrintUnformatted(json_data);
        result = (*env)->NewStringUTF(env, str_event);
    } while (0);

    if (str_event) {
        DUER_FREE(str_event);
        str_event = NULL;
    }

    if (json_data) {
        baidu_json_Delete(json_data);
        json_data = NULL;
    }

    return result;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    dcsDialogCancel
 * Signature: ()I
 */
jint lightduer_os_jni_dcs_dialog_cancel(JNIEnv *env, jclass clazz)
{
    duer_dcs_dialog_cancel();
    return DUER_OK;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    dcsOnLinkClicked
 * Signature: (Ljava/lang/String;)I
 */
jint lightduer_os_jni_dcs_on_link_clicked(JNIEnv *env, jclass clazz, jstring jurl)
{
    const char *url = (*env)->GetStringUTFChars(env, jurl, NULL);
    int ret = DUER_OK;

    if (url == NULL) {
        LOGE("dcsOnLinkClicked url is NULL\n");
        return DUER_ERR_FAILED;
    }

    ret = duer_dcs_on_link_clicked(url);

    if (url) {
        (*env)->ReleaseStringUTFChars(env, jurl, url);
        url = NULL;
    }

    return ret;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initDcsAlert
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/DcsAlertListener;)I
 */
jint lightduer_os_jni_init_dcs_alert(JNIEnv *env, jclass clazz, jobject jlistener)
{
    if (s_dcs_alert_listener != NULL) {
        (*env)->DeleteGlobalRef(env, s_dcs_alert_listener);
    }

    s_dcs_alert_listener = (*env)->NewGlobalRef(env, jlistener);
    if (s_dcs_alert_listener == NULL) {
        LOGE("initDcsAlert failed");
        return DUER_ERR_FAILED;
    }

    if (s_dcs_alert_info_class == NULL) {
        jclass dcs_alert_info_class = (*env)->FindClass(
                env, "com/baidu/lightduer/lib/jni/dcs/DcsAlertInfo");
        if (dcs_alert_info_class == NULL) {
            LOGE("DcsAlertInfo find fail!");
            return DUER_ERR_FAILED;
        }
        s_dcs_alert_info_class = (jclass) (*env)->NewGlobalRef(env, dcs_alert_info_class);
        (*env)->DeleteLocalRef(env, dcs_alert_info_class);
    }

    duer_dcs_alert_init();

    return DUER_OK;
}

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    reportDcsAlertEvent
 * Signature: (Ljava/lang/String;I)I
 */
jint lightduer_os_jni_report_dcs_alert_event(JNIEnv *env, jclass clazz,
        jstring jtoken, jint jtype)
{
    int result = DUER_OK;
    const char *token = (*env)->GetStringUTFChars(env, jtoken, NULL);

    result = duer_dcs_report_alert_event(token, (duer_dcs_alert_event_type)jtype);

    if (token) {
        (*env)->ReleaseStringUTFChars(env, jtoken, token);
        token = NULL;
    }

    return result;
}

duer_status_t duer_dcs_tone_alert_set_handler(const baidu_json *directive)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jstring jdirective = NULL;
    jclass clazz = NULL;
    jmethodID set_alert = NULL;
    char *str_directive = NULL;
    duer_status_t ret = DUER_OK;

    if (s_dcs_alert_listener == NULL) {
        LOGE("duer_dcs_tone_alert_set_handler s_dcs_alert_listener is NULL");
        return DUER_ERR_FAILED;
    }

    if (directive == NULL) {
        LOGE("duer_dcs_tone_alert_set_handler directive is NULL!");
        return DUER_ERR_FAILED;
    }

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return DUER_ERR_FAILED;
    }

    do {
        str_directive = baidu_json_PrintUnformatted(directive);
        jdirective = (*env)->NewStringUTF(env, str_directive);
        if (jdirective == NULL) {
            LOGE("new String jdirective failed");
            break;
        }

        clazz = (*env)->GetObjectClass(env, s_dcs_alert_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_dcs_alert_listener");
            break;
        }

        set_alert = (*env)->GetMethodID(env, clazz, "setAlert", "(Ljava/lang/String;)I");
        if (set_alert == NULL) {
            LOGE("method setAlert is not found!!\n");
            break;
        }

        ret = (*env)->CallIntMethod(env, s_dcs_alert_listener, set_alert, jdirective);
    } while (0);

    if (str_directive != NULL) {
        DUER_FREE(str_directive);
    }

    if (jdirective != NULL) {
        (*env)->DeleteLocalRef(env, jdirective);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }

    return ret;
}

void duer_dcs_alert_set_handler(const duer_dcs_alert_info_type *alert_info)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jstring jtype = NULL;
    jstring jtime = NULL;
    jstring jtoken = NULL;
    jobject jalert_info = NULL;
    jclass clazz = NULL;
    jmethodID set_alert = NULL;
    jmethodID constructor = NULL;

    if (alert_info == NULL) {
        LOGE("duer_dcs_alert_set_handler alert_info is NULL");
        return;
    }

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        jtype = (*env)->NewStringUTF(env, alert_info->type);
        if (jtype == NULL) {
            LOGE("new String jtype failed");
            break;
        }

        jtime = (*env)->NewStringUTF(env, alert_info->time);
        if (jtime == NULL) {
            LOGE("new String jtime failed");
            break;
        }

        jtoken = (*env)->NewStringUTF(env, alert_info->token);
        if (jtoken == NULL) {
            LOGE("new String jtoken failed");
            break;
        }

        constructor = (*env)->GetMethodID(env, s_dcs_alert_info_class, "<init>",
                    "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
        if (constructor == NULL) {
            LOGE("get DcsAlertInfo constructor failed");
            break;
        }

        jalert_info = (*env)->NewObject(env, s_dcs_alert_info_class, constructor,
                jtype, jtime, jtoken);
        if (jalert_info == NULL) {
            LOGE("new DcsAlertInfo failed");
            break;
        }

        clazz = (*env)->GetObjectClass(env, s_dcs_alert_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_dcs_alert_listener");
            break;
        }

        set_alert = (*env)->GetMethodID(env, clazz, "setAlert",
                "(Lcom/baidu/lightduer/lib/jni/dcs/DcsAlertInfo;)I");
        if (set_alert == NULL) {
            LOGE("method setAlert is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_dcs_alert_listener, set_alert, jalert_info);
    } while (0);

    if (jtype != NULL) {
        (*env)->DeleteLocalRef(env, jtype);
    }

    if (jtime != NULL) {
        (*env)->DeleteLocalRef(env, jtime);
    }

    if (jtoken != NULL) {
        (*env)->DeleteLocalRef(env, jtoken);
    }

    if (jalert_info != NULL) {
        (*env)->DeleteLocalRef(env, jalert_info);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

void duer_dcs_alert_delete_handler(const char *token)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jstring jtoken = NULL;
    jclass clazz = NULL;
    jmethodID delete_alert = NULL;

    if (token == NULL) {
        LOGE("duer_dcs_alert_delete_handler token is NULL");
        return;
    }

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        jtoken = (*env)->NewStringUTF(env, token);
        if (jtoken == NULL) {
            LOGE("new String jtoken failed");
            break;
        }

        clazz = (*env)->GetObjectClass(env, s_dcs_alert_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_dcs_alert_listener");
            break;
        }

        delete_alert = (*env)->GetMethodID(env, clazz, "deleteAlert", "(Ljava/lang/String;)I");
        if (delete_alert == NULL) {
            LOGE("method deleteAlert is not found!!\n");
            break;
        }

        (*env)->CallIntMethod(env, s_dcs_alert_listener, delete_alert, jtoken);
    } while (0);

    if (jtoken != NULL) {
        (*env)->DeleteLocalRef(env, jtoken);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

static void add_alert_array_to_list(JNIEnv *env, jobjectArray jalerts, baidu_json *alert_list)
{
    jfieldID type_id = NULL;
    jfieldID time_id = NULL;
    jfieldID token_id = NULL;
    jfieldID active_id = NULL;
    jmethodID method = NULL;
    jobject jalert = NULL;
    jobject jtype = NULL;
    jobject jtime = NULL;
    jobject jtoken = NULL;
    duer_dcs_alert_info_type alert;
    jsize len = 0;
    duer_bool is_active = DUER_FALSE;

    jsize arraylength = (*env)->GetArrayLength(env, jalerts);
    if (arraylength <= 0) {
        LOGE("no element in array");
        return ;
    }

    DUER_MEMSET(&alert, 0, sizeof(alert));

    do {
        type_id = (*env)->GetFieldID(env, s_dcs_alert_info_class, "type", "Ljava/lang/String;");
        if (type_id == NULL) {
            LOGE("get type field failed!\n");
            break;
        }

        time_id = (*env)->GetFieldID(env, s_dcs_alert_info_class, "time", "Ljava/lang/String;");
        if (time_id == NULL) {
            LOGE("get time field failed!\n");
            break;
        }

        token_id = (*env)->GetFieldID(env, s_dcs_alert_info_class, "token", "Ljava/lang/String;");
        if (token_id == NULL) {
            LOGE("get token field failed!\n");
            break;
        }

        active_id = (*env)->GetFieldID(env, s_dcs_alert_info_class, "active", "Z");
        if (active_id == NULL) {
            LOGE("get active field failed!\n");
            break;
        }

        for (int i = 0; i < arraylength; ++i) {

            jalert = (*env)->GetObjectArrayElement(env, jalerts, i);

            // get type field
            jtype = (jstring)(*env)->GetObjectField(env, jalert, type_id);
            len = (*env)->GetStringUTFLength(env, jtype);
            if (len == 0) {
                LOGE("the length of type is 0, i = %d", i);
                break;
            }
            alert.type = (char *)DUER_MALLOC(len + 1);
            if (alert.type == NULL) {
                LOGE("alloc fail! %d\n", i);
                break;
            }

            (*env)->GetStringUTFRegion(env, jtype, 0, len, alert.type);
            alert.type[len] = 0;

            // get time field
            jtime = (jstring)(*env)->GetObjectField(env, jalert, time_id);
            len = (*env)->GetStringUTFLength(env, jtime);
            if (len == 0) {
                LOGE("the length of time is 0, i = %d", i);
                break;
            }
            alert.time = (char *)DUER_MALLOC(len + 1);
            if (alert.time == NULL) {
                LOGE("alloc fail! %d\n", i);
                break;
            }

            (*env)->GetStringUTFRegion(env, jtime, 0, len, alert.time);
            alert.time[len] = 0;

            // get token field
            jtoken = (jstring)(*env)->GetObjectField(env, jalert, token_id);
            len = (*env)->GetStringUTFLength(env, jtoken);
            if (len == 0) {
                LOGE("the length of token is 0, i = %d", i);
                break;
            }
            alert.token = (char *)DUER_MALLOC(len + 1);
            if (alert.token == NULL) {
                LOGE("alloc fail! %d\n", i);
                break;
            }

            (*env)->GetStringUTFRegion(env, jtoken, 0, len, alert.token);
            alert.token[len] = 0;

            // get active field
            is_active = (duer_bool)(*env)->GetBooleanField(env, jalert, active_id);

            duer_insert_alert_list(alert_list, &alert, is_active);

            (*env)->DeleteLocalRef(env, jtype);
            jtype = NULL;
            (*env)->DeleteLocalRef(env, jtime);
            jtime = NULL;
            (*env)->DeleteLocalRef(env, jtoken);
            jtoken = NULL;
            (*env)->DeleteLocalRef(env, jalert);
            jalert = NULL;
            DUER_FREE(alert.type);
            alert.type = NULL;
            DUER_FREE(alert.time);
            alert.time = NULL;
            DUER_FREE(alert.token);
            alert.token = NULL;
        }
    } while (0);

    if (jalert != NULL) {
        (*env)->DeleteLocalRef(env, jalert);
    }

    if (jtype != NULL) {
        (*env)->DeleteLocalRef(env, jtype);
    }

    if (jtime != NULL) {
        (*env)->DeleteLocalRef(env, jtime);
    }

    if (jtoken != NULL) {
        (*env)->DeleteLocalRef(env, jtoken);
    }
}

void duer_dcs_get_all_alert(baidu_json *alert_list)
{
    int need_detach = 0;
    JNIEnv *env = NULL;
    jobjectArray jalerts = NULL;
    jclass clazz = NULL;
    jmethodID method = NULL;

    if (alert_list == NULL) {
        LOGE("duer_dcs_get_all_alert alert_list is NULL");
        return;
    }

    env = duer_jni_get_JNIEnv(&need_detach);
    if (env == NULL) {
        LOGE("can't get env\n");
        return;
    }

    do {
        clazz = (*env)->GetObjectClass(env, s_dcs_alert_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_dcs_alert_listener");
            break;
        }

        method = (*env)->GetMethodID(env, clazz, "getAllAlert",
                "()[Lcom/baidu/lightduer/lib/jni/dcs/DcsAlertInfo;");
        if (method == NULL) {
            LOGE("method getAllAlert is not found!!\n");
            break;
        }

        jalerts = (jobjectArray)(*env)->CallObjectMethod(env, s_dcs_alert_listener, method);
        if (jalerts == NULL) {
            LOGI("call getAllAlert return NULL");
            break;
        }

        add_alert_array_to_list(env, jalerts, alert_list);
    } while (0);

    if (jalerts != NULL) {
        (*env)->DeleteLocalRef(env, jalerts);
    }

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }

    if (need_detach) {
        duer_jni_detach_current_thread();
    }
}

jint lightduer_os_jni_dcs_capability_declare(JNIEnv *env, jclass clazz, jint jcapability)
{
    return duer_dcs_capability_declare((duer_u32_t)jcapability);
}
