#pragma once
#include "../PlatformMeasurement.h"

#ifdef __OBJC__
@class UIFont;
#else
typedef void UIFont;
#endif

namespace shiki {

class IOSTextMeasurement : public PlatformMeasurement {
public:
  explicit IOSTextMeasurement(UIFont* font);
  ~IOSTextMeasurement() override = default;

  TextMetrics measureRange(const std::string& text, size_t start, size_t length,
                          const ThemeStyle& style) override;

private:
  UIFont* font_;
};

} // namespace shiki
