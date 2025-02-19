import type { Token } from '../specs'
import { useEffect, useState } from 'react'
import { NativeShikiHighlighter } from '../specs'

interface UseShikiHighlighterOptions {
  code: string
  lang: string
  theme: string
  langData: unknown
  themeData: unknown
}

interface UseShikiHighlighterResult {
  tokens: Token[]
  isReady: boolean
  error: string | null
  status: string
}

export function useShikiHighlighter({
  code,
  lang,
  theme,
  langData,
  themeData,
}: UseShikiHighlighterOptions): UseShikiHighlighterResult {
  const [isReady, setIsReady] = useState(false)
  const [status, setStatus] = useState('Initializing...')
  const [tokens, setTokens] = useState<Token[]>([])
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    const initializeHighlighter = async () => {
      try {
        const formattedLangData = Array.isArray(langData) ? langData[0] : langData
        const langLoaded = await NativeShikiHighlighter.loadLanguage(
          lang,
          JSON.stringify(formattedLangData),
        )
        const themeLoaded = await NativeShikiHighlighter.loadTheme(
          theme,
          JSON.stringify(themeData),
        )

        if (!langLoaded || !themeLoaded) {
          throw new Error('Failed to load language or theme')
        }

        setStatus('Available')

        const tokenized = await NativeShikiHighlighter.codeToTokens(
          code,
          lang,
          theme,
        )

        setTokens(tokenized)
        setIsReady(true)
        setError(null)
      }
      catch (err: unknown) {
        const errorMessage = err instanceof Error ? err.message : 'An unknown error occurred'
        setError(errorMessage)
        console.error('Tokenization error:', err)
      }
    }

    initializeHighlighter()
  }, [code, lang, theme, langData, themeData])

  return {
    tokens,
    isReady,
    error,
    status,
  }
}
