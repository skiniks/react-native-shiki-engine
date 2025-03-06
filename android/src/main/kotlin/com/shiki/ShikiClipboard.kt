package com.shiki

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.turbomodule.core.interfaces.TurboModule

@ReactModule(name = ShikiClipboard.NAME)
class ShikiClipboard(private val reactContext: ReactApplicationContext) :
  ReactContextBaseJavaModule(reactContext),
  TurboModule {
  companion object {
    const val NAME = "ShikiClipboard"
  }

  override fun getName(): String = NAME

  @ReactMethod
  fun setString(text: String, promise: Promise) {
    try {
      val clipboard = reactContext.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
      val clip = ClipData.newPlainText("code", text)
      clipboard.setPrimaryClip(clip)
      promise.resolve(null)
    } catch (e: Exception) {
      promise.reject("clipboard_error", "Failed to set clipboard content", e)
    }
  }

  @ReactMethod
  fun getString(promise: Promise) {
    try {
      val clipboard = reactContext.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
      val text = clipboard.primaryClip?.getItemAt(0)?.text?.toString() ?: ""
      promise.resolve(text)
    } catch (e: Exception) {
      promise.reject("clipboard_error", "Failed to get clipboard content", e)
    }
  }
}
