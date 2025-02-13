#pragma once
#include "../../text/TextRange.h"

@class NSString;
@class UIFont;

namespace shiki {

class TextRangeIOS : public TextRange {
public:
  void measure(NSString* text, UIFont* font) override;
};

} // namespace shiki
