#ifndef ONIG_CONTEXT_HPP
#define ONIG_CONTEXT_HPP

#include <memory>
#include <vector>

#include "onig_regex.h"
#include "oniguruma.h"

struct CachedRegex {
  regex_t* regex = nullptr;
  size_t memory_size = 0;

  CachedRegex() = default;
  explicit CachedRegex(regex_t* r, size_t bytes) : regex(r), memory_size(bytes) {}

  ~CachedRegex() {
    if (regex) {
      onig_free(regex);
      regex = nullptr;
    }
  }

  CachedRegex(const CachedRegex&) = delete;
  CachedRegex& operator=(const CachedRegex&) = delete;
};

struct OnigContext {
  std::vector<std::shared_ptr<CachedRegex>> patterns;
  int pattern_count = 0;
  OnigRegion* region = nullptr;
  std::vector<int> last_captures;
  OnigResult last_result{};
};

#endif  // ONIG_CONTEXT_HPP
