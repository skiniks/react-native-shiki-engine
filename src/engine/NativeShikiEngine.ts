import type { TurboModule } from 'react-native'
import { TurboModuleRegistry } from 'react-native'

export interface CaptureIndex {
  start: number
  end: number
  length: number
}

export interface Match {
  index: number
  captureIndices: CaptureIndex[]
}

export interface Spec extends TurboModule {
  createScanner: (patterns: string[], maxCacheSize: number) => Promise<number>
  findNextMatchSync: (
    scannerId: number,
    text: string,
    startPosition: number
  ) => Promise<Match | null>
  destroyScanner: (scannerId: number) => Promise<void>
}

export default TurboModuleRegistry.getEnforcing<Spec>('RNShikiEngine')
