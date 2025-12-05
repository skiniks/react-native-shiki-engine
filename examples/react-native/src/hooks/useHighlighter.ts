import type { HighlighterContextType } from '../contexts/highlighter/context'
import { useContext } from 'react'
import { HighlighterContext } from '../contexts/highlighter/context'

export function useHighlighter(): HighlighterContextType {
  const context = useContext(HighlighterContext)
  if (!context) {
    throw new Error('useHighlighter must be used within a HighlighterProvider')
  }
  return context
}
