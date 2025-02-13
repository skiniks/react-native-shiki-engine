#pragma once
#include "text/TextRange.h"
#include <jni.h>

namespace shiki {

class TextRangeAndroid : public TextRange {
public:
  void measure(JNIEnv* env, jobject paint, jstring text) override;

private:
  jfloat measureWidth(JNIEnv* env, jobject paint, const jchar* chars);
  jfloat measureHeight(JNIEnv* env, jobject paint);
};

} // namespace shiki
