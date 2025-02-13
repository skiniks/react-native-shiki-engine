#pragma once
#include "../core/Constants.h"
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace shiki {

class ConcurrencyUtil {
public:
  static constexpr size_t DEFAULT_BATCH_SIZE = 32 * 1024; // 32KB

  explicit ConcurrencyUtil(size_t numThreads = std::thread::hardware_concurrency());
  ~ConcurrencyUtil();

  // Thread pool functionality
  template <class F> void enqueue(F&& f) {
    {
      std::lock_guard<std::mutex> lock(queueMutex_);
      tasks_.emplace(std::forward<F>(f));
    }
    condition_.notify_one();
  }

  // Batch processing
  template <typename T>
  void
  processBatches(const std::string& input, std::vector<T>& output,
                 std::function<void(const std::string&, size_t, size_t, std::vector<T>&)> processor,
                 size_t batchSize = DEFAULT_BATCH_SIZE);

  // Wait for all tasks to complete
  void waitForCompletion();

  template <typename F> auto submit(F&& f) -> std::future<typename std::invoke_result<F>::type> {
    using ReturnType = typename std::invoke_result<F>::type;
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::forward<F>(f));
    auto future = task->get_future();

    {
      std::lock_guard<std::mutex> lock(mutex_);
      tasks_.emplace([task]() { (*task)(); });
    }

    condition_.notify_one();
    return future;
  }

private:
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex queueMutex_;
  std::condition_variable condition_;
  bool stop_{false};
  std::mutex mutex_;

  void workerThread();
};

} // namespace shiki
