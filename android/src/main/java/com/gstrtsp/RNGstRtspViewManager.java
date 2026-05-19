package com.gstrtsp;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReadableArray;
import com.facebook.react.common.MapBuilder;
import com.facebook.react.uimanager.SimpleViewManager;
import com.facebook.react.uimanager.ThemedReactContext;
import com.facebook.react.uimanager.annotations.ReactProp;

import java.util.Map;

public class RNGstRtspViewManager extends SimpleViewManager<RNGstRtspView> {

    public static final String REACT_CLASS = "RNGstRtspView";

    public RNGstRtspViewManager(ReactApplicationContext ctx) {}

    @NonNull @Override public String getName() { return REACT_CLASS; }

    @NonNull @Override
    protected RNGstRtspView createViewInstance(@NonNull ThemedReactContext ctx) {
        return new RNGstRtspView(ctx);
    }

    @ReactProp(name = "url")
    public void setUrl(RNGstRtspView v, @Nullable String url) {
        if (url != null && !url.isEmpty()) v.setUrl(url);
    }

    @ReactProp(name = "paused", defaultBoolean = false)
    public void setPaused(RNGstRtspView v, boolean paused) { v.setPaused(paused); }

    @ReactProp(name = "latency", defaultInt = 200)
    public void setLatency(RNGstRtspView v, int ms) { v.setLatency(ms); }

    @Override
    public Map<String, Integer> getCommandsMap() {
        return MapBuilder.of("stop", 1);
    }

    @Override
    public void receiveCommand(@NonNull RNGstRtspView view, String cmd, @Nullable ReadableArray args) {
        if ("stop".equals(cmd)) view.stopPlayback();
    }

    @Override
    public @Nullable Map<String, Object> getExportedCustomDirectEventTypeConstants() {
        return MapBuilder.<String, Object>builder()
            .put("onPlaying",     MapBuilder.of("registrationName", "onPlaying"))
            .put("onStateChange", MapBuilder.of("registrationName", "onStateChange"))
            .put("onError",       MapBuilder.of("registrationName", "onError"))
            .build();
    }

    @Override
    public void onDropViewInstance(@NonNull RNGstRtspView view) {
        view.release();
        super.onDropViewInstance(view);
    }
}
