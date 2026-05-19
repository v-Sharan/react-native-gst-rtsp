package com.gstrtsp;

import com.facebook.react.ReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.uimanager.ViewManager;

import java.util.Collections;
import java.util.List;

public class RNGstRtspPackage implements ReactPackage {

    @Override
    public List<NativeModule> createNativeModules(ReactApplicationContext ctx) {
        return Collections.emptyList();
    }

    @Override
    public List<ViewManager> createViewManagers(ReactApplicationContext ctx) {
        return Collections.<ViewManager>singletonList(new RNGstRtspViewManager(ctx));
    }
}
