export type {
  Highlighter,
  HighlighterOptions,
  TokenizeOptions,
} from './createHighlighter'

export {
  createHighlighter,
  registerLanguage,
  registerTheme,
} from './createHighlighter'

export {
  codeToTokens,
  getLoadedLanguages,
  getLoadedThemes,
  loadLanguage,
  loadTheme,
  setDefaultLanguage,
  setDefaultTheme,
} from './shorthands'
