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

# ── Map NDK ABI names → GStreamer 1.28 folder names ──────────────────────────
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    GSTREAMER_ROOT := $(GSTREAMER_ROOT_ANDROID)/arm64
else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    GSTREAMER_ROOT := $(GSTREAMER_ROOT_ANDROID)/armv7
else ifeq ($(TARGET_ARCH_ABI),x86)
    GSTREAMER_ROOT := $(GSTREAMER_ROOT_ANDROID)/x86
else ifeq ($(TARGET_ARCH_ABI),x86_64)
    GSTREAMER_ROOT := $(GSTREAMER_ROOT_ANDROID)/x86_64
endif

# GStreamer 1.28 moved GStreamer.mk → gstreamer-1.0.mk and the share folder
# is at the root of the universal package (not per-ABI)
GSTREAMER_NDK_BUILD := $(GSTREAMER_ROOT_ANDROID)/share/gst-android/ndk-build

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

# Support both old (GStreamer.mk) and new (gstreamer-1.0.mk) naming
ifneq ($(wildcard $(GSTREAMER_NDK_BUILD)/gstreamer-1.0.mk),)
    include $(GSTREAMER_NDK_BUILD)/gstreamer-1.0.mk
else
    include $(GSTREAMER_NDK_BUILD)/GStreamer.mk
endif
