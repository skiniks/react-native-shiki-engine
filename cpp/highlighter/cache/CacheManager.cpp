#include "CacheManager.h"
#include "../core/Constants.h"
#include <algorithm>

namespace shiki {

void CacheManager::add(const std::string& key, CacheEntry entry) {
  // If key exists, update the entry
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    currentSize_ -= it->second.getMemoryUsage();
    it->second = std::move(entry);
    currentSize_ += entry.getMemoryUsage();
  } else {
    // Add new entry
    currentSize_ += entry.getMemoryUsage();
    cache_.emplace(key, std::move(entry));
  }

  // Check if we need to evict entries
  if (currentSize_ > cache::SIZE_LIMIT || cache_.size() > cache::MAX_ENTRIES) {
    evictUntilFits(currentSize_);
  }
}

std::optional<CacheEntry> CacheManager::get(const std::string& key) {
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    it->second.hitCount++;
    return it->second;
  }
  return std::nullopt;
}

void CacheManager::clear() {
  cache_.clear();
  currentSize_ = 0;
}

void CacheManager::evictLeastUsed() {
  if (cache_.empty())
    return;

  auto least_used =
      std::min_element(cache_.begin(), cache_.end(), [](const auto& a, const auto& b) {
        return a.second.hitCount < b.second.hitCount;
      });

  currentSize_ -= least_used->second.getMemoryUsage();
  cache_.erase(least_used);
}

void CacheManager::evictUntilFits(size_t requiredSize) {
  while (!cache_.empty() &&
         (currentSize_ > cache::SIZE_LIMIT || cache_.size() > cache::MAX_ENTRIES)) {
    evictLeastUsed();
  }
}

} // namespace shiki
