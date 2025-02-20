#pragma once
#include <string>
#include <vector>

#include "highlighter/theme/Theme.h"

namespace shiki {

// Core highlighting types
struct HighlightRange {
  size_t start;
  size_t length;
  ThemeStyle style;
};

struct OnigCapture {
  int start;
  int end;
  std::string value;
};

// Forward declarations
class Theme;
class Grammar;

}  // namespace shiki
