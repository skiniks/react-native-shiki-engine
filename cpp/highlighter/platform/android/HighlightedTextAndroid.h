#pragma once
#include <jni.h>

#include <string>

#include "highlighter/text/HighlightedText.h"

namespace shiki {

class HighlightedTextAndroid final : public HighlightedText {
 public:
  explicit HighlightedTextAndroid(jobject textView, JNIEnv* env);
  ~HighlightedTextAndroid() override;

  void updateNativeView() override;
  void clearHighlighting() override;
  void measureRange(TextRange& range) override;

 private:
  jobject textView_;
  JNIEnv* env_;
  jmethodID setTextMethod_;
  jmethodID clearMethod_;
  jmethodID measureMethod_;

  void applyStyle(const ThemeStyle& style, jint start, jint length);
};

}  // namespace shiki
