import type { TurboModule } from 'react-native'
import { Platform, TurboModuleRegistry } from 'react-native'

export interface Spec extends TurboModule {
  /**
   * Install the JSI bindings for the ShikiBridge
   * This should be called as early as possible in the application lifecycle
   */
  install: () => boolean
}

const mockShikiBridge: Spec = {
  install: () => {
    console.warn('ShikiBridge is not available on iOS')
    return false
  },
}

export default Platform.select({
  android: () => TurboModuleRegistry.getEnforcing<Spec>('ShikiBridge'),
  default: () => mockShikiBridge,
})()
