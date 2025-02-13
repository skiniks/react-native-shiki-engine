#include "../../theme/ThemeColor.h"
#import <UIKit/UIKit.h>

namespace {
// Helper function to convert hex string to RGB components
void parseHexColor(const std::string& hex, float& r, float& g, float& b) {
  std::string cleanHex = hex;
  if (cleanHex[0] == '#') {
    cleanHex = cleanHex.substr(1);
  }

  // Parse hex values
  unsigned int rgb;
  std::sscanf(cleanHex.c_str(), "%x", &rgb);

  r = ((rgb >> 16) & 0xFF) / 255.0f;
  g = ((rgb >> 8) & 0xFF) / 255.0f;
  b = (rgb & 0xFF) / 255.0f;
}
} // namespace

namespace shiki {

UIColor* ThemeColor::toUIColor() const {
  return [UIColor colorWithRed:red green:green blue:blue alpha:alpha];
}

} // namespace shiki
