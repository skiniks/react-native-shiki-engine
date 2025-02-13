import type { Token } from 'react-native-shiki-engine'
import { createContext } from 'react'

export interface HighlighterContextType {
  initialize: () => Promise<void>
  tokenize: (code: string, options: { lang: string, theme: string }) => Promise<Token[]>
}

export const HighlighterContext = createContext<HighlighterContextType | null>(null)
