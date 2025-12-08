import React from 'react'
import { ShikiExampleScreen } from '@/components'
import { HighlighterProvider } from '@/contexts'

export default function App() {
  return (
    <HighlighterProvider>
      <ShikiExampleScreen />
    </HighlighterProvider>
  )
}
