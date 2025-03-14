package com.shiki.view

import android.util.Log
import android.view.ViewGroup
import com.facebook.react.bridge.ReadableArray
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.uimanager.SimpleViewManager
import com.facebook.react.uimanager.ThemedReactContext
import com.facebook.react.uimanager.ViewManagerDelegate
import com.facebook.react.uimanager.annotations.ReactProp
import com.facebook.react.viewmanagers.ShikiHighlighterViewManagerDelegate
import com.facebook.react.viewmanagers.ShikiHighlighterViewManagerInterface

@ReactModule(name = ShikiHighlighterViewManager.REACT_CLASS)
class ShikiHighlighterViewManager :
  SimpleViewManager<ShikiHighlighterView>(),
  ShikiHighlighterViewManagerInterface<ShikiHighlighterView> {

  private val mDelegate = ShikiHighlighterViewManagerDelegate<ShikiHighlighterView, ShikiHighlighterViewManager>(this)

  companion object {
    const val REACT_CLASS = "ShikiHighlighterView"
    private const val TAG = "ShikiHighlighterViewManager"
  }

  override fun getName(): String = REACT_CLASS

  override fun getDelegate(): ViewManagerDelegate<ShikiHighlighterView> = mDelegate

  override fun createViewInstance(context: ThemedReactContext): ShikiHighlighterView {
    Log.d(TAG, "Creating ShikiHighlighterView instance")
    val view = ShikiHighlighterView(context)

    view.layoutParams = ViewGroup.LayoutParams(
      ViewGroup.LayoutParams.MATCH_PARENT,
      ViewGroup.LayoutParams.MATCH_PARENT
    )

    return view
  }

  @ReactProp(name = "text")
  override fun setText(view: ShikiHighlighterView, text: String?) {
    Log.d(TAG, "Setting text: ${text?.length ?: 0} characters")
    text?.let { view.setText(it) }
  }

  @ReactProp(name = "tokens")
  override fun setTokens(view: ShikiHighlighterView, tokens: ReadableArray?) {
    Log.d(TAG, "Setting tokens: ${tokens?.size() ?: 0} tokens")
    tokens?.let { view.setTokens(it) }
  }

  @ReactProp(name = "fontSize")
  override fun setFontSize(view: ShikiHighlighterView, fontSize: Float) {
    Log.d(TAG, "Setting fontSize: $fontSize")
    view.setFontSize(fontSize)
  }

  @ReactProp(name = "fontFamily")
  override fun setFontFamily(view: ShikiHighlighterView, fontFamily: String?) {
    Log.d(TAG, "Setting fontFamily: $fontFamily")
    view.setFontFamily(fontFamily)
  }

  @ReactProp(name = "fontWeight")
  override fun setFontWeight(view: ShikiHighlighterView, fontWeight: String?) {
    Log.d(TAG, "Setting fontWeight: $fontWeight")
    view.setFontWeight(fontWeight)
  }

  @ReactProp(name = "fontStyle")
  override fun setFontStyle(view: ShikiHighlighterView, fontStyle: String?) {
    Log.d(TAG, "Setting fontStyle: $fontStyle")
    view.setFontStyle(fontStyle)
  }

  @ReactProp(name = "showLineNumbers")
  override fun setShowLineNumbers(view: ShikiHighlighterView, showLineNumbers: Boolean) {
    Log.d(TAG, "Setting showLineNumbers: $showLineNumbers")
    view.setShowLineNumbers(showLineNumbers)
  }

  @ReactProp(name = "selectable")
  override fun setSelectable(view: ShikiHighlighterView, selectable: Boolean) {
    Log.d(TAG, "Setting selectable: $selectable")
    view.setSelectable(selectable)
  }

  @ReactProp(name = "contentInset")
  override fun setContentInset(view: ShikiHighlighterView, contentInset: ReadableMap?) {
    Log.d(TAG, "Setting contentInset: $contentInset")
    view.setContentInset(contentInset)
  }

  @ReactProp(name = "scrollEnabled")
  override fun setScrollEnabled(view: ShikiHighlighterView, enabled: Boolean) {
    Log.d(TAG, "Setting scrollEnabled: $enabled")
    view.setScrollEnabled(enabled)
  }

  @ReactProp(name = "language")
  override fun setLanguage(view: ShikiHighlighterView, language: String?) {
    Log.d(TAG, "Setting language: $language")
    view.setLanguage(language)
  }

  @ReactProp(name = "theme")
  override fun setTheme(view: ShikiHighlighterView, theme: String?) {
    Log.d(TAG, "Setting theme: $theme")
    view.setTheme(theme)
  }
}
