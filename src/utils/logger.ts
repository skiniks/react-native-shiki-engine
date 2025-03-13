/**
 * Logging utility for react-native-shiki
 * Automatically respects the environment (__DEV__) to control log visibility
 */

export const logger = {
  error: (message: string, ...args: any[]) => {
    console.error(message, ...args)
  },

  warn: (message: string, ...args: any[]) => {
    if (__DEV__) {
      console.warn(message, ...args)
    }
  },

  info: (message: string, ...args: any[]) => {
    if (__DEV__) {
      /* eslint-disable-next-line no-console */
      console.info(message, ...args)
    }
  },

  debug: (message: string, ...args: any[]) => {
    if (__DEV__) {
      /* eslint-disable-next-line no-console */
      console.log(`[DEBUG] ${message}`, ...args)
    }
  },
}

export default logger
