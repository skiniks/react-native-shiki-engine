import type { ViewProps } from 'react-native'
import type { Float, Int32 } from 'react-native/Libraries/Types/CodegenTypes'
import codegenNativeComponent from 'react-native/Libraries/Utilities/codegenNativeComponent'

export interface ThemeStyle {
  color: string
  backgroundColor?: string
  bold?: boolean
  italic?: boolean
  underline?: boolean
}

export interface Token {
  start: Int32
  length: Int32
  scope: string
  style: ThemeStyle
}

export interface ShikiHighlighterViewProps extends ViewProps {
  tokens: Token[]
  text: string
  fontSize?: Float
  fontFamily?: string
  scrollEnabled?: boolean
}

export default codegenNativeComponent<ShikiHighlighterViewProps>('ShikiHighlighterView')
