include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/platform/source-lpb100

LOCAL_MODULE := port-lpb100

LOCAL_STATIC_LIBRARIES := framework cjson connagent coap platform

LOCAL_SRC_FILES := $(wildcard $(MODULE_PATH)/*.c)

LOCAL_INCLUDES :=

include $(BUILD_STATIC_LIB)
