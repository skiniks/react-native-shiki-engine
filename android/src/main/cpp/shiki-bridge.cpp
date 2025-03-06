#include "shiki-bridge.h"

#include <jsi/jsi.h>

#include <android/log.h>
#include <jni.h>

#include <string>

#define TAG       "ShikiJSI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

using namespace facebook;

extern "C" JNIEXPORT void JNICALL
Java_com_shiki_ShikiBridge_nativeInstall(JNIEnv* env, jobject thiz, jobject jsiContextHolder) {
  LOGD("Initializing ShikiJSI bindings");

  jclass jsiContextHolderClass = env->GetObjectClass(jsiContextHolder);
  jmethodID getHolderMethod = env->GetMethodID(jsiContextHolderClass, "get", "()J");
  jlong jsiPtr = env->CallLongMethod(jsiContextHolder, getHolderMethod);

  jsi::Runtime* runtime = reinterpret_cast<jsi::Runtime*>(jsiPtr);
  if (runtime == nullptr) {
    LOGE("JSI Runtime is null");
    return;
  }

  try {
    LOGD("Installing JSI bindings");

    jsi::Object shikiModule(*runtime);

    shikiModule.setProperty(
      *runtime,
      "initHighlighter",
      jsi::Function::createFromHostFunction(
        *runtime,
        jsi::PropNameID::forAscii(*runtime, "initHighlighter"),
        0,
        [](jsi::Runtime& runtime, const jsi::Value& thisValue, const jsi::Value* arguments, size_t count)
          -> jsi::Value {
          LOGD("initHighlighter called");
          return jsi::Value(true);
        }
      )
    );

    runtime->global().setProperty(*runtime, "ShikiJSI", shikiModule);

    runtime->global().setProperty(*runtime, "__ShikiBridgeTurboModule", jsi::Value(true));

    LOGD("ShikiJSI bindings installed successfully");
  } catch (std::exception& e) {
    LOGE("Error installing ShikiJSI bindings: %s", e.what());
  }
}
