#include "HighlightedTextAndroid.h"

#include <jni.h>

namespace shiki {

HighlightedTextAndroid::HighlightedTextAndroid(jobject textView, JNIEnv* env) : textView_(textView), env_(env) {
  jclass viewClass = env_->GetObjectClass(textView_);
  setTextMethod_ = env_->GetMethodID(viewClass, "setText", "(Ljava/lang/CharSequence;)V");
  clearMethod_ = env_->GetMethodID(viewClass, "clear", "()V");
  measureMethod_ = env_->GetMethodID(viewClass, "measure", "(II)V");
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
  if (!textView_ || !env_) return;
  range.measure(env_, textView_, env_->NewStringUTF(text_.c_str()));
}

void HighlightedTextAndroid::applyStyle(const ThemeStyle& style, jint start, jint length) {
  // Implementation of style application for Android
}

}  // namespace shiki
