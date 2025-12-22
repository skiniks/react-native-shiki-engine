package com.shikiengine;

import androidx.annotation.NonNull;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.WritableMap;
import com.facebook.react.bridge.ReadableArray;
import com.facebook.react.module.annotations.ReactModule;

@ReactModule(name = NativeShikiEngineSpec.NAME)
public class ShikiEngineModule extends NativeShikiEngineSpec {
    static {
        try {
            System.loadLibrary("react-native-shiki-engine");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public ShikiEngineModule(ReactApplicationContext reactContext) {
        super();
    }

    @Override
    @NonNull
    public String getName() {
        return NativeShikiEngineSpec.NAME;
    }

    @Override
    public native double createScanner(ReadableArray patterns, double maxCacheSize);

    @Override
    public native WritableMap findNextMatchSync(double scannerId, String text, double startPosition);

    @Override
    public native void destroyScanner(double scannerId);
}
