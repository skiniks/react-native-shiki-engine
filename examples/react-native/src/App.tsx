import type { ThemedToken } from '@shikijs/core'
import { TokenDisplay } from '@shared/components/TokenDisplay'
import { useHighlighter } from '@shared/hooks/useHighlighter'
import { rustExample } from '@shared/snippets/rust-example'
import { styles } from '@shared/styles'
import React, { useEffect, useState } from 'react'
import { ScrollView, Text, View } from 'react-native'
import { SafeAreaProvider, SafeAreaView } from 'react-native-safe-area-context'
import { isNativeEngineAvailable } from 'react-native-shiki-engine'
import { HighlighterProvider } from './contexts/highlighter'

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

      <ScrollView style={styles.demoSection} showsVerticalScrollIndicator={false}>
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
      </ScrollView>
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
