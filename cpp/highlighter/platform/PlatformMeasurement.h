#pragma once
#include <string>

#include "../text/TextMetrics.h"
#include "../theme/ThemeStyle.h"

namespace shiki {

class PlatformMeasurement {
 public:
  virtual ~PlatformMeasurement() = default;
  virtual TextMetrics measureRange(const std::string& text, size_t start, size_t length, const ThemeStyle& style) = 0;
};

}  // namespace shiki
