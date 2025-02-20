#pragma once
#include <jni.h>

#include "highlighter/platform/PlatformMeasurement.h"

namespace shiki {

class AndroidTextMeasurement : public PlatformMeasurement {
 public:
  AndroidTextMeasurement(JNIEnv* env, jobject paint);
  ~AndroidTextMeasurement() override;

  TextRange::Metrics measureRange(const std::string& text, size_t start, size_t length, const ThemeStyle& style)
    override;

 private:
  JNIEnv* env_;
  jobject paint_;  // Global reference to Android Paint object
  jmethodID measureTextMethod_;
  jmethodID getFontMetricsMethod_;
  jmethodID setColorMethod_;
};

}  // namespace shiki
