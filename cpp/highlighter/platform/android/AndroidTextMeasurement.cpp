#include "AndroidTextMeasurement.h"

#include <string>

namespace shiki {

AndroidTextMeasurement::AndroidTextMeasurement(JNIEnv* env, jobject paint) : env_(env) {
  // Create global reference to paint object
  paint_ = env_->NewGlobalRef(paint);

  // Cache method IDs
  jclass paintClass = env_->GetObjectClass(paint_);
  measureTextMethod_ = env_->GetMethodID(paintClass, "measureText", "(Ljava/lang/String;II)F");
  getFontMetricsMethod_ = env_->GetMethodID(paintClass, "getFontMetrics", "()Landroid/graphics/Paint$FontMetrics;");
  setColorMethod_ = env_->GetMethodID(paintClass, "setColor", "(I)V");

  env_->DeleteLocalRef(paintClass);
}

AndroidTextMeasurement::~AndroidTextMeasurement() {
  if (env_ && paint_) {
    env_->DeleteGlobalRef(paint_);
  }
}

TextRange::Metrics
AndroidTextMeasurement::measureRange(const std::string& text, size_t start, size_t length, const ThemeStyle& style) {
  TextRange::Metrics metrics;

  if (!env_ || !paint_ || start + length > text.length()) {
    return metrics;
  }

  // Extract substring to measure
  std::string substring = text.substr(start, length);
  jstring jSubstring = env_->NewStringUTF(substring.c_str());
  if (!jSubstring) {
    return metrics;
  }

  // Apply style color if specified
  if (!style.color.empty()) {
    jint color = ((style.color.a & 0xFF) << 24) | ((style.color.r & 0xFF) << 16) | ((style.color.g & 0xFF) << 8) |
      (style.color.b & 0xFF);
    env_->CallVoidMethod(paint_, setColorMethod_, color);
  }

  // Measure text width
  metrics.width = env_->CallFloatMethod(paint_, measureTextMethod_, jSubstring, 0, length);

  // Get font metrics
  jobject fontMetrics = env_->CallObjectMethod(paint_, getFontMetricsMethod_);
  if (fontMetrics) {
    jclass metricsClass = env_->GetObjectClass(fontMetrics);
    jfieldID ascentField = env_->GetFieldID(metricsClass, "ascent", "F");
    jfieldID descentField = env_->GetFieldID(metricsClass, "descent", "F");

    float ascent = env_->GetFloatField(fontMetrics, ascentField);
    float descent = env_->GetFloatField(fontMetrics, descentField);

    metrics.height = descent - ascent;
    metrics.baseline = -ascent;

    env_->DeleteLocalRef(metricsClass);
    env_->DeleteLocalRef(fontMetrics);
  }

  env_->DeleteLocalRef(jSubstring);

  metrics.isValid = true;
  return metrics;
}

}  // namespace shiki
