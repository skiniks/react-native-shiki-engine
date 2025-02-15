#ifndef SHIKI_ONIGURUMA_REGEX_H
#define SHIKI_ONIGURUMA_REGEX_H

#include <oniguruma.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OnigurumaContextImpl;

typedef struct OnigurumaContext {
  struct OnigurumaContextImpl* impl;
  regex_t** regexes;
  OnigRegion* region;
  int pattern_count;
} OnigurumaContext;

typedef struct OnigurumaResult {
  int pattern_index;
  int* capture_indices;
  int capture_count;
  int match_start;
  int match_end;
} OnigurumaResult;

OnigurumaContext* create_scanner(const char** patterns, int pattern_count, size_t max_cache_size);
OnigurumaResult* find_next_match(OnigurumaContext* context, const char* text, int start_pos);
void free_result(OnigurumaResult* result);
void free_scanner(OnigurumaContext* context);

#ifdef __cplusplus
}
#endif

#endif  // SHIKI_ONIGURUMA_REGEX_H
