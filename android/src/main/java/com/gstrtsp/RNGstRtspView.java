package com.gstrtsp;

import android.content.Context;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.facebook.react.bridge.Arguments;
import com.facebook.react.bridge.ReactContext;
import com.facebook.react.bridge.WritableMap;
import com.facebook.react.uimanager.events.RCTEventEmitter;

public class RNGstRtspView extends SurfaceView implements SurfaceHolder.Callback {

    static {
        System.loadLibrary("gstreamer_android");
        System.loadLibrary("rn_gst_rtsp");
    }

    public static final int GST_STATE_NULL    = 1;
    public static final int GST_STATE_READY   = 2;
    public static final int GST_STATE_PAUSED  = 3;
    public static final int GST_STATE_PLAYING = 4;

    private final ReactContext mReactContext;
    private long    mHandle     = 0;
    private String  mPendingUrl = null;
    private boolean mSurfaceReady = false;

    public RNGstRtspView(Context context) {
        super(context);
        mReactContext = (ReactContext) context;
        getHolder().addCallback(this);
        mHandle = nativeInit();
    }

    /* ── Props set by ViewManager ── */

    public void setUrl(String url) {
        if (mSurfaceReady && mHandle != 0) {
            nativePlay(mHandle, url);
        } else {
            mPendingUrl = url;
        }
    }

    public void setPaused(boolean paused) {
        if (mHandle == 0) return;
        if (paused) nativePause(mHandle); else {
            if (mPendingUrl != null) nativePlay(mHandle, mPendingUrl);
        }
    }

    public void setLatency(int ms) {
        if (mHandle != 0) nativeSetLatency(mHandle, ms);
    }

    public void stopPlayback() {
        if (mHandle != 0) nativeStop(mHandle);
    }

    public void release() {
        if (mHandle != 0) { nativeRelease(mHandle); mHandle = 0; }
    }

    /* ── SurfaceHolder.Callback ── */

    @Override
    public void surfaceCreated(SurfaceHolder h) {
        mSurfaceReady = true;
        nativeSetSurface(mHandle, h.getSurface());
        if (mPendingUrl != null) {
            nativePlay(mHandle, mPendingUrl);
            mPendingUrl = null;
        }
    }

    @Override public void surfaceChanged(SurfaceHolder h, int f, int w, int ht) {
        nativeSetSurface(mHandle, h.getSurface());
    }

    @Override public void surfaceDestroyed(SurfaceHolder h) {
        mSurfaceReady = false;
        nativeSetSurface(mHandle, null);
    }

    /* ── Native → JS events ── */

    public void onNativeError(String message) {
        WritableMap p = Arguments.createMap(); p.putString("error", message);
        emit("onError", p);
    }

    public void onNativeState(int state) {
        String s;
        switch (state) {
            case GST_STATE_PLAYING: s = "playing"; break;
            case GST_STATE_PAUSED:  s = "paused";  break;
            case GST_STATE_READY:   s = "ready";   break;
            default:                s = "stopped"; break;
        }
        WritableMap p = Arguments.createMap(); p.putString("state", s);
        emit("onStateChange", p);
    }

    public void onNativePlaying() {
        emit("onPlaying", Arguments.createMap());
    }

    private void emit(String event, WritableMap params) {
        mReactContext.getJSModule(RCTEventEmitter.class)
            .receiveEvent(getId(), event, params);
    }

    /* ── JNI ── */
    private native long nativeInit();
    private native void nativeSetSurface(long h, Surface s);
    private native void nativePlay      (long h, String url);
    private native void nativePause     (long h);
    private native void nativeStop      (long h);
    private native void nativeSetLatency(long h, int ms);
    private native void nativeRelease   (long h);
}
