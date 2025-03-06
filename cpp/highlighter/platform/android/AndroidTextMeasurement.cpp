#include "AndroidTextMeasurement.h"

#include <string>

#include "highlighter/theme/ThemeColor.h"

namespace shiki {

AndroidTextMeasurement::AndroidTextMeasurement(JNIEnv* env, jobject paint) : env_(env) {
  // Create global reference to paint object
  paint_ = env_->NewGlobalRef(paint);

  // Cache method IDs
  jclass paintClass = env_->GetObjectClass(paint_);
  measureTextMethod_ = env_->GetMethodID(paintClass, "measureText", "(Ljava/lang/String;II)F");
  getFontMetricsMethod_ = env_->GetMethodID(paintClass, "getFontMetrics", "()Landroid/graphics/Paint$FontMetrics;");
  setColorMethod_ = env_->GetMethodID(paintClass, "setColor", "(I)V");
  setTypeface_ =
    env_->GetMethodID(paintClass, "setTypeface", "(Landroid/graphics/Typeface;)Landroid/graphics/Typeface;");
  getTextBoundsMethod_ =
    env_->GetMethodID(paintClass, "getTextBounds", "(Ljava/lang/String;IILandroid/graphics/Rect;)V");

  env_->DeleteLocalRef(paintClass);

  // Initialize metrics
  updateFontMetrics();
}

AndroidTextMeasurement::~AndroidTextMeasurement() {
  if (env_ && paint_) {
    env_->DeleteGlobalRef(paint_);
  }
}

void AndroidTextMeasurement::updateFontMetrics() {
  jobject metrics = env_->CallObjectMethod(paint_, getFontMetricsMethod_);
  if (metrics) {
    jclass metricsClass = env_->GetObjectClass(metrics);
    jfieldID ascentField = env_->GetFieldID(metricsClass, "ascent", "F");
    jfieldID descentField = env_->GetFieldID(metricsClass, "descent", "F");
    jfieldID leadingField = env_->GetFieldID(metricsClass, "leading", "F");

    cachedAscent_ = env_->GetFloatField(metrics, ascentField);
    cachedDescent_ = env_->GetFloatField(metrics, descentField);
    cachedLeading_ = env_->GetFloatField(metrics, leadingField);

    cachedLineHeight_ = cachedDescent_ - cachedAscent_ + cachedLeading_;

    env_->DeleteLocalRef(metricsClass);
    env_->DeleteLocalRef(metrics);
  }
}

jobject AndroidTextMeasurement::createTypeface(bool isBold, bool isItalic) {
  jclass typefaceClass = env_->FindClass("android/graphics/Typeface");
  jint style = 0;
  if (isBold) style |= 1;  // Typeface.BOLD
  if (isItalic) style |= 2;  // Typeface.ITALIC

  jmethodID createMethod =
    env_->GetStaticMethodID(typefaceClass, "create", "(Ljava/lang/String;I)Landroid/graphics/Typeface;");
  jobject typeface = env_->CallStaticObjectMethod(typefaceClass, createMethod, nullptr, style);

  env_->DeleteLocalRef(typefaceClass);
  return typeface;
}

void AndroidTextMeasurement::applyStyle(const ThemeStyle& style) {
  // Apply color
  if (!style.color.empty()) {
    // Create a ThemeColor from the hex string
    ThemeColor themeColor(style.color);

    // Convert to Android ARGB format
    jint color = ((int)(themeColor.alpha * 255) & 0xFF) << 24 | ((int)(themeColor.red * 255) & 0xFF) << 16 |
      ((int)(themeColor.green * 255) & 0xFF) << 8 | ((int)(themeColor.blue * 255) & 0xFF);

    env_->CallVoidMethod(paint_, setColorMethod_, color);
  }

  // Apply font style
  jobject typeface = createTypeface(style.bold, style.italic);
  if (typeface) {
    env_->CallObjectMethod(paint_, setTypeface_, typeface);
    env_->DeleteLocalRef(typeface);
  }
}

TextMetrics
AndroidTextMeasurement::measureRange(const std::string& text, size_t start, size_t length, const ThemeStyle& style) {
  TextMetrics metrics;

  if (!env_ || !paint_ || start + length > text.length()) {
    return metrics;
  }

  // Apply the style to the Paint object
  applyStyle(style);

  // Create a Java string from the substring
  jstring jtext = env_->NewStringUTF(text.c_str() + start);
  if (!jtext) {
    return metrics;
  }

  // Measure the text
  jint strLen = env_->GetStringLength(jtext);
  jint useLen = length < static_cast<size_t>(strLen) ? length : strLen;

  // Get width
  float width = env_->CallFloatMethod(paint_, measureTextMethod_, jtext, 0, useLen);
  metrics.width = width;
  metrics.height = cachedLineHeight_;
  metrics.baseline = -cachedAscent_;
  metrics.isValid = true;

  // Cleanup
  env_->DeleteLocalRef(jtext);

  return metrics;
}

TextMetrics AndroidTextMeasurement::measureLine(const std::string& text) {
  TextMetrics metrics;
  if (!env_ || !paint_) {
    return metrics;
  }

  ThemeStyle defaultStyle;
  return measureRange(text, 0, text.length(), defaultStyle);
}

float AndroidTextMeasurement::getLineHeight() const {
  return cachedLineHeight_;
}

float AndroidTextMeasurement::getBaseline() const {
  return -cachedAscent_;
}

}  // namespace shiki
