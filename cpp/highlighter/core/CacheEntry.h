#pragma once
#include "../text/TextRange.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace shiki {

struct CacheEntry {
  std::string code;
  std::string language;
  std::vector<TextRange> ranges;
  size_t timestamp;
  size_t hitCount{0};
  std::string themeHash; // Hash of theme settings to invalidate cache when theme changes

  CacheEntry() = default;

  CacheEntry(std::string c, std::string l, std::vector<TextRange> r, std::string th = "",
             size_t ts = 0)
      : code(std::move(c)), language(std::move(l)), ranges(std::move(r)), themeHash(std::move(th)),
        timestamp(ts) {}

  size_t getMemoryUsage() const {
    return code.capacity() + language.capacity() + (ranges.capacity() * sizeof(TextRange)) +
           themeHash.capacity();
  }

  bool isValid(const std::string& currentThemeHash) const {
    return themeHash == currentThemeHash;
  }
};

} // namespace shiki
