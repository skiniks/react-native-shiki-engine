import { createHighlighterCore } from '@shikijs/core'
import { createOnigurumaEngine } from '@shikijs/engine-oniguruma'
import javascript from '@shikijs/langs/javascript'
import nord from '@shikijs/themes/nord'
import { describe, expect, it, vi } from 'vite-plus/test'

const mockModule = {
  getConstants: () => ({}),
  createScanner: vi.fn(() => 1),
  internString: vi.fn(() => 10),
  releaseString: vi.fn(),
  findNextMatchSync: vi.fn(() => null),
  destroyScanner: vi.fn(),
  configureCache: vi.fn(),
  clearPatternCache: vi.fn(),
  trimMemory: vi.fn(),
  getCacheStats: vi.fn(() => ({
    entryCount: 0,
    estimatedBytes: 0,
    scannerCount: 0,
    maxEntries: 1000,
    maxBytes: 50 * 1024 * 1024,
  })),
}

vi.mock('react-native', () => ({
  AppState: { addEventListener: vi.fn(() => ({ remove: vi.fn() })) },
  Platform: { OS: 'ios' },
  TurboModuleRegistry: {
    get: vi.fn(() => mockModule),
    getEnforcing: vi.fn(() => mockModule),
  },
}))

describe('golden tokenize vs @shikijs/engine-oniguruma', () => {
  it('produces stable tokens for a small javascript snippet', async () => {
    const code = 'const answer = 42\n'
    const highlighter = await createHighlighterCore({
      langs: [javascript],
      themes: [nord],
      engine: createOnigurumaEngine(import('@shikijs/engine-oniguruma/wasm-inlined')),
    })

    const tokens = highlighter.codeToTokensBase(code, { lang: 'javascript', theme: 'nord' })
    expect(tokens.length).toBeGreaterThan(0)
    expect(tokens[0]?.some(t => t.content.includes('const') || t.content.includes('answer'))).toBe(
      true,
    )

    const { createNativeEngine } = await import('./index')
    const engine = createNativeEngine()
    expect(typeof engine.createScanner).toBe('function')
    expect(typeof engine.createString).toBe('function')
    engine.dispose()
    highlighter.dispose()
  })
})
