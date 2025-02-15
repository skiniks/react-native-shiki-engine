#include "OnigurumaRegex.h"

#include <algorithm>
#include <cstring>
#include <new>
#include <vector>

#include "../highlighter/cache/PatternCache.h"
#include "../highlighter/memory/MemoryTracker.h"
#include "../highlighter/utils/OnigurumaPtr.h"
#include "OnigurumaContext.h"

using namespace shiki;

/** Rough estimate of regex pattern memory usage. Base + pattern size. */
static size_t estimate_pattern_memory(const char* pattern, regex_t* regex) {
  return strlen(pattern) + 1024;  // Base size for regex structure
}

/** Creates UTF-8 regex scanner with pattern cache. nullptr on failure. */
OnigurumaContext* create_scanner(const char** patterns, int pattern_count, size_t max_cache_size) {
  static bool initialized = false;
  if (!initialized) {
    OnigEncodingType* encodings[] = {ONIG_ENCODING_UTF8};
    onig_initialize(encodings, 1);
    initialized = true;
  }

  try {
    auto context = std::make_unique<OnigurumaContext>();
    context->impl = new OnigurumaContextImpl();
    context->pattern_count = pattern_count;
    context->regexes = new regex_t*[static_cast<size_t>(pattern_count)];
    context->region = createRegion().release();  // Use our smart pointer factory

    auto& cache = PatternCache::getInstance();
    auto& memTracker = MemoryTracker::getInstance();

    for (int i = 0; i < pattern_count; i++) {
      regex_t* regex = cache.getCachedPattern(patterns[i]);

      if (!regex) {
        auto newRegex = createRegex(patterns[i]);  // Use our smart pointer factory
        if (!newRegex) {
          return nullptr;
        }

        size_t memSize = estimate_pattern_memory(patterns[i], newRegex.get());
        memTracker.trackAllocation(memSize, "regex_pattern");

        regex = newRegex.release();
        cache.cachePattern(patterns[i], regex);
      }

      context->regexes[i] = regex;
      context->impl->active_regexes.push_back(regex);
    }

    return context.release();
  } catch (const std::bad_alloc&) {
    return nullptr;
  }
}

/** Finds leftmost-longest match after start_pos across all patterns. */
OnigurumaResult* find_next_match(OnigurumaContext* context, const char* text, int start_pos) {
  if (!context || !text || start_pos < 0) return nullptr;

  std::unique_ptr<OnigurumaResult> best_result;
  int best_pos = -1;
  int best_length = -1;

  for (int i = 0; i < context->pattern_count; i++) {
    OnigRegion* region = context->region;
    onig_region_clear(region);

    int result = onig_search(
      context->regexes[i],
      (const OnigUChar*)text,
      (const OnigUChar*)(text + strlen(text)),
      (const OnigUChar*)(text + start_pos),
      (const OnigUChar*)(text + strlen(text)),
      region,
      ONIG_OPTION_NONE
    );

    if (result >= 0) {
      int match_pos = region->beg[0];
      int match_length = region->end[0] - region->beg[0];

      if (!best_result || match_pos < best_pos || (match_pos == best_pos && match_length > best_length)) {
        auto new_result = std::make_unique<OnigurumaResult>();
        new_result->pattern_index = i;
        new_result->match_start = match_pos;
        new_result->match_end = region->end[0];
        new_result->capture_count = region->num_regs - 1;

        // Track memory for capture indices
        size_t captureMemSize = new_result->capture_count * 2 * sizeof(int);
        MemoryTracker::getInstance().trackAllocation(captureMemSize, "regex_captures");

        new_result->capture_indices = new int[new_result->capture_count * 2];
        for (int j = 0; j < new_result->capture_count; j++) {
          new_result->capture_indices[j * 2] = region->beg[j + 1];
          new_result->capture_indices[j * 2 + 1] = region->end[j + 1];
        }

        best_result = std::move(new_result);
        best_pos = match_pos;
        best_length = match_length;
      }
    }
  }

  return best_result.release();
}

/** Safe cleanup of match result and capture indices. */
void free_result(OnigurumaResult* result) {
  if (result) {
    if (result->capture_indices) {
      size_t captureMemSize = result->capture_count * 2 * sizeof(int);
      MemoryTracker::getInstance().trackDeallocation(captureMemSize, "regex_captures");
      delete[] result->capture_indices;
    }
    delete result;
  }
}

/** RAII cleanup of scanner context and all cached patterns. */
void free_scanner(OnigurumaContext* context) {
  if (context) {
    auto& memTracker = MemoryTracker::getInstance();

    // Clean up patterns
    for (int i = 0; i < context->pattern_count; i++) {
      if (context->regexes[i]) {
        size_t memSize = estimate_pattern_memory("", context->regexes[i]);
        memTracker.trackDeallocation(memSize, "regex_pattern");
      }
    }

    onig_region_free(context->region, 1);
    delete[] context->regexes;
    delete context->impl;
    delete context;
  }
}
