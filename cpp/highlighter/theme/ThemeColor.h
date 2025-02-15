#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#ifdef __OBJC__
#  import <UIKit/UIKit.h>
#endif

namespace shiki {

class ThemeColor {
 public:
  explicit ThemeColor(const std::string& hex = "", float a = 1.0f);

  static std::shared_ptr<ThemeColor> fromHex(const std::string& hex);
  std::string toHex() const;

  bool isValid() const {
    return !hexColor.empty();
  }

  float red{0};
  float green{0};
  float blue{0};
  float alpha{1.0f};
  std::string hexColor;
  uint64_t hash_{0};

#ifdef __OBJC__
  UIColor* toUIColor() const;
  static std::shared_ptr<ThemeColor> fromUIColor(UIColor* color);
#endif

  static void clearCache();

 private:
  void parseHexColor() const;
  static uint64_t computeHash(const std::string& hex);

  static std::unordered_map<uint64_t, std::shared_ptr<ThemeColor>> colorCache_;
};

}  // namespace shiki
