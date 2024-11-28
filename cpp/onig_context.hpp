#ifndef ONIG_CONTEXT_HPP
#define ONIG_CONTEXT_HPP

#include "onig_regex.h"
#include "oniguruma.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct CachedPattern {
  regex_t* regex;
  time_t last_used;
  size_t memory_size;
};

struct OnigContextImpl {
  std::unordered_map<std::string, CachedPattern> pattern_cache;
  std::unordered_set<regex_t*> active_regexes;
};

inline size_t estimate_pattern_memory(const char* pattern, const regex_t* regex) {
  return strlen(pattern) + 1024;
}

#endif // ONIG_CONTEXT_HPP
