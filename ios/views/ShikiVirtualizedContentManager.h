#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#ifdef __cplusplus
#include <memory>
namespace shiki {
class VirtualizedContentManager;
}
#endif

NS_ASSUME_NONNULL_BEGIN

@interface ShikiVirtualizedContentManager : NSObject

#ifdef __cplusplus
@property(nonatomic, readonly) std::shared_ptr<shiki::VirtualizedContentManager>
    manager;
#endif

- (instancetype)init;
- (void)updateViewport:(CGFloat)firstVisibleLine
       lastVisibleLine:(CGFloat)lastVisibleLine
         contentHeight:(CGFloat)contentHeight
        viewportHeight:(CGFloat)viewportHeight
           isScrolling:(BOOL)isScrolling;

- (NSRange)getVisibleRangeForText:(NSString *)text;
- (BOOL)isRangeVisible:(NSRange)range inText:(NSString *)text;
- (NSUInteger)getLineCount:(NSString *)text;
- (CGFloat)getEstimatedHeight:(NSUInteger)lineCount
                   lineHeight:(CGFloat)lineHeight;
- (NSUInteger)getEstimatedLineAtPosition:(CGFloat)yOffset
                              lineHeight:(CGFloat)lineHeight;

@end

NS_ASSUME_NONNULL_END
