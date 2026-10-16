import type { PatternScanner, RegexEngine } from '@shikijs/types'
import type { FindOption, IOnigMatch, OnigString } from '@shikijs/vscode-textmate'
import type { Spec } from '../NativeShikiEngine'
import { TurboModuleRegistry } from 'react-native'
import { convertToOnigMatch } from './utils'

export interface NativeEngineOptions {
  maxCacheEntries?: number
  maxMemoryBytes?: number
}

export interface CacheStats {
  entryCount: number
  estimatedBytes: number
  scannerCount: number
  maxEntries: number
  maxBytes: number
}

export type NativeRegexEngine = RegexEngine & {
  getCacheStats: () => CacheStats
  clearPatternCache: () => void
  trimMemory: () => void
  dispose: () => void
}

function assertNonNegativeInt(value: number | undefined, name: string): number | undefined {
  if (value === undefined) return undefined
  if (!Number.isFinite(value) || value < 0 || !Number.isInteger(value))
    throw new TypeError(`${name} must be a non-negative integer`)
  return value
}

function getModule(): Spec | null {
  return TurboModuleRegistry.get<Spec>('ShikiEngine') ?? null
}

function createStringFinalizer(release: (stringId: number) => void): {
  register: (target: object, stringId: number) => void
  clear: () => void
} {
  const FinalizationRegistryCtor = (
    globalThis as typeof globalThis & {
      FinalizationRegistry?: typeof FinalizationRegistry
    }
  ).FinalizationRegistry

  if (typeof FinalizationRegistryCtor === 'function') {
    const registry = new FinalizationRegistryCtor(release)
    return {
      register(target, stringId) {
        registry.register(target, stringId, target)
      },
      clear() {
        // dispose() releases via stringIds; held values may still fire harmlessly
      },
    }
  }

  const WeakRefCtor = (
    globalThis as typeof globalThis & {
      WeakRef?: typeof WeakRef
    }
  ).WeakRef

  if (typeof WeakRefCtor === 'function') {
    const SWEEP_BATCH = 32
    const SWEEP_INTERVAL_MS = 1000
    const entries: { ref: WeakRef<object>; stringId: number }[] = []
    let cursor = 0
    let timer: ReturnType<typeof setInterval> | undefined

    const sweep = () => {
      const limit = Math.min(SWEEP_BATCH, entries.length)
      for (let n = 0; n < limit; n++) {
        if (entries.length === 0) {
          cursor = 0
          return
        }
        if (cursor >= entries.length) cursor = 0
        const entry = entries[cursor]
        if (!entry || entry.ref.deref() !== undefined) {
          cursor++
          continue
        }
        entries.splice(cursor, 1)
        release(entry.stringId)
      }
    }

    const ensureTimer = () => {
      if (timer !== undefined) return
      const handle = setInterval(sweep, SWEEP_INTERVAL_MS) as ReturnType<typeof setInterval> & {
        unref?: () => void
      }
      handle.unref?.()
      timer = handle
    }

    const stopTimer = () => {
      if (timer === undefined) return
      clearInterval(timer)
      timer = undefined
    }

    return {
      register(target, stringId) {
        entries.push({ ref: new WeakRefCtor(target), stringId })
        ensureTimer()
        if (entries.length >= SWEEP_BATCH) sweep()
      },
      clear() {
        stopTimer()
        entries.length = 0
        cursor = 0
      },
    }
  }

  return {
    register() {},
    clear() {},
  }
}

export function createNativeEngine(options: NativeEngineOptions = {}): NativeRegexEngine {
  const ShikiEngine = getModule()
  if (!ShikiEngine) throw new Error('Native engine not available')

  const maxCacheEntries = assertNonNegativeInt(options.maxCacheEntries, 'maxCacheEntries')
  const maxMemoryBytes = assertNonNegativeInt(options.maxMemoryBytes, 'maxMemoryBytes')

  ShikiEngine.configureCache(maxCacheEntries ?? 0, maxMemoryBytes ?? 0)

  const scannerIds = new Set<number>()
  const stringIds = new Set<number>()
  const stringIdByOnig = new WeakMap<object, number>()
  const stringFinalizers = createStringFinalizer((stringId: number) => {
    if (!stringIds.delete(stringId)) return
    try {
      ShikiEngine.releaseString(stringId)
    } catch {
      // module may already be torn down
    }
  })

  const releaseAllStrings = () => {
    for (const stringId of stringIds) {
      try {
        ShikiEngine.releaseString(stringId)
      } catch {
        // ignore
      }
    }
    stringIds.clear()
    stringFinalizers.clear()
  }

  const engine: NativeRegexEngine = {
    createScanner(patterns: (string | RegExp)[]): PatternScanner {
      if (
        !Array.isArray(patterns) ||
        patterns.some(p => typeof p !== 'string' && !(p instanceof RegExp))
      )
        throw new TypeError('Patterns must be an array of strings or RegExp objects')

      const stringPatterns = patterns.map(p => (typeof p === 'string' ? p : p.source))

      const scannerId = ShikiEngine.createScanner(stringPatterns, maxCacheEntries ?? 1000)
      if (typeof scannerId !== 'number') throw new TypeError('Failed to create native scanner')
      scannerIds.add(scannerId)

      return {
        findNextMatchSync(
          string: string | OnigString,
          startPosition: number,
          _options: FindOption,
        ): IOnigMatch | null {
          if (startPosition < 0) throw new RangeError('Start position must be >= 0')

          if (typeof string === 'string') {
            const stringId = ShikiEngine.internString(string)
            try {
              return convertToOnigMatch(
                ShikiEngine.findNextMatchSync(scannerId, stringId, startPosition),
              )
            } finally {
              ShikiEngine.releaseString(stringId)
            }
          }

          let stringId = stringIdByOnig.get(string)
          if (stringId === undefined) {
            if (typeof string.content !== 'string') {
              throw new TypeError('Expected a string or OnigString with string content')
            }
            stringId = ShikiEngine.internString(string.content)
            stringIds.add(stringId)
            stringIdByOnig.set(string, stringId)
            stringFinalizers.register(string, stringId)
          }

          return convertToOnigMatch(
            ShikiEngine.findNextMatchSync(scannerId, stringId, startPosition),
          )
        },

        dispose(): void {
          try {
            ShikiEngine.destroyScanner(scannerId)
          } finally {
            scannerIds.delete(scannerId)
          }
        },
      }
    },

    createString(s: string): OnigString {
      if (typeof s !== 'string') throw new TypeError('Input must be a string')
      const onig: OnigString = { content: s }
      const stringId = ShikiEngine.internString(s)
      stringIds.add(stringId)
      stringIdByOnig.set(onig, stringId)
      stringFinalizers.register(onig, stringId)
      return onig
    },

    getCacheStats(): CacheStats {
      return ShikiEngine.getCacheStats()
    },

    clearPatternCache(): void {
      ShikiEngine.clearPatternCache()
    },

    trimMemory(): void {
      ShikiEngine.trimMemory()
    },

    dispose(): void {
      for (const id of scannerIds) {
        try {
          ShikiEngine.destroyScanner(id)
        } catch {
          // ignore, scanner may already be gone
        }
      }
      scannerIds.clear()
      releaseAllStrings()
      ShikiEngine.clearPatternCache()
    },
  }

  return engine
}

export function isNativeEngineAvailable(): boolean {
  return getModule() != null
}
