#include "ThemeColor.h"
#include <iomanip>
#include <sstream>

namespace shiki {

std::unordered_map<std::string, std::shared_ptr<ThemeColor>>
    ThemeColor::colorCache_;

ThemeColor::ThemeColor(const std::string &hex, float a)
    : hexColor(hex.empty() ? "#000000" : hex), alpha(a), red(0), green(0),
      blue(0) {
  // Parse hex color on construction
  std::string cleanHex = hexColor;
  if (cleanHex[0] == '#') {
    cleanHex = cleanHex.substr(1);
  }

  unsigned int value;
  std::stringstream ss;
  ss << std::hex << cleanHex;
  ss >> value;

  if (cleanHex.length() == 6) {
    red = ((value >> 16) & 0xFF) / 255.0f;
    green = ((value >> 8) & 0xFF) / 255.0f;
    blue = (value & 0xFF) / 255.0f;
  } else if (cleanHex.length() == 8) {
    red = ((value >> 24) & 0xFF) / 255.0f;
    green = ((value >> 16) & 0xFF) / 255.0f;
    blue = ((value >> 8) & 0xFF) / 255.0f;
    alpha = (value & 0xFF) / 255.0f;
  }
}

ThemeColor ThemeColor::fromHex(const std::string &hex) {
  // Check cache first
  auto it = colorCache_.find(hex);
  if (it != colorCache_.end()) {
    return *it->second;
  }

  // Create new color and cache it
  auto color = std::make_shared<ThemeColor>(hex);
  colorCache_[hex] = color;
  return *color;
}

UIColor *ThemeColor::toUIColor() const {
  return [UIColor colorWithRed:red green:green blue:blue alpha:alpha];
}

std::shared_ptr<ThemeColor> ThemeColor::fromUIColor(UIColor *color) {
  CGFloat r, g, b, a;
  [color getRed:&r green:&g blue:&b alpha:&a];

  auto themeColor = std::make_shared<ThemeColor>();
  themeColor->red = r; // UIColor values are already in 0-1 range
  themeColor->green = g;
  themeColor->blue = b;
  themeColor->alpha = a;

  // Generate hex color string
  themeColor->hexColor = themeColor->toHex();

  return themeColor;
}

std::string ThemeColor::toHex() const {
  std::stringstream ss;
  ss << "#" << std::hex << std::setfill('0') << std::setw(2)
     << static_cast<int>(red * 255) << std::hex << std::setfill('0')
     << std::setw(2) << static_cast<int>(green * 255) << std::hex
     << std::setfill('0') << std::setw(2) << static_cast<int>(blue * 255);
  if (alpha != 1.0f) {
    ss << std::hex << std::setfill('0') << std::setw(2)
       << static_cast<int>(alpha * 255);
  }
  return ss.str();
}

} // namespace shiki
