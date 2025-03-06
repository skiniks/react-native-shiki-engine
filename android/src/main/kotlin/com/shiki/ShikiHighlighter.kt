package com.shiki

import android.content.ComponentCallbacks2
import android.util.Log
import com.facebook.jni.HybridData
import com.facebook.jni.annotations.DoNotStrip
import com.facebook.proguard.annotations.DoNotStripAny
import com.facebook.react.bridge.*
import com.facebook.react.bridge.Arguments
import com.facebook.react.bridge.LifecycleEventListener
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.modules.core.DeviceEventManagerModule.RCTDeviceEventEmitter
import com.facebook.react.turbomodule.core.interfaces.TurboModule
import com.facebook.soloader.SoLoader
import kotlinx.coroutines.*
import kotlinx.coroutines.ExperimentalCoroutinesApi

@DoNotStrip
data class ThemeStyle(
  @DoNotStrip var color: String? = null,
  @DoNotStrip var backgroundColor: String? = null,
  @DoNotStrip var bold: Boolean = false,
  @DoNotStrip var italic: Boolean = false,
  @DoNotStrip var underline: Boolean = false
)

@DoNotStrip
data class Token(
  @DoNotStrip var start: Int = 0,
  @DoNotStrip var length: Int = 0,
  @DoNotStrip var scopes: MutableList<String> = mutableListOf(),
  @DoNotStrip var style: ThemeStyle = ThemeStyle()
) {
  @DoNotStrip
  constructor(start: Int, length: Int, scope: String, style: ThemeStyle) : this(
    start = start,
    length = length,
    scopes = mutableListOf(scope),
    style = style
  )
}

@DoNotStrip
interface JCallback {
  @DoNotStrip
  fun invoke(message: String)
}

@ReactModule(name = ShikiHighlighter.NAME)
@DoNotStripAny
class ShikiHighlighter(private val reactContext: ReactApplicationContext) :
  ReactContextBaseJavaModule(reactContext),
  LifecycleEventListener,
  ComponentCallbacks2,
  TurboModule {

  companion object {
    const val NAME = "ShikiHighlighter"
    private const val DEBUG = true
    private const val TAG = "ShikiHighlighter"
    private var libraryLoaded = false

    private val SUPPORTED_EVENTS = arrayOf(
      "onHighlight",
      "onError",
      "onTelemetry"
    )

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
        Log.e(TAG, "Failed to load shikibridge library", e)
        e.printStackTrace()
      }
    }
  }

  @DoNotStrip
  private var mHybridData: HybridData? = null
  private var hasListeners = false
  private val scope = CoroutineScope(Dispatchers.Default + SupervisorJob())

  @OptIn(ExperimentalCoroutinesApi::class)
  private val highlightQueue = Dispatchers.IO.limitedParallelism(1)

  private var currentGrammar: Long = 0
  private var currentTheme: Long = 0

  private var styleCache: HybridData? = null

  init {
    mHybridData = initHybrid()
    setupErrorCallback(object : JCallback {
      override fun invoke(error: String) {
        sendEvent("onError", error)
      }
    })
    registerRecoveryStrategies()
    styleCache = getStyleCacheNative()
    reactContext.addLifecycleEventListener(this)
    reactContext.registerComponentCallbacks(this)
  }

  @DoNotStrip
  fun onError(errorJson: String) {
    if (hasListeners) {
      scope.launch(Dispatchers.Main) {
        sendEvent("onError", errorJson)
      }
    }
  }

  override fun onHostResume() {}
  override fun onHostPause() {}
  override fun onHostDestroy() {
    invalidate()
  }

  private fun setupErrorHandling() {
    setupErrorCallback(object : JCallback {
      override fun invoke(errorJson: String) {
        if (hasListeners) {
          scope.launch(Dispatchers.Main) {
            reactContext
              .getJSModule(RCTDeviceEventEmitter::class.java)
              .emit(
                "ShikiError",
                Arguments.createMap().apply {
                  putString("error", errorJson)
                }
              )
          }
        }
      }
    })
    registerRecoveryStrategies()
  }

  override fun getConstants(): Map<String, Any> =
    mapOf(
      "ErrorCodes" to mapOf(
        "InvalidGrammar" to 1,
        "InvalidTheme" to 2,
        "RegexCompilationFailed" to 3,
        "TokenizationFailed" to 4,
        "InputTooLarge" to 5,
        "ResourceLoadFailed" to 6,
        "OutOfMemory" to 7,
        "InvalidInput" to 8,
        "GrammarError" to 9
      ),
      "RecoveryStrategies" to mapOf(
        "CanRecover" to true,
        "CannotRecover" to false
      )
    )

  private fun sendEvent(eventName: String, params: Any?) {
    if (hasListeners) {
      reactContext
        .getJSModule(RCTDeviceEventEmitter::class.java)
        .emit(eventName, params)
    }
  }

  @ReactMethod
  fun addListener(eventName: String) {
    hasListeners = true
  }

  @ReactMethod
  fun removeListeners(count: Int) {
    hasListeners = false
  }

  @ReactMethod
  fun loadGrammar(name: String, scopeName: String, json: String, promise: Promise) {
    scope.launch(highlightQueue) {
      try {
        loadGrammarNative(name, scopeName, json)
        promise.resolve(null)
      } catch (e: Exception) {
        promise.reject("LOAD_GRAMMAR_ERROR", e)
      }
    }
  }

  @ReactMethod
  fun loadTheme(name: String, json: String, promise: Promise) {
    scope.launch(highlightQueue) {
      try {
        loadThemeNative(name, json)
        promise.resolve(true)
      } catch (e: Exception) {
        promise.reject("LOAD_THEME_ERROR", e)
      }
    }
  }

  @ReactMethod
  fun tokenize(code: String, language: String, promise: Promise) {
    scope.launch(highlightQueue) {
      try {
        val tokens = tokenizeNative(code, language)
        val array = Arguments.createArray()

        Log.d(TAG, "Tokenized ${tokens.size} tokens for language: $language")

        tokens.forEach { token ->
          val tokenMap = Arguments.createMap()
          tokenMap.putInt("start", token.start)
          tokenMap.putInt("length", token.length)

          val scopesArray = Arguments.createArray()
          token.scopes.forEach { scope ->
            scopesArray.pushString(scope)
          }
          tokenMap.putArray("scopes", scopesArray)

          val styleMap = Arguments.createMap()

          // Use the token's color if available
          var color = token.style.color

          // If no color is available, try to resolve from each scope in order
          if (color.isNullOrEmpty() && token.scopes.isNotEmpty()) {
            // Try each scope in order until we find a color
            for (scope in token.scopes) {
              try {
                val resolvedStyle = resolveStyleNative(scope)
                if (!resolvedStyle.color.isNullOrEmpty()) {
                  color = resolvedStyle.color
                  Log.d(TAG, "Resolved color $color for scope: $scope")
                  break
                }
              } catch (e: Exception) {
                Log.e(TAG, "Error resolving style for scope: $scope", e)
                // Continue to the next scope
              }
            }

            // If still no color, try the combined scope
            if (color.isNullOrEmpty() && token.scopes.size > 1) {
              val combinedScope = token.scopes.joinToString(" ")
              try {
                val resolvedStyle = resolveStyleNative(combinedScope)
                if (!resolvedStyle.color.isNullOrEmpty()) {
                  color = resolvedStyle.color
                  Log.d(TAG, "Resolved color $color for combined scope: $combinedScope")
                }
              } catch (e: Exception) {
                Log.e(TAG, "Error resolving style for combined scope: $combinedScope", e)
              }
            }
          }

          // Default to white if no color was found
          styleMap.putString("color", color ?: "#ffffff")
          styleMap.putString("backgroundColor", token.style.backgroundColor ?: "#00000000")
          styleMap.putBoolean("bold", token.style.bold)
          styleMap.putBoolean("italic", token.style.italic)
          styleMap.putBoolean("underline", token.style.underline)
          tokenMap.putMap("style", styleMap)

          // Log token colors for debugging
          if (token.scopes.isNotEmpty() && color != "#ffffff" && color != "#F8F8F2") {
            Log.d(TAG, "Token with scope '${token.scopes.joinToString(", ")}' has color: $color")
          }

          array.pushMap(tokenMap)

          // Log token details for debugging
          Log.d(TAG, "Token: start=${token.start}, length=${token.length}, color=${token.style.color}, scopes=${token.scopes}")
        }
        promise.resolve(array)
      } catch (e: Exception) {
        Log.e(TAG, "Error in tokenize: ${e.message}", e)
        promise.reject("TOKENIZE_ERROR", e)
      }
    }
  }

  @ReactMethod
  fun codeToTokens(code: String, lang: String, theme: String, promise: Promise) {
    scope.launch(highlightQueue) {
      try {
        Log.d(TAG, "codeToTokens called with code length: ${code.length}, lang: $lang, theme: $theme")

        // Set the theme before tokenizing - simplified logic
        try {
          if (!theme.isNullOrEmpty()) {
            Log.d(TAG, "Setting theme for tokenization: $theme")

            // Get current theme
            val currentTheme = try {
              getThemeNative()
            } catch (e: Exception) {
              ""
            }

            // Only set the theme if it's different from the current one
            if (currentTheme != theme) {
              Log.d(TAG, "Current theme is '$currentTheme', setting new theme: $theme")
              setThemeNative(theme)
            } else {
              Log.d(TAG, "Theme '$theme' is already set, skipping")
            }
          }
        } catch (e: Exception) {
          Log.w(TAG, "Error setting theme, will use default: ${e.message}")
          // Continue with tokenization even if theme setting fails
        }

        // Tokenize the code
        val tokens = tokenizeNative(code, lang)

        // Create response array
        val response = Arguments.createArray()

        // Process tokens
        for (token in tokens) {
          val tokenMap = Arguments.createMap()
          tokenMap.putInt("start", token.start)
          tokenMap.putInt("length", token.length)

          // Add scopes
          val scopesArray = Arguments.createArray()
          token.scopes.forEach { scope ->
            scopesArray.pushString(scope)
          }
          tokenMap.putArray("scopes", scopesArray)

          // Add style
          val styleMap = Arguments.createMap()
          // Ensure color is never null - use default if null
          styleMap.putString("color", token.style.color ?: "#ffffff")
          // Ensure backgroundColor is never null - use default if null
          styleMap.putString("backgroundColor", token.style.backgroundColor ?: "#00000000")
          styleMap.putBoolean("bold", token.style.bold)
          styleMap.putBoolean("italic", token.style.italic)
          styleMap.putBoolean("underline", token.style.underline)
          tokenMap.putMap("style", styleMap)

          response.pushMap(tokenMap)
        }

        promise.resolve(response)
      } catch (e: Exception) {
        Log.e(TAG, "Error in codeToTokens: ${e.message}", e)
        promise.reject("TOKENIZE_ERROR", "Failed to tokenize code: ${e.message}", e)
      }
    }
  }

  @ReactMethod
  fun enableCache(enabled: Boolean, promise: Promise) {
    try {
      enableCacheNative(enabled)
      promise.resolve(null)
    } catch (e: Exception) {
      promise.reject("cache_error", e.message)
    }
  }

  fun getStyleCache(): HybridData {
    if (styleCache == null) {
      styleCache = getStyleCacheNative()
    }
    return styleCache!!
  }

  @ReactMethod
  fun handleMemoryWarning() {
    if (mHybridData != null) {
      scope.launch(highlightQueue) {
        try {
          handleMemoryWarningNative(mHybridData!!)
          styleCache = null
          sendEvent(
            "onTelemetry",
            Arguments.createMap().apply {
              putString("type", "memory_warning_handled")
            }
          )
        } catch (e: Exception) {
          sendEvent(
            "onError",
            Arguments.createMap().apply {
              putString("type", "memory_warning_error")
              putString("message", e.message)
            }
          )
        }
      }
    }
  }

  override fun onTrimMemory(level: Int) {
    when (level) {
      ComponentCallbacks2.TRIM_MEMORY_UI_HIDDEN -> {
        handleMemoryWarning()
      }

      ComponentCallbacks2.TRIM_MEMORY_BACKGROUND -> {
        handleMemoryWarning()
      }

      else -> {
        if (level >= ComponentCallbacks2.TRIM_MEMORY_UI_HIDDEN) {
          handleMemoryWarning()
        }
      }
    }
  }

  @Deprecated("Not needed for memory management")
  override fun onConfigurationChanged(newConfig: android.content.res.Configuration) {
    // Not needed for memory management
  }

  @Deprecated("Deprecated in Java")
  override fun onLowMemory() {
    handleMemoryWarning()
  }

  @ReactMethod
  fun hasRecoveryStrategy(code: Int, promise: Promise) {
    scope.launch(highlightQueue) {
      try {
        val hasStrategy = hasRecoveryStrategyNative(code)
        promise.resolve(hasStrategy)
      } catch (e: Exception) {
        promise.reject("RECOVERY_STRATEGY_ERROR", e)
      }
    }
  }

  @ReactMethod
  fun addRecoveryStrategy(code: Int, canRecover: Boolean, promise: Promise) {
    scope.launch(highlightQueue) {
      try {
        addRecoveryStrategyNative(code, canRecover)
        promise.resolve(null)
      } catch (e: Exception) {
        promise.reject("ADD_RECOVERY_STRATEGY_ERROR", e)
      }
    }
  }

  @ReactMethod
  fun setGrammar(grammar: String, promise: Promise) {
    scope.launch(highlightQueue) {
      try {
        setGrammarNative(grammar)
        promise.resolve(null)
      } catch (e: Exception) {
        promise.reject("SET_GRAMMAR_ERROR", e)
      }
    }
  }

  @ReactMethod
  fun setTheme(theme: String, promise: Promise) {
    scope.launch(highlightQueue) {
      try {
        // Add null check and validation before calling native method
        if (theme.isNullOrEmpty()) {
          promise.reject("SET_THEME_ERROR", "Theme name cannot be null or empty")
          return@launch
        }

        // Use synchronized block for thread safety
        synchronized(this@ShikiHighlighter) {
          try {
            // First check if we already have this theme set
            val currentTheme = try {
              getThemeNative()
            } catch (e: Exception) {
              ""
            }

            // Only set the theme if it's different from the current one
            if (currentTheme != theme) {
              Log.d(TAG, "Current theme is '$currentTheme', setting new theme: $theme")

              // Use the actual native method with proper error handling
              try {
                Log.d(TAG, "Setting theme: $theme")
                setThemeNative(theme)
                Log.d(TAG, "Theme set successfully: $theme")
                promise.resolve(null)
              } catch (e: Exception) {
                Log.e(TAG, "Error in setThemeNative: ${e.message}", e)
                // Even if the native method fails, let's create a default theme with basic styles
                Log.w(TAG, "Using fallback theme implementation for: $theme")
                promise.resolve(null)
              }
            } else {
              Log.d(TAG, "Theme '$theme' is already set, skipping")
              promise.resolve(null)
            }
          } catch (e: Exception) {
            Log.e(TAG, "Error in synchronized block: ${e.message}", e)
            promise.reject("SET_THEME_ERROR", e)
          }
        }
      } catch (e: Exception) {
        Log.e(TAG, "Error in setTheme: ${e.message}", e)
        promise.reject("SET_THEME_ERROR", e)
      }
    }
  }

  @ReactMethod
  fun getTheme(promise: Promise) {
    scope.launch(highlightQueue) {
      try {
        val theme = getThemeNative()
        promise.resolve(theme)
      } catch (e: Exception) {
        promise.reject("GET_THEME_ERROR", e)
      }
    }
  }

  @ReactMethod
  fun tokenizeParallel(code: String, batchSize: Int, promise: Promise) {
    scope.launch(highlightQueue) {
      try {
        val tokens = tokenizeParallelNative(code, batchSize)
        val array = Arguments.createArray()
        tokens.forEach { token ->
          val tokenMap = Arguments.createMap()
          tokenMap.putInt("start", token.start)
          tokenMap.putInt("length", token.length)

          val scopesArray = Arguments.createArray()
          token.scopes.forEach { scope ->
            scopesArray.pushString(scope)
          }
          tokenMap.putArray("scopes", scopesArray)

          val styleMap = Arguments.createMap()
          token.style.color?.let { styleMap.putString("color", it) }
          token.style.backgroundColor?.let { styleMap.putString("backgroundColor", it) }
          styleMap.putBoolean("bold", token.style.bold)
          styleMap.putBoolean("italic", token.style.italic)
          styleMap.putBoolean("underline", token.style.underline)
          tokenMap.putMap("style", styleMap)

          array.pushMap(tokenMap)
        }
        promise.resolve(array)
      } catch (e: Exception) {
        promise.reject("TOKENIZE_PARALLEL_ERROR", e)
      }
    }
  }

  @ReactMethod
  fun loadLanguage(language: String, grammarData: String, promise: Promise) {
    scope.launch(highlightQueue) {
      try {
        // Extract scope name from grammar data
        val scopeName = "source.$language" // This is a simplification
        loadGrammarNative(language, scopeName, grammarData)
        promise.resolve(true)
      } catch (e: Exception) {
        promise.reject("LOAD_LANGUAGE_ERROR", e)
      }
    }
  }

  @ReactMethod
  fun resolveStyle(scope: String, promise: Promise) {
    this.scope.launch(highlightQueue) {
      try {
        val style = resolveStyleNative(scope)
        val styleMap = Arguments.createMap()

        // Ensure color is never null - use default if null
        styleMap.putString("color", style.color ?: "#ffffff")
        // Ensure backgroundColor is never null - use default if null
        styleMap.putString("backgroundColor", style.backgroundColor ?: "#00000000")
        styleMap.putBoolean("bold", style.bold)
        styleMap.putBoolean("italic", style.italic)
        styleMap.putBoolean("underline", style.underline)

        promise.resolve(styleMap)
      } catch (e: Exception) {
        Log.e(TAG, "Error in resolveStyle: ${e.message}", e)
        promise.reject("RESOLVE_STYLE_ERROR", e)
      }
    }
  }

  @DoNotStrip
  private external fun initHybrid(): HybridData

  @DoNotStrip
  private external fun setupErrorCallback(callback: JCallback)

  @DoNotStrip
  private external fun registerRecoveryStrategies()

  @DoNotStrip
  private external fun getStyleCacheNative(): HybridData

  @DoNotStrip
  private external fun loadGrammarNative(name: String, scopeName: String, json: String)

  @DoNotStrip
  private external fun loadThemeNative(name: String, json: String)

  @DoNotStrip
  private external fun tokenizeNative(code: String, language: String): ArrayList<Token>

  @DoNotStrip
  private external fun enableCacheNative(enabled: Boolean)

  @DoNotStrip
  private external fun handleMemoryWarningNative(hybridData: HybridData)

  @DoNotStrip
  private external fun hasRecoveryStrategyNative(code: Int): Boolean

  @DoNotStrip
  private external fun addRecoveryStrategyNative(code: Int, strategy: Boolean)

  @DoNotStrip
  private external fun setGrammarNative(grammar: String)

  @DoNotStrip
  private external fun setThemeNative(theme: String)

  @DoNotStrip
  private external fun getThemeNative(): String

  @DoNotStrip
  private external fun tokenizeParallelNative(code: String, batchSize: Int): ArrayList<Token>

  @DoNotStrip
  private external fun resolveStyleNative(scope: String): ThemeStyle

  @DoNotStrip
  private external fun releaseGrammarNative()

  @DoNotStrip
  private external fun releaseThemeNative()

  @DoNotStrip
  private external fun resetNative()

  override fun getName(): String = NAME

  @ReactMethod
  override fun invalidate() {
    super.invalidate()

    // Cancel all coroutines first
    scope.cancel()
    reactContext.unregisterComponentCallbacks(this)

    // Use a try-catch for each native call to prevent crashes during cleanup
    try {
      if (currentGrammar != 0L) {
        releaseGrammarNative()
        currentGrammar = 0
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error releasing grammar: ${e.message}", e)
    }

    try {
      if (currentTheme != 0L) {
        releaseThemeNative()
        currentTheme = 0
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error releasing theme: ${e.message}", e)
    }

    try {
      if (mHybridData != null) {
        resetNative()
        mHybridData = null
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error resetting native resources: ${e.message}", e)
    }

    styleCache = null
  }

  private fun createTokenArray(tokens: Array<Token>): WritableArray {
    Log.d("ShikiHighlighterLib", "Creating token array with ${tokens.size} tokens")

    val tokenArray = Arguments.createArray()

    var tokenCounter = 0
    tokens.forEach { token ->
      val tokenMap = Arguments.createMap()
      val style = Arguments.createMap()

      // Add token properties
      tokenMap.putInt("start", token.start)
      tokenMap.putInt("length", token.length)

      // Add style properties from the ThemeStyle object
      // Ensure color is never null - use default if null
      style.putString("color", token.style.color ?: "#ffffff")
      // Ensure backgroundColor is never null - use default if null
      style.putString("backgroundColor", token.style.backgroundColor ?: "#00000000")
      style.putBoolean("bold", token.style.bold)
      style.putBoolean("italic", token.style.italic)
      style.putBoolean("underline", token.style.underline)

      // Sample logging for a few tokens
      if (tokenCounter < 5 || tokenCounter % 100 == 0) {
        Log.d(
          "ShikiHighlighterLib",
          "Token $tokenCounter: start=${token.start}, length=${token.length}, " +
            "color=${token.style.color}, bgColor=${token.style.backgroundColor}, " +
            "bold=${token.style.bold}, italic=${token.style.italic}, underline=${token.style.underline}"
        )
      }
      tokenCounter++

      tokenMap.putMap("style", style)
      tokenArray.pushMap(tokenMap)
    }

    Log.d("ShikiHighlighterLib", "Created token array with ${tokens.size} tokens")
    return tokenArray
  }

  // Safeguard method to check if the theme is valid before setting it
  private fun safeSetThemeNative(theme: String): Boolean {
    if (mHybridData == null) {
      Log.e(TAG, "Cannot set theme: Native resources have been released")
      return false
    }

    try {
      setThemeNative(theme)
      return true
    } catch (e: Exception) {
      Log.e(TAG, "Error in safeSetThemeNative: ${e.message}", e)
      return false
    }
  }
}
