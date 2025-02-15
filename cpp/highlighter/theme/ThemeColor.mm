#include "ThemeColor.h"
#include <iomanip>
#include <sstream>
#include <xxhash.h>

namespace shiki {

std::unordered_map<uint64_t, std::shared_ptr<ThemeColor>>
    ThemeColor::colorCache_;

static uint64_t computeColorHash(const std::string &hex) {
  return XXH3_64bits(hex.c_str(), hex.length());
}

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

std::shared_ptr<ThemeColor> ThemeColor::fromHex(const std::string &hex) {
  std::string normalizedHex = hex[0] == '#' ? hex : "#" + hex;

  uint64_t hashKey = computeColorHash(normalizedHex);

  // Check cache first
  auto it = colorCache_.find(hashKey);
  if (it != colorCache_.end()) {
    return it->second;
  }

  auto color = std::make_shared<ThemeColor>();
  color->hexColor = normalizedHex;

  colorCache_[hashKey] = color;
  return color;
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

std::string ThemeColor::toHex() const { return hexColor; }

void ThemeColor::clearCache() { colorCache_.clear(); }

} // namespace shiki
