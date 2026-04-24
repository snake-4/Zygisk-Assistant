LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/elfio
LOCAL_MODULE := zygisk
LOCAL_SRC_FILES := fd_reopener.cpp utils.cpp map_parser.cpp mountinfo_parser.cpp modules.cpp main.cpp atexit.cpp
LOCAL_STATIC_LIBRARIES := libcxx libsystemproperties libfdutils
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)

include jni/libcxx/Android.mk
include jni/aosp_system_properties/Android.mk
include jni/aosp_fd_utils/Android.mk
