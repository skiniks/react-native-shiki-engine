package com.shiki

import com.facebook.react.ReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.uimanager.ViewManager
import com.shiki.ShikiBridge
import com.shiki.ShikiClipboard
import com.shiki.ShikiHighlighter
import com.shiki.view.ShikiHighlighterViewManager
import java.util.ArrayList

class ShikiPackage : ReactPackage {
  override fun createNativeModules(reactContext: ReactApplicationContext): MutableList<NativeModule> {
    val modules = mutableListOf<NativeModule>()
    modules.add(ShikiHighlighter(reactContext))
    modules.add(ShikiClipboard(reactContext))
    modules.add(ShikiBridge(reactContext))
    return modules
  }

  override fun createViewManagers(reactContext: ReactApplicationContext): List<ViewManager<*, *>> {
    val viewManagers = ArrayList<ViewManager<*, *>>()
    viewManagers.add(ShikiHighlighterViewManager())
    return viewManagers
  }
}
