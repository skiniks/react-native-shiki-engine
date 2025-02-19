import type { ShikiHighlighterViewProps } from '../specs'
import React from 'react'
import { ShikiHighlighterViewNativeComponent } from '../specs'

export class ShikiHighlighterView extends React.Component<ShikiHighlighterViewProps> {
  render() {
    const { tokens, text, fontSize, fontFamily, scrollEnabled, ...rest } = this.props
    return (
      <ShikiHighlighterViewNativeComponent
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
