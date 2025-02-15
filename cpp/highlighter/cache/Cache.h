#pragma once

#include <chrono>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace shiki {

template <typename K, typename V>
class Cache {
 public:
  enum class Priority { LOW = 0, NORMAL = 1, HIGH = 2 };

  struct CacheEntry {
    V value;
    size_t size;
    size_t hits{0};
    double frequency{1.0};  // Frequency factor for LRFU
    Priority priority{Priority::NORMAL};
    typename std::list<K>::iterator lruIter;
    std::chrono::steady_clock::time_point lastAccess;

    CacheEntry(V v, size_t s, typename std::list<K>::iterator it, Priority p)
      : value(std::move(v)), size(s), lruIter(it), priority(p) {}

    // Calculate entry score for eviction decisions
    double getScore() const {
      const double priorityFactor = static_cast<double>(priority) + 1.0;
      return (hits * frequency * priorityFactor);
    }
  };

  struct CacheMetrics {
    size_t hits{0};
    size_t misses{0};
    size_t evictions{0};
    size_t size{0};
    size_t memoryUsage{0};
    double hitRate() const {
      size_t total = hits + misses;
      return total > 0 ? (double)hits / total : 0.0;
    }
    std::string toString() const {
      std::stringstream ss;
      ss << "Cache Metrics:\n"
         << "  Hits: " << hits << "\n"
         << "  Misses: " << misses << "\n"
         << "  Evictions: " << evictions << "\n"
         << "  Size: " << size << "\n"
         << "  Memory Usage: " << memoryUsage << " bytes\n"
         << "  Hit Rate: " << (hitRate() * 100) << "%";
      return ss.str();
    }
  };

  explicit Cache(size_t maxSize, size_t maxEntries) : maxSize_(maxSize), maxEntries_(maxEntries), currentSize_(0) {}

  void add(const K& key, V value, size_t size, Priority priority) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Remove existing entry if present
    auto it = cache_.find(key);
    if (it != cache_.end()) {
      currentSize_ -= it->second.size;
      lruList_.erase(it->second.lruIter);
      cache_.erase(it);
    }

    // Evict entries until we have space
    while (!cache_.empty() && (currentSize_ + size > maxSize_ || cache_.size() >= maxEntries_)) {
      evictLeastValuable();
    }

    // Add new entry
    lruList_.push_front(key);
    cache_.emplace(key, CacheEntry(std::move(value), size, lruList_.begin(), priority));
    currentSize_ += size;
    metrics_.size = cache_.size();
    metrics_.memoryUsage = currentSize_;
  }

  std::optional<V> get(const K& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it != cache_.end()) {
      // Move to front of LRU
      lruList_.erase(it->second.lruIter);
      lruList_.push_front(key);
      it->second.lruIter = lruList_.begin();

      // Update frequency and hits
      it->second.hits++;
      it->second.frequency *= 1.2;  // Increase frequency factor
      it->second.lastAccess = std::chrono::steady_clock::now();

      metrics_.hits++;
      return it->second.value;
    }
    metrics_.misses++;
    return std::nullopt;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    lruList_.clear();
    currentSize_ = 0;
    metrics_.size = 0;
    metrics_.memoryUsage = 0;
  }

  // Metrics
  size_t size() const {
    return cache_.size();
  }
  size_t memoryUsage() const {
    return currentSize_;
  }

  CacheMetrics getMetrics() const {
    return metrics_;
  }

  void logMetrics(const std::string& cacheName) const {
    std::cout << "=== " << cacheName << " ===\n"
              << metrics_.toString() << "\n"
              << "Memory Usage: " << (currentSize_ * 100.0 / maxSize_) << "% of limit\n"
              << "Entry Count: " << cache_.size() << "/" << maxEntries_ << "\n"
              << "==================\n";
  }

 private:
  void evictLeastValuable() {
    if (lruList_.empty()) return;

    // Find entry with lowest score
    auto lowestScoreIt = cache_.begin();
    double lowestScore = std::numeric_limits<double>::max();

    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
      double score = it->second.getScore();
      if (score < lowestScore) {
        lowestScore = score;
        lowestScoreIt = it;
      }
    }

    if (lowestScoreIt != cache_.end()) {
      currentSize_ -= lowestScoreIt->second.size;
      lruList_.erase(lowestScoreIt->second.lruIter);
      cache_.erase(lowestScoreIt);
      metrics_.evictions++;
    }
  }

  size_t maxSize_;
  size_t maxEntries_;
  size_t currentSize_;

  mutable std::mutex mutex_;
  std::unordered_map<K, CacheEntry> cache_;
  std::list<K> lruList_;
  CacheMetrics metrics_;
};

}  // namespace shiki
