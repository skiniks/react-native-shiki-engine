#include "MemoryManager.h"
#include <cstdlib>

#ifdef __ANDROID__
#include <malloc.h>
#else
#include <mach/mach.h>
#endif

namespace shiki {

float MemoryManager::getMemoryUsage() {
#ifdef __ANDROID__
  struct mallinfo mi = mallinfo();
  return static_cast<float>(mi.uordblks) / mi.arena;
#else
  mach_task_basic_info_data_t info;
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) ==
      KERN_SUCCESS) {
    return static_cast<float>(info.resident_size) / info.virtual_size;
  }
  return 0.0f;
#endif
}

void MemoryManager::checkMemoryPressure() {
  float usage = getMemoryUsage();
  if (usage > CRITICAL_MEMORY_THRESHOLD && criticalPressureCallback_) {
    criticalPressureCallback_();
  } else if (usage > LOW_MEMORY_THRESHOLD && highPressureCallback_) {
    highPressureCallback_();
  }
}

} // namespace shiki
