#import "ShikiVirtualizedContentManager.h"
#include "../../cpp/highlighter/virtualization/VirtualizedContentManager.h"

@implementation ShikiVirtualizedContentManager {
  std::shared_ptr<shiki::VirtualizedContentManager> _cppManager;
  shiki::ViewportInfo _viewport;
}

- (instancetype)init {
  if (self = [super init]) {
    _cppManager = std::make_shared<shiki::VirtualizedContentManager>();
  }
  return self;
}

#ifdef __cplusplus
- (std::shared_ptr<shiki::VirtualizedContentManager>)manager {
  return _cppManager;
}
#endif

- (void)updateViewport:(CGFloat)firstVisibleLine
       lastVisibleLine:(CGFloat)lastVisibleLine
         contentHeight:(CGFloat)contentHeight
        viewportHeight:(CGFloat)viewportHeight
           isScrolling:(BOOL)isScrolling {
  _viewport = shiki::ViewportInfo{
      static_cast<size_t>(firstVisibleLine), static_cast<size_t>(lastVisibleLine),
      static_cast<float>(contentHeight), static_cast<float>(viewportHeight), isScrolling};
  // Store viewport info for later use in other methods
}

- (NSRange)getVisibleRangeForText:(NSString*)text {
  if (!text)
    return NSMakeRange(0, 0);

  std::string cppText = text.UTF8String;
  auto range = _cppManager->getVisibleRange(_viewport, cppText);
  return NSMakeRange(range.start, range.length);
}

- (BOOL)isRangeVisible:(NSRange)range inText:(NSString*)text {
  if (!text)
    return NO;
  std::string cppText = text.UTF8String;
  shiki::TextRange textRange{range.location, range.length};
  return _cppManager->isRangeVisible(textRange, _viewport, cppText);
}

- (NSUInteger)getLineCount:(NSString*)text {
  if (!text)
    return 0;
  std::string cppText = text.UTF8String;
  return _cppManager->getLineCount(cppText);
}

- (CGFloat)getEstimatedHeight:(NSUInteger)lineCount lineHeight:(CGFloat)lineHeight {
  return _cppManager->getEstimatedHeight(lineCount, static_cast<float>(lineHeight));
}

- (NSUInteger)getEstimatedLineAtPosition:(CGFloat)yOffset lineHeight:(CGFloat)lineHeight {
  return _cppManager->getEstimatedLineAtPosition(static_cast<float>(yOffset),
                                                 static_cast<float>(lineHeight));
}

@end
