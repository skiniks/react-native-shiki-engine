import type { HighlighterCore } from '@shikijs/core'
import type { HighlighterContextType } from './context'
import { createHighlighterCore } from '@shikijs/core'
import rust from '@shikijs/langs/rust'
import dracula from '@shikijs/themes/dracula'
import React from 'react'
import { createNativeEngine, isNativeEngineAvailable } from 'react-native-shiki-engine'

import { HighlighterContext } from './context'

let highlighterInstance: HighlighterCore | null = null
let initializationPromise: Promise<void> | null = null

export function HighlighterProvider({ children }: { children: React.ReactNode }) {
  const value = React.useMemo<HighlighterContextType>(
    () => ({
      initialize: async () => {
        if (!initializationPromise) {
          initializationPromise = (async () => {
            const available = isNativeEngineAvailable()
            if (!available)
              throw new Error('Native engine not available.')

            highlighterInstance = await createHighlighterCore({
              langs: [rust],
              themes: [dracula],
              engine: createNativeEngine(),
            })
          })()
        }

        await initializationPromise
      },

      tokenize: (code: string, options: { lang: string, theme: string }) => {
        if (!highlighterInstance) {
          throw new Error('Highlighter not initialized. Call initialize() first.')
        }
        return highlighterInstance.codeToTokensBase(code, options)
      },

      dispose: () => {
        if (highlighterInstance) {
          highlighterInstance.dispose()
          highlighterInstance = null
          initializationPromise = null
        }
      },
    }),
    [],
  )

  return <HighlighterContext.Provider value={value}>{children}</HighlighterContext.Provider>
}
