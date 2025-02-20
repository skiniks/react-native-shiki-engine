#pragma once
#include <memory>
#include <vector>

#include "highlighter/text/TextDiff.h"

namespace shiki {

struct ViewportInfo {
  size_t firstVisibleLine;
  size_t lastVisibleLine;
  float contentHeight;
  float viewportHeight;
  bool isScrolling;
};

class VirtualizedContentManager {
 public:
  static constexpr size_t BUFFER_LINES = 50;  // Lines to render above/below viewport

  // Get visible range with buffer
  TextRange getVisibleRange(const ViewportInfo& viewport, const std::string& text);

  // Check if range is visible
  bool isRangeVisible(const TextRange& range, const ViewportInfo& viewport, const std::string& text);

  // Calculate line information
  size_t getLineCount(const std::string& text);
  std::vector<size_t> getLineStarts(const std::string& text);

  // Get estimated position info
  float getEstimatedHeight(size_t lineCount, float lineHeight);
  size_t getEstimatedLineAtPosition(float yOffset, float lineHeight);

 private:
  // Cache of line start positions
  std::vector<size_t> lineStartCache_;
  std::string lastProcessedText_;

  void updateLineCache(const std::string& text);
};

}  // namespace shiki
