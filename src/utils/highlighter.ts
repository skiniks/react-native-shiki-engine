import { Platform } from 'react-native'

/**
 * Check if the native highlighter module is available on the current platform
 */
export function isHighlighterAvailable(): boolean {
  return Platform.OS === 'ios' || Platform.OS === 'android'
}
