import { NativeModules, Platform } from 'react-native'

const LINKING_ERROR
  = `The package 'react-native-shiki' doesn't seem to be linked. Make sure: \n\n${
    Platform.select({ ios: '- You have run \'pod install\'\n', default: '' })
  }- You rebuilt the app after installing the package\n`
  + `- You are not using Expo Go\n`

const ShikiClipboard = NativeModules.ShikiClipboard
  ? NativeModules.ShikiClipboard
  : new Proxy(
    {},
    {
      get() {
        throw new Error(LINKING_ERROR)
      },
    },
  )

export const Clipboard = {
  /**
   * Sets the content of the clipboard
   * @param text The content to be stored in the clipboard
   */
  setString(text: string): Promise<void> {
    return ShikiClipboard.setString(text)
  },

  /**
   * Gets the content of the clipboard
   * @returns Promise that resolves with the content of the clipboard
   */
  getString(): Promise<string> {
    return ShikiClipboard.getString()
  },
}
