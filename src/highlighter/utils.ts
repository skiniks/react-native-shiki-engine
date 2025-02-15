import { TurboModuleRegistry } from 'react-native'

export function isNativeHighlighterAvailable(): boolean {
  try {
    return TurboModuleRegistry.getEnforcing('RNShikiHighlighterModule') != null
  }
  catch {
    return false
  }
}
