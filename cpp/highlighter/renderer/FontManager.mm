#include "FontManager.h"
#import <UIKit/UIKit.h>
#include <unordered_map>

namespace shiki {

FontManager& FontManager::getInstance() {
  static FontManager instance;
  return instance;
}

UIFont* FontManager::createFont(const FontConfig& config) {
  UIFont* font = [UIFont systemFontOfSize:config.fontSize];

  if (!config.fontFamily.empty()) {
    font = [UIFont fontWithName:@(config.fontFamily.c_str()) size:config.fontSize];
  }

  UIFontDescriptor* descriptor = font.fontDescriptor;
  UIFontDescriptorSymbolicTraits traits = descriptor.symbolicTraits;

  // Map font weight to UIFont weight
  static const std::unordered_map<std::string, std::string> weightMap = {
      {"thin", "UltraLight"}, {"extralight", "Thin"}, {"light", "Light"},
      {"normal", "Regular"},  {"medium", "Medium"},   {"semibold", "Semibold"},
      {"bold", "Bold"},       {"extrabold", "Heavy"}, {"black", "Black"}};

  // Apply font weight
  if (!config.fontWeight.empty()) {
    auto it = weightMap.find(config.fontWeight);
    if (it != weightMap.end()) {
      NSString* weightName = @(it->second.c_str());
      UIFontDescriptor* weightedDescriptor = [descriptor
          fontDescriptorByAddingAttributes:@{UIFontDescriptorFaceAttribute : weightName}];
      font = [UIFont fontWithDescriptor:weightedDescriptor size:config.fontSize];
    }
  }

  // Apply font style
  if (config.fontStyle == "italic") {
    traits |= UIFontDescriptorTraitItalic;
  }

  UIFontDescriptor* newDescriptor = [descriptor fontDescriptorWithSymbolicTraits:traits];
  if (newDescriptor) {
    font = [UIFont fontWithDescriptor:newDescriptor size:config.fontSize];
  }

  return font;
}

UIFont* FontManager::createFontWithStyle(UIFont* baseFont, bool bold, bool italic) {
  if (!baseFont)
    return nil;

  UIFontDescriptor* descriptor = baseFont.fontDescriptor;
  UIFontDescriptorSymbolicTraits traits = descriptor.symbolicTraits;

  if (bold) {
    traits |= UIFontDescriptorTraitBold;
  }
  if (italic) {
    traits |= UIFontDescriptorTraitItalic;
  }

  UIFontDescriptor* newDescriptor = [descriptor fontDescriptorWithSymbolicTraits:traits];
  UIFont* styledFont = [UIFont fontWithDescriptor:newDescriptor size:baseFont.pointSize];

  return styledFont ?: baseFont;
}

std::string FontManager::mapFontWeight(const std::string& weight) {
  // Map CSS-style font weights to system font weights
  static const std::unordered_map<std::string, std::string> weightMap = {
      {"100", "Thin"},    {"200", "UltraLight"}, {"300", "Light"},
      {"400", "Regular"}, {"500", "Medium"},     {"600", "Semibold"},
      {"700", "Bold"},    {"800", "Heavy"},      {"900", "Black"}};

  auto it = weightMap.find(weight);
  return it != weightMap.end() ? it->second : "Regular";
}

std::string FontManager::mapFontStyle(const std::string& style) {
  // Map CSS-style font styles to system font styles
  if (style == "italic")
    return "Italic";
  if (style == "oblique")
    return "Italic";
  return "Regular";
}

} // namespace shiki
