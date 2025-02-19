import { Platform } from 'react-native'
import { NativeShikiClipboard } from '../specs'

const LINKING_ERROR
  = `The package 'react-native-shiki' doesn't seem to be linked. Make sure: \n\n${
    Platform.select({ ios: '- You have run \'pod install\'\n', default: '' })
  }- You rebuilt the app after installing the package\n`
  + `- You are not using Expo Go\n`

if (!NativeShikiClipboard) {
  throw new Error(LINKING_ERROR)
}

export const Clipboard = {
  /**
   * Sets the content of the clipboard
   * @param text The content to be stored in the clipboard
   */
  setString(text: string): Promise<void> {
    return NativeShikiClipboard.setString(text)
  },

  /**
   * Gets the content of the clipboard
   * @returns Promise that resolves with the content of the clipboard
   */
  getString(): Promise<string> {
    return NativeShikiClipboard.getString()
  },
}
