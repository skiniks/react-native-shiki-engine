#include "WorkPrioritizer.h"
#include "ConcurrencyUtil.h"
#include <algorithm>
#include <thread>

namespace shiki {

WorkPrioritizer::WorkPrioritizer()
  : memoryManager_(MemoryManager::getInstance())
  , concurrencyUtil_(std::make_unique<ConcurrencyUtil>(std::thread::hardware_concurrency())) {

  // Register memory pressure callbacks
  memoryManager_.setHighPressureCallback([this]() { onHighMemoryPressure(); });
  memoryManager_.setCriticalPressureCallback([this]() { onCriticalMemoryPressure(); });
}

WorkPrioritizer::~WorkPrioritizer() = default;

void WorkPrioritizer::submitWork(WorkItem item) {
  std::lock_guard<std::mutex> lock(mutex_);

  // Track category stats
  categoryWorkCounts_[item.category]++;

  // Add to queue
  workQueue_.push(std::move(item));

  // Notify processing thread
  condition_.notify_one();
}

void WorkPrioritizer::processPendingWork() {
  std::unique_lock<std::mutex> lock(mutex_);

  while (!workQueue_.empty()) {
    // Get next work item
    WorkItem item = workQueue_.top();
    workQueue_.pop();

    // Check if we can schedule it
    if (!canScheduleWork(item)) {
      // Re-queue if we can't schedule now
      workQueue_.push(std::move(item));
      break;
    }

    // Schedule the work
    scheduleWork(std::move(item));
  }
}

bool WorkPrioritizer::canScheduleWork(const WorkItem& item) const {
  // Check active work count
  if (activeWorkCount_ >= maxConcurrentWork_) {
    return false;
  }

  // Check memory pressure
  if (memoryManager_.getCurrentUsage() + item.estimatedCost > memoryThreshold_) {
    return false;
  }

  return true;
}

void WorkPrioritizer::scheduleWork(WorkItem item) {
  activeWorkCount_++;

  // Wrap the task to handle completion
  auto wrappedTask = [this, item = std::move(item)]() {
    try {
      // Execute the actual work
      item.task();

      // Update stats
      completedWorkCount_++;
      categoryWorkCounts_[item.category]--;

    } catch (...) {
      // Handle errors
      droppedWorkCount_++;
    }

    activeWorkCount_--;
    condition_.notify_one();
  };

  // Submit to thread pool
  concurrencyUtil_->enqueue(std::move(wrappedTask));
}

void WorkPrioritizer::onHighMemoryPressure() {
  std::lock_guard<std::mutex> lock(mutex_);

  // Drop some low priority work
  dropLowPriorityWork();

  // Lower concurrent work limit
  maxConcurrentWork_ = std::max(size_t(1), maxConcurrentWork_ / 2);
}

void WorkPrioritizer::onCriticalMemoryPressure() {
  std::lock_guard<std::mutex> lock(mutex_);

  // Drop all low and normal priority work
  while (!workQueue_.empty()) {
    auto& item = workQueue_.top();
    if (item.priority <= WorkPriority::NORMAL) {
      droppedWorkCount_++;
      categoryWorkCounts_[item.category]--;
      workQueue_.pop();
    } else {
      break;
    }
  }

  // Set concurrent work to minimum
  maxConcurrentWork_ = 1;
}

void WorkPrioritizer::dropLowPriorityWork() {
  std::vector<WorkItem> retained;

  // Remove low priority items
  while (!workQueue_.empty()) {
    auto item = workQueue_.top();
    workQueue_.pop();

    if (item.priority > WorkPriority::LOW) {
      retained.push_back(std::move(item));
    } else {
      droppedWorkCount_++;
      categoryWorkCounts_[item.category]--;
    }
  }

  // Re-add retained items
  for (auto& item : retained) {
    workQueue_.push(std::move(item));
  }
}

WorkPrioritizer::WorkMetrics WorkPrioritizer::getMetrics() const {
  std::lock_guard<std::mutex> lock(mutex_);

  WorkMetrics metrics{};
  metrics.completedCount = completedWorkCount_;
  metrics.droppedCount = droppedWorkCount_;
  metrics.categoryMetrics = categoryWorkCounts_;

  // Count pending work by priority
  std::priority_queue<WorkItem> tempQueue = workQueue_;
  while (!tempQueue.empty()) {
    const auto& item = tempQueue.top();
    switch (item.priority) {
      case WorkPriority::LOW:
        metrics.pendingLow++;
        break;
      case WorkPriority::NORMAL:
        metrics.pendingNormal++;
        break;
      case WorkPriority::HIGH:
        metrics.pendingHigh++;
        break;
      case WorkPriority::CRITICAL:
        metrics.pendingCritical++;
        break;
    }
    tempQueue.pop();
  }

  return metrics;
}

} // namespace shiki
