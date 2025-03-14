import type { Token } from '../specs'
import { useCallback, useEffect, useState } from 'react'
import { NativeShikiHighlighter } from '../specs'

// Static cache to track loaded resources
const loadedResources = {
  languages: new Set<string>(),
  themes: new Set<string>(),
  isLoading: false,
}

interface UseShikiHighlighterOptions {
  code?: string
  lang?: string
  theme?: string
  langData?: unknown
  themeData?: unknown
}

interface UseShikiHighlighterResult {
  tokens: Token[]
  isReady: boolean
  error: string | null
  status: string
  getLoadedLanguages: () => Promise<string[]>
  getLoadedThemes: () => Promise<string[]>
  setDefaultLanguage: (language: string) => Promise<void>
  setDefaultTheme: (theme: string) => Promise<void>
}

// Helper function to add delay between operations
const delay = (ms: number) => new Promise(resolve => setTimeout(resolve, ms))

export function useShikiHighlighter({
  code = '',
  lang = '',
  theme = '',
  langData,
  themeData,
}: UseShikiHighlighterOptions): UseShikiHighlighterResult {
  const [isReady, setIsReady] = useState(false)
  const [status, setStatus] = useState('Initializing...')
  const [tokens, setTokens] = useState<Token[]>([])
  const [error, setError] = useState<string | null>(null)

  const getLoadedLanguages = useCallback(async () => {
    try {
      return await NativeShikiHighlighter.getLoadedLanguages()
    }
    catch (error) {
      console.error('Error getting loaded languages:', error)
      return []
    }
  }, [])

  const getLoadedThemes = useCallback(async () => {
    try {
      return await NativeShikiHighlighter.getLoadedThemes()
    }
    catch (error) {
      console.error('Error getting loaded themes:', error)
      return []
    }
  }, [])

  const setDefaultLanguage = useCallback(async (language: string) => {
    try {
      await NativeShikiHighlighter.setDefaultLanguage(language)
    }
    catch (error) {
      console.error('Error setting default language:', error)
      throw error
    }
  }, [])

  const setDefaultTheme = useCallback(async (theme: string) => {
    try {
      await NativeShikiHighlighter.setDefaultTheme(theme)
    }
    catch (error) {
      console.error('Error setting default theme:', error)
      throw error
    }
  }, [])

  useEffect(() => {
    let isMounted = true
    let loadedLang = false
    let loadedTheme = false

    const initializeHighlighter = async () => {
      try {
        // Skip tokenization if code, lang, or theme is not provided
        if (!code || !lang || !theme) {
          return
        }

        // Skip if language or theme data is not provided
        if (!langData || !themeData) {
          return
        }

        // Check if another loading operation is in progress
        if (loadedResources.isLoading) {
          setStatus('Waiting for previous operation to complete...')
          await delay(500) // Wait before proceeding

          if (!isMounted)
            return
        }

        loadedResources.isLoading = true
        const formattedLangData = Array.isArray(langData) ? langData[0] : langData

        // Load language with error handling - only if not already loaded
        if (!loadedResources.languages.has(lang)) {
          try {
            setStatus(`Loading language ${lang}...`)
            loadedLang = await NativeShikiHighlighter.loadLanguage(
              lang,
              JSON.stringify(formattedLangData),
            )

            if (loadedLang) {
              loadedResources.languages.add(lang)
            }

            // Add a small delay after loading language
            await delay(100)
            if (!isMounted)
              return
          }
          catch (langError) {
            console.error('Error loading language:', langError)
            throw new Error(`Failed to load language: ${langError instanceof Error ? langError.message : String(langError)}`)
          }
        }
        else {
          loadedLang = true
        }

        // Load theme with error handling - only if not already loaded
        if (!loadedResources.themes.has(theme)) {
          try {
            // Ensure themeData is properly formatted
            const themeJson = typeof themeData === 'string'
              ? themeData
              : JSON.stringify(themeData)

            setStatus(`Loading theme ${theme}...`)
            loadedTheme = await NativeShikiHighlighter.loadTheme(
              theme,
              themeJson,
            )

            if (loadedTheme) {
              loadedResources.themes.add(theme)
            }

            // Add a small delay after loading theme
            await delay(100)
            if (!isMounted)
              return
          }
          catch (themeError) {
            console.error('Error loading theme:', themeError)
            throw new Error(`Failed to load theme: ${themeError instanceof Error ? themeError.message : String(themeError)}`)
          }
        }
        else {
          loadedTheme = true
        }

        if (!loadedLang || !loadedTheme) {
          throw new Error('Failed to load language or theme')
        }

        if (!isMounted)
          return // Check if component is still mounted

        setStatus('Tokenizing code...')

        // Get tokens with error handling
        let tokenized: Token[] = []
        try {
          tokenized = await NativeShikiHighlighter.codeToTokens(
            code,
            lang,
            theme,
          )
        }
        catch (tokenError) {
          console.error('Error getting tokens:', tokenError)
          throw new Error(`Failed to get tokens: ${tokenError instanceof Error ? tokenError.message : String(tokenError)}`)
        }
        finally {
          // Mark loading as complete
          loadedResources.isLoading = false
        }

        if (!isMounted)
          return // Check if component is still mounted

        // Validate tokens
        if (!Array.isArray(tokenized)) {
          console.warn('Tokenized result is not an array, using empty array instead')
          tokenized = []
        }

        // Filter out invalid tokens (those with length <= 0)
        const validTokens = tokenized.filter(token =>
          token
          && typeof token.start === 'number'
          && typeof token.length === 'number'
          && token.start >= 0
          && token.length > 0
          && token.start + token.length <= code.length,
        )

        if (validTokens.length !== tokenized.length) {
          console.warn(`Filtered out ${tokenized.length - validTokens.length} invalid tokens`)
        }

        if (!isMounted)
          return // Check if component is still mounted

        setTokens(validTokens)
        setIsReady(true)
        setError(null)
        setStatus('Available')
      }
      catch (err: unknown) {
        // Make sure to reset loading state in case of error
        loadedResources.isLoading = false

        if (!isMounted)
          return // Check if component is still mounted

        const errorMessage = err instanceof Error ? err.message : 'An unknown error occurred'
        setError(errorMessage)
        console.error('Tokenization error:', err)

        // Reset state
        setTokens([])
        setIsReady(false)
        setStatus('Error')
      }
    }

    initializeHighlighter()

    // Cleanup function to prevent memory leaks and ensure proper resource disposal
    return () => {
      isMounted = false

      // We don't clean up loaded languages and themes on unmount anymore
      // since we're caching them statically. This prevents multiple load/unload cycles
      // which might be causing the crashes.

      // However, we do want to make sure any in-progress loading is marked as complete
      loadedResources.isLoading = false
    }
  }, [code, lang, theme, langData, themeData])

  return {
    tokens,
    isReady,
    error,
    status,
    getLoadedLanguages,
    getLoadedThemes,
    setDefaultLanguage,
    setDefaultTheme,
  }
}
