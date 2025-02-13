import type { HighlighterContextType } from './context'
import rust from '@shikijs/langs/dist/rust.mjs'
import dracula from '@shikijs/themes/dist/dracula.mjs'
import React from 'react'
import { isNativeEngineAvailable, NativeShikiHighlighter } from 'react-native-shiki-engine'
import { HighlighterContext } from './context'

export function HighlighterProvider({ children }: { children: React.ReactNode }) {
  const value = React.useMemo<HighlighterContextType>(
    () => ({
      initialize: async () => {
        const available = isNativeEngineAvailable()

        if (!available)
          throw new Error('Native engine not available.')

        // Parse the grammar array and extract the first object
        const grammarArray = Array.isArray(rust) ? rust : [rust]
        if (grammarArray.length === 0) {
          throw new Error('No grammar definitions found')
        }
        const grammarData = JSON.stringify(grammarArray[0])

        try {
          await NativeShikiHighlighter.loadLanguage('rust', grammarData)
        }
        catch (error) {
          console.error('Failed to load rust grammar:', error)
          throw error
        }

        const themeData = JSON.stringify(dracula)
        try {
          await NativeShikiHighlighter.loadTheme('dracula', themeData)
        }
        catch (error) {
          console.error('Failed to load dracula theme:', error)
          throw error
        }
      },

      tokenize: async (code: string, options: { lang: string, theme: string }) => {
        const tokens = await NativeShikiHighlighter.highlightCode(code, options.lang, options.theme)
        return tokens
      },

      dispose: () => {
        // No explicit disposal needed for the native highlighter
      },
    }),
    [],
  )

  return <HighlighterContext.Provider value={value}>{children}</HighlighterContext.Provider>
}
