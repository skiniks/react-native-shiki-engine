import rust from '@shikijs/langs/rust'
import dracula from '@shikijs/themes/dracula'
import React, { useCallback, useEffect, useRef, useState } from 'react'
import { ActivityIndicator, ScrollView, StyleSheet, Switch, Text, TouchableOpacity, View } from 'react-native'
import { ShikiHighlighterView, useShikiHighlighter, useViewConfig } from 'react-native-shiki'
import { rustSnippet } from './snippets/rust'

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  title: {
    fontSize: 20,
    fontWeight: 'bold',
    marginVertical: 16,
    marginHorizontal: 16,
  },
  codePreview: {
    margin: 16,
    backgroundColor: '#282a36',
    borderRadius: 8,
    overflow: 'hidden',
    minHeight: 200,
  },
  codeContainer: {
    minHeight: 200,
  },
  controlsContainer: {
    margin: 16,
    backgroundColor: 'white',
    borderRadius: 8,
    padding: 16,
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: 'bold',
    marginTop: 16,
    marginBottom: 8,
  },
  controlRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginVertical: 8,
  },
  controlLabel: {
    flex: 1,
    fontSize: 14,
  },
  controlValue: {
    flex: 1,
    flexDirection: 'row',
    justifyContent: 'flex-end',
    alignItems: 'center',
  },
  button: {
    width: 30,
    height: 30,
    borderRadius: 15,
    backgroundColor: '#e0e0e0',
    alignItems: 'center',
    justifyContent: 'center',
    marginHorizontal: 4,
  },
  optionButton: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 4,
    backgroundColor: '#e0e0e0',
    marginHorizontal: 4,
  },
  selectedOption: {
    backgroundColor: '#a0a0a0',
  },
  insetValue: {
    width: 30,
    textAlign: 'center',
  },
  loadingContainer: {
    minHeight: 200,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 20,
  },
  loadingText: {
    marginTop: 16,
    fontSize: 14,
    fontWeight: 'bold',
    color: '#f8f8f2',
  },
  errorContainer: {
    minHeight: 200,
    justifyContent: 'center',
    alignItems: 'center',
    padding: 20,
  },
  errorText: {
    marginTop: 16,
    fontSize: 14,
    fontWeight: 'bold',
    color: '#ff5555',
  },
  statusText: {
    marginTop: 8,
    fontSize: 12,
    color: '#6272a4',
  },
})

const fontFamilies = ['Menlo', 'Courier', 'Monaco', 'System']
const fontWeights = ['regular', 'bold', 'light', 'medium']
const fontStyles = ['normal', 'italic']

export function AdvancedView() {
  const [fontSize, setFontSize] = useState(14)
  const [fontFamily, setFontFamily] = useState('Menlo')
  const [fontWeight, setFontWeight] = useState('regular')
  const [fontStyle, setFontStyle] = useState('normal')
  const [showLineNumbers, setShowLineNumbers] = useState(false)
  const [scrollEnabled, setScrollEnabled] = useState(true)
  const [selectable, setSelectable] = useState(true)
  const [contentInset, setContentInset] = useState({
    top: 8,
    right: 8,
    bottom: 8,
    left: 8,
  })
  const [isLoading, setIsLoading] = useState(true)
  const [renderError, setRenderError] = useState<string | null>(null)
  const mountedRef = useRef(true)
  const hasRenderedRef = useRef(false)
  const renderAttemptsRef = useRef(0)

  const { tokens, isReady, error, status } = useShikiHighlighter({
    code: rustSnippet,
    lang: 'rust',
    theme: 'dracula',
    langData: rust,
    themeData: dracula,
  })

  useEffect(() => {
    if (isReady && tokens && tokens.length > 0) {
      if (mountedRef.current) {
        setIsLoading(false)
      }
    }
  }, [isReady, tokens])

  const viewConfig = useViewConfig({
    fontSize,
    fontFamily,
    fontWeight,
    fontStyle,
    showLineNumbers,
    scrollEnabled,
    selectable,
    contentInset,
  })

  const updateInset = (key: keyof typeof contentInset, delta: number) => {
    setContentInset(prev => ({
      ...prev,
      [key]: Math.max(0, prev[key] + delta),
    }))
  }

  const renderShikiHighlighter = useCallback(() => {
    if (isLoading) {
      return (
        <View style={styles.loadingContainer}>
          <ActivityIndicator size="large" color="#6272a4" />
          <Text style={styles.loadingText}>Loading syntax highlighter...</Text>
          <Text style={styles.statusText}>{status}</Text>
        </View>
      )
    }

    if (error) {
      return (
        <View style={styles.errorContainer}>
          <Text style={styles.errorText}>{error}</Text>
        </View>
      )
    }

    if (renderError) {
      return (
        <View style={styles.errorContainer}>
          <Text style={styles.errorText}>{renderError}</Text>
        </View>
      )
    }

    if (!isReady || !tokens || tokens.length === 0) {
      return (
        <View style={styles.errorContainer}>
          <Text style={styles.errorText}>No tokens available</Text>
          <Text style={styles.statusText}>{status}</Text>
        </View>
      )
    }

    try {
      renderAttemptsRef.current += 1

      // Additional validation of tokens
      const validTokens = tokens.filter(token =>
        token
        && typeof token.start === 'number'
        && typeof token.length === 'number'
        && token.start >= 0
        && token.length > 0
        && token.start + token.length <= rustSnippet.length,
      )

      if (validTokens.length === 0) {
        return (
          <View style={styles.errorContainer}>
            <Text style={styles.errorText}>No valid tokens available</Text>
          </View>
        )
      }

      // If we've successfully rendered before, don't try again
      if (hasRenderedRef.current) {
        return (
          <ShikiHighlighterView
            style={styles.codeContainer}
            tokens={validTokens}
            text={rustSnippet}
            {...viewConfig}
          />
        )
      }

      const result = (
        <ShikiHighlighterView
          style={styles.codeContainer}
          tokens={validTokens}
          text={rustSnippet}
          {...viewConfig}
        />
      )

      // Mark as successfully rendered
      hasRenderedRef.current = true
      renderAttemptsRef.current = 0
      return result
    }
    catch (err) {
      console.error('Error rendering ShikiHighlighterView:', err)
      const errorMessage = err instanceof Error ? err.message : String(err)

      // Update state only if component is still mounted
      if (mountedRef.current) {
        setRenderError(`Error rendering code: ${errorMessage}`)
      }

      return (
        <View style={styles.errorContainer}>
          <Text style={styles.errorText}>
            Error rendering code:
            {' '}
            {errorMessage}
          </Text>
        </View>
      )
    }
  }, [error, isLoading, renderError, status, tokens, viewConfig, isReady])

  return (
    <ScrollView style={styles.container}>
      <Text style={styles.title}>Advanced View Customization</Text>

      <View style={styles.codePreview}>
        {renderShikiHighlighter()}
      </View>

      <View style={styles.controlsContainer}>
        <Text style={styles.sectionTitle}>Font Controls</Text>

        <View style={styles.controlRow}>
          <Text style={styles.controlLabel}>
            Font Size:
            {fontSize}
          </Text>
          <View style={styles.controlValue}>
            <TouchableOpacity
              style={styles.button}
              onPress={() => setFontSize(Math.max(8, fontSize - 1))}
            >
              <Text>-</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={styles.button}
              onPress={() => setFontSize(Math.min(24, fontSize + 1))}
            >
              <Text>+</Text>
            </TouchableOpacity>
          </View>
        </View>

        <View style={styles.controlRow}>
          <Text style={styles.controlLabel}>Font Family:</Text>
          <View style={styles.controlValue}>
            <ScrollView horizontal showsHorizontalScrollIndicator={false}>
              {fontFamilies.map(family => (
                <TouchableOpacity
                  key={family}
                  style={[
                    styles.optionButton,
                    fontFamily === family && styles.selectedOption,
                  ]}
                  onPress={() => setFontFamily(family)}
                >
                  <Text>{family}</Text>
                </TouchableOpacity>
              ))}
            </ScrollView>
          </View>
        </View>

        <View style={styles.controlRow}>
          <Text style={styles.controlLabel}>Font Weight:</Text>
          <View style={styles.controlValue}>
            <ScrollView horizontal showsHorizontalScrollIndicator={false}>
              {fontWeights.map(weight => (
                <TouchableOpacity
                  key={weight}
                  style={[
                    styles.optionButton,
                    fontWeight === weight && styles.selectedOption,
                  ]}
                  onPress={() => setFontWeight(weight)}
                >
                  <Text>{weight}</Text>
                </TouchableOpacity>
              ))}
            </ScrollView>
          </View>
        </View>

        <View style={styles.controlRow}>
          <Text style={styles.controlLabel}>Font Style:</Text>
          <View style={styles.controlValue}>
            <ScrollView horizontal showsHorizontalScrollIndicator={false}>
              {fontStyles.map(style => (
                <TouchableOpacity
                  key={style}
                  style={[
                    styles.optionButton,
                    fontStyle === style && styles.selectedOption,
                  ]}
                  onPress={() => setFontStyle(style)}
                >
                  <Text>{style}</Text>
                </TouchableOpacity>
              ))}
            </ScrollView>
          </View>
        </View>

        <Text style={styles.sectionTitle}>View Options</Text>

        <View style={styles.controlRow}>
          <Text style={styles.controlLabel}>Show Line Numbers:</Text>
          <View style={styles.controlValue}>
            <Switch
              value={showLineNumbers}
              onValueChange={setShowLineNumbers}
            />
          </View>
        </View>

        <View style={styles.controlRow}>
          <Text style={styles.controlLabel}>Scroll Enabled:</Text>
          <View style={styles.controlValue}>
            <Switch
              value={scrollEnabled}
              onValueChange={setScrollEnabled}
            />
          </View>
        </View>

        <View style={styles.controlRow}>
          <Text style={styles.controlLabel}>Selectable:</Text>
          <View style={styles.controlValue}>
            <Switch
              value={selectable}
              onValueChange={setSelectable}
            />
          </View>
        </View>

        <Text style={styles.sectionTitle}>Content Inset</Text>

        <View style={styles.controlRow}>
          <Text style={styles.controlLabel}>Top:</Text>
          <View style={styles.controlValue}>
            <TouchableOpacity
              style={styles.button}
              onPress={() => updateInset('top', -4)}
            >
              <Text>-</Text>
            </TouchableOpacity>
            <Text style={styles.insetValue}>{contentInset.top}</Text>
            <TouchableOpacity
              style={styles.button}
              onPress={() => updateInset('top', 4)}
            >
              <Text>+</Text>
            </TouchableOpacity>
          </View>
        </View>

        <View style={styles.controlRow}>
          <Text style={styles.controlLabel}>Right:</Text>
          <View style={styles.controlValue}>
            <TouchableOpacity
              style={styles.button}
              onPress={() => updateInset('right', -4)}
            >
              <Text>-</Text>
            </TouchableOpacity>
            <Text style={styles.insetValue}>{contentInset.right}</Text>
            <TouchableOpacity
              style={styles.button}
              onPress={() => updateInset('right', 4)}
            >
              <Text>+</Text>
            </TouchableOpacity>
          </View>
        </View>

        <View style={styles.controlRow}>
          <Text style={styles.controlLabel}>Bottom:</Text>
          <View style={styles.controlValue}>
            <TouchableOpacity
              style={styles.button}
              onPress={() => updateInset('bottom', -4)}
            >
              <Text>-</Text>
            </TouchableOpacity>
            <Text style={styles.insetValue}>{contentInset.bottom}</Text>
            <TouchableOpacity
              style={styles.button}
              onPress={() => updateInset('bottom', 4)}
            >
              <Text>+</Text>
            </TouchableOpacity>
          </View>
        </View>

        <View style={styles.controlRow}>
          <Text style={styles.controlLabel}>Left:</Text>
          <View style={styles.controlValue}>
            <TouchableOpacity
              style={styles.button}
              onPress={() => updateInset('left', -4)}
            >
              <Text>-</Text>
            </TouchableOpacity>
            <Text style={styles.insetValue}>{contentInset.left}</Text>
            <TouchableOpacity
              style={styles.button}
              onPress={() => updateInset('left', 4)}
            >
              <Text>+</Text>
            </TouchableOpacity>
          </View>
        </View>
      </View>
    </ScrollView>
  )
}
