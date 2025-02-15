#include "TextDiff.h"

#include <algorithm>

namespace shiki {

std::vector<TextRange>
TextDiff::findChangedRegions(const std::string& oldText, const std::string& newText, size_t contextLines) {
  std::vector<TextRange> changes;

  // Simple character-by-character diff
  size_t i = 0, j = 0;
  size_t lastMatchPos = 0;

  while (i < oldText.length() && j < newText.length()) {
    if (oldText[i] != newText[j]) {
      // Find next matching position
      size_t oldK = i, newK = j;
      while (oldK < oldText.length() && newK < newText.length() && oldText[oldK] != newText[newK]) {
        oldK++;
        newK++;
      }

      // Record changed region
      TextRange range;
      range.start = lastMatchPos;
      range.length = std::max(oldK - i, newK - j);

      // Expand to full lines with context
      expandRangeToFullLines(range, newText, contextLines);
      changes.push_back(range);

      i = oldK;
      j = newK;
      lastMatchPos = j;
    } else {
      i++;
      j++;
    }
  }

  // Handle remaining text
  if (i < oldText.length() || j < newText.length()) {
    TextRange range;
    range.start = lastMatchPos;
    range.length = std::max(oldText.length() - i, newText.length() - j);
    expandRangeToFullLines(range, newText, contextLines);
    changes.push_back(range);
  }

  return mergeRanges(changes);
}

size_t TextDiff::findLineNumber(const std::string& text, size_t position) {
  size_t line = 0;
  size_t pos = 0;

  while (pos < position && pos < text.length()) {
    if (text[pos] == '\n') line++;
    pos++;
  }

  return line;
}

TextRange TextDiff::getLineRange(const std::string& text, size_t lineNumber) {
  TextRange range{0, 0};
  size_t currentLine = 0;
  size_t pos = 0;

  // Find line start
  while (currentLine < lineNumber && pos < text.length()) {
    if (text[pos] == '\n') currentLine++;
    pos++;
  }
  range.start = pos;

  // Find line end
  while (pos < text.length() && text[pos] != '\n') {
    pos++;
  }
  range.length = pos - range.start;

  return range;
}

void TextDiff::expandRangeToFullLines(TextRange& range, const std::string& text, size_t contextLines) {
  // Find start of first line
  size_t start = range.start;
  while (start > 0 && text[start - 1] != '\n') {
    start--;
  }

  // Find end of last line
  size_t end = range.start + range.length;
  while (end < text.length() && text[end] != '\n') {
    end++;
  }

  // Add context lines
  for (size_t i = 0; i < contextLines; i++) {
    // Add previous line
    size_t prevStart = start;
    while (prevStart > 0 && text[prevStart - 1] != '\n') {
      prevStart--;
    }
    start = prevStart;

    // Add next line
    while (end < text.length() && text[end] != '\n') {
      end++;
    }
    if (end < text.length()) end++;
  }

  range.start = start;
  range.length = end - start;
}

std::vector<TextRange> TextDiff::mergeRanges(const std::vector<TextRange>& ranges, size_t maxGap) {
  if (ranges.empty()) return ranges;

  std::vector<TextRange> merged;
  TextRange current = ranges[0];

  for (size_t i = 1; i < ranges.size(); i++) {
    const auto& next = ranges[i];

    // If ranges are close enough, merge them
    if (next.start - (current.start + current.length) <= maxGap) {
      current.length = (next.start + next.length) - current.start;
    } else {
      merged.push_back(current);
      current = next;
    }
  }

  merged.push_back(current);
  return merged;
}

}  // namespace shiki
