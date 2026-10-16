#ifndef ONIG_REGEX_H
#define ONIG_REGEX_H

#include "oniguruma.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_MAX_CACHE_ENTRIES 1000

typedef struct OnigContext OnigContext;

typedef struct OnigResult {
  int pattern_index;
  int* capture_indices;
  int capture_count;
  int match_start;
  int match_end;
} OnigResult;

typedef struct OnigCacheStats {
  size_t entry_count;
  size_t estimated_bytes;
  size_t scanner_count;
  size_t max_entries;
  size_t max_bytes;
} OnigCacheStats;

void configure_pattern_cache(size_t max_entries, size_t max_bytes);

void clear_unused_pattern_cache(void);

void trim_pattern_cache(void);

OnigCacheStats get_pattern_cache_stats(void);

OnigContext* create_scanner(const char** patterns, int pattern_count, size_t max_cache_size);

OnigResult* find_next_match(OnigContext* context, const char* text, int start_pos);
void free_result(OnigResult* result);
void free_scanner(OnigContext* context);

#ifdef __cplusplus
}
#endif

#endif  // ONIG_REGEX_H
