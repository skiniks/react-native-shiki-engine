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
  scopes: string[]
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
  /**
   * Convert code to tokens using the specified language and theme
   * If language or theme are not provided, the default ones will be used
   */
  codeToTokens: (code: string, language?: string, theme?: string) => Promise<Token[]>

  /**
   * Load a language grammar into the highlighter
   */
  loadLanguage: (language: string, grammarData: string) => Promise<boolean>

  /**
   * Load a theme into the highlighter
   */
  loadTheme: (theme: string, themeData: string) => Promise<boolean>

  /**
   * Set the default language to use when none is specified
   */
  setDefaultLanguage: (language: string) => Promise<boolean>

  /**
   * Set the default theme to use when none is specified
   */
  setDefaultTheme: (theme: string) => Promise<boolean>

  /**
   * Get a list of all loaded languages
   */
  getLoadedLanguages: () => Promise<string[]>

  /**
   * Get a list of all loaded themes
   */
  getLoadedThemes: () => Promise<string[]>

  /**
   * Enable or disable the token cache
   */
  enableCache: (enabled: boolean) => Promise<boolean>

  // Optional event emitter methods
  addListener: (eventName: string) => void
  removeListeners: (count: number) => void
}

export default TurboModuleRegistry.getEnforcing<Spec>('ShikiHighlighter')
