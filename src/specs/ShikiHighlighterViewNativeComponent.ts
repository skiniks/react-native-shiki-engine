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
  scopes: string[]
  style: ThemeStyle
}

export interface ContentInset {
  top?: Float
  right?: Float
  bottom?: Float
  left?: Float
}

export interface ShikiHighlighterViewProps extends ViewProps {
  tokens: Token[]
  text: string
  language?: string
  theme?: string
  fontSize?: Float
  fontFamily?: string
  fontWeight?: string
  fontStyle?: string
  showLineNumbers?: boolean
  selectable?: boolean
  contentInset?: ContentInset
  scrollEnabled?: boolean
}

export default codegenNativeComponent<ShikiHighlighterViewProps>('ShikiHighlighterView')
