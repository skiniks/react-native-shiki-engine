import type { ShikiHighlighterViewProps } from '../specs/ShikiHighlighterViewNativeComponent'
import React from 'react'
import ShikiHighlighterViewNative from '../specs/ShikiHighlighterViewNativeComponent'

export class ShikiHighlighterView extends React.Component<ShikiHighlighterViewProps> {
  render() {
    const { tokens, text, fontSize, fontFamily, scrollEnabled, ...rest } = this.props
    return (
      <ShikiHighlighterViewNative
        tokens={tokens}
        text={text}
        fontSize={fontSize}
        fontFamily={fontFamily}
        scrollEnabled={scrollEnabled}
        {...rest}
      />
    )
  }
}
