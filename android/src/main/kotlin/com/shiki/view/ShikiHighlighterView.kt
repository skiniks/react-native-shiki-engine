package com.shiki.view

import android.content.Context
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Typeface
import android.text.SpannableStringBuilder
import android.text.style.ForegroundColorSpan
import android.text.style.LineHeightSpan
import android.text.style.StyleSpan
import android.text.style.TypefaceSpan
import android.text.style.UnderlineSpan
import android.util.Log
import android.util.TypedValue
import android.view.View.MeasureSpec
import android.view.ViewGroup
import android.widget.TextView
import com.facebook.react.bridge.ReadableArray
import com.facebook.react.bridge.ReadableMap
import com.facebook.react.views.view.ReactViewGroup
import com.facebook.soloader.SoLoader

class ShikiHighlighterView(context: Context) : ReactViewGroup(context) {
  private val textView: TextView = TextView(context)
  private var text: String = ""
  private var tokens: ReadableArray? = null
  private var fontSize: Float = 14f
  private var fontFamily: String? = null
  private var fontWeight: String = "regular"
  private var fontStyle: String = "normal"
  private var showLineNumbers: Boolean = false
  private var selectable: Boolean = true
  private var language: String? = null
  private var theme: String? = null
  private var contentInset = mapOf(
    "top" to 0,
    "right" to 0,
    "bottom" to 0,
    "left" to 0
  )
  private var scrollEnabled: Boolean = true

  companion object {
    private const val TAG = "ShikiHighlighterView"
    private const val LINE_SPACING = 0.5f
    private const val PARAGRAPH_SPACING = 1.0f
    private const val DEFAULT_FONT_SIZE = 14f
    private const val DEFAULT_BATCH_SIZE = 32 * 1024

    private val SYSTEM_FONTS = mapOf(
      "SFMono-Regular" to "monospace",
      "Menlo-Regular" to "monospace",
      "Menlo" to "monospace",
      "monospace" to "monospace",
      "SF-Mono" to "monospace",
      "Courier" to "serif-monospace",
      "Monaco" to "sans-serif-monospace",
      "System" to "sans-serif-monospace"
    )

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
        Log.e(TAG, "Failed to load shikibridge library", e)
        e.printStackTrace()
      }
    }
  }

  init {
    // Configure the TextView
    textView.apply {
      layoutParams = LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT,
        ViewGroup.LayoutParams.WRAP_CONTENT
      )
      setTextIsSelectable(true)
      // Do NOT set a default text color - this would override token colors
      // Let each token have its own color
      setTextColor(Color.TRANSPARENT) // This allows individual spans to control colors
      Log.d(TAG, "TextView initial text color set to TRANSPARENT to allow token colors to show")

      setPadding(dpToPx(12), dpToPx(12), dpToPx(12), dpToPx(12))
      setHorizontallyScrolling(true)
      setBackgroundColor(Color.TRANSPARENT)
      Log.d(TAG, "TextView background color set to TRANSPARENT")
      includeFontPadding = false
      textSize = DEFAULT_FONT_SIZE

      // Set fixed minimum dimensions to ensure visibility
      minWidth = dpToPx(300)
      minHeight = dpToPx(200)

      // Make sure the text is visible
      visibility = VISIBLE
    }

    // Add TextView directly to this view
    addView(textView)

    // Set a dark background color for contrast with colored text
    setBackgroundColor(Color.parseColor("#282A36"))
    Log.d(TAG, "ShikiHighlighterView background color set to #282A36")

    // Set layout parameters for this view
    layoutParams = ViewGroup.LayoutParams(
      ViewGroup.LayoutParams.MATCH_PARENT,
      ViewGroup.LayoutParams.MATCH_PARENT
    )

    updateFont()
  }

  fun setText(text: String) {
    Log.d(TAG, "setText called with text length: ${text.length}")
    Log.d(TAG, "Text preview: ${if (text.length > 50) text.substring(0, 50) + "..." else text}")
    this.text = text
    processTextWithBatches()
    // Force a layout update
    requestLayout()
    invalidate()
  }

  fun setTokens(tokens: ReadableArray) {
    Log.d(TAG, "setTokens called with tokens count: ${tokens.size()}")

    // Log a sample of tokens and their colors for debugging
    if (tokens.size() > 0) {
      try {
        val sampleCount = Math.min(10, tokens.size())
        Log.d(TAG, "Sample of first $sampleCount tokens:")
        var colorCount = 0
        for (i in 0 until sampleCount) {
          val token = tokens.getMap(i)
          if (token != null) {
            val start = token.getInt("start")
            val length = token.getInt("length")
            val style = token.getMap("style")
            val colorStr = style?.getString("color") ?: "null"

            // Check if style contains color
            if (style != null && style.hasKey("color") && style.getString("color") != null) {
              colorCount++
            }

            Log.d(
              TAG,
              "Token $i: start=$start, length=$length, color=$colorStr, style keys: ${style?.toHashMap()?.keys?.joinToString(
                ", "
              ) ?: "none"}"
            )
          }
        }
        Log.d(TAG, "$colorCount out of $sampleCount sample tokens have color information")

        // Count total tokens with color
        var totalWithColor = 0
        for (i in 0 until tokens.size()) {
          val token = tokens.getMap(i)
          if (token != null) {
            val style = token.getMap("style")
            if (style != null && style.hasKey("color") && style.getString("color") != null) {
              totalWithColor++
            }
          }
        }
        Log.d(TAG, "$totalWithColor out of ${tokens.size()} total tokens have color information")
      } catch (e: Exception) {
        Log.e(TAG, "Error logging token samples: ${e.message}")
      }
    }

    this.tokens = tokens
    processTextWithBatches()
    // Force a layout update
    requestLayout()
    invalidate()
  }

  fun setFontSize(size: Float) {
    fontSize = size
    textView.textSize = size
    processTextWithBatches()
  }

  fun setFontFamily(family: String?) {
    Log.d(TAG, "Setting fontFamily: $family (previous: $fontFamily)")

    // Only update if the font family has changed
    if (family != fontFamily) {
      fontFamily = family
      Log.d(TAG, "Font family changed, updating font")
      updateFont()

      // Make sure to reapply text formatting
      if (!text.isEmpty() && tokens != null) {
        Log.d(TAG, "Reapplying text formatting after font family change")
        applyTextFormatting()
      }

      // Force a layout update
      requestLayout()
      invalidate()
    } else {
      Log.d(TAG, "Font family unchanged, skipping update")
    }
  }

  fun setFontWeight(fontWeight: String?) {
    try {
      Log.d(TAG, "Setting fontWeight: $fontWeight")
      this.fontWeight = fontWeight ?: "regular"

      if (!text.isEmpty() && tokens != null) {
        applyTextFormatting()
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error in setFontWeight: ${e.message}", e)
    }
  }

  fun setFontStyle(fontStyle: String?) {
    try {
      Log.d(TAG, "Setting fontStyle: $fontStyle")
      this.fontStyle = fontStyle ?: "normal"

      if (!text.isEmpty() && tokens != null) {
        applyTextFormatting()
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error in setFontStyle: ${e.message}", e)
    }
  }

  fun setShowLineNumbers(showLineNumbers: Boolean) {
    try {
      Log.d(TAG, "Setting showLineNumbers: $showLineNumbers")
      this.showLineNumbers = showLineNumbers

      if (!text.isEmpty() && tokens != null) {
        applyTextFormatting()
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error in setShowLineNumbers: ${e.message}", e)
    }
  }

  fun setSelectable(selectable: Boolean) {
    this.selectable = selectable
    textView.setTextIsSelectable(selectable)
  }

  fun setContentInset(contentInset: ReadableMap?) {
    if (contentInset != null) {
      val top = if (contentInset.hasKey("top")) contentInset.getInt("top") else 0
      val right = if (contentInset.hasKey("right")) contentInset.getInt("right") else 0
      val bottom = if (contentInset.hasKey("bottom")) contentInset.getInt("bottom") else 0
      val left = if (contentInset.hasKey("left")) contentInset.getInt("left") else 0

      this.contentInset = mapOf(
        "top" to top,
        "right" to right,
        "bottom" to bottom,
        "left" to left
      )

      textView.setPadding(
        dpToPx(left),
        dpToPx(top),
        dpToPx(right),
        dpToPx(bottom)
      )
    }
  }

  fun setScrollEnabled(enabled: Boolean) {
    try {
      Log.d(TAG, "Setting scrollEnabled: $enabled")
      scrollEnabled = enabled

      textView.isVerticalScrollBarEnabled = enabled

      if (enabled) {
        textView.scrollBarStyle = android.view.View.SCROLLBARS_INSIDE_OVERLAY
        textView.isVerticalFadingEdgeEnabled = true
        textView.setVerticalScrollbarPosition(android.view.View.SCROLLBAR_POSITION_RIGHT)

        textView.overScrollMode = android.view.View.OVER_SCROLL_ALWAYS
      }

      if (enabled) {
        textView.setHorizontallyScrolling(true)

        // For vertical scrolling, we need to use ScrollingMovementMethod
        textView.movementMethod = android.text.method.ScrollingMovementMethod.getInstance()

        // Make sure the TextView can take focus for scrolling
        textView.isFocusable = true
        textView.isFocusableInTouchMode = true

        // Set max height to allow scrolling
        textView.maxHeight = Int.MAX_VALUE
      } else {
        // Disable scrolling
        textView.movementMethod = null
        textView.isFocusable = selectable
        textView.isFocusableInTouchMode = selectable
      }

      // Update layout parameters
      val params = textView.layoutParams
      if (params != null) {
        if (enabled) {
          // When scrollable, allow the TextView to be as tall as needed
          params.height = ViewGroup.LayoutParams.WRAP_CONTENT
        } else {
          // When not scrollable, match the parent height
          params.height = ViewGroup.LayoutParams.MATCH_PARENT
        }
        textView.layoutParams = params
      }

      // Force layout update
      textView.requestLayout()
      invalidate()

      Log.d(TAG, "ScrollEnabled set to $enabled successfully")
    } catch (e: Exception) {
      Log.e(TAG, "Error setting scrollEnabled: ${e.message}", e)
    }
  }

  fun setLanguage(language: String?) {
    try {
      Log.d(TAG, "Setting language: $language")
      // Store the language value, but don't do anything with it yet
      // We'll use it when we actually need to tokenize
      this.language = language

      // Don't trigger any processing here, just store the value
    } catch (e: Exception) {
      Log.e(TAG, "Error in setLanguage: ${e.message}", e)
    }
  }

  fun setTheme(theme: String?) {
    try {
      Log.d(TAG, "Setting theme: $theme")
      // Store the theme value, but don't do anything with it yet
      // We'll use it when we actually need to tokenize
      this.theme = theme

      // If we have text and tokens, we might need to reapply formatting
      if (!text.isEmpty() && tokens != null) {
        applyTextFormatting()
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error in setTheme: ${e.message}", e)
    }
  }

  private fun updateFont() {
    try {
      Log.d(TAG, "Updating font to family: $fontFamily, weight: $fontWeight, style: $fontStyle")

      var typefaceStyle = Typeface.NORMAL
      if (fontWeight == "bold") {
        typefaceStyle = Typeface.BOLD
      } else if (fontWeight == "medium") {
        // Medium is not directly supported, use BOLD
        typefaceStyle = Typeface.BOLD
      }

      if (fontStyle == "italic") {
        typefaceStyle = typefaceStyle or Typeface.ITALIC
      }

      // Map the font family to a system font if needed
      val mappedFontFamily = if (fontFamily != null) {
        val systemFont = SYSTEM_FONTS[fontFamily]
        if (systemFont != null) {
          Log.d(TAG, "Mapped font family '$fontFamily' to system font '$systemFont'")
          systemFont
        } else {
          Log.d(TAG, "Using font family '$fontFamily' directly (no mapping found)")
          // If no mapping found, default to monospace for code
          Log.d(TAG, "No mapping found for '$fontFamily', defaulting to monospace")
          "monospace"
        }
      } else {
        "monospace"
      }

      // Try to create the typeface with the provided font family
      val typeface = try {
        // Try to use the exact font family name provided
        val tf = Typeface.create(mappedFontFamily, typefaceStyle)
        Log.d(TAG, "Created typeface with font family '$mappedFontFamily' and style $typefaceStyle")
        // Log additional typeface information
        Log.d(TAG, "Typeface details - isBold: ${tf.isBold}, isItalic: ${tf.isItalic}")
        tf
      } catch (e: Exception) {
        Log.w(TAG, "Could not create typeface with font family '$mappedFontFamily', falling back to monospace", e)
        // Fall back to monospace if the provided font family doesn't work
        Typeface.create("monospace", typefaceStyle)
      }

      textView.typeface = typeface

      Log.d(TAG, "Font updated successfully with style $typefaceStyle")

      // Force a redraw and reapply text formatting
      textView.invalidate()
      invalidate()

      // If we have text and tokens, reapply the formatting
      if (!text.isEmpty() && tokens != null) {
        Log.d(TAG, "Reapplying text formatting after font update")
        applyTextFormatting()
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error updating font: ${e.message}", e)
      // Fallback to default monospace font
      textView.typeface = Typeface.MONOSPACE
    }
  }

  private fun processTextWithBatches() {
    Log.d(TAG, "processTextWithBatches called")
    try {
      if (text.isEmpty()) {
        Log.d(TAG, "Text is empty, setting empty text")
        textView.text = ""
        return
      }

      // Check if tokens are null or empty
      if (tokens == null || tokens?.size() == 0) {
        Log.d(TAG, "Tokens are null or empty, setting plain text")
        textView.text = text
        return
      }

      try {
        // Use the applyTextFormatting method to handle both token styling and line numbers
        applyTextFormatting()
      } catch (e: Exception) {
        Log.e(TAG, "Error in applyTextFormatting: ${e.message}", e)
        // Fallback to plain text if formatting fails
        textView.text = text
      }

      // Force layout update
      post {
        try {
          textView.requestLayout()
          invalidate()
        } catch (e: Exception) {
          Log.e(TAG, "Error in post-layout update: ${e.message}", e)
        }
      }

      Log.d(TAG, "TextView text length: ${textView.text?.length}")
      Log.d(TAG, "TextView text color: ${textView.currentTextColor}")
      Log.d(TAG, "TextView visibility: ${textView.visibility}")
      Log.d(TAG, "TextView width: ${textView.width}, height: ${textView.height}")
      Log.d(TAG, "Language: $language, Theme: $theme")

      Log.d(TAG, "processTextWithBatches completed")
    } catch (e: Exception) {
      Log.e(TAG, "Error in processTextWithBatches: ${e.message}", e)
      // Fallback to plain text if processing fails
      try {
        textView.text = text
      } catch (e2: Exception) {
        Log.e(TAG, "Error setting fallback text: ${e2.message}", e2)
      }
    }
  }

  private fun applyTokenStyle(text: SpannableStringBuilder, start: Int, length: Int, style: ReadableMap?) {
    // Only log detailed info for a sample of tokens to avoid log flooding
    val shouldLogDetails = Math.random() < 0.05 // Log about 5% of tokens

    if (shouldLogDetails) {
      Log.d(TAG, "applyTokenStyle called with start: $start, length: $length")
    }

    if (style == null) {
      if (shouldLogDetails) {
        Log.d(TAG, "Style is null")
      }
      return
    }

    // Skip tokens with invalid ranges
    if (start < 0 || length <= 0 || start + length > text.length) {
      Log.e(TAG, "Invalid token range: start=$start, length=$length, textLength=${text.length}")
      return
    }

    if (shouldLogDetails) {
      val styleKeys = style.toHashMap().keys.joinToString(", ")
      Log.d(TAG, "Style keys: [$styleKeys]")
    }

    // Apply color from the theme
    var colorApplied = false
    if (style.hasKey("color")) {
      val colorStr = style.getString("color")
      if (colorStr != null && colorStr.isNotEmpty()) {
        if (shouldLogDetails) {
          Log.d(TAG, "Color: $colorStr")
        }

        // Don't adjust colors - use the theme colors directly
        parseColor(colorStr)?.let { color ->
          if (shouldLogDetails) {
            Log.d(TAG, "Parsed color: $color (${Integer.toHexString(color)})")
          }

          try {
            text.setSpan(
              ForegroundColorSpan(color),
              start,
              start + length,
              SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
            )
            colorApplied = true
            if (shouldLogDetails) {
              Log.d(TAG, "Applied color span successfully")
            }
          } catch (e: Exception) {
            Log.e(TAG, "Error applying color span: ${e.message}")
            e.printStackTrace()
          }
        }
      }
    }

    // If no color was applied, set a default color (white for dark themes)
    if (!colorApplied) {
      if (shouldLogDetails) {
        Log.d(TAG, "No color specified or couldn't be parsed, using default light color")
      }

      try {
        text.setSpan(
          ForegroundColorSpan(Color.WHITE),
          start,
          start + length,
          SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
        )
      } catch (e: Exception) {
        Log.e(TAG, "Error applying default color span: ${e.message}")
        e.printStackTrace()
      }
    }

    // Apply background color if available
    if (style.hasKey("backgroundColor")) {
      val bgColorStr = style.getString("backgroundColor")
      if (bgColorStr != null && bgColorStr.isNotEmpty()) {
        if (shouldLogDetails) {
          Log.d(TAG, "Background color: $bgColorStr")
        }

        parseColor(bgColorStr)?.let { bgColor ->
          if (shouldLogDetails) {
            Log.d(TAG, "Parsed background color: $bgColor")
          }

          try {
            text.setSpan(
              android.text.style.BackgroundColorSpan(bgColor),
              start,
              start + length,
              SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
            )
            if (shouldLogDetails) {
              Log.d(TAG, "Applied background color span successfully")
            }
          } catch (e: Exception) {
            Log.e(TAG, "Error applying background color span: ${e.message}")
            e.printStackTrace()
          }
        }
      }
    }

    var fontStyle = Typeface.NORMAL
    if (style.hasKey("bold") && style.getBoolean("bold")) {
      if (shouldLogDetails) {
        Log.d(TAG, "Bold: true")
      }
      fontStyle = fontStyle or Typeface.BOLD
    }

    if (style.hasKey("italic") && style.getBoolean("italic")) {
      if (shouldLogDetails) {
        Log.d(TAG, "Italic: true")
      }
      fontStyle = fontStyle or Typeface.ITALIC
    }

    if (fontStyle != Typeface.NORMAL) {
      if (shouldLogDetails) {
        Log.d(TAG, "Setting font style: $fontStyle")
      }

      try {
        text.setSpan(
          StyleSpan(fontStyle),
          start,
          start + length,
          SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
        )
        if (shouldLogDetails) {
          Log.d(TAG, "Applied style span successfully")
        }
      } catch (e: Exception) {
        Log.e(TAG, "Error applying style span: ${e.message}")
        e.printStackTrace()
      }
    }

    // Apply underline if needed
    if (style.hasKey("underline") && style.getBoolean("underline")) {
      if (shouldLogDetails) {
        Log.d(TAG, "Underline: true")
      }

      try {
        text.setSpan(
          UnderlineSpan(),
          start,
          start + length,
          SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
        )
        if (shouldLogDetails) {
          Log.d(TAG, "Applied underline span successfully")
        }
      } catch (e: Exception) {
        Log.e(TAG, "Error applying underline span: ${e.message}")
        e.printStackTrace()
      }
    }

    // Apply line height
    try {
      text.setSpan(
        object : LineHeightSpan {
          override fun chooseHeight(
            text: CharSequence,
            start: Int,
            end: Int,
            spanstartv: Int,
            lineHeight: Int,
            fm: Paint.FontMetricsInt
          ) {
            fm.descent += LINE_SPACING.toInt()
            fm.bottom += PARAGRAPH_SPACING.toInt()
          }
        },
        start,
        start + length,
        SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
      )
      if (shouldLogDetails) {
        Log.d(TAG, "Applied line height span successfully")
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error applying line height span: ${e.message}")
      e.printStackTrace()
    }

    if (shouldLogDetails) {
      Log.d(TAG, "applyTokenStyle completed for token at position $start")
    }
  }

  private external fun processBatchNative(text: String, start: Int, length: Int): Array<Any>

  private fun parseColor(colorStr: String): Int? =
    try {
      if (colorStr.isEmpty()) {
        Log.d(TAG, "Empty color string, returning null")
        null
      } else {
        // Try parsing different formats of the color string
        val formattedColor = when {
          colorStr.startsWith("#") -> colorStr

          colorStr.matches(Regex("[0-9a-fA-F]{6}")) -> "#$colorStr"

          // For hex without #
          colorStr.matches(Regex("[0-9a-fA-F]{8}")) -> "#$colorStr"

          // For hex with alpha without #
          else -> colorStr // Try as is for named colors
        }

        try {
          val color = Color.parseColor(formattedColor)
          Log.d(TAG, "Successfully parsed color: $colorStr -> $formattedColor -> ${Integer.toHexString(color)}")
          color
        } catch (e: IllegalArgumentException) {
          // Try as a named color directly
          try {
            val namedColor = Color.parseColor(colorStr)
            Log.d(TAG, "Successfully parsed named color: $colorStr -> ${Integer.toHexString(namedColor)}")
            namedColor
          } catch (e2: IllegalArgumentException) {
            // Try with # prefix as last resort
            try {
              if (!colorStr.startsWith("#")) {
                val prefixedColor = "#$colorStr"
                val color = Color.parseColor(prefixedColor)
                Log.d(TAG, "Successfully parsed color with # prefix: $colorStr -> $prefixedColor -> ${Integer.toHexString(color)}")
                color
              } else {
                throw e2
              }
            } catch (e3: IllegalArgumentException) {
              Log.e(TAG, "Failed to parse color '$colorStr' in any format: ${e3.message}")
              null
            }
          }
        }
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error parsing color: $colorStr, ${e.message}")
      null
    }

  private fun dpToPx(dp: Int): Int =
    TypedValue.applyDimension(
      TypedValue.COMPLEX_UNIT_DIP,
      dp.toFloat(),
      context.resources.displayMetrics
    ).toInt()

  override fun onLayout(
    changed: Boolean,
    left: Int,
    top: Int,
    right: Int,
    bottom: Int
  ) {
    Log.d(TAG, "onLayout called: changed=$changed, left=$left, top=$top, right=$right, bottom=$bottom")

    // Calculate our dimensions
    val width = right - left
    val height = bottom - top

    Log.d(TAG, "ShikiHighlighterView dimensions: width=$width, height=$height")

    // Layout the TextView to fill our entire space
    textView.layout(0, 0, width, height)

    Log.d(TAG, "TextView layout set to: left=0, top=0, right=$width, bottom=$height")
    Log.d(TAG, "TextView dimensions after layout: width=${textView.width}, height=${textView.height}")

    // If the TextView still has zero dimensions, force a layout update
    if (textView.width == 0 || textView.height == 0) {
      Log.d(TAG, "TextView still has zero dimensions, forcing update")

      post {
        // Update layout params
        val params = textView.layoutParams
        params.width = width
        params.height = height
        textView.layoutParams = params

        // Request layout
        textView.requestLayout()
        invalidate()
      }
    }
  }

  override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
    Log.d(
      TAG,
      "onMeasure called with widthMeasureSpec=${MeasureSpec.toString(
        widthMeasureSpec
      )}, heightMeasureSpec=${MeasureSpec.toString(heightMeasureSpec)}"
    )

    val widthMode = MeasureSpec.getMode(widthMeasureSpec)
    val widthSize = MeasureSpec.getSize(widthMeasureSpec)
    val heightMode = MeasureSpec.getMode(heightMeasureSpec)
    val heightSize = MeasureSpec.getSize(heightMeasureSpec)

    Log.d(TAG, "Width mode: ${getModeString(widthMode)}, size: $widthSize")
    Log.d(TAG, "Height mode: ${getModeString(heightMode)}, size: $heightSize")

    // Ensure we have a minimum size
    var finalWidth = widthSize
    var finalHeight = heightSize

    if (widthMode == MeasureSpec.UNSPECIFIED || widthSize == 0) {
      finalWidth = dpToPx(300)
      Log.d(TAG, "Using default width: $finalWidth")
    }

    if (heightMode == MeasureSpec.UNSPECIFIED || heightSize == 0) {
      finalHeight = dpToPx(200)
      Log.d(TAG, "Using default height: $finalHeight")
    }

    // Set fixed dimensions for the TextView
    val params = textView.layoutParams
    params.width = finalWidth
    params.height = finalHeight
    textView.layoutParams = params

    // Force the TextView to have a fixed size
    textView.minWidth = finalWidth
    textView.minHeight = finalHeight

    // Measure the TextView with exact dimensions
    textView.measure(
      MeasureSpec.makeMeasureSpec(finalWidth, MeasureSpec.EXACTLY),
      MeasureSpec.makeMeasureSpec(finalHeight, MeasureSpec.EXACTLY)
    )

    Log.d(TAG, "TextView measured dimensions: ${textView.measuredWidth}x${textView.measuredHeight}")

    // Set our own dimensions
    setMeasuredDimension(finalWidth, finalHeight)

    Log.d(TAG, "Final measured dimensions: ${measuredWidth}x$measuredHeight")
  }

  override fun onAttachedToWindow() {
    super.onAttachedToWindow()
    Log.d(TAG, "onAttachedToWindow called")

    // Ensure our view hierarchy is properly set up
    if (parent != null) {
      Log.d(
        TAG,
        "Parent view: ${parent.javaClass.simpleName}, width: ${(parent as? ViewGroup)?.width}, height: ${(parent as? ViewGroup)?.height}"
      )
    }

    // Force a layout update
    post {
      requestLayout()
      invalidate()
    }
  }

  private fun getModeString(mode: Int): String =
    when (mode) {
      MeasureSpec.EXACTLY -> "EXACTLY"
      MeasureSpec.AT_MOST -> "AT_MOST"
      MeasureSpec.UNSPECIFIED -> "UNSPECIFIED"
      else -> "UNKNOWN"
    }

  /**
   * Debug function to print a sample of tokens and their styles
   */
  private fun debugSampleTokens(tokens: ReadableArray) {
    try {
      val maxSample = Math.min(tokens.size(), 20) // Look at up to 20 tokens

      Log.d("ShikiHighlighterView", "===== TOKEN SAMPLE DEBUG =====")
      for (i in 0 until maxSample) {
        val token = tokens.getMap(i)
        val content = if (token?.hasKey("content") == true) token.getString("content") else "No content"
        val startIndex = if (token?.hasKey("startIndex") == true) token.getInt("startIndex") else -1
        val endIndex = if (token?.hasKey("endIndex") == true) token.getInt("endIndex") else -1
        val style = if (token?.hasKey("style") == true) token.getMap("style") else null

        val colorInfo = if (style != null && style.hasKey("color")) {
          "Color: ${style.getString("color")}"
        } else {
          "No color"
        }

        val bgColorInfo = if (style != null && style.hasKey("backgroundColor")) {
          "BgColor: ${style.getString("backgroundColor")}"
        } else {
          "No bgColor"
        }

        val formatInfo = if (style != null) {
          val bold = if (style.hasKey("bold")) style.getBoolean("bold") else false
          val italic = if (style.hasKey("italic")) style.getBoolean("italic") else false
          val underline = if (style.hasKey("underline")) style.getBoolean("underline") else false
          "Formatting: bold=$bold, italic=$italic, underline=$underline"
        } else {
          "No formatting"
        }

        Log.d("ShikiHighlighterView", "Token $i: '$content' ($startIndex:$endIndex) - $colorInfo - $bgColorInfo - $formatInfo")
      }
      Log.d("ShikiHighlighterView", "===== END TOKEN SAMPLE =====")
    } catch (e: Exception) {
      Log.e("ShikiHighlighterView", "Error debugging tokens: ${e.message}")
    }
  }

  private fun applyTextFormatting() {
    if (text.isEmpty()) {
      textView.text = ""
      return
    }

    if (tokens == null) {
      textView.text = text
      return
    }

    try {
      val spannableBuilder = SpannableStringBuilder(text)

      try {
        // Apply token styling
        applyTokenStyling(spannableBuilder)
      } catch (e: Exception) {
        Log.e(TAG, "Error in applyTokenStyling: ${e.message}", e)
        // Continue with the original text if token styling fails
      }

      try {
        // Apply line numbers if enabled
        if (showLineNumbers) {
          val lineNumberedText = createLineNumberedText(spannableBuilder)
          textView.text = lineNumberedText
          Log.d(TAG, "Applied line numbers to text")
        } else {
          textView.text = spannableBuilder
        }

        // Log success
        Log.d(TAG, "Successfully set text to TextView with ${textView.text.length} characters")
        Log.d(TAG, "Using language: $language, theme: $theme")
      } catch (e: Exception) {
        Log.e(TAG, "Error setting text to TextView: ${e.message}", e)
        // Fallback to plain text if setting text fails
        textView.text = text
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error in applyTextFormatting: ${e.message}", e)
      // Fallback to plain text if formatting fails
      try {
        textView.text = text
        Log.d(TAG, "Fallback to plain text successful")
      } catch (e2: Exception) {
        Log.e(TAG, "Error setting fallback text: ${e2.message}", e2)
      }
    }
  }

  private fun createLineNumberedText(styledText: SpannableStringBuilder): SpannableStringBuilder {
    try {
      // Count the number of lines
      val originalText = styledText.toString()
      val lines = originalText.split("\n")
      val lineCount = lines.size

      // Calculate the width needed for line numbers
      val maxLineNumberWidth = lineCount.toString().length + 1 // +1 for the space

      // Create a new SpannableStringBuilder for the result
      val result = SpannableStringBuilder()

      // Track the current position in the original text
      var currentPos = 0

      val lineNumberColor = Color.parseColor("#6272A4") // Dracula comment color

      lines.forEachIndexed { index, line ->
        // Calculate the start and end positions for this line in the original text
        val lineStart = currentPos
        val lineEnd = lineStart + line.length

        // Create the line number prefix with consistent width
        val lineNumber = (index + 1).toString().padStart(maxLineNumberWidth)
        val prefix = "$lineNumber | "
        val prefixStart = result.length

        // Add the prefix with styling
        result.append(prefix)
        val prefixEnd = result.length

        // Style the line number differently
        result.setSpan(
          ForegroundColorSpan(lineNumberColor),
          prefixStart,
          prefixEnd,
          SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
        )

        // Apply monospace font to line numbers
        result.setSpan(
          TypefaceSpan("monospace"),
          prefixStart,
          prefixEnd,
          SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
        )

        // Add the line content with its original styling
        if (lineStart < lineEnd && lineStart < styledText.length) {
          val endPos = minOf(lineEnd, styledText.length)

          // Get all spans from the original text for this line
          val contentStart = result.length
          result.append(styledText.subSequence(lineStart, endPos))

          // Copy spans from the original text
          val spans = styledText.getSpans(lineStart, endPos, Any::class.java)
          for (span in spans) {
            val spanStart = styledText.getSpanStart(span)
            val spanEnd = styledText.getSpanEnd(span)
            val spanFlags = styledText.getSpanFlags(span)

            // Calculate the new span positions in the result text
            val newSpanStart = contentStart + (spanStart - lineStart).coerceAtLeast(0)
            val newSpanEnd = contentStart + (spanEnd - lineStart).coerceAtMost(endPos - lineStart)

            if (newSpanEnd > newSpanStart) {
              result.setSpan(span, newSpanStart, newSpanEnd, spanFlags)
            }
          }
        }

        // Add a newline if not the last line
        if (index < lines.size - 1) {
          result.append("\n")
        }

        // Update the current position (add 1 for the newline character)
        currentPos = lineEnd + (if (index < lines.size - 1) 1 else 0)
      }

      return result
    } catch (e: Exception) {
      Log.e(TAG, "Error creating line numbered text: ${e.message}", e)
      // Return the original text if there's an error
      return styledText
    }
  }

  private fun applyTokenStyling(spannableBuilder: SpannableStringBuilder) {
    try {
      // Apply token styling
      tokens?.let { tokensArray ->
        // Filter out invalid tokens before processing
        val validTokens = ArrayList<ReadableMap>()
        for (i in 0 until tokensArray.size()) {
          val token = tokensArray.getMap(i)
          if (token != null) {
            val start = token.getInt("start")
            val length = token.getInt("length")

            // Only include tokens with valid ranges
            if (start >= 0 && length > 0 && start + length <= spannableBuilder.length) {
              validTokens.add(token)
            } else {
              Log.d(TAG, "Skipping invalid token: start=$start, length=$length, textLength=${spannableBuilder.length}")
            }
          }
        }

        Log.d(TAG, "Processing ${validTokens.size} valid tokens out of ${tokensArray.size()} total tokens")

        // Process only valid tokens
        for (token in validTokens) {
          try {
            val start = token.getInt("start")
            val length = token.getInt("length")
            val style = token.getMap("style") ?: continue

            // Apply color
            if (style.hasKey("color")) {
              val colorStr = style.getString("color")
              if (colorStr != null) {
                try {
                  val color = Color.parseColor(colorStr)
                  spannableBuilder.setSpan(
                    ForegroundColorSpan(color),
                    start,
                    start + length,
                    SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
                  )
                } catch (e: Exception) {
                  Log.e(TAG, "Error parsing color: $colorStr", e)
                }
              }
            }

            // Apply font weight
            if (style.hasKey("bold") && style.getBoolean("bold")) {
              spannableBuilder.setSpan(
                StyleSpan(Typeface.BOLD),
                start,
                start + length,
                SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
              )
            } else if (this.fontWeight == "bold") {
              spannableBuilder.setSpan(
                StyleSpan(Typeface.BOLD),
                start,
                start + length,
                SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
              )
            } else if (this.fontWeight == "medium") {
              // Medium is not directly supported, use BOLD
              spannableBuilder.setSpan(
                StyleSpan(Typeface.BOLD),
                start,
                start + length,
                SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
              )
            }

            // Apply font style
            if (style.hasKey("italic") && style.getBoolean("italic")) {
              spannableBuilder.setSpan(
                StyleSpan(Typeface.ITALIC),
                start,
                start + length,
                SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
              )
            } else if (this.fontStyle == "italic") {
              spannableBuilder.setSpan(
                StyleSpan(Typeface.ITALIC),
                start,
                start + length,
                SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
              )
            }

            // Apply font family
            if (this.fontFamily != null) {
              try {
                // Map the font family to a system font if needed
                val mappedFontFamily = SYSTEM_FONTS[fontFamily] ?: "monospace"
                Log.d(TAG, "Applying font family to token: '$mappedFontFamily' (original: '$fontFamily')")

                // For Android API 28+, we could use TypefaceSpan(fontFamily) directly
                // But for compatibility, we'll use the string-based constructor
                spannableBuilder.setSpan(
                  TypefaceSpan(mappedFontFamily),
                  start,
                  start + length,
                  SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
                )
              } catch (e: Exception) {
                Log.e(TAG, "Error applying font family: ${e.message}", e)
              }
            }

            // Apply underline
            if (style.hasKey("underline") && style.getBoolean("underline")) {
              spannableBuilder.setSpan(
                UnderlineSpan(),
                start,
                start + length,
                SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
              )
            }
          } catch (e: Exception) {
            Log.e(TAG, "Error processing token: ${e.message}", e)
            // Continue with the next token
          }
        }
      }
    } catch (e: Exception) {
      Log.e(TAG, "Error in applyTokenStyling: ${e.message}", e)
    }
  }
}
