#ifndef ONIG_REGEX_H
#define ONIG_REGEX_H

#include <oniguruma.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CACHE_SIZE       1000
#define CACHE_EXPIRY_SECONDS 3600
#define CACHE_MEMORY_LIMIT   (50 * 1024 * 1024)

struct OnigContextImpl;

typedef struct OnigContext {
  struct OnigContextImpl* impl;
  regex_t** regexes;
  OnigRegion* region;
  int pattern_count;
  size_t max_cache_size;
  size_t current_memory_usage;
} OnigContext;

typedef struct OnigResult {
  int pattern_index;
  int* capture_indices;
  int capture_count;
  int match_start;
  int match_end;
} OnigResult;

OnigContext* create_scanner(const char** patterns, int pattern_count, size_t max_cache_size);
OnigResult* find_next_match(OnigContext* context, const char* text, int start_pos);
void free_result(OnigResult* result);
void free_scanner(OnigContext* context);

#ifdef __cplusplus
}
#endif

#endif  // ONIG_REGEX_H
