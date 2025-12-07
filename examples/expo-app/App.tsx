import type { ThemedToken } from '@shikijs/core'
import React, { useEffect, useState } from 'react'
import { Platform, Text, View } from 'react-native'
import { SafeAreaProvider, SafeAreaView } from 'react-native-safe-area-context'
import { isNativeEngineAvailable } from 'react-native-shiki-engine'
import { TokenDisplay } from '../shared/components/TokenDisplay'
import { useHighlighter } from '../shared/hooks/useHighlighter'
import { rustExample } from '../shared/snippets/rust-example'
import { styles } from '../shared/styles'
import { HighlighterProvider } from './src/contexts/highlighter'

function ShikiDemo() {
  const [engineStatus, setEngineStatus] = useState('Initializing...')
  const [tokens, setTokens] = useState<ThemedToken[][]>([])
  const [error, setError] = useState('')
  const highlighter = useHighlighter()

  const platformName = Platform.OS === 'web' ? 'Web' : Platform.OS === 'ios' ? 'iOS' : 'Android'

  useEffect(() => {
    const initializeApp = async () => {
      try {
        const available = isNativeEngineAvailable()
        const engineType = Platform.OS === 'web' ? 'WASM Engine' : available ? 'Native Engine' : 'Not Available'
        setEngineStatus(engineType)

        await highlighter.initialize()

        const tokenized = highlighter.tokenize(rustExample, {
          lang: 'rust',
          theme: 'dracula',
        })

        setTokens(tokenized)
      }
      catch (err: unknown) {
        if (err instanceof Error) {
          setError(err.message)
          console.error('Tokenization error:', err)
        }
        else {
          setError('An unknown error occurred.')
          console.error('Unknown error:', err)
        }
      }
    }

    initializeApp()

    return () => {
      highlighter.dispose()
    }
  }, [highlighter])

  return (
    <SafeAreaView style={styles.container} edges={['top', 'left', 'right']}>
      <View style={styles.header}>
        <Text style={styles.title}>React Native Shiki Engine</Text>
        <View style={styles.statusContainer}>
          <Text style={styles.statusLabel}>Engine:</Text>
          <Text style={styles.statusValue}>{engineStatus}</Text>
          <View style={styles.platformBadge}>
            <Text style={styles.platformText}>{platformName}</Text>
          </View>
        </View>
      </View>

      <View style={styles.demoSection}>
        <Text style={styles.languageTag}>rust</Text>
        {error
          ? (
              <View style={styles.errorContainer}>
                <Text style={styles.errorText}>{error}</Text>
              </View>
            )
          : (
              <TokenDisplay tokens={tokens} />
            )}
      </View>
    </SafeAreaView>
  )
}

export default function App() {
  return (
    <SafeAreaProvider>
      <HighlighterProvider>
        <ShikiDemo />
      </HighlighterProvider>
    </SafeAreaProvider>
  )
}
