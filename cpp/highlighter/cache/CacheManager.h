#pragma once
#include <optional>
#include <string>
#include <unordered_map>

#include "../core/CacheEntry.h"
#include "../core/Constants.h"
#include "Cache.h"

namespace shiki {

class CacheManager {
 public:
  static CacheManager& getInstance() {
    static CacheManager instance;
    return instance;
  }

  void add(const std::string& key, CacheEntry entry) {
    size_t entrySize = entry.getMemoryUsage();

    // Determine priority based on hit count and language
    Cache<std::string, CacheEntry>::Priority priority;
    if (entry.language == "javascript" || entry.language == "typescript" || entry.language == "jsx" ||
        entry.language == "tsx") {
      priority = Cache<std::string, CacheEntry>::Priority::HIGH;
    } else {
      priority = entry.hitCount > 5 ? Cache<std::string, CacheEntry>::Priority::HIGH
                                    : Cache<std::string, CacheEntry>::Priority::NORMAL;
    }

    cache_.add(key, std::move(entry), entrySize, priority);
  }

  std::optional<CacheEntry> get(const std::string& key) {
    return cache_.get(key);
  }

  void clear() {
    cache_.clear();
  }

  size_t getCurrentSize() const {
    return cache_.memoryUsage();
  }

  size_t getEntryCount() const {
    return cache_.size();
  }

  Cache<std::string, CacheEntry>::CacheMetrics getMetrics() const {
    return cache_.getMetrics();
  }

 private:
  CacheManager() : cache_(cache::SIZE_LIMIT, cache::MAX_ENTRIES) {}

  Cache<std::string, CacheEntry> cache_;
};

}  // namespace shiki
