LOCAL_PATH := $(call my-dir)

ifndef GSTREAMER_ROOT_ANDROID
$(error GSTREAMER_ROOT_ANDROID is not defined. \
  Set gstreamerRoot in gradle.properties or export GSTREAMER_ROOT_ANDROID. \
  Download SDK: https://gstreamer.freedesktop.org/data/pkg/android/)
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := rn_gst_rtsp
LOCAL_SRC_FILES := gst_rtsp_jni.c
LOCAL_SHARED_LIBRARIES := gstreamer_android
LOCAL_LDLIBS   := -llog -landroid -lEGL -lGLESv2
LOCAL_CFLAGS   := -Wall -Wno-unused-variable
include $(BUILD_SHARED_LIBRARY)

GSTREAMER_ROOT            := $(GSTREAMER_ROOT_ANDROID)/$(TARGET_ARCH_ABI)
GSTREAMER_NDK_BUILD       := $(GSTREAMER_ROOT)/share/gst-android/ndk-build/

GSTREAMER_PLUGINS := \
    coreelements    \
    rtspsrc         \
    rtpmanager      \
    rtp             \
    udp             \
    tcp             \
    soup            \
    videoparsersbad \
    audioparsers    \
    rtph264depay    \
    rtph265depay    \
    rtpmp4adepay    \
    rtppcmudepay    \
    h264parse       \
    h265parse       \
    aacparse        \
    avdec_h264      \
    avdec_h265      \
    avdec_aac       \
    mulawdec        \
    autodetect      \
    glimagesink     \
    videoconvert    \
    videoscale      \
    audioconvert    \
    audioresample   \
    opengl

GSTREAMER_EXTRA_DEPS := gstreamer-video-1.0 gstreamer-rtsp-1.0
include $(GSTREAMER_NDK_BUILD)/gstreamer-1.0.mk
