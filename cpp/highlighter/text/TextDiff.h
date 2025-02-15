#pragma once
#include <string>
#include <vector>

#include "TextRange.h"

namespace shiki {

class TextDiff {
 public:
  // Compute differences between old and new text
  static std::vector<TextRange>
  findChangedRegions(const std::string& oldText, const std::string& newText, size_t contextLines = 1);

  // Find the line number for a position
  static size_t findLineNumber(const std::string& text, size_t position);

  // Get line start/end positions
  static TextRange getLineRange(const std::string& text, size_t lineNumber);

 private:
  // Helper methods for diff algorithm
  static std::vector<TextRange> mergeRanges(const std::vector<TextRange>& ranges, size_t maxGap = 100);

  static void expandRangeToFullLines(TextRange& range, const std::string& text, size_t contextLines);
};

}  // namespace shiki
