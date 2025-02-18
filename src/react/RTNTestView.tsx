import type { NativeProps } from '../specs/RTNTestViewNativeComponent'
import React from 'react'
import RTNTestViewNative from '../specs/RTNTestViewNativeComponent'

export class RTNTestView extends React.Component<NativeProps> {
  render() {
    const { text, textColor, fontStyle, ...rest } = this.props
    return (
      <RTNTestViewNative
        text={text}
        textColor={textColor}
        fontStyle={fontStyle}
        {...rest}
      />
    )
  }
}
