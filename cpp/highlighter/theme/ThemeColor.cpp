#include "ThemeColor.h"

#include <xxhash.h>

#include <cstdio>
#include <iomanip>
#include <sstream>

namespace shiki {

std::unordered_map<uint64_t, std::shared_ptr<ThemeColor>> ThemeColor::colorCache_;

uint64_t ThemeColor::computeHash(const std::string& hex) {
  XXH3_state_t* const state = XXH3_createState();
  if (!state) return 0;

  XXH3_64bits_reset(state);
  XXH3_64bits_update(state, hex.data(), hex.size());
  const uint64_t hash = XXH3_64bits_digest(state);
  XXH3_freeState(state);

  return hash;
}

void ThemeColor::parseHexColor() const {
  if (!isValid()) return;

  std::string cleanHex = hexColor;
  if (cleanHex[0] == '#') {
    cleanHex = cleanHex.substr(1);
  }

  // Handle both 6-digit and 8-digit hex colors
  if (cleanHex.length() == 6 || cleanHex.length() == 8) {
    // Parse each component separately to avoid stringstream issues
    unsigned int r = 0, g = 0, b = 0, a = 255;

    if (cleanHex.length() == 8) {
      sscanf(cleanHex.c_str(), "%2x%2x%2x%2x", &r, &g, &b, &a);
    } else {
      sscanf(cleanHex.c_str(), "%2x%2x%2x", &r, &g, &b);
    }

    const_cast<ThemeColor*>(this)->red = r / 255.0f;
    const_cast<ThemeColor*>(this)->green = g / 255.0f;
    const_cast<ThemeColor*>(this)->blue = b / 255.0f;
    const_cast<ThemeColor*>(this)->alpha = a / 255.0f;
  }
}

ThemeColor::ThemeColor(const std::string& hex, float a)
  : hexColor(hex.empty() ? "" : (hex[0] == '#' ? hex : "#" + hex)),
    hash_(computeHash(hexColor)),
    alpha(a),
    red(0),
    green(0),
    blue(0) {
  if (!hexColor.empty()) {
    parseHexColor();
  }
}

std::shared_ptr<ThemeColor> ThemeColor::fromHex(const std::string& hex) {
  if (hex.empty()) {
    return nullptr;
  }

  std::string normalizedHex = hex[0] == '#' ? hex : "#" + hex;
  uint64_t hashKey = computeHash(normalizedHex);

  // Check cache first
  auto it = colorCache_.find(hashKey);
  if (it != colorCache_.end()) {
    return it->second;
  }

  // Create new color if not in cache
  auto color = std::make_shared<ThemeColor>(normalizedHex);
  colorCache_[hashKey] = color;
  return color;
}

std::string ThemeColor::toHex() const {
  if (hexColor.empty()) return "";  // Return empty string instead of defaulting to black

  char hexStr[10];
  if (alpha != 1.0f) {
    snprintf(
      hexStr,
      sizeof(hexStr),
      "#%02X%02X%02X%02X",
      static_cast<int>(red * 255),
      static_cast<int>(green * 255),
      static_cast<int>(blue * 255),
      static_cast<int>(alpha * 255)
    );
  } else {
    snprintf(
      hexStr,
      sizeof(hexStr),
      "#%02X%02X%02X",
      static_cast<int>(red * 255),
      static_cast<int>(green * 255),
      static_cast<int>(blue * 255)
    );
  }
  return std::string(hexStr);
}

}  // namespace shiki
