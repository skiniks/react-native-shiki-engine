import type { Token } from '../specs'
import type { HighlighterOptions, TokenizeOptions } from './createHighlighter'
import { createHighlighter } from './createHighlighter'

let cachedHighlighter: Awaited<ReturnType<typeof createHighlighter>> | null = null

async function getHighlighter(options: HighlighterOptions = {}) {
  if (!cachedHighlighter) {
    cachedHighlighter = await createHighlighter(options)
  }
  return cachedHighlighter
}

/**
 * Tokenize code with the specified language and theme.
 * This function will create and cache a highlighter instance internally.
 * If language or theme are not provided, the default ones will be used.
 */
export async function codeToTokens(
  code: string,
  options?: TokenizeOptions,
  highlighterOptions: HighlighterOptions = {},
): Promise<Token[]> {
  const highlighter = await getHighlighter(highlighterOptions)
  return highlighter.tokenize(code, options)
}

/**
 * Load a language grammar into the cached highlighter
 */
export async function loadLanguage(language: string): Promise<void> {
  const highlighter = await getHighlighter()
  await highlighter.loadLanguage(language)
}

/**
 * Load a theme into the cached highlighter
 */
export async function loadTheme(theme: string): Promise<void> {
  const highlighter = await getHighlighter()
  await highlighter.loadTheme(theme)
}

/**
 * Set the default language to use when none is specified
 */
export async function setDefaultLanguage(language: string): Promise<void> {
  const highlighter = await getHighlighter()
  await highlighter.setDefaultLanguage(language)
}

/**
 * Set the default theme to use when none is specified
 */
export async function setDefaultTheme(theme: string): Promise<void> {
  const highlighter = await getHighlighter()
  await highlighter.setDefaultTheme(theme)
}

/**
 * Get a list of all loaded languages
 */
export async function getLoadedLanguages(): Promise<string[]> {
  const highlighter = await getHighlighter()
  return highlighter.getLoadedLanguages()
}

/**
 * Get a list of all loaded themes
 */
export async function getLoadedThemes(): Promise<string[]> {
  const highlighter = await getHighlighter()
  return highlighter.getLoadedThemes()
}
