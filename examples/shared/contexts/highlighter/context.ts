import type { CodeToTokensBaseOptions, ThemedToken } from '@shikijs/core'
import { createContext } from 'react'

export type TokenizeOptions = CodeToTokensBaseOptions

export interface HighlighterContextType {
  initialize: () => Promise<void>
  tokenize: (code: string, options: TokenizeOptions) => ThemedToken[][]
  dispose: () => void
}

export const HighlighterContext = createContext<HighlighterContextType | null>(null)
