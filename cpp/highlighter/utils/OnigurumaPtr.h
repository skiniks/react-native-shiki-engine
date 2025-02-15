#pragma once
#include <oniguruma.h>

#include <memory>

namespace shiki {

struct OnigurumaRegexDeleter {
  void operator()(regex_t* regex) const {
    if (regex) {
      onig_free(regex);
    }
  }
};

struct OnigurumaRegionDeleter {
  void operator()(OnigRegion* region) const {
    if (region) {
      onig_region_free(region, 1);
    }
  }
};

using OnigurumaRegexPtr = std::unique_ptr<regex_t, OnigurumaRegexDeleter>;
using OnigurumaRegionPtr = std::unique_ptr<OnigRegion, OnigurumaRegionDeleter>;

inline OnigurumaRegexPtr createRegex(const char* pattern) {
  regex_t* regex = nullptr;
  OnigErrorInfo einfo;

  int result = onig_new(
    &regex,
    reinterpret_cast<const OnigUChar*>(pattern),
    reinterpret_cast<const OnigUChar*>(pattern + strlen(pattern)),
    ONIG_OPTION_DEFAULT,
    ONIG_ENCODING_UTF8,
    ONIG_SYNTAX_DEFAULT,
    &einfo
  );

  if (result != ONIG_NORMAL) {
    return nullptr;
  }

  return OnigurumaRegexPtr(regex);
}

inline OnigurumaRegionPtr createRegion() {
  return OnigurumaRegionPtr(onig_region_new());
}

}  // namespace shiki
