# React Native Shiki Engine

Oniguruma regex engine implementation for React Native, providing high-performance syntax highlighting capabilities through [Shiki](https://github.com/shikijs/shiki). This module implements a JSI-based native bridge to Oniguruma, enabling efficient pattern matching and syntax highlighting in React Native applications.

> [!IMPORTANT]
> React Native Shiki Engine requires the New Architecture to be enabled (react-native 0.77+)

## Features

- **High-performance** native regex engine using Oniguruma, optimized for syntax highlighting
- **Fully synchronous** pattern matching with no async/await, no Promises, no Bridge
- Uses [**JSI**](https://reactnative.dev/docs/the-new-architecture/landing-page#fast-javascriptnative-interfacing) and [**C++ TurboModules**](https://github.com/reactwg/react-native-new-architecture/blob/main/docs/turbo-modules-xplat.md) for direct JavaScript-to-native communication
- **Smart pattern caching** system for optimal performance
- **Memory efficient** with automatic cleanup of unused patterns
- **Full compatibility** with Shiki's regex engine requirements
- Written in **modern C++** with robust memory management
- **Multiple pattern** support with efficient cache management

## Installation

### React Native

```sh
pnpm add react-native-shiki-engine @shikijs/core
cd ios && pod install
```

You'll also need to install the languages and themes you want to use:

```sh
pnpm add @shikijs/langs @shikijs/themes
```

### Expo

```sh
npx expo install react-native-shiki-engine @shikijs/core @shikijs/langs @shikijs/themes
npx expo prebuild
```

For web support in Expo, also install:

```sh
npx expo install @shikijs/engine-oniguruma
```

## Usage

Create a single highlighter at the app level with React Context. The examples use `rust` and `dracula`; the snippets below follow the same pattern as [`examples/react-native`](https://github.com/skiniks/react-native-shiki-engine/tree/main/examples/react-native).

### 1. Define the context

`src/contexts/highlighter/context.ts`:

```ts
import type { CodeToTokensBaseOptions, ThemedToken } from '@shikijs/core'
import { createContext } from 'react'

export type TokenizeOptions = CodeToTokensBaseOptions

export interface HighlighterContextType {
  initialize: () => Promise<void>
  tokenize: (code: string, options: TokenizeOptions) => ThemedToken[][]
  dispose: () => void
}

export const HighlighterContext = createContext<HighlighterContextType | null>(null)
```

### 2. Create the provider

`src/contexts/highlighter/index.tsx`:

```tsx
import type { HighlighterCore } from '@shikijs/core'
import { createHighlighterCore } from '@shikijs/core'
import rust from '@shikijs/langs/rust'
import typescript from '@shikijs/langs/typescript'
import dracula from '@shikijs/themes/dracula'
import React from 'react'
import { createNativeEngine, isNativeEngineAvailable } from 'react-native-shiki-engine'
import { HighlighterContext, type HighlighterContextType } from './context'

let highlighterInstance: HighlighterCore | null = null
let initializationPromise: Promise<void> | null = null

export function HighlighterProvider({ children }: { children: React.ReactNode }) {
  const value = React.useMemo<HighlighterContextType>(
    () => ({
      initialize: async () => {
        initializationPromise ??= (async () => {
          if (!isNativeEngineAvailable()) throw new Error('Native engine not available.')

          highlighterInstance = await createHighlighterCore({
            langs: [rust, typescript],
            themes: [dracula],
            engine: createNativeEngine(),
          })
        })()

        await initializationPromise
      },

      tokenize: (code, options) => {
        if (!highlighterInstance)
          throw new Error('Highlighter not initialized. Call initialize() first.')
        return highlighterInstance.codeToTokensBase(code, options)
      },

      dispose: () => {
        highlighterInstance?.dispose()
        highlighterInstance = null
        initializationPromise = null
      },
    }),
    [],
  )

  return <HighlighterContext value={value}>{children}</HighlighterContext>
}
```

### 3. Add a hook and use it in your app

`src/hooks/useHighlighter.ts`:

```ts
import { use } from 'react'
import { HighlighterContext } from '../contexts/highlighter/context'

export function useHighlighter() {
  const context = use(HighlighterContext)
  if (!context) throw new Error('useHighlighter must be used within a HighlighterProvider')
  return context
}
```

Wrap your app with the provider, initialize once, then tokenize:

```tsx
import React, { useEffect, useState } from 'react'
import { Text } from 'react-native'
import { HighlighterProvider } from './contexts/highlighter'
import { useHighlighter } from './hooks/useHighlighter'

const code = `fn main() {
  println!("Hello, world!");
}`

function HighlightedCode() {
  const highlighter = useHighlighter()
  const [output, setOutput] = useState('')

  useEffect(() => {
    const run = async () => {
      await highlighter.initialize()
      const tokens = highlighter.tokenize(code, { lang: 'rust', theme: 'dracula' })
      setOutput(
        tokens
          .flat()
          .map(token => token.content)
          .join(''),
      )
    }

    void run()
    return () => highlighter.dispose()
  }, [highlighter])

  return <Text>{output}</Text>
}

export default function App() {
  return (
    <HighlighterProvider>
      <HighlightedCode />
    </HighlighterProvider>
  )
}
```

> [!IMPORTANT]
>
> Keep a single highlighter instance for the lifetime of your app. Do not create new highlighters inside render, `useEffect`, or event handlers.

See the [examples directory](https://github.com/skiniks/react-native-shiki-engine/tree/main/examples) for full apps with token rendering and shared UI.

### Advanced Configuration

The native engine exposes optional cache budgets and diagnostics:

```typescript
const engine = createNativeEngine({
  // Soft cap on compiled patterns in the process-wide cache
  maxCacheEntries: 2000,
  // Soft cap on estimated cache memory (bytes). When omitted, the native
  // layer picks a budget from available device memory (capped at 50MiB).
  maxMemoryBytes: 32 * 1024 * 1024,
})

engine.getCacheStats() // { entryCount, estimatedBytes, scannerCount, maxEntries, maxBytes }
engine.trimMemory() // drop unused cached patterns under pressure
engine.clearPatternCache() // drop unused entries now
engine.dispose() // destroy scanners created by this engine + clear unused cache

// Optional: also trim from JS on iOS memoryWarning + app backgrounding
import { attachMemoryPressureHandler } from 'react-native-shiki-engine'
const detach = attachMemoryPressureHandler(engine)
// later: detach()
```

Pass options when creating the engine inside `HighlighterProvider`:

```tsx
engine: createNativeEngine({ maxCacheEntries: 2000 }),
```

### Tokenization options (Shiki)

These are Shiki options on `codeToTokensBase` / `codeToTokens`. They apply equally with the native engine—no native API changes.

#### Cap pathological lines

Long or adversarial lines can stall TextMate. Cap them the same way VS Code does:

```ts
highlighter.codeToTokensBase(code, {
  lang: 'typescript',
  theme: 'dracula',
  // Skip tokenization when a line is this long or longer (0 = no limit)
  tokenizeMaxLineLength: 2000,
  // Per-line time budget in ms (Shiki default: 500)
  tokenizeTimeLimit: 500,
})
```

When a line hits `tokenizeMaxLineLength`, Shiki emits a single unstyled token for that line instead of running the grammar.

#### Continue from a grammar state

For incremental highlighting (type annotations, streamed chunks, editor-style continuation), capture state and resume:

```ts
const state = highlighter.getLastGrammarState('let a: ', {
  lang: 'typescript',
  theme: 'dracula',
})

const tokens = highlighter.codeToTokensBase('Pick<MyObject, string>[]', {
  lang: 'typescript',
  theme: 'dracula',
  grammarState: state,
})
```

For a one-off context shift without keeping a state object, pass `grammarContextCode` instead (ignored when `grammarState` is set):

```ts
highlighter.codeToTokensBase('Pick<MyObject, string>[]', {
  lang: 'typescript',
  theme: 'dracula',
  grammarContextCode: 'let a: ',
})
```

See Shiki’s [Grammar State](https://shiki.style/guide/grammar-state) guide for details.

## Web Platform Support (Expo)

For Expo apps targeting web, the native engine is not available, it relies on TurboModules and JSI. Use Metro platform extensions (`.web.tsx`) to swap in the WASM engine on web.

The [expo-app example](https://github.com/skiniks/react-native-shiki-engine/tree/main/examples/expo-app) uses this layout:

```text
App.tsx                              # native entry
App.web.tsx                          # web entry (optional SafeAreaProvider wrapper)
src/
  contexts/highlighter/
    context.ts                       # shared context type (see examples/shared in the repo)
    index.tsx                        # native HighlighterProvider
    index.web.tsx                    # web HighlighterProvider
  components/
    ShikiExampleScreen.tsx           # native screen
    ShikiExampleScreen.web.tsx       # web screen (no native engine check)
```

### Setup for Expo Web

1. Install the WASM engine:

```sh
npx expo install @shikijs/engine-oniguruma
```

2. Enable the New Architecture for native builds in `app.json`:

```json
{
  "expo": {
    "newArchEnabled": true
  }
}
```

3. Add the shared context type at `src/contexts/highlighter/context.ts` (same `TokenizeOptions` shape as in [Usage](#usage) above), or reuse `examples/shared/contexts/highlighter/context.ts`.

4. Create platform-specific providers:

**Native** — `src/contexts/highlighter/index.tsx`:

```tsx
import type { HighlighterContextType } from './context'
import type { HighlighterCore } from '@shikijs/core'
import { HighlighterContext } from './context'
import { createHighlighterCore } from '@shikijs/core'
import rust from '@shikijs/langs/rust'
import dracula from '@shikijs/themes/dracula'
import React from 'react'
import { createNativeEngine, isNativeEngineAvailable } from 'react-native-shiki-engine'

let highlighterInstance: HighlighterCore | null = null
let initializationPromise: Promise<void> | null = null

export function HighlighterProvider({ children }: { children: React.ReactNode }) {
  const value = React.useMemo<HighlighterContextType>(
    () => ({
      initialize: async () => {
        initializationPromise ??= (async () => {
          if (!isNativeEngineAvailable()) throw new Error('Native engine not available.')

          highlighterInstance = await createHighlighterCore({
            langs: [rust],
            themes: [dracula],
            engine: createNativeEngine(),
          })
        })()

        await initializationPromise
      },

      tokenize: (code, options) => {
        if (!highlighterInstance)
          throw new Error('Highlighter not initialized. Call initialize() first.')
        return highlighterInstance.codeToTokensBase(code, options)
      },

      dispose: () => {
        highlighterInstance?.dispose()
        highlighterInstance = null
        initializationPromise = null
      },
    }),
    [],
  )

  return <HighlighterContext value={value}>{children}</HighlighterContext>
}
```

**Web** — `src/contexts/highlighter/index.web.tsx`:

```tsx
import type { HighlighterContextType } from './context'
import type { HighlighterCore } from '@shikijs/core'
import { HighlighterContext } from './context'
import { createHighlighterCore } from '@shikijs/core'
import { createOnigurumaEngine } from '@shikijs/engine-oniguruma'
import rust from '@shikijs/langs/rust'
import dracula from '@shikijs/themes/dracula'
import React from 'react'

let highlighterInstance: HighlighterCore | null = null
let initializationPromise: Promise<void> | null = null

export function HighlighterProvider({ children }: { children: React.ReactNode }) {
  const value = React.useMemo<HighlighterContextType>(
    () => ({
      initialize: async () => {
        initializationPromise ??= (async () => {
          highlighterInstance = await createHighlighterCore({
            langs: [rust],
            themes: [dracula],
            engine: createOnigurumaEngine(import('@shikijs/engine-oniguruma/wasm-inlined')),
          })
        })()

        await initializationPromise
      },

      tokenize: (code, options) => {
        if (!highlighterInstance)
          throw new Error('Highlighter not initialized. Call initialize() first.')
        return highlighterInstance.codeToTokensBase(code, options)
      },

      dispose: () => {
        highlighterInstance?.dispose()
        highlighterInstance = null
        initializationPromise = null
      },
    }),
    [],
  )

  return <HighlighterContext value={value}>{children}</HighlighterContext>
}
```

5. Wrap your app at the project root (`App.tsx`):

```tsx
import { ShikiExampleScreen } from '@/components/ShikiExampleScreen'
import { HighlighterProvider } from '@/contexts/highlighter/index'

export default function App() {
  return (
    <HighlighterProvider>
      <ShikiExampleScreen />
    </HighlighterProvider>
  )
}
```

Metro picks `index.web.tsx` when bundling for web and `index.tsx` on native. The same import path works on both platforms.

See the [expo-app example](https://github.com/skiniks/react-native-shiki-engine/tree/main/examples/expo-app) for the full app, including `App.web.tsx` and platform-specific screens.

## Technical Architecture

### Native Implementation

The module uses a three-layer architecture optimizing for both performance and developer experience:

1. **JavaScript Layer** (`src/`)

   - TypeScript interfaces and JS wrapper for type safety
   - Efficient pattern lifecycle management with automatic cleanup
   - Seamless integration with Shiki's API
   - Error boundaries with graceful degradation

2. **C++ TurboModule** (`cpp/`)

   - JSI-backed TurboModule (New Architecture)
   - Shared ownership of compiled patterns across scanners
   - Thread-safe process-wide pattern cache
   - UTF-16 index mapping with ASCII fast-path

3. **Oniguruma Core** (vendored)
   - Native Oniguruma regex engine
   - Multi-pattern search via leftmost `onig_search` over shared compiled patterns
   - UTF-8 matching with Unicode support
   - Configurable match/retry limits on init

### Pattern Caching

Compiled patterns live in a **single process-wide cache**, shared by every scanner:

- Patterns are keyed by source string and held with `shared_ptr`
- Scanners keep strong refs while alive; eviction only frees entries with no live scanners
- Size is an **estimate** (`4096 + patternLength * 32 + captures * 128`), derived from public Oniguruma capture counts, not a measured `regex_t` heap footprint
- Default memory budget is derived from available device memory (iOS `os_proc_available_memory` / Android `ActivityManager.getMemoryInfo`), floored at 8MiB and capped at 50MiB
- JS can override with `maxCacheEntries` / `maxMemoryBytes`, and call `trimMemory` / `clearPatternCache` / `getCacheStats`
- iOS automatically trims on `UIApplicationDidReceiveMemoryWarning`; Android trims on `ComponentCallbacks2.onTrimMemory`
- Apps can also call `attachMemoryPressureHandler(engine)` for AppState-driven trims (iOS `memoryWarning` + background on both platforms)

## Supported Platforms

|   Platform    | Architecture |             Description              | Status |
| :-----------: | :----------: | :----------------------------------: | :----: |
| iOS Simulator |    x86_64    |        Intel-based simulators        |   ✅   |
| iOS Simulator |    arm64     |       Apple Silicon simulators       |   ✅   |
|  iOS Device   |    arm64     |        All modern iOS devices        |   ✅   |
|    Android    |  arm64-v8a   | Modern Android devices (64-bit ARM)  |   ✅   |
|    Android    | armeabi-v7a  |  Older Android devices (32-bit ARM)  |   ✅   |
|    Android    |     x86      | Android emulators (32-bit Intel/AMD) |   ✅   |
|    Android    |    x86_64    | Android emulators (64-bit Intel/AMD) |   ✅   |

## Contributing

Contributions are welcome! Please read our [Contributing Guide](.github/CONTRIBUTING.md) for details on:

- Code of conduct
- Development workflow
- Pull request process
- Coding standards

## License

MIT License - see the [LICENSE](LICENSE) file for details

## Acknowledgments

- [Shiki](https://github.com/shikijs/shiki) - The beautiful yet powerful syntax highlighter we bring to React Native
- [Oniguruma](https://github.com/kkos/oniguruma) - The blazing-fast regex engine powering our native implementation
- [React Native](https://reactnative.dev/) - Making native module development possible with JSI and TurboModules

## Support

For questions, bug reports, or feature requests:

- [GitHub Issues](https://github.com/skiniks/react-native-shiki-engine/issues)
- [GitHub Discussions](https://github.com/skiniks/react-native-shiki-engine/discussions)
