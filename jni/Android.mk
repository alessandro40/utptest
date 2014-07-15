TOP_LOCAL_PATH := $(call my-dir)

include $(TOP_LOCAL_PATH)/libutp/Android.mk
include $(TOP_LOCAL_PATH)/libevent/Android.mk

LOCAL_PATH := $(TOP_LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_STATIC_LIBRARIES := \
	    event2 \
	    utp

LOCAL_MODULE := myutp

LOCAL_LDLIBS := -llog

LOCAL_C_INCLUDES := \
	libevent/ \
	libevent/include/ \
	libevent/android/

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%)

LOCAL_SRC_FILES := \
	    utp_context.cpp \
	    utp_socket.cpp \
	    test.cpp \
	    test_wrap.cpp

include $(BUILD_SHARED_LIBRARY)
