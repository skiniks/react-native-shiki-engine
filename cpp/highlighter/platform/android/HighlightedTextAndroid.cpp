#include "HighlightedTextAndroid.h"

#include <jni.h>

#include <memory>

#include "AndroidTextMeasurement.h"
#include "highlighter/platform/PlatformMeasurement.h"

namespace shiki {

HighlightedTextAndroid::HighlightedTextAndroid(jobject textView, JNIEnv* env)
  : text::HighlightedText(""), textView_(textView), env_(env) {
  jclass viewClass = env_->GetObjectClass(textView_);
  setTextMethod_ = env_->GetMethodID(viewClass, "setText", "(Ljava/lang/CharSequence;)V");
  clearMethod_ = env_->GetMethodID(viewClass, "clear", "()V");
  measureMethod_ = env_->GetMethodID(viewClass, "measure", "(II)V");

  env_->DeleteLocalRef(viewClass);
}

HighlightedTextAndroid::~HighlightedTextAndroid() {
  // No global references to clean up in this implementation
}

void HighlightedTextAndroid::updateNativeView() {
  if (!textView_ || !env_) return;

  jstring text = env_->NewStringUTF(text_.c_str());
  env_->CallVoidMethod(textView_, setTextMethod_, text);

  for (const auto& range : ranges_) {
    applyStyle(range.style, range.start, range.length);
  }

  env_->DeleteLocalRef(text);
}

void HighlightedTextAndroid::clearHighlighting() {
  if (!textView_ || !env_) return;
  env_->CallVoidMethod(textView_, clearMethod_);
}

void HighlightedTextAndroid::measureRange(TextRange& range) {
  if (!textView_ || !env_ || !range.isValid()) return;

  // Create a Paint object from the TextView to measure text
  jclass viewClass = env_->GetObjectClass(textView_);
  jmethodID getPaintMethod = env_->GetMethodID(viewClass, "getPaint", "()Landroid/text/TextPaint;");
  jobject paint = env_->CallObjectMethod(textView_, getPaintMethod);

  if (paint) {
    // Create a measurement object
    std::shared_ptr<PlatformMeasurement> measurer = std::make_shared<AndroidTextMeasurement>(env_, paint);

    // Call the measure method on the range
    range.measure(text_, measurer);

    // Clean up
    env_->DeleteLocalRef(paint);
  }

  env_->DeleteLocalRef(viewClass);
}

void HighlightedTextAndroid::applyStyle(const ThemeStyle& style, jint start, jint length) {
  // Implementation of style application for Android
  // This would call native methods to apply styling to TextView
}

}  // namespace shiki
