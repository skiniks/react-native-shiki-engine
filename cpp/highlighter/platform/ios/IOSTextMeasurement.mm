#include "IOSTextMeasurement.h"
#import <UIKit/UIKit.h>

namespace shiki {

IOSTextMeasurement::IOSTextMeasurement(UIFont *font) : font_(font) {}

TextMetrics IOSTextMeasurement::measureRange(const std::string &text,
                                             size_t start, size_t length,
                                             const ThemeStyle &style) {
  TextMetrics metrics;

  if (!font_ || start + length > text.length()) {
    return metrics;
  }

  // Extract the substring to measure
  NSString *substring = [[NSString alloc] initWithBytes:text.data() + start
                                                 length:length
                                               encoding:NSUTF8StringEncoding];
  if (!substring) {
    return metrics;
  }

  // Create attributes dictionary with font and style
  NSMutableDictionary *attributes = [NSMutableDictionary dictionary];
  [attributes setObject:font_ forKey:NSFontAttributeName];

  // Apply style-specific attributes
  ThemeColor color = style.getThemeColor();
  if (color.isValid()) {
    [attributes setObject:color.toUIColor()
                   forKey:NSForegroundColorAttributeName];
  }

  // Create attributed string
  NSAttributedString *attrString =
      [[NSAttributedString alloc] initWithString:substring
                                      attributes:attributes];

  // Measure the text
  CGRect bounds =
      [attrString boundingRectWithSize:CGSizeMake(CGFLOAT_MAX, CGFLOAT_MAX)
                               options:NSStringDrawingUsesLineFragmentOrigin
                               context:nil];

  metrics.width = bounds.size.width;
  metrics.height = bounds.size.height;
  metrics.baseline = font_.descender;
  metrics.isValid = true;

  return metrics;
}

} // namespace shiki
