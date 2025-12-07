# React Native Shiki Engine

Oniguruma regex engine implementation for React Native, providing high-performance syntax highlighting capabilities through [Shiki](https://github.com/shikijs/shiki). This module implements a JSI-based native bridge to Oniguruma, enabling efficient pattern matching and syntax highlighting in React Native applications.

> [!IMPORTANT]
> React Native Shiki Engine requires the New Architecture to be enabled (react-native 0.71+)

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
yarn add react-native-shiki-engine @shikijs/core
cd ios && pod install
```

You'll also need to install the languages and themes you want to use:

```sh
yarn add @shikijs/langs @shikijs/themes
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

### Basic Integration

```tsx
import { createHighlighterCore } from '@shikijs/core'
import javascript from '@shikijs/langs/javascript'
import nord from '@shikijs/themes/nord'
import { createNativeEngine, isNativeEngineAvailable } from 'react-native-shiki-engine'

if (!isNativeEngineAvailable()) {
  throw new Error('Native engine is not available')
}

const highlighter = await createHighlighterCore({
  themes: [nord],
  langs: [javascript],
  engine: createNativeEngine(),
})

const tokens = highlighter.codeToTokensBase(code, {
  lang: 'javascript',
  theme: 'nord',
})
```

> [!IMPORTANT]
> ### Performance Note: The Highlighter Instance
>
> Create and maintain a single Highlighter instance at the app level. Avoid instantiating new highlighters inside components or frequently called functions.
>
> #### Recommended:
> - Store the highlighter instance in a global singleton or context
> - Initialize it during app startup
> - Reuse the same instance across your entire React Native application
>
> #### Not Recommended:
> - Creating new instances inside component render methods
> - Initializing highlighters inside useEffect or event handlers
> - Multiple instances for the same language/theme combination
>
> See the [examples directory](https://github.com/skiniks/react-native-shiki-engine/tree/main/examples) for reference implementations using React Context to maintain a single highlighter instance.

### Advanced Configuration

The native engine supports configuration options to optimize performance:

```typescript
createNativeEngine({
  // Maximum number of patterns to cache
  maxCacheSize: 1000,
})
```

## Web Platform Support (Expo)

For Expo apps targeting web, this native engine is not compatible as it relies on React Native's TurboModules and JSI. To support web platforms, use platform-specific files with Metro's `.web.tsx` extension.

### Setup for Expo Web

1. Install the WASM engine:
```sh
npx expo install @shikijs/engine-oniguruma
```

2. Create platform-specific context files:

**Native platforms** (`contexts/highlighter/index.tsx`):
```tsx
import { createHighlighterCore } from '@shikijs/core'
import javascript from '@shikijs/langs/javascript'
import nord from '@shikijs/themes/nord'
import { createNativeEngine } from 'react-native-shiki-engine'

const highlighter = await createHighlighterCore({
  themes: [nord],
  langs: [javascript],
  engine: createNativeEngine(),
})
```

**Web platform** (`contexts/highlighter/index.web.tsx`):
```tsx
import { createHighlighterCore } from '@shikijs/core'
import { createOnigurumaEngine } from '@shikijs/engine-oniguruma'
import javascript from '@shikijs/langs/javascript'
import nord from '@shikijs/themes/nord'

const highlighter = await createHighlighterCore({
  themes: [nord],
  langs: [javascript],
  engine: createOnigurumaEngine(import('@shikijs/engine-oniguruma/wasm-inlined')),
})
```

Metro will automatically use the `.web.tsx` file when bundling for web, and the regular `.tsx` file for native platforms.

See the [expo-app example](https://github.com/skiniks/react-native-shiki-engine/tree/main/examples/expo-app) for a complete implementation.

## Technical Architecture

### Native Implementation

The module uses a three-layer architecture optimizing for both performance and developer experience:

1. **JavaScript Layer** (`src/`)

   - TypeScript interfaces and JS wrapper for type safety
   - Efficient pattern lifecycle management with automatic cleanup
   - Seamless integration with Shiki's API
   - Error boundaries with graceful degradation

2. **JSI Bridge** (`cpp/`)

   - Zero-copy JavaScript-to-native communication
   - Smart pointer-based memory management
   - Thread-safe pattern caching with LRU eviction
   - Host object lifetime tracking

3. **Oniguruma Core** (vendored)
   - High-performance native regex engine
   - Optimized pattern matching with capture groups
   - Full Unicode support with UTF-8/16 encoding
   - Non-backtracking algorithm for predictable performance

### Pattern Caching

The engine implements a sophisticated multi-level caching system:

- **L1 Cache**: Hot patterns in JSI host objects

  - Zero-copy access from JavaScript
  - Reference-counted lifetime management
  - Automatic cleanup on context destruction

- **L2 Cache**: Compiled patterns in native memory

  - LRU eviction with generational collection
  - Adaptive sizing based on memory pressure
  - Thread-safe concurrent access
  - Configurable eviction policies

- **Memory Management**
  - Proactive cleanup of unused patterns
  - Automatic defragmentation
  - Memory pressure handling
  - Configurable high/low watermarks

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

Contributions are welcome! Please read our [Contributing Guide](CONTRIBUTING.md) for details on:

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
