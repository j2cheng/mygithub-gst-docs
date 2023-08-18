LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := tutorial-5
LOCAL_SRC_FILES := tutorial-5.c dummy.cpp
LOCAL_SRC_FILES += csio/csio.cpp \
                   csio/csioCommBase.cpp \
                   csio/gst_element_print_properties.cpp \
                   csio/gstmanager/gstManager.cpp \
                   csio/gstmanager/gstTx/gst_app_server.cpp \
                   csio/armhdcp/Hdcp_Module.cpp

$(warning $(LOCAL_PATH))
$(warning "=============================================================")

#HDCP_INC := csio/hdcp/include
#LOCAL_CFLAGS += -I$(LOCAL_PATH)/$(HDCP_INC)
### LOCAL_C_INCLUDES  := $(LOCAL_PATH)/$(HDCP_INC)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/core/inc \
                    /home/builduser/gst-docs/AOSP/core/libutils/include \
                    /home/builduser/gst-docs/AOSP/core/libcutils/include \
                    /home/builduser/gst-docs/AOSP/core/libsystem/include \
                    /home/builduser/gst-docs/AOSP/native/libs/nativewindow/include \
                    /home/builduser/gst-docs/AOSP/native/libs/nativebase/include

LOCAL_SHARED_LIBRARIES := gstreamer_android
LOCAL_LDLIBS := -llog -landroid
include $(BUILD_SHARED_LIBRARY)

ifndef GSTREAMER_ROOT_ANDROID
$(error GSTREAMER_ROOT_ANDROID is not defined!)
endif

ifeq ($(TARGET_ARCH_ABI),armeabi)
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)/arm
else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)/armv7
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)/android_arm64
else ifeq ($(TARGET_ARCH_ABI),x86)
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)/x86
else ifeq ($(TARGET_ARCH_ABI),x86_64)
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)/x86_64
else
$(error Target arch ABI not supported: $(TARGET_ARCH_ABI))
endif

GSTREAMER_NDK_BUILD_PATH  := $(GSTREAMER_ROOT)/share/gst-android/ndk-build/
include $(GSTREAMER_NDK_BUILD_PATH)/plugins.mk
GSTREAMER_PLUGINS         := $(GSTREAMER_PLUGINS_CORE) \
                             $(GSTREAMER_PLUGINS_PLAYBACK) \
                             $(GSTREAMER_PLUGINS_CODECS) \
                             $(GSTREAMER_PLUGINS_NET) \
                             $(GSTREAMER_PLUGINS_SYS) \
                             $(GSTREAMER_PLUGINS_CODECS_RESTRICTED)
                             
G_IO_MODULES              := openssl
GSTREAMER_EXTRA_DEPS      := gstreamer-video-1.0 gstreamer-rtsp-server-1.0
include $(GSTREAMER_NDK_BUILD_PATH)/gstreamer-1.0.mk


######try to include libDxHdcp.so#######################################################################################################################
# include $(CLEAR_VARS)
# LOCAL_MODULE := include libDxHdcp.so
# LOCAL_MODULE_CLASS := SHARED_LIBRARIES
# LOCAL_MODULE_TAGS := eng
# LOCAL_SRC_FILES := $(LOCAL_PATH)/core/64bit-debug/libDxHdcp.so
# include $(BUILD_PREBUILT)
######end of include libDxHdcp.so##################################################################################################