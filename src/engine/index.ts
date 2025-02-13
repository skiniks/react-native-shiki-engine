import type { Match } from './NativeShikiEngine'
import { TurboModuleRegistry } from 'react-native'
import NativeShikiEngine from './NativeShikiEngine'

export class OnigScanner {
  private scannerId: number | null = null

  constructor(private patterns: string[], private maxCacheSize: number = 1000) {}

  async initialize(): Promise<void> {
    if (this.scannerId !== null)
      return

    this.scannerId = await NativeShikiEngine.createScanner(this.patterns, this.maxCacheSize)
  }

  async findNextMatch(text: string, startPosition: number): Promise<Match | null> {
    if (this.scannerId === null)
      throw new Error('Scanner not initialized')

    return await NativeShikiEngine.findNextMatchSync(this.scannerId, text, startPosition)
  }

  async dispose(): Promise<void> {
    if (this.scannerId !== null) {
      await NativeShikiEngine.destroyScanner(this.scannerId)
      this.scannerId = null
    }
  }
}

export function createNativeEngine(patterns: string[], options: { maxCacheSize?: number } = {}): OnigScanner {
  return new OnigScanner(patterns, options.maxCacheSize)
}

export function isNativeEngineAvailable(): boolean {
  try {
    return TurboModuleRegistry.getEnforcing('RNShikiEngine') != null
  }
  catch {
    return false
  }
}
