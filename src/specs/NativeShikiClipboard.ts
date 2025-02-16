import type { TurboModule } from 'react-native'
import { TurboModuleRegistry } from 'react-native'

export interface Spec extends TurboModule {
  setString: (text: string) => Promise<void>
  getString: () => Promise<string>
}

export default TurboModuleRegistry.getEnforcing<Spec>('ShikiClipboard')
