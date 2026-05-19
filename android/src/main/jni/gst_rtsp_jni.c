/**
 * gst_rtsp_jni.c
 * GStreamer JNI bridge — React Native RTSP module
 *
 * Pipeline (dynamic pad, resolved from SDP at connect time):
 *   rtspsrc ──► rtph264depay ──► h264parse ──► avdec_h264 ──► videoconvert ──► glimagesink
 *           └──► rtpmp4adepay ──► aacparse  ──► avdec_aac  ──► audioconvert ──► autoaudiosink
 */

#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <pthread.h>

#define TAG  "RNGstRtsp"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)

typedef struct {
    jobject        java_instance;
    JavaVM        *java_vm;
    GMainContext  *context;
    GMainLoop     *main_loop;
    GstElement    *pipeline;
    GstElement    *rtspsrc;
    GstElement    *video_sink;
    ANativeWindow *native_window;
    GstState       target_state;
    gboolean       is_live;
    pthread_t      gst_thread;
    jmethodID      on_error_mid;
    jmethodID      on_state_mid;
    jmethodID      on_playing_mid;
} GstRtspCtx;

static JNIEnv *attach_env(GstRtspCtx *ctx) {
    JNIEnv *env;
    (*ctx->java_vm)->AttachCurrentThread(ctx->java_vm, &env, NULL);
    return env;
}

/* ─── Bus callbacks ─────────────────────────────────────────────────────── */

static void on_error(GstBus *bus, GstMessage *msg, GstRtspCtx *ctx) {
    GError *err; gchar *dbg;
    gst_message_parse_error(msg, &err, &dbg);
    LOGE("Error: %s", err->message);
    JNIEnv *env = attach_env(ctx);
    jstring jmsg = (*env)->NewStringUTF(env, err->message);
    (*env)->CallVoidMethod(env, ctx->java_instance, ctx->on_error_mid, jmsg);
    (*env)->DeleteLocalRef(env, jmsg);
    g_error_free(err); g_free(dbg);
}

static void on_state_changed(GstBus *bus, GstMessage *msg, GstRtspCtx *ctx) {
    if (GST_MESSAGE_SRC(msg) != GST_OBJECT(ctx->pipeline)) return;
    GstState old, newstate, pending;
    gst_message_parse_state_changed(msg, &old, &newstate, &pending);
    JNIEnv *env = attach_env(ctx);
    (*env)->CallVoidMethod(env, ctx->java_instance, ctx->on_state_mid, (jint)newstate);
    if (newstate == GST_STATE_PLAYING) {
        (*env)->CallVoidMethod(env, ctx->java_instance, ctx->on_playing_mid);
    }
}

static void on_eos(GstBus *bus, GstMessage *msg, GstRtspCtx *ctx) {
    LOGI("EOS");
    gst_element_set_state(ctx->pipeline, GST_STATE_NULL);
}

/* ─── Dynamic pad linking ───────────────────────────────────────────────── */

static void on_pad_added(GstElement *src, GstPad *new_pad, GstRtspCtx *ctx) {
    GstCaps      *caps = gst_pad_get_current_caps(new_pad);
    GstStructure *str  = gst_caps_get_structure(caps, 0);
    const gchar  *name = gst_structure_get_name(str);
    if (!g_str_has_prefix(name, "application/x-rtp")) goto done;

    const gchar *media = gst_structure_get_string(str, "media");
    const gchar *enc   = gst_structure_get_string(str, "encoding-name");

    if (g_strcmp0(media, "video") == 0) {
        GstElement *depay = NULL, *parse = NULL, *dec = NULL, *conv;
        if (g_strcmp0(enc, "H264") == 0) {
            depay = gst_element_factory_make("rtph264depay", NULL);
            parse = gst_element_factory_make("h264parse",    NULL);
            dec   = gst_element_factory_make("avdec_h264",   NULL);
        } else if (g_strcmp0(enc, "H265") == 0) {
            depay = gst_element_factory_make("rtph265depay", NULL);
            parse = gst_element_factory_make("h265parse",    NULL);
            dec   = gst_element_factory_make("avdec_h265",   NULL);
        } else { LOGE("Unsupported video: %s", enc); goto done; }

        conv = gst_element_factory_make("videoconvert", NULL);
        gst_bin_add_many(GST_BIN(ctx->pipeline), depay, parse, dec, conv, NULL);
        gst_element_link_many(depay, parse, dec, conv, ctx->video_sink, NULL);
        GstPad *sinkpad = gst_element_get_static_pad(depay, "sink");
        gst_pad_link(new_pad, sinkpad);
        gst_object_unref(sinkpad);
        gst_element_sync_state_with_parent(depay);
        gst_element_sync_state_with_parent(parse);
        gst_element_sync_state_with_parent(dec);
        gst_element_sync_state_with_parent(conv);
        LOGI("Video linked: %s", enc);

    } else if (g_strcmp0(media, "audio") == 0) {
        GstElement *depay = NULL, *dec = NULL, *conv, *sink;
        if (g_strcmp0(enc, "MPEG4-GENERIC") == 0 || g_strcmp0(enc, "MP4A-LATM") == 0) {
            depay = gst_element_factory_make("rtpmp4adepay", NULL);
            dec   = gst_element_factory_make("avdec_aac",    NULL);
        } else if (g_strcmp0(enc, "PCMU") == 0) {
            depay = gst_element_factory_make("rtppcmudepay", NULL);
            dec   = gst_element_factory_make("mulawdec",     NULL);
        } else { goto done; }
        conv = gst_element_factory_make("audioconvert",  NULL);
        sink = gst_element_factory_make("autoaudiosink", NULL);
        gst_bin_add_many(GST_BIN(ctx->pipeline), depay, dec, conv, sink, NULL);
        gst_element_link_many(depay, dec, conv, sink, NULL);
        GstPad *sinkpad = gst_element_get_static_pad(depay, "sink");
        gst_pad_link(new_pad, sinkpad);
        gst_object_unref(sinkpad);
        gst_element_sync_state_with_parent(depay);
        gst_element_sync_state_with_parent(dec);
        gst_element_sync_state_with_parent(conv);
        gst_element_sync_state_with_parent(sink);
        LOGI("Audio linked: %s", enc);
    }
done:
    gst_caps_unref(caps);
}

static void *gst_thread_fn(void *arg) {
    GstRtspCtx *ctx = (GstRtspCtx *)arg;
    g_main_context_push_thread_default(ctx->context);
    g_main_loop_run(ctx->main_loop);
    g_main_context_pop_thread_default(ctx->context);
    return NULL;
}

/* ═══════════════ JNI exports ═══════════════════════════════════════════════ */

JNIEXPORT jlong JNICALL
Java_com_gstrtsp_RNGstRtspView_nativeInit(JNIEnv *env, jobject thiz) {
    gst_init(NULL, NULL);
    GstRtspCtx *ctx = g_new0(GstRtspCtx, 1);
    (*env)->GetJavaVM(env, &ctx->java_vm);
    ctx->java_instance = (*env)->NewGlobalRef(env, thiz);

    jclass cls = (*env)->GetObjectClass(env, thiz);
    ctx->on_error_mid   = (*env)->GetMethodID(env, cls, "onNativeError",   "(Ljava/lang/String;)V");
    ctx->on_state_mid   = (*env)->GetMethodID(env, cls, "onNativeState",   "(I)V");
    ctx->on_playing_mid = (*env)->GetMethodID(env, cls, "onNativePlaying", "()V");

    ctx->context   = g_main_context_new();
    ctx->main_loop = g_main_loop_new(ctx->context, FALSE);
    ctx->pipeline  = gst_pipeline_new("rn-rtsp");
    ctx->rtspsrc   = gst_element_factory_make("rtspsrc",     "src");
    ctx->video_sink= gst_element_factory_make("glimagesink", "vsink");

    g_object_set(ctx->rtspsrc,
        "latency", 200, "protocols", 0x7,
        "tcp-timeout", 5000000, "do-rtsp-keep-alive", TRUE, NULL);

    gst_bin_add_many(GST_BIN(ctx->pipeline), ctx->rtspsrc, ctx->video_sink, NULL);
    g_signal_connect(ctx->rtspsrc, "pad-added", G_CALLBACK(on_pad_added), ctx);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(ctx->pipeline));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(bus, "message::error",         G_CALLBACK(on_error),         ctx);
    g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_state_changed), ctx);
    g_signal_connect(bus, "message::eos",           G_CALLBACK(on_eos),           ctx);
    gst_object_unref(bus);

    pthread_create(&ctx->gst_thread, NULL, gst_thread_fn, ctx);
    return (jlong)(intptr_t)ctx;
}

JNIEXPORT void JNICALL
Java_com_gstrtsp_RNGstRtspView_nativeSetSurface(JNIEnv *env, jobject thiz,
                                                 jlong handle, jobject surface) {
    GstRtspCtx *ctx = (GstRtspCtx *)(intptr_t)handle;
    if (!ctx) return;
    if (ctx->native_window) { ANativeWindow_release(ctx->native_window); ctx->native_window = NULL; }
    if (surface) {
        ctx->native_window = ANativeWindow_fromSurface(env, surface);
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(ctx->video_sink), (guintptr)ctx->native_window);
    } else {
        gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(ctx->video_sink), (guintptr)NULL);
    }
}

JNIEXPORT void JNICALL
Java_com_gstrtsp_RNGstRtspView_nativePlay(JNIEnv *env, jobject thiz,
                                           jlong handle, jstring jurl) {
    GstRtspCtx *ctx = (GstRtspCtx *)(intptr_t)handle;
    if (!ctx) return;
    const char *url = (*env)->GetStringUTFChars(env, jurl, NULL);
    g_object_set(ctx->rtspsrc, "location", url, NULL);
    (*env)->ReleaseStringUTFChars(env, jurl, url);
    ctx->target_state = GST_STATE_PLAYING;
    gst_element_set_state(ctx->pipeline, GST_STATE_PLAYING);
}

JNIEXPORT void JNICALL
Java_com_gstrtsp_RNGstRtspView_nativePause(JNIEnv *env, jobject thiz, jlong handle) {
    GstRtspCtx *ctx = (GstRtspCtx *)(intptr_t)handle;
    if (!ctx) return;
    gst_element_set_state(ctx->pipeline, GST_STATE_PAUSED);
}

JNIEXPORT void JNICALL
Java_com_gstrtsp_RNGstRtspView_nativeStop(JNIEnv *env, jobject thiz, jlong handle) {
    GstRtspCtx *ctx = (GstRtspCtx *)(intptr_t)handle;
    if (!ctx) return;
    gst_element_set_state(ctx->pipeline, GST_STATE_NULL);
}

JNIEXPORT void JNICALL
Java_com_gstrtsp_RNGstRtspView_nativeSetLatency(JNIEnv *env, jobject thiz,
                                                 jlong handle, jint ms) {
    GstRtspCtx *ctx = (GstRtspCtx *)(intptr_t)handle;
    if (!ctx) return;
    g_object_set(ctx->rtspsrc, "latency", (gint)ms, NULL);
}

JNIEXPORT void JNICALL
Java_com_gstrtsp_RNGstRtspView_nativeRelease(JNIEnv *env, jobject thiz, jlong handle) {
    GstRtspCtx *ctx = (GstRtspCtx *)(intptr_t)handle;
    if (!ctx) return;
    gst_element_set_state(ctx->pipeline, GST_STATE_NULL);
    g_main_loop_quit(ctx->main_loop);
    pthread_join(ctx->gst_thread, NULL);
    gst_object_unref(ctx->pipeline);
    g_main_loop_unref(ctx->main_loop);
    g_main_context_unref(ctx->context);
    if (ctx->native_window) ANativeWindow_release(ctx->native_window);
    (*env)->DeleteGlobalRef(env, ctx->java_instance);
    g_free(ctx);
}
