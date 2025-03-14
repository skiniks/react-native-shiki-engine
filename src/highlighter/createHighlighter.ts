import type { Token } from '../specs'
import { NativeShikiHighlighter } from '../specs'
import { isHighlighterAvailable } from '../utils'

const languageRegistry = new Map<string, string>()
const themeRegistry = new Map<string, string>()

export interface HighlighterOptions {
  themes?: string[]
  langs?: string[]
  /**
   * Whether to enable caching. Defaults to true.
   * Disabling cache can reduce memory usage at the cost of performance.
   */
  enableCache?: boolean
  /**
   * Default language to use when none is specified
   */
  defaultLanguage?: string
  /**
   * Default theme to use when none is specified
   */
  defaultTheme?: string
}

export interface TokenizeOptions {
  /**
   * Language to use for tokenization
   * If not provided, the default language will be used
   */
  lang?: string
  /**
   * Theme to use for tokenization
   * If not provided, the default theme will be used
   */
  theme?: string
}

export interface Highlighter {
  /**
   * Tokenize the code with the given language and theme
   * If language or theme are not provided, the default ones will be used
   */
  tokenize: (code: string, options?: TokenizeOptions) => Promise<Token[]>

  /**
   * Load a language by name
   */
  loadLanguage: (language: string) => Promise<void>

  /**
   * Load a theme by name
   */
  loadTheme: (theme: string) => Promise<void>

  /**
   * Set the default language to use when none is specified
   */
  setDefaultLanguage: (language: string) => Promise<void>

  /**
   * Set the default theme to use when none is specified
   */
  setDefaultTheme: (theme: string) => Promise<void>

  /**
   * Get a list of all loaded languages
   */
  getLoadedLanguages: () => Promise<string[]>

  /**
   * Get a list of all loaded themes
   */
  getLoadedThemes: () => Promise<string[]>
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

  // Configure cache
  await NativeShikiHighlighter.enableCache(options.enableCache !== false)

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

  if (options.defaultLanguage) {
    await NativeShikiHighlighter.setDefaultLanguage(options.defaultLanguage)
  }

  if (options.defaultTheme) {
    await NativeShikiHighlighter.setDefaultTheme(options.defaultTheme)
  }

  return {
    tokenize: async (code: string, options: TokenizeOptions = {}) => {
      return NativeShikiHighlighter.codeToTokens(code, options.lang, options.theme)
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

    setDefaultLanguage: async (language: string) => {
      await NativeShikiHighlighter.setDefaultLanguage(language)
    },

    setDefaultTheme: async (theme: string) => {
      await NativeShikiHighlighter.setDefaultTheme(theme)
    },

    getLoadedLanguages: async () => {
      return NativeShikiHighlighter.getLoadedLanguages()
    },

    getLoadedThemes: async () => {
      return NativeShikiHighlighter.getLoadedThemes()
    },
  }
}
