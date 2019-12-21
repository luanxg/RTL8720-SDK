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

#ifndef BAIDU_DUER_LIGHTDUER_JNI_DCS_H
#define BAIDU_DUER_LIGHTDUER_JNI_DCS_H

#include "lightduer_jni.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initFramework
 * Signature: ()I
 */
jint lightduer_os_jni_init_framework(JNIEnv *, jclass);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initVoiceInput
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/VoiceInputListener;)I
 */
jint lightduer_os_jni_init_voice_input(JNIEnv *, jclass, jobject);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onListenStarted
 * Signature: ()I
 */
jint lightduer_os_jni_on_listen_started(JNIEnv *, jclass);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onListenStarted
 * Signature: ()I
 */
jint lightduer_os_jni_on_listen_stopped(JNIEnv *, jclass);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initVoiceOutput
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/VoiceOutputListener;)I
 */
jint lightduer_os_jni_init_voice_output(JNIEnv *, jclass, jobject);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onSpeakFinished
 * Signature: ()I
 */
jint lightduer_os_jni_on_speak_finished(JNIEnv *, jclass);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initSpeakControl
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/SpeakControlListener;)I
 */
jint lightduer_os_jni_init_speaker_control(JNIEnv *, jclass, jobject);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onVolumeChange
 * Signature: ()I
 */
jint lightduer_os_jni_on_volume_change(JNIEnv *, jclass);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onMute
 * Signature: ()I
 */
jint lightduer_os_jni_on_mute(JNIEnv *, jclass);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initAudioPlayer
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/AudioPlayerListener;)I
 */
jint lightduer_os_jni_init_audio_player(JNIEnv *, jclass, jobject);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    sendPlayControlCommand
 * Signature: (I)I
 */
jint lightduer_os_jni_send_play_control_command(JNIEnv *, jclass, jint);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onAudioFinished
 * Signature: ()I
 */
jint lightduer_os_jni_on_audio_finished(JNIEnv *, jclass);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    audioPlayFailed
 * Signature: (ILjava/lang/String;)I
 */
jint lightduer_os_jni_audio_play_failed(JNIEnv *, jclass, jint, jstring);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    audioReportMetadata
 * Signature: (Ljava/lang/String;)I
 */
jint lightduer_os_jni_audio_report_metadata(JNIEnv *, jclass, jstring);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    onAudioStuttered
 * Signature: (Z)I
 */
jint lightduer_os_jni_on_audio_stuttered(JNIEnv *, jclass, jboolean);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    syncState
 * Signature: ()I
 */
jint lightduer_os_jni_sync_state(JNIEnv *, jclass);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    closeMultiDialog
 * Signature: ()I
 */
jint lightduer_os_jni_close_multi_dialog(JNIEnv *, jclass);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initScreenDisplay
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/ScreenDisplayListener;)I
 */
jint lightduer_os_jni_init_screen_display(JNIEnv *env, jclass clazz, jobject jlistener);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initDeviceControl
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/DeviceControlListener;)I
 */
jint lightduer_os_jni_init_device_control(JNIEnv *env, jclass clazz, jobject jlistener);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    nativeAddDcsDirective
 * Signature: ([Lcom/baidu/lightduer/lib/jni/dcs/DcsDirective; \
                Lcom/baidu/lightduer/lib/jni/dcs/DcsDirective$IDirectiveHandler; \
                Ljava/lang/String;)I
 */
jint lightduer_os_jni_add_dcs_directive(JNIEnv *, jclass, jobjectArray, jobject, jstring);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    registerClientContextProvider
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/DcsClientContextProvider;)I
 */
jint lightduer_os_jni_register_client_context_provider(JNIEnv *, jclass, jobject);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    createDcsEvent
 * Signature: (Ljava/lang/String;Ljava/lang/String;Z)Ljava/lang/String;
 */
jstring lightduer_os_jni_create_dcs_event(JNIEnv *, jclass, jstring, jstring, jboolean);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    dcsDialogCancel
 * Signature: (Ljava/lang/String;)I
 */
jint lightduer_os_jni_dcs_dialog_cancel(JNIEnv *, jclass);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    dcsOnLinkClicked
 * Signature: ()I
 */
jint lightduer_os_jni_dcs_on_link_clicked(JNIEnv *, jclass, jstring);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    initDcsAlert
 * Signature: (Lcom/baidu/lightduer/lib/jni/dcs/DcsAlertListener;)I
 */
jint lightduer_os_jni_init_dcs_alert(JNIEnv *env, jclass clazz, jobject jlistener);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    reportDcsAlertEvent
 * Signature: (Ljava/lang/String;I)I
 */
jint lightduer_os_jni_report_dcs_alert_event(JNIEnv *env, jclass clazz,
        jstring jtoken, jint jtype);

/*
 * Class:     com_baidu_lightduer_lib_jni_LightduerOsJni
 * Method:    dcsCapabilityDeclare
 * Signature: (I)I
 */
jint lightduer_os_jni_dcs_capability_declare(JNIEnv *env, jclass clazz, jint jcapability);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIGHTDUER_JNI_DCS_H
