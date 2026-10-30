#ifndef PLATFORM_MEMORY_HPP
#define PLATFORM_MEMORY_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>

#if defined(__APPLE__)
#  include <TargetConditionals.h>
#  include <sys/sysctl.h>
#  if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
#    include <os/proc.h>
#  endif
#endif

#if defined(__ANDROID__)
inline std::atomic<size_t>& android_available_memory_cache() {
  static std::atomic<size_t> cached{0};
  return cached;
}

inline void shiki_set_android_available_memory(size_t bytes) {
  android_available_memory_cache().store(bytes, std::memory_order_relaxed);
}
#endif

inline size_t platform_available_memory_bytes() {
#if defined(__APPLE__)
#  if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
  const size_t available = os_proc_available_memory();
  if (available > 0) {
    return available;
  }
#  endif
  {
    int64_t mem = 0;
    size_t len = sizeof(mem);
    if (sysctlbyname("hw.memsize", &mem, &len, nullptr, 0) == 0 && mem > 0) {
      return static_cast<size_t>(mem / 4);
    }
  }
#elif defined(__ANDROID__)
  return android_available_memory_cache().load(std::memory_order_relaxed);
#endif
  return 0;
}

inline size_t default_cache_memory_budget() {
  constexpr size_t kHardCap = 50ull * 1024ull * 1024ull;
  constexpr size_t kFloor = 8ull * 1024ull * 1024ull;
  const size_t available = platform_available_memory_bytes();
  if (available == 0) {
    return kFloor;
  }
  const size_t fraction = available / 8;
  if (fraction < kFloor) {
    return kFloor;
  }
  return fraction < kHardCap ? fraction : kHardCap;
}

#endif  // PLATFORM_MEMORY_HPP
