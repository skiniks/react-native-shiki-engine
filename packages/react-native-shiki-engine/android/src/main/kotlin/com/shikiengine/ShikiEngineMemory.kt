package com.shikiengine

import android.app.ActivityManager
import android.content.ComponentCallbacks2
import android.content.Context
import android.content.res.Configuration

internal class ShikiEngineMemory private constructor() : ComponentCallbacks2 {
  override fun onTrimMemory(level: Int) {
    nativeOnTrimMemory(level)
  }

  override fun onConfigurationChanged(newConfig: Configuration) = Unit

  override fun onLowMemory() {
    nativeOnLowMemory()
  }

  companion object {
    private val INSTANCE = ShikiEngineMemory()

    @Volatile private var registered = false

    @Volatile private var libraryLoaded = false

    init {
      try {
        System.loadLibrary("react-native-shiki-engine")
        libraryLoaded = true
      } catch (e: UnsatisfiedLinkError) {
        e.printStackTrace()
      }
    }

    @JvmStatic
    @Synchronized
    fun ensureRegistered(context: Context) {
      if (!libraryLoaded) return
      val app = context.applicationContext
      publishAvailableMemory(app)
      if (registered) return
      app.registerComponentCallbacks(INSTANCE)
      registered = true
    }

    private fun publishAvailableMemory(context: Context) {
      val am = context.getSystemService(Context.ACTIVITY_SERVICE) as? ActivityManager ?: return
      val info = ActivityManager.MemoryInfo()
      am.getMemoryInfo(info)
      if (info.availMem > 0) {
        nativeSetAvailableMemory(info.availMem)
      }
    }

    @JvmStatic
    private external fun nativeOnTrimMemory(level: Int)

    @JvmStatic
    private external fun nativeOnLowMemory()

    @JvmStatic
    private external fun nativeSetAvailableMemory(bytes: Long)
  }
}
