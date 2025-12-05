import type { TurboModule } from 'react-native'
import { TurboModuleRegistry } from 'react-native'

export interface Spec extends TurboModule {
  // eslint-disable-next-line ts/no-empty-object-type
  readonly getConstants: () => {}
  readonly createScanner: (patterns: readonly string[], maxCacheSize: number) => number
  readonly findNextMatchSync: (
    scannerId: number,
    text: string,
    startPosition: number,
  ) => {
    readonly index: number
    readonly captureIndices: ReadonlyArray<{
      readonly start: number
      readonly end: number
      readonly length: number
    }>
  } | null
  readonly destroyScanner: (scannerId: number) => void
}

export default TurboModuleRegistry.getEnforcing<Spec>('ShikiEngine')
