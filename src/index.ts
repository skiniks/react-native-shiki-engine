import './utils/bridge'

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

export { default as NativeShikiBridge } from './specs/NativeShikiBridge'
export { default as NativeShikiClipboard } from './specs/NativeShikiClipboard'
export { default as NativeShikiHighlighter } from './specs/NativeShikiHighlighter'

export { Clipboard, initializeShikiBridge, isHighlighterAvailable } from './utils'
