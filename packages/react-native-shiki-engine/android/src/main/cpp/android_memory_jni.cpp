#include <jni.h>

#include "onig_regex.h"
#include "platform_memory.hpp"

// Keep in sync with android.content.ComponentCallbacks2.
// Levels are not a monotonic severity scale (UI_HIDDEN=20 > RUNNING_CRITICAL=15).
static constexpr jint TRIM_MEMORY_RUNNING_MODERATE = 5;
static constexpr jint TRIM_MEMORY_RUNNING_LOW = 10;
static constexpr jint TRIM_MEMORY_RUNNING_CRITICAL = 15;
static constexpr jint TRIM_MEMORY_UI_HIDDEN = 20;
static constexpr jint TRIM_MEMORY_BACKGROUND = 40;
static constexpr jint TRIM_MEMORY_MODERATE = 60;
static constexpr jint TRIM_MEMORY_COMPLETE = 80;

extern "C" JNIEXPORT void JNICALL
Java_com_shikiengine_ShikiEngineMemory_nativeSetAvailableMemory(JNIEnv*, jclass, jlong bytes) {
  if (bytes > 0) {
    shiki_set_android_available_memory(static_cast<size_t>(bytes));
  }
}

extern "C" JNIEXPORT void JNICALL Java_com_shikiengine_ShikiEngineMemory_nativeOnLowMemory(JNIEnv*, jclass) {
  clear_unused_pattern_cache();
}

extern "C" JNIEXPORT void JNICALL
Java_com_shikiengine_ShikiEngineMemory_nativeOnTrimMemory(JNIEnv*, jclass, jint level) {
  switch (level) {
    case TRIM_MEMORY_RUNNING_CRITICAL:
    case TRIM_MEMORY_UI_HIDDEN:
    case TRIM_MEMORY_BACKGROUND:
    case TRIM_MEMORY_MODERATE:
    case TRIM_MEMORY_COMPLETE:
      clear_unused_pattern_cache();
      break;
    case TRIM_MEMORY_RUNNING_MODERATE:
    case TRIM_MEMORY_RUNNING_LOW:
      trim_pattern_cache();
      break;
    default:
      break;
  }
}
