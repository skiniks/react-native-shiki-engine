import { Platform } from 'react-native'
import NativeShikiBridge from '../specs/NativeShikiBridge'
import logger from './logger'

let initialized = false

/**
 * Function to initialize the Shiki Bridge that sets up JSI bindings
 * This should be called as early as possible in the app lifecycle
 */
export function initializeShikiBridge(): boolean {
  if (Platform.OS === 'ios') {
    logger.warn('ShikiBridge: Skipping initialization on iOS (Android-only feature)')
    return true
  }

  if (initialized) {
    logger.warn('ShikiBridge: Already initialized')
    return true
  }

  try {
    if (!NativeShikiBridge) {
      logger.error('ShikiBridge: Native module not found')
      return false
    }

    const success = NativeShikiBridge.install()

    if (success) {
      logger.warn('ShikiBridge: JSI bindings initialized successfully')
      initialized = true

      const globalObj = globalThis as any
      if (globalObj.ShikiJSI) {
        logger.warn('ShikiBridge: Global ShikiJSI object is available')
      }
      else {
        logger.warn('ShikiBridge: Global ShikiJSI object not found after initialization')
      }

      return true
    }
    else {
      logger.error('ShikiBridge: Failed to initialize JSI bindings')
      return false
    }
  }
  catch (error) {
    logger.error('ShikiBridge: Error during initialization', error)
    return false
  }
}

if (Platform.OS === 'android' && NativeShikiBridge) {
  initializeShikiBridge()
}
