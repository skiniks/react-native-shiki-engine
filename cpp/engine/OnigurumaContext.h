#ifndef SHIKI_ONIGURUMA_CONTEXT_H
#define SHIKI_ONIGURUMA_CONTEXT_H

#include <vector>

#include "../highlighter/cache/PatternCache.h"
#include "OnigurumaRegex.h"

struct OnigurumaContextImpl {
  std::vector<regex_t*> active_regexes;
};

#endif  // SHIKI_ONIGURUMA_CONTEXT_H
