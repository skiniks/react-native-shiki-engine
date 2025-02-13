#pragma once
#include "../core/CacheEntry.h"
#include "../core/Constants.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace shiki {

class CacheManager {
public:
  static CacheManager& getInstance() {
    static CacheManager instance;
    return instance;
  }

  void add(const std::string& key, CacheEntry entry);
  std::optional<CacheEntry> get(const std::string& key);
  void clear();

  size_t getCurrentSize() const {
    return currentSize_;
  }
  size_t getEntryCount() const {
    return cache_.size();
  }

private:
  CacheManager() = default;

  std::unordered_map<std::string, CacheEntry> cache_;
  size_t currentSize_{0};

  void evictLeastUsed();
  void evictUntilFits(size_t requiredSize);
};

} // namespace shiki
