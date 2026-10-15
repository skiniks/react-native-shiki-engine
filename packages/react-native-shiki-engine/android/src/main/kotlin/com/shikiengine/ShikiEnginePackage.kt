package com.shikiengine

import com.facebook.react.BaseReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.module.model.ReactModuleInfoProvider

class ShikiEnginePackage : BaseReactPackage() {
  init {
    try {
      System.loadLibrary("react-native-shiki-engine")
    } catch (e: UnsatisfiedLinkError) {
      e.printStackTrace()
    }
  }

  override fun getModule(name: String, reactContext: ReactApplicationContext): NativeModule? {
    ShikiEngineMemory.ensureRegistered(reactContext)
    return null
  }

  override fun getReactModuleInfoProvider(): ReactModuleInfoProvider =
    ReactModuleInfoProvider {
      emptyMap()
    }
}
