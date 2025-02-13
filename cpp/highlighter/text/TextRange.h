#pragma once
#include "../theme/ThemeStyle.h"
#include <cstdint>
#include <string>

namespace shiki {

class TextRange {
public:
  TextRange() : start(0), length(0) {}
  TextRange(size_t s, size_t l) : start(s), length(l) {}
  TextRange(size_t s, size_t l, const ThemeStyle& st) : start(s), length(l), style(st) {}
  virtual ~TextRange() = default;

  // Make this non-abstract by providing a default implementation
  virtual void measure() {}

  size_t start;
  size_t length;

  ThemeStyle style;
  int fontStyle{0};

  // Platform-independent size metrics
  float width{0};
  float height{0};

  bool isValid() const {
    return start >= 0 && length > 0;
  }
};

} // namespace shiki
