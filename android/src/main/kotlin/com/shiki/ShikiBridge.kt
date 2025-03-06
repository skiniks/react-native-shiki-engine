package com.shiki

import android.util.Log
import com.facebook.react.bridge.JavaScriptContextHolder
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.turbomodule.core.interfaces.TurboModule
import com.facebook.soloader.SoLoader
import java.util.Collections

@ReactModule(name = ShikiBridge.NAME)
class ShikiBridge(reactContext: ReactApplicationContext) :
  ReactContextBaseJavaModule(reactContext),
  TurboModule {

  companion object {
    const val NAME = "ShikiBridge"
    private const val TAG = "ShikiBridge"
    private var libraryLoaded = false

    init {
      try {
        // Only load the library if it hasn't been loaded
        if (!libraryLoaded) {
          Log.d(TAG, "Loading shikibridge library")
          try {
            System.loadLibrary("shikibridge")
            libraryLoaded = true
            Log.d(TAG, "Successfully loaded shikibridge library using System.loadLibrary")
          } catch (e: UnsatisfiedLinkError) {
            try {
              // Fallback to SoLoader
              SoLoader.loadLibrary("shikibridge")
              libraryLoaded = true
              Log.d(TAG, "Successfully loaded shikibridge library using SoLoader")
            } catch (e2: Exception) {
              Log.e(TAG, "Failed to load shikibridge library with SoLoader", e2)
              throw e2
            }
          }
        } else {
          Log.d(TAG, "shikibridge library already loaded")
        }
      } catch (e: Exception) {
        // Log error
        Log.e(TAG, "Failed to load shikibridge library", e)
        e.printStackTrace()
      }
    }
  }

  private var installedBindings = false

  // Method to install JSI bindings
  @ReactMethod(isBlockingSynchronousMethod = true)
  fun install(): Boolean {
    try {
      if (installedBindings) {
        Log.d(TAG, "Bindings already installed")
        return true
      }

      val jsContext = reactApplicationContext.javaScriptContextHolder
      if (jsContext != null) {
        synchronized(this) {
          if (!installedBindings) {
            Log.d(TAG, "Installing JSI bindings")
            nativeInstall(jsContext)
            installedBindings = true
            Log.d(TAG, "JSI bindings installed successfully")
          }
        }
        return true
      } else {
        Log.e(TAG, "JavaScript context is null")
      }
    } catch (exception: Exception) {
      Log.e(TAG, "Exception during JSI binding installation", exception)
      exception.printStackTrace()
    }

    return false
  }

  override fun getName(): String = NAME

  override fun getConstants(): Map<String, Any> = Collections.emptyMap()

  private external fun nativeInstall(jsContext: JavaScriptContextHolder)
}
