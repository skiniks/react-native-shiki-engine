import React from 'react'
import { ShikiExampleScreen } from '@/components/ShikiExampleScreen'
import { HighlighterProvider } from '@/contexts/highlighter/index'

export default function App() {
  return (
    <HighlighterProvider>
      <ShikiExampleScreen />
    </HighlighterProvider>
  )
}
