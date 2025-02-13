#pragma once
#include "../core/Constants.h"
#include "ThemeColor.h"
#include <optional>
#include <string>

namespace shiki {

struct ThemeStyle {
  std::string color;
  std::string backgroundColor; // Only set if explicitly specified
  std::string fontStyle;
  std::string scope; // Added scope field for theme parsing

  bool bold{false};
  bool italic{false};
  bool underline{false};

  // Line number specific styling
  struct LineNumberStyle {
    std::optional<std::string> color;
    std::optional<std::string> backgroundColor;
    bool bold = false;
    bool italic = false;
  } lineNumbers;

  bool isEmpty() const {
    return color.empty() && backgroundColor.empty() && fontStyle.empty() && scope.empty() &&
           !bold && !italic && !underline;
  }

  void applySettings(const ThemeStyle& style) {
    if (!style.color.empty()) {
      color = style.color;
    }
    // Only apply background color if explicitly set
    if (!style.backgroundColor.empty()) {
      backgroundColor = style.backgroundColor;
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
    if (!lineNumbers.color && !color.empty()) {
      LineNumberStyle style;
      style.color = color;
      style.backgroundColor = backgroundColor; // Only if set
      return style;
    }
    return lineNumbers;
  }

  ThemeColor getThemeColor() const {
    // Don't create a ThemeColor if no color is specified
    return color.empty() ? ThemeColor() : ThemeColor(color);
  }

  ThemeColor getBackgroundColor() const {
    // Only return a background color if explicitly set
    return backgroundColor.empty() ? ThemeColor() : ThemeColor(backgroundColor);
  }

  // Merge another style into this one
  void merge(const ThemeStyle& other) {
    if (!other.color.empty()) {
      color = other.color[0] == '#' ? other.color : "#" + other.color;
    }
    if (!other.backgroundColor.empty()) {
      backgroundColor =
          other.backgroundColor[0] == '#' ? other.backgroundColor : "#" + other.backgroundColor;
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
    return !color.empty() || !backgroundColor.empty() || !fontStyle.empty() || bold || italic ||
           underline;
  }

  // Create a style with default values
  static ThemeStyle createDefault() {
    ThemeStyle style;
    style.color = "#F8F8F2"; // Default foreground color
    return style;
  }
};

} // namespace shiki
