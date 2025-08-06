#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "highlighter/core/Constants.h"

namespace shiki {

class ConcurrencyUtil;

enum class WorkPriority { LOW = 0, NORMAL = 1, HIGH = 2, CRITICAL = 3 };

struct WorkItem {
  std::function<void()> task;
  WorkPriority priority;
  std::chrono::steady_clock::time_point timestamp;
  size_t estimatedCost;  // Estimated memory/CPU cost
  std::string category;  // For tracking and analytics
  bool isVisible;  // Whether the work affects visible content

  WorkItem(std::function<void()> t, WorkPriority p, size_t cost, std::string cat, bool visible = false)
    : task(std::move(t)),
      priority(p),
      timestamp(std::chrono::steady_clock::now()),
      estimatedCost(cost),
      category(std::move(cat)),
      isVisible(visible) {}

  // Compare work items based on priority and age
  bool operator<(const WorkItem& other) const {
    if (priority != other.priority) {
      return priority < other.priority;
    }
    return timestamp > other.timestamp;  // Older items have higher priority
  }
};

class WorkPrioritizer {
 public:
  static WorkPrioritizer& getInstance() {
    static WorkPrioritizer instance;
    return instance;
  }

  // Submit work to be prioritized
  void submitWork(WorkItem item);

  // Process pending work
  void processPendingWork();

  // Memory pressure callbacks
  void onHighMemoryPressure();
  void onCriticalMemoryPressure();

  // Configuration
  void setMaxConcurrentWork(size_t max) {
    maxConcurrentWork_ = max;
  }
  void setMemoryThreshold(size_t threshold) {
    memoryThreshold_ = threshold;
  }

  // Analytics
  struct WorkMetrics {
    size_t pendingLow;
    size_t pendingNormal;
    size_t pendingHigh;
    size_t pendingCritical;
    size_t completedCount;
    size_t droppedCount;
    std::unordered_map<std::string, size_t> categoryMetrics;
  };

  WorkMetrics getMetrics() const;

 private:
  WorkPrioritizer();
  ~WorkPrioritizer();

  // Work queues for different priorities
  std::priority_queue<WorkItem> workQueue_;

  // State tracking
  std::atomic<size_t> activeWorkCount_{0};
  std::atomic<size_t> completedWorkCount_{0};
  std::atomic<size_t> droppedWorkCount_{0};

  // Configuration
  std::atomic<size_t> maxConcurrentWork_{4};
  std::atomic<size_t> memoryThreshold_{100 * 1024 * 1024};  // 100MB

  // Category tracking
  std::unordered_map<std::string, size_t> categoryWorkCounts_;

  // Synchronization
  mutable std::mutex mutex_;
  std::condition_variable condition_;

  // Memory management now handled by shikitori

  // Thread pool
  std::unique_ptr<ConcurrencyUtil> concurrencyUtil_;

  // Helper methods
  bool canScheduleWork(const WorkItem& item) const;
  void scheduleWork(WorkItem item);
  void dropLowPriorityWork();
};

}  // namespace shiki
