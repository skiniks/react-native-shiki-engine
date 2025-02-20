#include "highlighter/theme/ThemeColor.h"
#import <UIKit/UIKit.h>

namespace shiki {

UIColor *ThemeColor::toUIColor() const {
  return [UIColor colorWithRed:red green:green blue:blue alpha:alpha];
}

} // namespace shiki
