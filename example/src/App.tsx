import typescript from '@shikijs/langs/typescript'
import githubDark from '@shikijs/themes/github-dark'
import React, { useState } from 'react'
import { Text, TouchableOpacity, View } from 'react-native'
import { ShikiHighlighterView, useClipboard, useShikiHighlighter } from 'react-native-shiki'
import { AdvancedView } from './AdvancedView'
import { BackArrow } from './components/BackArrow'
import { typescriptSnippet } from './snippets/typescript'
import { styles } from './styles'

function App() {
  const [showAdvanced, setShowAdvanced] = useState(false)
  const { copyStatus, copyToClipboard } = useClipboard()
  const { tokens, isReady, error, status } = useShikiHighlighter({
    code: typescriptSnippet,
    lang: 'typescript',
    theme: 'github-dark',
    langData: typescript,
    themeData: githubDark,
  })

  const handleCopy = () => copyToClipboard(typescriptSnippet)

  if (showAdvanced) {
    return (
      <View style={styles.container}>
        <View style={styles.header}>
          <TouchableOpacity onPress={() => setShowAdvanced(false)}>
            <View style={{
              flexDirection: 'row',
              alignItems: 'center',
              paddingVertical: 4,
            }}
            >
              <BackArrow color="#88C0D0" size={16} />
              <Text style={[
                styles.backButton,
                {
                  marginLeft: 8,
                  marginBottom: 0,
                },
              ]}
              >
                Back to Basic Example
              </Text>
            </View>
          </TouchableOpacity>
          <Text style={styles.title}>Advanced View Customization</Text>
        </View>
        <AdvancedView />
      </View>
    )
  }

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>React Native Shiki</Text>
        <View style={styles.statusContainer}>
          <Text style={styles.statusLabel}>Highlighter Status:</Text>
          <Text style={styles.statusValue}>{status}</Text>
        </View>
      </View>

      <TouchableOpacity
        style={styles.advancedButton}
        onPress={() => setShowAdvanced(true)}
      >
        <Text style={styles.advancedButtonText}>
          Show Advanced Example
        </Text>
      </TouchableOpacity>

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
                    text={typescriptSnippet}
                    fontSize={14}
                    scrollEnabled
                    selectable
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
    </View>
  )
}

export default App
