import type { TurboModule } from 'react-native'
import { TurboModuleRegistry } from 'react-native'

export interface Spec extends TurboModule {
  /**
   * Install the JSI bindings for the ShikiBridge
   * This should be called as early as possible in the application lifecycle
   */
  install: () => boolean
}

export default TurboModuleRegistry.getEnforcing<Spec>('ShikiBridge')
