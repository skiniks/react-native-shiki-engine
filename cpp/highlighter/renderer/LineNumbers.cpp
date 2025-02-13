#include "LineNumbers.h"
#include <cmath>

namespace shiki {

LineNumbers::LineNumbers(const Config& config) : config_(config) {}

void LineNumbers::calculateLines(const std::string& code, float contentWidth, float lineHeight) {
  if (!config_.show) {
    totalWidth_ = 0;
    lines_.clear();
    return;
  }

  // Count total lines
  size_t lineCount = countLines(code);

  // Calculate width needed for largest line number
  totalWidth_ = calculateMaxLineWidth(lineCount) + (2 * config_.padding);

  // Calculate line positions
  lines_.clear();
  lines_.reserve(lineCount);

  float currentY = 0;
  for (size_t i = 1; i <= lineCount; i++) {
    LineInfo info;
    info.number = i;
    info.y = currentY;
    info.width = totalWidth_ - (2 * config_.padding);
    lines_.push_back(info);

    currentY += lineHeight;
  }
}

size_t LineNumbers::countLines(const std::string& code) const {
  if (code.empty())
    return 1;

  size_t count = 1;
  for (char c : code) {
    if (c == '\n')
      count++;
  }
  return count;
}

float LineNumbers::calculateMaxLineWidth(size_t maxLineNumber) const {
  // Calculate width needed for largest line number
  // Assuming monospace font where each digit has the same width
  size_t digits = static_cast<size_t>(std::log10(maxLineNumber)) + 1;
  return digits * config_.fontSize * 0.6f; // 0.6 is approximate width/height ratio for digits
}

} // namespace shiki
