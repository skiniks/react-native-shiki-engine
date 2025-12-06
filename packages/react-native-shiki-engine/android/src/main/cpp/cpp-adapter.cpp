#include <jsi/jsi.h>

#include <android/log.h>
#include <fbjni/fbjni.h>
#include <jni.h>

#include "onig_regex.h"

using namespace facebook::jni;
using namespace facebook::jsi;

#define TAG       "ShikiEngine"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

extern "C" JNIEXPORT jdouble JNICALL Java_com_shikiengine_ShikiEngineModule_createScanner(
  JNIEnv* env,
  jobject thiz,
  jobjectArray patterns,
  jdouble maxCacheSize
) {
  try {
    jsize length = env->GetArrayLength(patterns);
    std::vector<std::string> patternStrings;
    std::vector<const char*> patternPtrs;

    patternStrings.reserve(length);
    patternPtrs.reserve(length);

    for (jsize i = 0; i < length; i++) {
      jstring str = (jstring)env->GetObjectArrayElement(patterns, i);
      const char* chars = env->GetStringUTFChars(str, nullptr);
      patternStrings.push_back(chars);
      patternPtrs.push_back(patternStrings.back().c_str());
      env->ReleaseStringUTFChars(str, chars);
      env->DeleteLocalRef(str);
    }

    OnigContext* context = create_scanner(patternPtrs.data(), length, static_cast<size_t>(maxCacheSize));
    if (!context) {
      LOGE("Failed to create scanner");
      return -1;
    }

    // Store the pointer value using bit manipulation
    uint64_t ptr = reinterpret_cast<uint64_t>(context);
    return static_cast<jdouble>(ptr);
  } catch (const std::exception& e) {
    LOGE("Exception in createScanner: %s", e.what());
    return -1;
  }
}

extern "C" JNIEXPORT jobject JNICALL Java_com_shikiengine_ShikiEngineModule_findNextMatchSync(
  JNIEnv* env,
  jobject thiz,
  jdouble scannerId,
  jstring text,
  jdouble startPosition
) {
  try {
    uint64_t ptr = static_cast<uint64_t>(scannerId);
    OnigContext* context = reinterpret_cast<OnigContext*>(ptr);
    if (!context) {
      LOGE("Invalid scanner ID");
      return nullptr;
    }

    const char* textChars = env->GetStringUTFChars(text, nullptr);
    OnigResult* result = find_next_match(context, textChars, static_cast<int>(startPosition));
    env->ReleaseStringUTFChars(text, textChars);

    if (!result) {
      return nullptr;
    }

    // Get the WritableMap class and constructor
    jclass writableMapClass = env->FindClass("com/facebook/react/bridge/WritableNativeMap");
    jmethodID constructor = env->GetMethodID(writableMapClass, "<init>", "()V");
    jobject writableMap = env->NewObject(writableMapClass, constructor);

    // Get the putInt and putArray methods
    jmethodID putInt = env->GetMethodID(writableMapClass, "putInt", "(Ljava/lang/String;I)V");
    jmethodID putArray =
      env->GetMethodID(writableMapClass, "putArray", "(Ljava/lang/String;Lcom/facebook/react/bridge/WritableArray;)V");

    // Create capture indices array
    jclass writableArrayClass = env->FindClass("com/facebook/react/bridge/WritableNativeArray");
    jmethodID arrayConstructor = env->GetMethodID(writableArrayClass, "<init>", "()V");
    jobject captureIndices = env->NewObject(writableArrayClass, arrayConstructor);
    jmethodID pushMap = env->GetMethodID(writableArrayClass, "pushMap", "(Lcom/facebook/react/bridge/WritableMap;)V");

    for (int i = 0; i < result->capture_count / 2; i++) {
      jobject capture = env->NewObject(writableMapClass, constructor);
      env->CallVoidMethod(capture, putInt, env->NewStringUTF("start"), result->capture_indices[i * 2]);
      env->CallVoidMethod(capture, putInt, env->NewStringUTF("end"), result->capture_indices[i * 2 + 1]);
      env->CallVoidMethod(
        capture,
        putInt,
        env->NewStringUTF("length"),
        result->capture_indices[i * 2 + 1] - result->capture_indices[i * 2]
      );
      env->CallVoidMethod(captureIndices, pushMap, capture);
      env->DeleteLocalRef(capture);
    }

    // Set the result properties
    env->CallVoidMethod(writableMap, putInt, env->NewStringUTF("index"), result->pattern_index);
    env->CallVoidMethod(writableMap, putArray, env->NewStringUTF("captureIndices"), captureIndices);

    free_result(result);
    return writableMap;
  } catch (const std::exception& e) {
    LOGE("Exception in findNextMatch: %s", e.what());
    return nullptr;
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_shikiengine_ShikiEngineModule_destroyScanner(JNIEnv* env, jobject thiz, jdouble scannerId) {
  try {
    uint64_t ptr = static_cast<uint64_t>(scannerId);
    OnigContext* context = reinterpret_cast<OnigContext*>(ptr);
    if (context) {
      free_scanner(context);
    }
  } catch (const std::exception& e) {
    LOGE("Exception in destroyScanner: %s", e.what());
  }
}
