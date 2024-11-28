import type { PatternScanner, RegexEngine } from '@shikijs/types'
import type { IOnigMatch, OnigString } from '@shikijs/vscode-textmate'
import { TurboModuleRegistry } from 'react-native'
import ShikiEngine from '../NativeShikiEngine'
import { convertToOnigMatch } from './utils'

export function createNativeEngine(options: { maxCacheSize?: number } = {}): RegexEngine {
  const { maxCacheSize = 1000 } = options

  if (!isNativeEngineAvailable()) {
    throw new Error('Native engine not available')
  }

  return {
    createScanner(patterns: string[]): PatternScanner {
      if (!Array.isArray(patterns) || patterns.some(p => typeof p !== 'string')) {
        throw new TypeError('Patterns must be an array of strings')
      }

      const scannerId = ShikiEngine.createScanner(patterns, maxCacheSize)
      if (typeof scannerId !== 'number') {
        throw new TypeError('Failed to create native scanner')
      }

      return {
        findNextMatchSync(string: string | OnigString, startPosition: number): IOnigMatch | null {
          if (startPosition < 0)
            throw new RangeError('Start position must be >= 0')

          const stringContent = typeof string === 'string' ? string : string.content
          if (typeof stringContent !== 'string')
            throw new TypeError('Invalid input string')

          try {
            const result = ShikiEngine.findNextMatchSync(scannerId, stringContent, startPosition)
            return convertToOnigMatch(result)
          }
          catch (err) {
            if (__DEV__)
              console.error('Error in findNextMatchSync:', err)
            throw err
          }
        },

        dispose(): void {
          try {
            ShikiEngine.destroyScanner(scannerId)
          }
          catch (err) {
            if (__DEV__)
              console.error('Error disposing scanner:', err)
          }
        },
      }
    },

    createString(s: string): OnigString {
      if (typeof s !== 'string')
        throw new TypeError('Input must be a string')
      return { content: s }
    },
  }
}

export function isNativeEngineAvailable(): boolean {
  try {
    return TurboModuleRegistry.getEnforcing('ShikiEngine') != null
  }
  catch {
    return false
  }
}
