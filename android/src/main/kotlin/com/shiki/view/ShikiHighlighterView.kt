package com.shiki.view

import android.content.Context
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Typeface
import android.text.SpannableStringBuilder
import android.text.style.ForegroundColorSpan
import android.text.style.LineHeightSpan
import android.text.style.StyleSpan
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
      "monospace" to "monospace",
      "SF-Mono" to "monospace",
      "Menlo" to "monospace"
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
    fontFamily = family
    updateFont()
    processTextWithBatches()
  }

  fun setScrollEnabled(enabled: Boolean) {
    scrollEnabled = enabled
  }

  private fun updateFont() {
    val systemFontName = SYSTEM_FONTS[fontFamily] ?: "monospace"
    val typeface = when {
      fontFamily?.contains("SF-Mono") == true -> Typeface.MONOSPACE
      fontFamily?.contains("Menlo") == true -> Typeface.MONOSPACE
      else -> Typeface.create(systemFontName, Typeface.NORMAL)
    }
    textView.typeface = typeface
  }

  private fun processTextWithBatches() {
    Log.d(TAG, "processTextWithBatches called")
    if (text.isEmpty()) {
      Log.d(TAG, "Text is empty, setting empty text")
      textView.text = ""
      return
    }

    val spannableText = SpannableStringBuilder(text)

    // Check if we have tokens to apply
    if (tokens != null && tokens!!.size() > 0) {
      Log.d(TAG, "Processing ${tokens!!.size()} tokens for highlighting")

      var tokensWithStyles = 0
      var tokensWithColors = 0

      // Debug: Print a sample of tokens for inspection
      debugSampleTokens(tokens!!)

      // Apply each token's style
      for (i in 0 until tokens!!.size()) {
        try {
          val token = tokens!!.getMap(i)
          if (token != null) {
            val start = token.getInt("start")
            val length = token.getInt("length")
            val style = token.getMap("style")

            if (style != null) {
              tokensWithStyles++
              if (style.hasKey("color") && style.getString("color") != null) {
                tokensWithColors++
              }
            }

            // Ensure start and length are valid
            if (start >= 0 && length > 0 && start + length <= text.length) {
              if (i % 50 == 0) { // Log only every 50th token to avoid flooding
                Log.d(TAG, "Applying token style at position $start with length $length")
                // Log the token text for debugging
                val tokenText = text.substring(start, start + length)
                Log.d(TAG, "Token text: '$tokenText'")
              }

              applyTokenStyle(spannableText, start, length, style)
            } else {
              Log.e(TAG, "Invalid token range: start=$start, length=$length, textLength=${text.length}")
            }
          }
        } catch (e: Exception) {
          Log.e(TAG, "Error processing token at index $i: ${e.message}")
          e.printStackTrace()
        }
      }

      Log.d(TAG, "Applied styles to all tokens: $tokensWithStyles tokens had styles, $tokensWithColors tokens had color information")
      Log.d(TAG, "All tokens processed successfully")
    } else {
      // If no tokens, apply white color to all text
      Log.d(TAG, "No tokens available, applying default white color to all text")
      spannableText.setSpan(
        ForegroundColorSpan(Color.WHITE),
        0,
        text.length,
        SpannableStringBuilder.SPAN_EXCLUSIVE_EXCLUSIVE
      )
    }

    // Set the styled text to the TextView
    textView.text = spannableText
    Log.d(TAG, "Set spannable text to TextView")

    // Force layout update
    post {
      textView.requestLayout()
      invalidate()
    }

    // Ensure the text is visible by checking its properties
    Log.d(TAG, "TextView text length: ${textView.text?.length}")
    Log.d(TAG, "TextView text color: ${textView.currentTextColor}")
    Log.d(TAG, "TextView visibility: ${textView.visibility}")
    Log.d(TAG, "TextView width: ${textView.width}, height: ${textView.height}")

    Log.d(TAG, "processTextWithBatches completed")
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
}
