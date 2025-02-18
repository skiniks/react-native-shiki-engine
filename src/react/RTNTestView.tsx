import type { ViewProps } from 'react-native'
import React from 'react'
import RTNTestViewNative from '../specs/RTNTestViewNativeComponent'

export interface RTNTestViewProps extends ViewProps {
  text?: string
}

export class RTNTestView extends React.Component<RTNTestViewProps> {
  render() {
    const { text, ...rest } = this.props
    return (
      <RTNTestViewNative
        text={text}
        {...rest}
      />
    )
  }
}
