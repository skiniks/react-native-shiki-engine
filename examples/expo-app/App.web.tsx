import React from 'react'
import { SafeAreaProvider } from 'react-native-safe-area-context'
import { ShikiExampleScreen } from '@/components/ShikiExampleScreen'
import { HighlighterProvider } from '@/contexts/highlighter'

export default function App() {
  return (
    <SafeAreaProvider>
      <HighlighterProvider>
        <ShikiExampleScreen />
      </HighlighterProvider>
    </SafeAreaProvider>
  )
}
