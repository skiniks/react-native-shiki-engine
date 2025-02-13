import type { TurboModule } from 'react-native'
import { TurboModuleRegistry } from 'react-native'

export interface ThemeStyle {
  color: string
  backgroundColor?: string
  bold?: boolean
  italic?: boolean
  underline?: boolean
}

export interface Token {
  start: number
  length: number
  scope: string
  style: ThemeStyle
}

export interface ErrorCodes {
  InvalidGrammar: number
  InvalidTheme: number
  OutOfMemory: number
}

export interface Spec extends TurboModule {
  readonly getConstants: () => {
    ErrorCodes: ErrorCodes
  }

  // Required methods
  highlightCode: (code: string, language: string, theme: string) => Promise<Token[]>
  loadLanguage: (language: string, grammarData: string) => Promise<boolean>
  loadTheme: (theme: string, themeData: string) => Promise<boolean>

  // Optional event emitter methods
  addListener: (eventName: string) => void
  removeListeners: (count: number) => void
}

export default TurboModuleRegistry.getEnforcing<Spec>('RNShikiHighlighterModule')
