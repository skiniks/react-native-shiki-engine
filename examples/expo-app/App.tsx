import React from 'react'
import { ShikiExampleScreen } from '@/components/ShikiExampleScreen'
import { HighlighterProvider } from '@/contexts/highlighter'

export default function App() {
  return (
    <HighlighterProvider>
      <ShikiExampleScreen />
    </HighlighterProvider>
  )
}
