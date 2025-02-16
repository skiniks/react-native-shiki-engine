import type { HighlighterOptions, TokenizeOptions } from './createHighlighter'
import type { Token } from './NativeShikiHighlighter'
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
 */
export async function codeToTokens(
  code: string,
  options: TokenizeOptions,
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
