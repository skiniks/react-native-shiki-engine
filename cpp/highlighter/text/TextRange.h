#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../platform/PlatformMeasurement.h"
#include "../theme/ThemeStyle.h"
#include "TextMetrics.h"

namespace shiki {

class TextRange {
 public:
  TextRange() : start(0), length(0) {}
  TextRange(size_t s, size_t l) : start(s), length(l) {}
  TextRange(size_t s, size_t l, const ThemeStyle& st) : start(s), length(l), style(st) {}
  virtual ~TextRange() = default;

  size_t start;
  size_t length;
  ThemeStyle style;

  virtual void measure(const std::string& text, const std::shared_ptr<PlatformMeasurement>& measurer) {
    if (measurer) {
      metrics_ = measurer->measureRange(text, start, length, style);
    }
  }

  bool isValid() const {
    return start >= 0 && length > 0;
  }

  bool contains(size_t position) const {
    return position >= start && position < (start + length);
  }

  bool overlaps(const TextRange& other) const {
    return !(other.start >= (start + length) || (other.start + other.length) <= start);
  }

  bool operator==(const TextRange& other) const {
    return start == other.start && length == other.length;
  }

  const TextMetrics& getMetrics() const {
    return metrics_;
  }

  TextRange intersect(const TextRange& other) const {
    size_t newStart = std::max(start, other.start);
    size_t newEnd = std::min(start + length, other.start + other.length);
    if (newStart < newEnd) {
      return TextRange(newStart, newEnd - newStart);
    }
    return TextRange(0, 0);
  }

  TextRange merge(const TextRange& other) const {
    if (!overlaps(other)) {
      return *this;
    }
    size_t newStart = std::min(start, other.start);
    size_t newEnd = std::max(start + length, other.start + other.length);
    return TextRange(newStart, newEnd - newStart);
  }

 protected:
  TextMetrics metrics_;
};

}  // namespace shiki
