import { afterEach, describe, expect, it, vi } from 'vite-plus/test'

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
    entryCount: 2,
    estimatedBytes: 4096,
    scannerCount: 1,
    maxEntries: 1000,
    maxBytes: 50 * 1024 * 1024,
  })),
}

vi.mock('react-native', () => ({
  AppState: {
    addEventListener: vi.fn(() => ({ remove: vi.fn() })),
  },
  Platform: { OS: 'ios' },
  TurboModuleRegistry: {
    get: vi.fn(() => mockModule),
    getEnforcing: vi.fn(() => mockModule),
  },
}))

describe('createNativeEngine options', () => {
  afterEach(() => {
    vi.clearAllMocks()
    vi.mocked(mockModule.createScanner).mockReturnValue(1)
  })

  it('rejects invalid cache options', async () => {
    const { createNativeEngine } = await import('./index')
    expect(() => createNativeEngine({ maxCacheEntries: -1 })).toThrow(TypeError)
    expect(() => createNativeEngine({ maxCacheEntries: 1.5 })).toThrow(TypeError)
    expect(() => createNativeEngine({ maxMemoryBytes: Number.NaN })).toThrow(TypeError)
  })

  it('configures cache and creates scanners', async () => {
    const { createNativeEngine, isNativeEngineAvailable } = await import('./index')
    expect(isNativeEngineAvailable()).toBe(true)

    const engine = createNativeEngine({ maxCacheEntries: 100, maxMemoryBytes: 1024 })
    expect(mockModule.configureCache).toHaveBeenCalledWith(100, 1024)

    const scanner = engine.createScanner(['foo', /bar/])
    expect(mockModule.createScanner).toHaveBeenCalledWith(['foo', 'bar'], 100)

    expect(() => scanner.findNextMatchSync('hello', 0, 0)).toThrow(/createString/)

    const onig = engine.createString('hello')
    expect(mockModule.internString).toHaveBeenCalledWith('hello')
    expect(scanner.findNextMatchSync(onig, 0, 0)).toBeNull()
    expect(mockModule.findNextMatchSync).toHaveBeenCalledWith(1, 10, 0)

    scanner.dispose?.()
    expect(mockModule.destroyScanner).toHaveBeenCalledWith(1)

    engine.dispose()
    expect(mockModule.releaseString).toHaveBeenCalledWith(10)
    expect(mockModule.clearPatternCache).toHaveBeenCalled()
  })

  it('exposes cache stats, trim, and clear helpers', async () => {
    const { createNativeEngine } = await import('./index')
    const engine = createNativeEngine()

    expect(engine.getCacheStats()).toEqual({
      entryCount: 2,
      estimatedBytes: 4096,
      scannerCount: 1,
      maxEntries: 1000,
      maxBytes: 50 * 1024 * 1024,
    })

    engine.trimMemory()
    expect(mockModule.trimMemory).toHaveBeenCalled()

    engine.clearPatternCache()
    expect(mockModule.clearPatternCache).toHaveBeenCalled()
  })

  it('dispose destroys tracked scanners even if destroy throws', async () => {
    mockModule.createScanner.mockReturnValueOnce(1).mockReturnValueOnce(2)
    mockModule.destroyScanner.mockImplementationOnce(() => {
      throw new Error('already gone')
    })

    const { createNativeEngine } = await import('./index')
    const engine = createNativeEngine()
    engine.createScanner(['a'])
    engine.createScanner(['b'])
    expect(() => {
      engine.dispose()
    }).not.toThrow()
    expect(mockModule.destroyScanner).toHaveBeenCalledWith(1)
    expect(mockModule.destroyScanner).toHaveBeenCalledWith(2)
  })

  it('reports unavailable when TurboModule is missing', async () => {
    const { TurboModuleRegistry } = await import('react-native')
    vi.mocked(TurboModuleRegistry.get).mockReturnValue(null)

    const { createNativeEngine, isNativeEngineAvailable } = await import('./index')
    expect(isNativeEngineAvailable()).toBe(false)
    expect(() => createNativeEngine()).toThrow(/not available/)

    vi.mocked(TurboModuleRegistry.get).mockReturnValue(mockModule)
  })

  it('works when FinalizationRegistry is missing (Hermes)', async () => {
    const original = globalThis.FinalizationRegistry
    // @ts-expect-error - simulate Hermes without FinalizationRegistry
    delete globalThis.FinalizationRegistry

    try {
      vi.resetModules()
      const { createNativeEngine } = await import('./index')
      const engine = createNativeEngine()
      const onig = engine.createString('hermes')
      expect(onig.content).toBe('hermes')
      engine.dispose()
      expect(mockModule.releaseString).toHaveBeenCalledWith(10)
    } finally {
      globalThis.FinalizationRegistry = original
      vi.resetModules()
    }
  })
})

describe('attachMemoryPressureHandler', () => {
  it('subscribes to AppState events and trims on pressure', async () => {
    const { AppState } = await import('react-native')
    const handlers = new Map<string, (...args: unknown[]) => void>()
    const addEventListener = vi.fn((type: string, handler: (...args: unknown[]) => void) => {
      handlers.set(type, handler)
      return { remove: vi.fn() }
    })
    AppState.addEventListener = addEventListener

    const { attachMemoryPressureHandler, createNativeEngine } = await import('../index')
    const engine = createNativeEngine()
    const detach = attachMemoryPressureHandler(engine)

    const memoryWarning = handlers.get('memoryWarning')
    expect(memoryWarning).toBeTypeOf('function')
    memoryWarning?.()
    expect(mockModule.trimMemory).toHaveBeenCalled()

    mockModule.trimMemory.mockClear()
    const changeHandler = handlers.get('change')
    expect(changeHandler).toBeTypeOf('function')
    changeHandler?.('background')
    expect(mockModule.trimMemory).toHaveBeenCalled()

    detach()
  })
})

describe('convertToOnigMatch', () => {
  it('maps packed native results', async () => {
    const { convertToOnigMatch } = await import('./utils')
    expect(convertToOnigMatch(null)).toBeNull()
    expect(convertToOnigMatch([2, 1, 4, 5, 8])).toEqual({
      index: 2,
      captureIndices: [
        { start: 1, end: 4, length: 3 },
        { start: 5, end: 8, length: 3 },
      ],
    })
  })
})
