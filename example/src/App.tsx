import rust from '@shikijs/langs/dist/rust.mjs'
import dracula from '@shikijs/themes/dist/dracula.mjs'
import React from 'react'
import { SafeAreaView, Text, TouchableOpacity, View } from 'react-native'
import { ShikiHighlighterView, useClipboard, useShikiHighlighter } from 'react-native-shiki'
import { rustSnippet } from './snippets/rust'
import { styles } from './styles'

function App() {
  const { copyStatus, copyToClipboard } = useClipboard()
  const { tokens, isReady, error, status } = useShikiHighlighter({
    code: rustSnippet,
    lang: 'rust',
    theme: 'dracula',
    langData: rust,
    themeData: dracula,
  })

  const handleCopy = () => copyToClipboard(rustSnippet)

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>React Native Shiki</Text>
        <View style={styles.statusContainer}>
          <Text style={styles.statusLabel}>Highlighter Status:</Text>
          <Text style={styles.statusValue}>{status}</Text>
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
