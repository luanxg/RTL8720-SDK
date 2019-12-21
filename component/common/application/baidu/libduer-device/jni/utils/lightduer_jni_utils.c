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
#define TAG "lightduer_jni_utils"

#include "lightduer_jni.h"
#include "lightduer_jni_utils.h"
#include "lightduer_jni_log.h"
#include "lightduer_lib.h"
#include "lightduer_memory.h"

static jclass s_lightduerContextClass = NULL;
static jclass s_lightduerMessageClass = NULL;
static jclass s_lightduerAddressClass = NULL;

int duer_jni_utils_init(JNIEnv *jenv)
{
    int result = DUER_ERR_FAILED;

    if (s_lightduerAddressClass == NULL) {
        jclass lightduerAddressClass = (*jenv)->FindClass(jenv, PACKAGE"/utils/LightduerAddress");
        if (lightduerAddressClass == NULL) {
            LOGE("lightduerAddressClass find fail!");
            return result;
        }
        s_lightduerAddressClass = (jclass) (*jenv)->NewGlobalRef(jenv, lightduerAddressClass);
        (*jenv)->DeleteLocalRef(jenv, lightduerAddressClass);
    }

    if (s_lightduerContextClass == NULL) {
        jclass lightduerContextClass = (*jenv)->FindClass(jenv, PACKAGE"/utils/LightduerContext");
        if (lightduerContextClass == NULL) {
            LOGE("lightduerContextClass find fail!");
            return result;
        }
        s_lightduerContextClass = (jclass) (*jenv)->NewGlobalRef(jenv, lightduerContextClass);
        (*jenv)->DeleteLocalRef(jenv, lightduerContextClass);
    }

    if (s_lightduerMessageClass == NULL) {
        jclass lightduerMessageClass = (*jenv)->FindClass(jenv, PACKAGE"/utils/LightduerMessage");
        if (lightduerMessageClass == NULL) {
            LOGE("lightduerMessageClass find fail!");
            return result;
        }
        s_lightduerMessageClass = (jclass) (*jenv)->NewGlobalRef(jenv, lightduerMessageClass);
        (*jenv)->DeleteLocalRef(jenv, lightduerMessageClass);
    }

    return DUER_OK;
}

jobject duer_jni_get_object_LightduerContext(JNIEnv *jenv, duer_context ctx)
{
    jobject context = NULL;
    jmethodID constructorContext = NULL;

    do {
        if (s_lightduerContextClass == NULL) {
            LOGE("s_lightduerContextClass is NULL!");
            break;
        }

        constructorContext =
                (*jenv)->GetMethodID(jenv, s_lightduerContextClass, "<init>", "(J)V");
        if (constructorContext == NULL) {
            LOGE("get constructorContext failed");
            break;
        }

        context = (*jenv)->NewObject(jenv, s_lightduerContextClass, constructorContext, (jlong) ctx);
    } while(0);

    return context;
}

jobject duer_jni_get_object_LightduerMessage(JNIEnv *jenv, duer_msg_t *msg)
{
    char *c_path = NULL;
    char *c_query = NULL;
    jobject message = NULL;
    jmethodID constructorMessage = NULL;
    jbyteArray token_array = NULL;
    jstring path = NULL;
    jstring query = NULL;
    jbyteArray payload_array = NULL;

    do {
        if (s_lightduerMessageClass == NULL) {
            LOGE("s_lightduerMessageClass is NULL!");
            break;
        }

        constructorMessage =
                (*jenv)->GetMethodID(
                        jenv,
                        s_lightduerMessageClass,
                        "<init>",
                        "(SSIIIII[BLjava/lang/String;Ljava/lang/String;[B)V");
        if (constructorMessage == NULL) {
            LOGE("get constructorMessage failed");
            break;
        }

        jbyte *token_byte = (jbyte*)msg->token;
        token_array = (*jenv)->NewByteArray(jenv, msg->token_len);
        (*jenv)->SetByteArrayRegion(jenv, token_array, 0, msg->token_len, token_byte);
        LOGD("token_len:%d \n", msg->token_len);
        //for (int i =0; i < msg->token_len; ++i) {
        //    LOGD("token[%d]: 0x%x \n", i, msg->token[i]);
        //}

        c_path = (char*)DUER_MALLOC(msg->path_len + 1);
        if (c_path == NULL) {
            LOGE("alloc c_path fail!");
            break;
        }
        LOGD("path_len:%d \n", msg->path_len);
        DUER_MEMCPY(c_path, msg->path, msg->path_len);
        c_path[msg->path_len] = 0;
        LOGD("c_path:%s \n", c_path);
        path = (*jenv)->NewStringUTF(jenv, c_path);

        c_query = (char*)DUER_MALLOC(msg->query_len + 1);
        if (c_query == NULL) {
            LOGE("alloc c_query fail!");
            break;
        }
        LOGD("query_len:%d\n", msg->query_len);
        DUER_MEMCPY(c_query, msg->query, msg->query_len);
        c_query[msg->query_len] = 0;
        LOGD("query:%s\n", c_query);
        query = (*jenv)->NewStringUTF(jenv, c_query);

        jbyte *payload_byte = (jbyte*)msg->payload;
        payload_array = (*jenv)->NewByteArray(jenv, msg->payload_len);
        (*jenv)->SetByteArrayRegion(jenv, payload_array, 0, msg->payload_len, payload_byte);

        LOGI("msg_type:%d, msg_code:%d, msg_id:%d \n", msg->msg_type, msg->msg_code, msg->msg_id);
        message = (*jenv)->NewObject(jenv, s_lightduerMessageClass, constructorMessage,
                                          msg->msg_type, msg->msg_code, msg->msg_id,
                                          msg->token_len, msg->path_len, msg->query_len, msg->payload_len,
                                          token_array, path, query, payload_array);


    } while(0);

    if (payload_array) {
        (*jenv)->DeleteLocalRef(jenv, payload_array);
        payload_array = NULL;
    }
    if (query) {
        (*jenv)->DeleteLocalRef(jenv, query);
        query = NULL;
    }
    if (path) {
        (*jenv)->DeleteLocalRef(jenv, path);
        path = NULL;
    }
    if (token_array) {
        (*jenv)->DeleteLocalRef(jenv, token_array);
        token_array = NULL;
    }

    if (c_path) {
        DUER_FREE(c_path);
        c_path = NULL;
    }
    if (c_query) {
        DUER_FREE(c_query);
        c_query = NULL;
    }

    return message;
}

jobject duer_jni_get_object_LightduerAddress(JNIEnv *jenv, duer_addr_t *addr)
{
    char *c_host = NULL;
    jobject address = NULL;

    do {
        if (s_lightduerAddressClass == NULL) {
            LOGE("s_lightduerAddressClass is NULL!");
            break;
        }

        jmethodID constructorAddress =
                (*jenv)->GetMethodID(jenv, s_lightduerAddressClass, "<init>", "(SILjava/lang/String;)V");
        if (constructorAddress == NULL) {
            LOGE("get constructorAddress failed");
            break;
        }

        c_host = (char*)DUER_MALLOC(addr->host_size + 1);
        if (c_host == NULL) {
            LOGE("alloc host fail!");
            break;
        }
        //LOGD("host size: %lu \n", addr->host_size);
        DUER_MEMCPY(c_host, addr->host, addr->host_size);
        c_host[addr->host_size] = 0;
        LOGD("host: %s \n", c_host);
        jstring host = (*jenv)->NewStringUTF(jenv, c_host);
        address = (*jenv)->NewObject(jenv, s_lightduerAddressClass, constructorAddress,
                                          addr->type, addr->port, host);
    } while(0);

    if (c_host) {
        DUER_FREE(c_host);
        c_host = NULL;
    }

    return address;
}

int duer_jni_get_native_duer_message(JNIEnv *jenv, jobject message, duer_msg_t *msg)
{
    int result = DUER_ERR_FAILED;
    char *newPath = NULL;
    char *newQuery = NULL;
    jbyte *newToken = NULL;
    jbyte *newPayload = NULL;

    do {
        if (s_lightduerMessageClass == NULL) {
            LOGE("s_lightduerMessageClass is NULL!\n");
            break;
        }

        jfieldID fieldtype = (*jenv)->GetFieldID(jenv, s_lightduerMessageClass, "msgType", "S");
        if (fieldtype == NULL) {
            LOGE("get field type failed!\n");
            break;
        }
        jshort typeValue = (*jenv)->GetShortField(jenv, message, fieldtype);
        msg->msg_type = typeValue;

        jfieldID fieldcode = (*jenv)->GetFieldID(jenv, s_lightduerMessageClass, "msgCode", "S");
        if (fieldcode == NULL) {
            LOGE("get field code failed!\n");
            break;
        }
        jshort codeValue = (*jenv)->GetShortField(jenv, message, fieldcode);
        msg->msg_code = codeValue;

        jfieldID fieldId = (*jenv)->GetFieldID(jenv, s_lightduerMessageClass, "msgId", "I");
        if (fieldId == NULL) {
            LOGE("get field id failed!\n");
            break;
        }
        jint idValue = (*jenv)->GetIntField(jenv, message, fieldId);
        msg->msg_id = idValue;

        jfieldID fieldtokenLen = (*jenv)->GetFieldID(jenv, s_lightduerMessageClass, "tokenLen", "I");
        if (fieldtokenLen == NULL) {
            LOGE("get field tokenLen failed!\n");
            break;
        }
        LOGI("response msg id:%d, code:%d, type:%d\n",
                msg->msg_id, msg->msg_code, msg->msg_type);
        jint tokenLenValue = (*jenv)->GetIntField(jenv, message, fieldtokenLen);
        msg->token_len = tokenLenValue;
        jfieldID fieldpathLen = (*jenv)->GetFieldID(jenv, s_lightduerMessageClass, "pathLen", "I");
        if (fieldpathLen == NULL) {
            LOGE("get field pathLen failed!\n");
            break;
        }
        jint pathLenValue = (*jenv)->GetIntField(jenv, message, fieldpathLen);
        msg->path_len = pathLenValue;
        jfieldID fieldquerylen = (*jenv)->GetFieldID(jenv, s_lightduerMessageClass, "querylen", "I");
        if (fieldquerylen == NULL) {
            LOGE("get field querylen failed!\n");
            break;
        }
        jint queryLenValue = (*jenv)->GetIntField(jenv, message, fieldquerylen);
        msg->query_len = queryLenValue;
        jfieldID fieldpayloadLen = (*jenv)->GetFieldID(jenv, s_lightduerMessageClass, "payloadLen", "I");
        if (fieldpayloadLen == NULL) {
            LOGE("get field payloadLen failed!\n");
            break;
        }
        jint payloadLenValue = (*jenv)->GetIntField(jenv, message, fieldpayloadLen);
        msg->payload_len = payloadLenValue;
        LOGI("response: tokenlen:%d, query_len:%d, path_len:%d, payload_len:%d \n",
                msg->token_len, msg->query_len,
                msg->path_len, msg->payload_len);

        jfieldID fieldPath = (*jenv)->GetFieldID(jenv, s_lightduerMessageClass, "path", "Ljava/lang/String;");
        if (fieldPath == NULL) {
            LOGE("get field path failed!\n");
            break;
        }
        jstring path = (jstring) (*jenv)->GetObjectField(jenv, message, fieldPath);
        //const char* c_path = (*jenv)->GetStringUTFChars(jenv, path, NULL);
        //LOGI("c_path:%s", c_path);
        //(*jenv)->ReleaseStringUTFChars(jenv, path, c_path);
        jsize pathLength = (*jenv)->GetStringUTFLength(jenv, path);
        newPath = (char *) DUER_MALLOC(pathLength + 1);
        if (newPath == NULL) {
            LOGE("alloc path fail!\n");
            break;
        }
        (*jenv)->GetStringUTFRegion(jenv, path, 0, pathLength, newPath);
        newPath[pathLength] = 0;
        msg->path = (duer_u8_t *)newPath;
        LOGD("path:%s, len1:%d, len2:%d;(len1 == len2)\n", newPath, pathLength, msg->path_len);
        jfieldID fieldQuery= (*jenv)->GetFieldID(jenv, s_lightduerMessageClass, "query", "Ljava/lang/String;");
        if (fieldQuery == NULL) {
            LOGE("get field query failed!\n");
            break;
        }
        jstring query = (jstring) (*jenv)->GetObjectField(jenv, message, fieldQuery);
        jsize queryLength = (*jenv)->GetStringUTFLength(jenv, query);
        newQuery = (char *) DUER_MALLOC(queryLength + 1);
        if (newQuery == NULL) {
            LOGE("alloc query fail!\n");
            break;
        }
        (*jenv)->GetStringUTFRegion(jenv, query, 0, queryLength, newQuery);
        newQuery[queryLength] = 0;
        msg->query = (duer_u8_t *)newQuery;
        LOGD("query:%s, len1:%d, len2:%d;(len1 == len2)\n",
                newQuery, queryLength, msg->query_len);

        jfieldID  fieldToken = (*jenv)->GetFieldID(jenv, s_lightduerMessageClass, "token", "[B");
        if (fieldToken == NULL) {
            LOGE("get field token failed!\n");
            break;
        }
        jbyteArray  token = (jbyteArray) (*jenv)->GetObjectField(jenv, message, fieldToken);
        jsize tokenLength = (*jenv)->GetArrayLength(jenv, token);
        newToken = (jbyte *) DUER_MALLOC(tokenLength);
        if (newToken == NULL) {
            LOGE("newToken alloc fail!\n");
            break;
        }
        LOGD("newToken: %p, tokenLength:%d, token_len:%d (tokenLength == token_len)\n",
                newToken, tokenLength, msg->token_len);
        (*jenv)->GetByteArrayRegion(jenv, token, 0, tokenLength, newToken);
        msg->token = (duer_u8_t *)newToken;
        //for(int i =0; i < tokenLength; ++i) {
        //    LOGD("token[%d]: 0x%x \n", i, msg->token[i]);
        //}
        jfieldID  fieldPayload = (*jenv)->GetFieldID(jenv, s_lightduerMessageClass, "payload", "[B");
        if (fieldPayload == NULL) {
            LOGE("get field payload failed!\n");
            break;
        }
        jbyteArray  payload = (jbyteArray) (*jenv)->GetObjectField(jenv, message, fieldPayload);
        jsize payloadLength = (*jenv)->GetArrayLength(jenv, payload);
        newPayload = (jbyte *) DUER_MALLOC(payloadLength);
        if (newPayload == NULL) {
            LOGE("newPayload alloc fail!\n");
            break;
        }
        LOGD("newPayload: %p, payloadLength:%d, payload_len:%d (payloadLength == payload_len)\n",
             newPayload, payloadLength, msg->payload_len);
        (*jenv)->GetByteArrayRegion(jenv, payload, 0, payloadLength, newPayload);
        msg->payload = (duer_u8_t *)newPayload;

        result = DUER_OK;
    } while(0);

    return result;
}



