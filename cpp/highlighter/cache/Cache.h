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
    size_t size{0};
    size_t hits{0};
    double frequency{1.0};
    Priority priority{Priority::NORMAL};
    typename std::list<K>::iterator lruIter;
    std::chrono::steady_clock::time_point lastAccess{std::chrono::steady_clock::now()};
    double timeDecayFactor{1.0};
    size_t consecutiveHits{0};
    double sizeScore{1.0};

    CacheEntry() = default;

    CacheEntry(V v, size_t s, typename std::list<K>::iterator it, Priority p)
      : value(std::move(v)), size(s), lruIter(it), priority(p), lastAccess(std::chrono::steady_clock::now()) {}

    // Calculate entry score for eviction decisions
    double getScore() const {
      const double priorityFactor = static_cast<double>(priority) + 1.0;
      const double timeFactor = calculateTimeFactor();
      const double consecutiveFactor = std::log2(consecutiveHits + 1.0);

      // Combine factors with weights
      return (hits * frequency * priorityFactor * timeFactor * consecutiveFactor * sizeScore * timeDecayFactor);
    }

   private:
    double calculateTimeFactor() const {
      auto now = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::hours>(now - lastAccess).count();
      // Exponential decay based on time since last access
      return std::exp(-0.1 * duration);
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

    // Check if we need to make space
    while (!entries_.empty() && (currentSize_ + size > maxSize_ || entries_.size() >= maxEntries_)) {
      evictEntry();
    }

    // Add new entry to LRU list
    lruList_.push_front(key);

    // Calculate initial size score based on cache capacity
    double sizeScore = 1.0 - (static_cast<double>(size) / maxSize_);
    sizeScore = std::max(0.1, std::min(1.0, sizeScore));

    // Create and insert new entry
    auto entry = CacheEntry(std::move(value), size, lruList_.begin(), priority);
    entry.sizeScore = sizeScore;
    entries_[key] = std::move(entry);
    currentSize_ += size;
    metrics_.size = entries_.size();
    metrics_.memoryUsage = currentSize_;
  }

  std::optional<V> get(const K& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entries_.find(key);

    if (it != entries_.end()) {
      auto& entry = it->second;

      // Update metrics
      entry.hits++;
      entry.consecutiveHits++;
      entry.lastAccess = std::chrono::steady_clock::now();

      // Update frequency using a sliding window
      const double alpha = 0.1;  // Sliding window factor
      entry.frequency = (1.0 - alpha) * entry.frequency + alpha;

      // Move to front of LRU list
      lruList_.erase(entry.lruIter);
      lruList_.push_front(key);
      entry.lruIter = lruList_.begin();

      metrics_.hits++;
      return entry.value;
    }

    metrics_.misses++;
    return std::nullopt;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
    lruList_.clear();
    currentSize_ = 0;
    metrics_.size = 0;
    metrics_.memoryUsage = 0;
  }

  // Metrics
  size_t size() const {
    return entries_.size();
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
              << "Entry Count: " << entries_.size() << "/" << maxEntries_ << "\n"
              << "==================\n";
  }

 private:
  void evictEntry() {
    // Find entry with lowest score
    auto lowestScoreIt = std::min_element(entries_.begin(), entries_.end(), [](const auto& a, const auto& b) {
      return a.second.getScore() < b.second.getScore();
    });

    if (lowestScoreIt != entries_.end()) {
      currentSize_ -= lowestScoreIt->second.size;
      lruList_.erase(lowestScoreIt->second.lruIter);
      entries_.erase(lowestScoreIt);
      metrics_.evictions++;
    }
  }

  size_t maxSize_;
  size_t maxEntries_;
  size_t currentSize_;

  mutable std::mutex mutex_;
  std::unordered_map<K, CacheEntry> entries_;
  std::list<K> lruList_;
  CacheMetrics metrics_;
};

}  // namespace shiki
