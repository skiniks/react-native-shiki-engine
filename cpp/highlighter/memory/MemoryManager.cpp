#include "MemoryManager.h"

#include <cstdlib>

#ifdef __ANDROID__
#  include <android/log.h>
#  include <android/native_window.h>
#  include <jni.h>
#  include <malloc.h>
#elif defined(__APPLE__)
#  include <CoreFoundation/CoreFoundation.h>
#  include <dispatch/dispatch.h>
#  include <mach/mach.h>
#  include <sys/sysctl.h>
#endif

namespace shiki {

void MemoryManager::initializePlatformSignals() {
#ifdef __ANDROID__
  // Android memory pressure signals are handled through JNI callbacks
  // The actual registration happens in the Java layer
  __android_log_print(ANDROID_LOG_INFO, "MemoryManager", "Initialized platform signals for Android");
#elif defined(__APPLE__)
  // iOS/macOS memory pressure monitoring using dispatch source
  dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  dispatch_source_t source = dispatch_source_create(
    DISPATCH_SOURCE_TYPE_MEMORYPRESSURE,
    0,
    DISPATCH_MEMORYPRESSURE_NORMAL | DISPATCH_MEMORYPRESSURE_WARN | DISPATCH_MEMORYPRESSURE_CRITICAL,
    queue
  );

  if (source) {
    dispatch_source_set_event_handler(source, ^{
      dispatch_source_memorypressure_flags_t level = dispatch_source_get_data(source);
      if (level & DISPATCH_MEMORYPRESSURE_CRITICAL) {
        if (criticalPressureCallback_) criticalPressureCallback_();
      } else if (level & DISPATCH_MEMORYPRESSURE_WARN) {
        if (highPressureCallback_) highPressureCallback_();
      }
    });
    dispatch_resume(source);
  }
#endif
}

#ifdef __ANDROID__
void MemoryManager::onTrimMemory(JNIEnv* env, jobject obj, jint level) {
  auto& instance = getInstance();

  // Log memory trim event
  __android_log_print(ANDROID_LOG_INFO, "MemoryManager", "onTrimMemory called with level: %d", level);

  // Handle different memory trim levels
  switch (level) {
    case TRIM_MEMORY_RUNNING_MODERATE:
    case TRIM_MEMORY_RUNNING_LOW:
      if (instance.highPressureCallback_) {
        instance.highPressureCallback_();
      }
      break;

    case TRIM_MEMORY_RUNNING_CRITICAL:
    case TRIM_MEMORY_COMPLETE:
      if (instance.criticalPressureCallback_) {
        instance.criticalPressureCallback_();
      }
      break;
  }

  // Get current memory info
  struct mallinfo mi = mallinfo();
  __android_log_print(
    ANDROID_LOG_INFO,
    "MemoryManager",
    "Current memory usage - arena: %d, used: %d, free: %d",
    mi.arena,
    mi.uordblks,
    mi.fordblks
  );
}
#endif

float MemoryManager::getMemoryUsage() {
#ifdef __ANDROID__
  struct mallinfo mi = mallinfo();
  return static_cast<float>(mi.uordblks) / mi.arena;
#elif defined(__APPLE__)
  mach_task_basic_info_data_t info;
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
    return static_cast<float>(info.resident_size) / info.virtual_size;
  }
  return 0.0f;
#else
  return 0.0f;
#endif
}

void MemoryManager::checkMemoryPressure() {
  float usage = getMemoryUsage();
  if (usage > memory::CRITICAL_PRESSURE && criticalPressureCallback_) {
    criticalPressureCallback_();
  } else if (usage > memory::HIGH_PRESSURE && highPressureCallback_) {
    highPressureCallback_();
  }
}

MemoryManager::~MemoryManager() {
#ifdef __APPLE__
  if (memoryPressureSource_) {
    dispatch_source_cancel(memoryPressureSource_);
    dispatch_release(memoryPressureSource_);
  }
#endif
}

}  // namespace shiki
