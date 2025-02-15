#import "LineNumberView.h"

@implementation LineNumberView {
  NSMutableArray<NSString *> *_lineNumbers;
}

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    _lineNumbers = [NSMutableArray new];
    _padding = 8.0;
    self.backgroundColor = [UIColor clearColor];
  }
  return self;
}

- (void)updateLineNumbers {
  [_lineNumbers removeAllObjects];
  for (NSUInteger i = 1; i <= _numberOfLines; i++) {
    [_lineNumbers
        addObject:[NSString stringWithFormat:@"%lu", (unsigned long)i]];
  }
  [self setNeedsDisplay];
}

- (void)drawRect:(CGRect)rect {
  [super drawRect:rect];

  CGContextRef context = UIGraphicsGetCurrentContext();
  CGContextSetFillColorWithColor(context, self.backgroundColor.CGColor);
  CGContextFillRect(context, rect);

  if (!_font || !_textColor || _lineNumbers.count == 0)
    return;

  [_textColor set];
  CGFloat y = 0;

  for (NSString *number in _lineNumbers) {
    CGSize size = [number sizeWithAttributes:@{NSFontAttributeName : _font}];
    CGFloat x = self.bounds.size.width - size.width - _padding;
    [number drawAtPoint:CGPointMake(x, y)
         withAttributes:@{
           NSFontAttributeName : _font,
           NSForegroundColorAttributeName : _textColor
         }];
    y += _lineHeight;
  }
}

- (CGFloat)requiredWidth {
  if (_lineNumbers.count == 0)
    return 0;

  // Calculate width based on largest line number
  NSString *lastNumber =
      [NSString stringWithFormat:@"%lu", (unsigned long)_numberOfLines];
  CGSize size = [lastNumber sizeWithAttributes:@{NSFontAttributeName : _font}];
  return size.width + (_padding * 2);
}

@end
