#include "VirtualizedContentManager.h"

#include <algorithm>

namespace shiki {

TextRange VirtualizedContentManager::getVisibleRange(const ViewportInfo& viewport, const std::string& text) {
  // Update line cache if text changed
  if (text != lastProcessedText_) {
    updateLineCache(text);
  }

  // Calculate visible range with buffer
  size_t startLine = viewport.firstVisibleLine > BUFFER_LINES ? viewport.firstVisibleLine - BUFFER_LINES : 0;

  size_t endLine = std::min(viewport.lastVisibleLine + BUFFER_LINES, lineStartCache_.size() - 1);

  // Convert to text range
  TextRange range;
  range.start = lineStartCache_[startLine];

  if (endLine < lineStartCache_.size() - 1) {
    range.length = lineStartCache_[endLine + 1] - range.start;
  } else {
    range.length = text.length() - range.start;
  }

  return range;
}

bool VirtualizedContentManager::isRangeVisible(
  const TextRange& range,
  const ViewportInfo& viewport,
  const std::string& text
) {
  // Update line cache if needed
  if (text != lastProcessedText_) {
    updateLineCache(text);
  }

  // Find lines containing the range
  auto startIt = std::lower_bound(lineStartCache_.begin(), lineStartCache_.end(), range.start);
  auto endIt = std::lower_bound(lineStartCache_.begin(), lineStartCache_.end(), range.start + range.length);

  size_t startLine = startIt - lineStartCache_.begin();
  size_t endLine = endIt - lineStartCache_.begin();

  // Check if range overlaps visible area (including buffer)
  return !(endLine < viewport.firstVisibleLine - BUFFER_LINES || startLine > viewport.lastVisibleLine + BUFFER_LINES);
}

void VirtualizedContentManager::updateLineCache(const std::string& text) {
  lineStartCache_.clear();
  lineStartCache_.push_back(0);  // First line always starts at 0

  for (size_t i = 0; i < text.length(); i++) {
    if (text[i] == '\n') {
      lineStartCache_.push_back(i + 1);
    }
  }

  lastProcessedText_ = text;
}

size_t VirtualizedContentManager::getLineCount(const std::string& text) {
  if (text != lastProcessedText_) {
    updateLineCache(text);
  }
  return lineStartCache_.size();
}

std::vector<size_t> VirtualizedContentManager::getLineStarts(const std::string& text) {
  if (text != lastProcessedText_) {
    updateLineCache(text);
  }
  return lineStartCache_;
}

float VirtualizedContentManager::getEstimatedHeight(size_t lineCount, float lineHeight) {
  return lineCount * lineHeight;
}

size_t VirtualizedContentManager::getEstimatedLineAtPosition(float yOffset, float lineHeight) {
  return static_cast<size_t>(yOffset / lineHeight);
}

}  // namespace shiki
