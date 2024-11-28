package com.shikiengine;

import androidx.annotation.NonNull;
import com.facebook.react.TurboReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.model.ReactModuleInfo;
import com.facebook.react.module.model.ReactModuleInfoProvider;

import java.util.HashMap;
import java.util.Map;

public class ShikiEnginePackage extends TurboReactPackage {
    @Override
    @NonNull
    public NativeModule getModule(@NonNull String name, @NonNull ReactApplicationContext reactContext) {
        if (name.equals(NativeShikiEngineSpec.NAME)) {
            return new ShikiEngineModule(reactContext);
        }
        return null;
    }

    @Override
    @NonNull
    public ReactModuleInfoProvider getReactModuleInfoProvider() {
        return () -> {
            final Map<String, ReactModuleInfo> moduleInfo = new HashMap<>();
            boolean isTurboModule = BuildConfig.IS_NEW_ARCHITECTURE_ENABLED;

            moduleInfo.put(
                NativeShikiEngineSpec.NAME,
                new ReactModuleInfo(
                    NativeShikiEngineSpec.NAME,
                    NativeShikiEngineSpec.NAME,
                    false,
                    false,
                    true,
                    false,
                    isTurboModule
                )
            );
            return moduleInfo;
        };
    }
}
