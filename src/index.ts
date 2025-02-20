export {
  codeToTokens,
  createHighlighter,
  type Highlighter,
  type HighlighterOptions,
  loadLanguage,
  loadTheme,
  registerLanguage,
  registerTheme,
  type TokenizeOptions,
} from './highlighter'

export { useClipboard, useShikiHighlighter } from './hooks'

export { ShikiHighlighterView } from './react'

export type { ShikiHighlighterViewProps, Token } from './specs'

export { Clipboard, isHighlighterAvailable } from './utils'
