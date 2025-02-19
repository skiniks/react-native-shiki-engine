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
  loadLanguage,
  loadTheme,
} from './shorthands'
