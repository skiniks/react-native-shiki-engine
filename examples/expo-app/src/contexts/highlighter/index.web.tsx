import type { HighlighterContextType, TokenizeOptions } from '@shared/contexts/highlighter/context'
import type { HighlighterCore } from '@shikijs/core'
import { HighlighterContext } from '@shared/contexts/highlighter/context'
import { createHighlighterCore } from '@shikijs/core'
import { createOnigurumaEngine } from '@shikijs/engine-oniguruma'
import rust from '@shikijs/langs/rust'
import typescript from '@shikijs/langs/typescript'
import dracula from '@shikijs/themes/dracula'
import githubDark from '@shikijs/themes/github-dark'
import React from 'react'

let highlighterInstance: HighlighterCore | null = null
let initializationPromise: Promise<void> | null = null

export function HighlighterProvider({ children }: { children: React.ReactNode }) {
  const value = React.useMemo<HighlighterContextType>(
    () => ({
      initialize: async () => {
        initializationPromise ??= (async () => {
          highlighterInstance = await createHighlighterCore({
            langs: [rust, typescript],
            themes: [dracula, githubDark],
            engine: createOnigurumaEngine(import('@shikijs/engine-oniguruma/wasm-inlined')),
          })
        })()

        await initializationPromise
      },

      tokenize: (code: string, options: TokenizeOptions) => {
        if (!highlighterInstance)
          throw new Error('Highlighter not initialized. Call initialize() first.')
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

  React.useEffect(() => {
    return () => {
      value.dispose()
    }
  }, [value])

  return <HighlighterContext value={value}>{children}</HighlighterContext>
}
