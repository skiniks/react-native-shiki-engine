#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#ifdef __OBJC__
#  import <UIKit/UIKit.h>
#endif

namespace shiki {

class ThemeColor {
 private:
  std::string hexColor;
  float alpha;
  float red;
  float green;
  float blue;
  static std::unordered_map<std::string, std::shared_ptr<ThemeColor>> colorCache_;

  void parseHexColor() const;

 public:
  ThemeColor() : hexColor("#000000"), alpha(1.0f), red(0), green(0), blue(0) {}
  explicit ThemeColor(const std::string& hex, float a = 1.0f);

  std::string getHexColor() const {
    return hexColor;
  }
  float getAlpha() const {
    return alpha;
  }
  float getRed() const {
    return red;
  }
  float getGreen() const {
    return green;
  }
  float getBlue() const {
    return blue;
  }

  static ThemeColor fromHex(const std::string& hex);
  std::string toHex() const;

  bool isValid() const {
    return !hexColor.empty() && hexColor[0] == '#';
  }

#ifdef __OBJC__
  UIColor* toUIColor() const;
  static std::shared_ptr<ThemeColor> fromUIColor(UIColor* color);
#endif
};

}  // namespace shiki
