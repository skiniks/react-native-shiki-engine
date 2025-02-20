#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "MemoryTracker.h"
#include "highlighter/core/Constants.h"
#include "highlighter/utils/ScopedResource.h"

#ifdef __APPLE__
#  include <dispatch/dispatch.h>
#endif

namespace shiki {

namespace text {
  static constexpr size_t MAX_TEXT_SIZE = 1024 * 1024;  // 1MB
}

class MemoryManager {
 public:
  static constexpr size_t LOW_MEMORY_THRESHOLD = 50 * 1024 * 1024;  // 50MB
  static constexpr size_t CRITICAL_MEMORY_THRESHOLD = 100 * 1024 * 1024;  // 100MB

  static MemoryManager& getInstance() {
    static MemoryManager instance;
    return instance;
  }

  ~MemoryManager();

  // Platform memory pressure handling
  void initializePlatformSignals();

#ifdef __ANDROID__
  static void onTrimMemory(JNIEnv* env, jobject obj, jint level);
#endif

  // Memory pressure callbacks
  void setHighPressureCallback(std::function<void()> callback) {
    highPressureCallback_ = std::move(callback);
  }

  void setCriticalPressureCallback(std::function<void()> callback) {
    criticalPressureCallback_ = std::move(callback);
  }

  // Enhanced memory tracking
  void trackAllocation(size_t size, const std::string& category = "general") {
    currentUsage_ += size;
    MemoryTracker::getInstance().trackAllocation(size, category);
    checkMemoryPressure();
  }

  void trackDeallocation(size_t size, const std::string& category = "general") {
    currentUsage_ -= size;
    MemoryTracker::getInstance().trackDeallocation(size, category);
  }

  size_t getCurrentUsage() const {
    return currentUsage_;
  }

  MemoryMetrics getMemoryMetrics() const {
    return MemoryTracker::getInstance().getMetrics();
  }

  // Resource cleanup
  void registerCleanupHandler(std::function<void()> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    cleanupHandlers_.push_back(std::move(handler));
  }

  void cleanup(bool force = false) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (force || MemoryTracker::getInstance().isUnderPressure()) {
      for (const auto& handler : cleanupHandlers_) {
        handler();
      }
    }
  }

  // Text size validation
  static bool isTextSizeValid(const std::string& text) {
    return text.length() <= text::MAX_TEXT_SIZE;
  }

  template <typename T>
  static void limitRanges(std::vector<T>& ranges) {
    if (ranges.size() > text::MAX_RANGES) {
      ranges.resize(text::MAX_RANGES);
    }
  }

 private:
  MemoryManager() {
    initializePlatformSignals();
  }

  // Prevent copying
  MemoryManager(const MemoryManager&) = delete;
  MemoryManager& operator=(const MemoryManager&) = delete;

  std::atomic<size_t> currentUsage_{0};
  std::vector<std::function<void()>> cleanupHandlers_;
  std::mutex mutex_;

  std::function<void()> highPressureCallback_;
  std::function<void()> criticalPressureCallback_;

#ifdef __APPLE__
  dispatch_source_t memoryPressureSource_{nullptr};
#endif

  float getMemoryUsage();
  void checkMemoryPressure();
};

}  // namespace shiki
