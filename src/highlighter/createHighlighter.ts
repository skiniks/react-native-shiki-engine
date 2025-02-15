import type { Token } from './NativeShikiHighlighter'
import { isHighlighterAvailable } from '../utils'
import NativeShikiHighlighter from './NativeShikiHighlighter'

const languageRegistry = new Map<string, string>()
const themeRegistry = new Map<string, string>()

export interface HighlighterOptions {
  themes?: string[]
  langs?: string[]
}

export interface TokenizeOptions {
  lang: string
  theme: string
}

export interface Highlighter {
  /**
   * Tokenize the code with the given language and theme
   */
  tokenize: (code: string, options: TokenizeOptions) => Promise<Token[]>

  /**
   * Load a language by name
   */
  loadLanguage: (language: string) => Promise<void>

  /**
   * Load a theme by name
   */
  loadTheme: (theme: string) => Promise<void>
}

/**
 * Register a language grammar
 */
export function registerLanguage(name: string, data: unknown): void {
  languageRegistry.set(name, JSON.stringify(Array.isArray(data) ? data[0] : data))
}

/**
 * Register a theme
 */
export function registerTheme(name: string, data: unknown): void {
  themeRegistry.set(name, JSON.stringify(data))
}

export async function createHighlighter(options: HighlighterOptions = {}): Promise<Highlighter> {
  if (!isHighlighterAvailable()) {
    throw new Error('Native highlighter is not available on this platform')
  }

  // Load initial themes if provided
  if (options.themes?.length) {
    for (const theme of options.themes) {
      const themeData = themeRegistry.get(theme)
      if (!themeData) {
        throw new Error(`Theme '${theme}' not found. Make sure to register it first using registerTheme.`)
      }
      await NativeShikiHighlighter.loadTheme(theme, themeData)
    }
  }

  // Load initial languages if provided
  if (options.langs?.length) {
    for (const lang of options.langs) {
      const grammarData = languageRegistry.get(lang)
      if (!grammarData) {
        throw new Error(`Language '${lang}' not found. Make sure to register it first using registerLanguage.`)
      }
      await NativeShikiHighlighter.loadLanguage(lang, grammarData)
    }
  }

  return {
    tokenize: async (code: string, options: TokenizeOptions) => {
      return NativeShikiHighlighter.highlightCode(code, options.lang, options.theme)
    },

    loadLanguage: async (language: string) => {
      const grammarData = languageRegistry.get(language)
      if (!grammarData) {
        throw new Error(`Language '${language}' not found. Make sure to register it first using registerLanguage.`)
      }
      await NativeShikiHighlighter.loadLanguage(language, grammarData)
    },

    loadTheme: async (theme: string) => {
      const themeData = themeRegistry.get(theme)
      if (!themeData) {
        throw new Error(`Theme '${theme}' not found. Make sure to register it first using registerTheme.`)
      }
      await NativeShikiHighlighter.loadTheme(theme, themeData)
    },
  }
}
