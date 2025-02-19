import type { Token } from 'react-native-shiki'
import rust from '@shikijs/langs/dist/rust.mjs'
import dracula from '@shikijs/themes/dist/dracula.mjs'
import React, { useEffect, useState } from 'react'
import { SafeAreaView, Text, TouchableOpacity, View } from 'react-native'
import { Clipboard, ShikiHighlighterView } from 'react-native-shiki'
import NativeShikiHighlighter from '../../src/specs/NativeShikiHighlighter'
import { rustSnippet } from './snippets/rust'
import { styles } from './styles'

function App() {
  const [isReady, setIsReady] = useState(false)
  const [highlighterStatus, setHighlighterStatus] = useState('Initializing...')
  const [tokens, setTokens] = useState<Token[]>([])
  const [error, setError] = useState('')
  const [copyStatus, setCopyStatus] = useState('Copy')

  const handleCopy = async () => {
    try {
      await Clipboard.setString(rustSnippet)
      setCopyStatus('Copied!')
      setTimeout(() => setCopyStatus('Copy'), 2000)
    }
    catch (err) {
      console.error('Failed to copy:', err)
      setCopyStatus('Failed to copy')
      setTimeout(() => setCopyStatus('Copy'), 2000)
    }
  }

  useEffect(() => {
    const initializeHighlighter = async () => {
      try {
        // Load language and theme directly through the Turbo Module
        const langData = Array.isArray(rust) ? rust[0] : rust
        const langLoaded = await NativeShikiHighlighter.loadLanguage('rust', JSON.stringify(langData))
        const themeLoaded = await NativeShikiHighlighter.loadTheme('dracula', JSON.stringify(dracula))

        if (!langLoaded || !themeLoaded) {
          throw new Error('Failed to load language or theme')
        }

        setHighlighterStatus('Available')

        // Get tokens directly from the Turbo Module
        const tokenized = await NativeShikiHighlighter.codeToTokens(rustSnippet, 'rust', 'dracula')

        setTokens(tokenized)
        setIsReady(true)
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

    initializeHighlighter()
  }, [])

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>React Native Shiki</Text>
        <View style={styles.statusContainer}>
          <Text style={styles.statusLabel}>Highlighter Status:</Text>
          <Text style={styles.statusValue}>{highlighterStatus}</Text>
        </View>
      </View>

      <View style={styles.demoSection}>
        {error
          ? (
              <View style={styles.errorContainer}>
                <Text style={styles.errorText}>{error}</Text>
              </View>
            )
          : (
              <>
                {isReady && tokens.length > 0 && (
                  <ShikiHighlighterView
                    style={styles.codeContainer}
                    tokens={tokens}
                    text={rustSnippet}
                    fontSize={14}
                    scrollEnabled
                  />
                )}
                <TouchableOpacity
                  style={styles.copyButton}
                  onPress={handleCopy}
                >
                  <Text style={styles.copyButtonText}>
                    {copyStatus}
                  </Text>
                </TouchableOpacity>
              </>
            )}
      </View>
    </SafeAreaView>
  )
}

export default App
