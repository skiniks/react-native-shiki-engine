import type { ThemedToken } from '@shikijs/core'
import React, { useEffect, useState } from 'react'
import { Text, View } from 'react-native'
import { SafeAreaProvider, SafeAreaView } from 'react-native-safe-area-context'
import { isNativeEngineAvailable } from 'react-native-shiki-engine'
import { TokenDisplay } from './components/TokenDisplay'
import { HighlighterProvider } from './contexts/highlighter'
import { useHighlighter } from './hooks/useHighlighter'
import { rustExample } from './snippets/rust-example'
import { styles } from './styles'

function ShikiDemo() {
  const [engineStatus, setEngineStatus] = useState('Initializing...')
  const [tokens, setTokens] = useState<ThemedToken[][]>([])
  const [error, setError] = useState('')
  const highlighter = useHighlighter()

  useEffect(() => {
    const initializeApp = async () => {
      try {
        const available = isNativeEngineAvailable()
        setEngineStatus(available ? 'Available' : 'Not Available')

        if (!available)
          throw new Error('Native engine not available.')

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
          <Text style={styles.statusLabel}>Engine Status:</Text>
          <Text style={styles.statusValue}>{engineStatus}</Text>
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

function App() {
  return (
    <SafeAreaProvider>
      <HighlighterProvider>
        <ShikiDemo />
      </HighlighterProvider>
    </SafeAreaProvider>
  )
}

export default App
