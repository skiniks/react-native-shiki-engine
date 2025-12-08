import React from 'react'
import { SafeAreaProvider } from 'react-native-safe-area-context'
import { ShikiExampleScreen } from '@/components'
import { HighlighterProvider } from '@/contexts'

export default function App() {
  return (
    <SafeAreaProvider>
      <HighlighterProvider>
        <ShikiExampleScreen />
      </HighlighterProvider>
    </SafeAreaProvider>
  )
}
