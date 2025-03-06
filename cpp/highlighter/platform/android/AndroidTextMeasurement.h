#pragma once
#include <jni.h>

#include "highlighter/platform/PlatformMeasurement.h"
#include "highlighter/text/TextRange.h"
#include "highlighter/theme/ThemeStyle.h"

namespace shiki {

class AndroidTextMeasurement : public PlatformMeasurement {
 public:
  AndroidTextMeasurement(JNIEnv* env, jobject paint);
  ~AndroidTextMeasurement() override;

  TextMetrics measureRange(const std::string& text, size_t start, size_t length, const ThemeStyle& style) override;
  TextMetrics measureLine(const std::string& text);
  float getLineHeight() const;
  float getBaseline() const;

 private:
  JNIEnv* env_;
  jobject paint_;  // Global reference to Android Paint object
  jmethodID measureTextMethod_;
  jmethodID getFontMetricsMethod_;
  jmethodID setColorMethod_;
  jmethodID setTypeface_;
  jmethodID getTextBoundsMethod_;

  // Cache font metrics to avoid JNI calls
  float cachedAscent_;
  float cachedDescent_;
  float cachedLeading_;
  float cachedLineHeight_;

  void updateFontMetrics();
  void applyStyle(const ThemeStyle& style);
  jobject createTypeface(bool isBold, bool isItalic);
};

}  // namespace shiki
