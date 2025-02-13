#ifndef SHIKI_ONIGURUMA_CONTEXT_H
#define SHIKI_ONIGURUMA_CONTEXT_H

#include "OnigurumaRegex.h"
#include "../highlighter/cache/PatternCache.h"
#include <vector>

struct OnigurumaContextImpl {
  std::vector<regex_t*> active_regexes;
};

#endif // SHIKI_ONIGURUMA_CONTEXT_H
