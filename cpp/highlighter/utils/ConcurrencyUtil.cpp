#include "ConcurrencyUtil.h"

namespace shiki {

ConcurrencyUtil::ConcurrencyUtil(size_t numThreads) : stop_(false) {
  for (size_t i = 0; i < numThreads; ++i) {
    workers_.emplace_back(&ConcurrencyUtil::workerThread, this);
  }
}

ConcurrencyUtil::~ConcurrencyUtil() {
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    stop_ = true;
  }
  condition_.notify_all();
  for (auto& worker : workers_) {
    worker.join();
  }
}

void ConcurrencyUtil::waitForCompletion() {
  std::unique_lock<std::mutex> lock(queueMutex_);
  condition_.wait(lock, [this] { return tasks_.empty(); });
}

void ConcurrencyUtil::workerThread() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(queueMutex_);
      condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
      if (stop_ && tasks_.empty()) {
        return;
      }
      task = std::move(tasks_.front());
      tasks_.pop();
    }
    task();
  }
}

}  // namespace shiki
