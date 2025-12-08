import type { HighlighterContextType } from '../contexts'
import { use } from 'react'
import { HighlighterContext } from '../contexts'

export function useHighlighter(): HighlighterContextType {
  const context = use(HighlighterContext)
  if (!context) {
    throw new Error('useHighlighter must be used within a HighlighterProvider')
  }
  return context
}
