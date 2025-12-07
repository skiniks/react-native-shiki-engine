import type { HighlighterCore } from '@shikijs/core'
import type { HighlighterContextType } from '../../../../shared/contexts/highlighter/context'
import { createHighlighterCore } from '@shikijs/core'
import { createOnigurumaEngine } from '@shikijs/engine-oniguruma'
import rust from '@shikijs/langs/rust'
import dracula from '@shikijs/themes/dracula'
import React from 'react'
import { HighlighterContext } from '../../../../shared/contexts/highlighter/context'

let highlighterInstance: HighlighterCore | null = null
let initializationPromise: Promise<void> | null = null

export function HighlighterProvider({ children }: { children: React.ReactNode }) {
  const value = React.useMemo<HighlighterContextType>(
    () => ({
      initialize: async () => {
        if (!initializationPromise) {
          initializationPromise = (async () => {
            highlighterInstance = await createHighlighterCore({
              langs: [rust],
              themes: [dracula],
              engine: createOnigurumaEngine(import('@shikijs/engine-oniguruma/wasm-inlined')),
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

  return <HighlighterContext value={value}>{children}</HighlighterContext>
}
