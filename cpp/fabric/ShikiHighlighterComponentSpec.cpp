#include "ShikiHighlighterComponentSpec.h"

#include <react/renderer/core/propsConversions.h>

namespace facebook {
namespace react {

  const char ShikiHighlighterComponentName[] = "ShikiHighlighter";

  ShikiHighlighterProps::ShikiHighlighterProps(
    const PropsParserContext& context,
    const ShikiHighlighterProps& sourceProps,
    const RawProps& rawProps
  )
    : ViewProps(context, sourceProps, rawProps),
      fontSize(convertRawProp(context, rawProps, "fontSize", sourceProps.fontSize, sourceProps.fontSize)),
      fontFamily(convertRawProp(context, rawProps, "fontFamily", sourceProps.fontFamily, sourceProps.fontFamily)),
      fontWeight(convertRawProp(context, rawProps, "fontWeight", sourceProps.fontWeight, sourceProps.fontWeight)),
      fontStyle(convertRawProp(context, rawProps, "fontStyle", sourceProps.fontStyle, sourceProps.fontStyle)),
      showLineNumbers(
        convertRawProp(context, rawProps, "showLineNumbers", sourceProps.showLineNumbers, sourceProps.showLineNumbers)
      ),
      text(convertRawProp(context, rawProps, "text", sourceProps.text, sourceProps.text)),
      language(convertRawProp(context, rawProps, "language", sourceProps.language, sourceProps.language)),
      theme(convertRawProp(context, rawProps, "theme", sourceProps.theme, sourceProps.theme)) {}

}  // namespace react
}  // namespace facebook
