import NativeShikiBridge from '../specs/NativeShikiBridge'

let initialized = false

/**
 * Function to initialize the Shiki Bridge that sets up JSI bindings
 * This should be called as early as possible in the app lifecycle
 */
export function initializeShikiBridge(): boolean {
  if (initialized) {
    console.warn('ShikiBridge: Already initialized')
    return true
  }

  try {
    if (!NativeShikiBridge) {
      console.error('ShikiBridge: Native module not found')
      return false
    }

    const success = NativeShikiBridge.install()

    if (success) {
      console.warn('ShikiBridge: JSI bindings initialized successfully')
      initialized = true

      const globalObj = globalThis as any
      if (globalObj.ShikiJSI) {
        console.warn('ShikiBridge: Global ShikiJSI object is available')
      }
      else {
        console.warn('ShikiBridge: Global ShikiJSI object not found after initialization')
      }

      return true
    }
    else {
      console.error('ShikiBridge: Failed to initialize JSI bindings')
      return false
    }
  }
  catch (error) {
    console.error('ShikiBridge: Error during initialization', error)
    return false
  }
}

initializeShikiBridge()
