#pragma once
#include "../../text/HighlightedText.h"
#include <string>

@class UITextView;
@class UIColor;
@class NSString;

namespace shiki {

class HighlightedTextIOS final : public HighlightedText {
public:
  explicit HighlightedTextIOS(UITextView* textView);
  ~HighlightedTextIOS() override;

  void updateNativeView() override;
  void clearHighlighting() override;
  void measureRange(TextRange& range) override;

private:
  UITextView* textView_;
  NSMutableAttributedString* attributedString_;
  UIColor* colorFromHexString(NSString* hexString);
  void applyStyle(const ThemeStyle& style, NSRange range);
};

} // namespace shiki
