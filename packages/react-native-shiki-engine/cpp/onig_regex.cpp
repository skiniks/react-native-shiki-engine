#include "onig_regex.h"
#include "onig_context.hpp"
#include <algorithm>
#include <cstring>
#include <new>
#include <vector>

/** Rough estimate of regex pattern memory usage. Base + pattern size. */
static size_t estimate_pattern_memory(const char* pattern, regex_t* regex) {
  return strlen(pattern) + 1024;
}

/** Evicts cached patterns when memory limit exceeded, oldest first. */
static void check_memory_pressure(OnigContext* context) {
  if (context->current_memory_usage > CACHE_MEMORY_LIMIT) {
    std::vector<std::pair<std::string, time_t>> patterns;
    for (const auto& entry : context->impl->pattern_cache) {
      patterns.push_back({entry.first, entry.second.last_used});
    }

    std::sort(patterns.begin(), patterns.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    for (const auto& pattern : patterns) {
      if (context->current_memory_usage <= CACHE_MEMORY_LIMIT) {
        break;
      }

      auto it = context->impl->pattern_cache.find(pattern.first);
      if (it != context->impl->pattern_cache.end()) {
        context->current_memory_usage -= it->second.memory_size;
        onig_free(it->second.regex);
        context->impl->pattern_cache.erase(it);
      }
    }
  }
}

/** Removes expired patterns based on last access time. */
static void cleanup_cache(OnigContext* context) {
  time_t current_time = time(nullptr);
  auto it = context->impl->pattern_cache.begin();

  while (it != context->impl->pattern_cache.end()) {
    if (current_time - it->second.last_used > CACHE_EXPIRY_SECONDS) {
      context->current_memory_usage -= it->second.memory_size;
      onig_free(it->second.regex);
      it = context->impl->pattern_cache.erase(it);
    } else {
      ++it;
    }
  }
}

/** Retrieves cached pattern, updates LRU timestamp if found. */
static regex_t* get_cached_pattern(OnigContext* context, const char* pattern) {
  auto it = context->impl->pattern_cache.find(pattern);
  if (it != context->impl->pattern_cache.end()) {
    it->second.last_used = time(nullptr);
    return it->second.regex;
  }
  return nullptr;
}

/** LRU pattern caching with memory limit enforcement. */
static void cache_pattern(OnigContext* context, const char* pattern, regex_t* regex) {
  if (context->impl->pattern_cache.size() >= context->max_cache_size) {
    cleanup_cache(context);

    if (context->impl->pattern_cache.size() >= context->max_cache_size) {
      time_t oldest_time = time(nullptr);
      std::string oldest_pattern;

      for (const auto& entry : context->impl->pattern_cache) {
        if (entry.second.last_used < oldest_time) {
          oldest_time = entry.second.last_used;
          oldest_pattern = entry.first;
        }
      }

      auto it = context->impl->pattern_cache.find(oldest_pattern);
      if (it != context->impl->pattern_cache.end()) {
        context->current_memory_usage -= it->second.memory_size;
        onig_free(it->second.regex);
        context->impl->pattern_cache.erase(it);
      }
    }
  }

  size_t memory_size = estimate_pattern_memory(pattern, regex);
  check_memory_pressure(context);

  CachedPattern cached_pattern{regex, time(nullptr), memory_size};

  context->impl->pattern_cache[pattern] = cached_pattern;
  context->current_memory_usage += memory_size;
}

/** Creates UTF-8 regex scanner with LRU pattern cache. nullptr on failure. */
OnigContext* create_scanner(const char** patterns, int pattern_count, size_t max_cache_size) {
  static bool initialized = false;
  if (!initialized) {
    OnigEncodingType* encodings[] = {ONIG_ENCODING_UTF8};
    onig_initialize(encodings, 1);
    initialized = true;
  }

  try {
    OnigContext* context = new OnigContext();
    context->impl = new OnigContextImpl();
    context->pattern_count = pattern_count;
    context->max_cache_size = max_cache_size;
    context->current_memory_usage = 0;
    context->regexes = new regex_t*[static_cast<size_t>(pattern_count)];
    context->region = onig_region_new();

    for (int i = 0; i < pattern_count; i++) {
      regex_t* regex = get_cached_pattern(context, patterns[i]);

      if (!regex) {
        OnigErrorInfo einfo;
        int result = onig_new(&regex, (const OnigUChar*)patterns[i],
                              (const OnigUChar*)(patterns[i] + strlen(patterns[i])),
                              ONIG_OPTION_DEFAULT, ONIG_ENCODING_UTF8, ONIG_SYNTAX_DEFAULT, &einfo);

        if (result != ONIG_NORMAL) {
          onig_region_free(context->region, 1);
          delete[] context->regexes;
          delete context->impl;
          delete context;
          return nullptr;
        }

        cache_pattern(context, patterns[i], regex);
      }

      context->regexes[i] = regex;
      context->impl->active_regexes.insert(regex);
    }

    return context;
  } catch (const std::bad_alloc&) {
    return nullptr;
  }
}

/** Finds leftmost-longest match after start_pos across all patterns. */
OnigResult* find_next_match(OnigContext* context, const char* text, int start_pos) {
  if (!context || !text || start_pos < 0) {
    return nullptr;
  }

  try {
    OnigResult* result = new OnigResult();
    result->pattern_index = -1;
    result->capture_indices = nullptr;
    result->capture_count = 0;
    result->match_start = -1;
    result->match_end = -1;

    int text_length = strlen(text);
    int best_match_pos = -1;
    int best_match_len = -1;

    for (int i = 0; i < context->pattern_count; i++) {
      onig_region_clear(context->region);

      int match_pos =
          onig_search(context->regexes[i], (OnigUChar*)text, (OnigUChar*)(text + text_length),
                      (OnigUChar*)(text + start_pos), (OnigUChar*)(text + text_length),
                      context->region, ONIG_OPTION_NONE);

      if (match_pos >= 0) {
        if (best_match_pos < 0 || match_pos < best_match_pos ||
            (match_pos == best_match_pos &&
             context->region->end[0] - context->region->beg[0] > best_match_len)) {

          best_match_pos = match_pos;
          best_match_len = context->region->end[0] - context->region->beg[0];
          result->pattern_index = i;
          result->match_start = context->region->beg[0];
          result->match_end = context->region->end[0];

          delete[] result->capture_indices;
          result->capture_count = context->region->num_regs * 2;
          result->capture_indices = new int[result->capture_count];

          for (int j = 0; j < context->region->num_regs; j++) {
            result->capture_indices[j * 2] = context->region->beg[j];
            result->capture_indices[j * 2 + 1] = context->region->end[j];
          }
        }
      }
    }

    if (result->pattern_index >= 0) {
      return result;
    }

    delete result;
    return nullptr;
  } catch (const std::bad_alloc&) {
    return nullptr;
  }
}

/** Safe cleanup of match result and capture indices. */
void free_result(OnigResult* result) {
  if (result) {
    delete[] result->capture_indices;
    delete result;
  }
}

/** RAII cleanup of scanner context and all cached patterns. */
void free_scanner(OnigContext* context) {
  if (context) {
    if (context->region) {
      onig_region_free(context->region, 1);
    }

    for (auto regex : context->impl->active_regexes) {
      onig_free(regex);
    }

    delete[] context->regexes;
    delete context->impl;
    delete context;
  }
}
