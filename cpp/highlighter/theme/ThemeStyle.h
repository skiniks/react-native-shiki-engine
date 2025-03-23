#pragma once
#include <optional>
#include <string>

#include "ThemeColor.h"
#include "highlighter/core/Constants.h"

namespace shiki {

struct ThemeStyle {
  std::string foreground;
  std::string background;
  std::string fontStyle;
  std::string scope;

  bool bold{false};
  bool italic{false};
  bool underline{false};

  // Line number specific styling
  struct LineNumberStyle {
    std::optional<std::string> foreground;
    std::optional<std::string> background;
    bool bold = false;
    bool italic = false;
  } lineNumbers;

  bool isEmpty() const {
    return foreground.empty() && background.empty() && fontStyle.empty() && scope.empty() && !bold && !italic &&
      !underline;
  }

  void applySettings(const ThemeStyle& style) {
    if (!style.foreground.empty()) {
      foreground = style.foreground;
    }
    // Only apply background if explicitly set
    if (!style.background.empty()) {
      background = style.background;
    }
    if (!style.fontStyle.empty()) {
      fontStyle = style.fontStyle;
    }
    if (!style.scope.empty()) {
      scope = style.scope;
    }
    bold |= style.bold;
    italic |= style.italic;
    underline |= style.underline;
  }

  // Helper method to get line number style
  LineNumberStyle getLineNumberStyle() const {
    if (!lineNumbers.foreground && !foreground.empty()) {
      LineNumberStyle style;
      style.foreground = foreground;
      style.background = background;  // Only if set
      return style;
    }
    return lineNumbers;
  }

  ThemeColor getThemeColor() const {
    // Don't create a ThemeColor if no foreground is specified
    return foreground.empty() ? ThemeColor() : ThemeColor(foreground);
  }

  ThemeColor getBackgroundColor() const {
    // Only return a background color if explicitly set
    return background.empty() ? ThemeColor() : ThemeColor(background);
  }

  // Merge another style into this one
  void merge(const ThemeStyle& other) {
    if (!other.foreground.empty()) {
      foreground = other.foreground[0] == '#' ? other.foreground : "#" + other.foreground;
    }
    if (!other.background.empty()) {
      background = other.background[0] == '#' ? other.background : "#" + other.background;
    }
    if (!other.fontStyle.empty()) {
      fontStyle = other.fontStyle;
    }
    bold |= other.bold;
    italic |= other.italic;
    underline |= other.underline;
  }

  // Check if this style has any properties set
  bool hasProperties() const {
    return !foreground.empty() || !background.empty() || !fontStyle.empty() || !scope.empty() || bold || italic ||
      underline;
  }

  bool operator==(const ThemeStyle& other) const {
    return foreground == other.foreground && background == other.background && fontStyle == other.fontStyle &&
      scope == other.scope && bold == other.bold && italic == other.italic && underline == other.underline;
  }

  static ThemeStyle getDefaultStyle() {
    ThemeStyle style;
    return style;
  }
};

}  // namespace shiki
