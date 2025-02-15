#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

#include "../core/Constants.h"

namespace shiki {

struct MemoryMetrics {
  size_t totalAllocated{0};
  size_t totalDeallocated{0};
  size_t peakUsage{0};
  size_t currentUsage{0};
  size_t allocationCount{0};
  size_t deallocationCount{0};

  // Category-specific metrics
  std::unordered_map<std::string, size_t> categoryUsage;
  std::unordered_map<std::string, size_t> categoryPeaks;
};

class MemoryTracker {
 public:
  static MemoryTracker& getInstance() {
    static MemoryTracker instance;
    return instance;
  }

  // Track allocations with optional category
  void trackAllocation(size_t size, const std::string& category = "general") {
    std::lock_guard<std::mutex> lock(mutex_);

    totalAllocated_.store(totalAllocated_.load(std::memory_order_relaxed) + size, std::memory_order_relaxed);
    size_t new_usage = currentUsage_.load(std::memory_order_relaxed) + size;
    currentUsage_.store(new_usage, std::memory_order_relaxed);
    allocationCount_.fetch_add(1, std::memory_order_relaxed);

    categoryUsage_[category] += size;

    // Update peaks
    size_t current_peak = peakUsage_.load(std::memory_order_relaxed);
    while (
      new_usage > current_peak &&
      !peakUsage_.compare_exchange_weak(current_peak, new_usage, std::memory_order_relaxed, std::memory_order_relaxed)
    ) {
      // Loop continues if compare_exchange_weak fails
    }

    categoryPeaks_[category] = std::max(categoryPeaks_[category], categoryUsage_[category]);

    // Record timestamp
    lastAllocationTime_ = std::chrono::steady_clock::now();
  }

  void trackDeallocation(size_t size, const std::string& category = "general") {
    std::lock_guard<std::mutex> lock(mutex_);

    totalDeallocated_.store(totalDeallocated_.load(std::memory_order_relaxed) + size, std::memory_order_relaxed);
    currentUsage_.fetch_sub(size, std::memory_order_relaxed);
    deallocationCount_.fetch_add(1, std::memory_order_relaxed);

    if (categoryUsage_.find(category) != categoryUsage_.end()) {
      categoryUsage_[category] -= size;
    }
  }

  // Get current metrics
  MemoryMetrics getMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    MemoryMetrics metrics;
    metrics.totalAllocated = totalAllocated_.load(std::memory_order_relaxed);
    metrics.totalDeallocated = totalDeallocated_.load(std::memory_order_relaxed);
    metrics.peakUsage = peakUsage_.load(std::memory_order_relaxed);
    metrics.currentUsage = currentUsage_.load(std::memory_order_relaxed);
    metrics.allocationCount = allocationCount_.load(std::memory_order_relaxed);
    metrics.deallocationCount = deallocationCount_.load(std::memory_order_relaxed);
    metrics.categoryUsage = categoryUsage_;
    metrics.categoryPeaks = categoryPeaks_;

    return metrics;
  }

  // Memory pressure detection
  bool isUnderPressure() const {
    return currentUsage_.load(std::memory_order_relaxed) > memory::LOW_THRESHOLD;
  }

  bool isUnderCriticalPressure() const {
    return currentUsage_.load(std::memory_order_relaxed) > memory::CRITICAL_THRESHOLD;
  }

  // Reset metrics
  void reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    totalAllocated_.store(0, std::memory_order_relaxed);
    totalDeallocated_.store(0, std::memory_order_relaxed);
    peakUsage_.store(0, std::memory_order_relaxed);
    currentUsage_.store(0, std::memory_order_relaxed);
    allocationCount_.store(0, std::memory_order_relaxed);
    deallocationCount_.store(0, std::memory_order_relaxed);
    categoryUsage_.clear();
    categoryPeaks_.clear();
  }

 private:
  MemoryTracker() = default;

  mutable std::mutex mutex_;

  std::atomic<size_t> totalAllocated_{0};
  std::atomic<size_t> totalDeallocated_{0};
  std::atomic<size_t> peakUsage_{0};
  std::atomic<size_t> currentUsage_{0};
  std::atomic<size_t> allocationCount_{0};
  std::atomic<size_t> deallocationCount_{0};

  std::unordered_map<std::string, size_t> categoryUsage_;
  std::unordered_map<std::string, size_t> categoryPeaks_;

  std::chrono::steady_clock::time_point lastAllocationTime_;
};

}  // namespace shiki
