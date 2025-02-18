import type { HostComponent, ViewProps } from 'react-native'
import type { DirectEventHandler } from 'react-native/Libraries/Types/CodegenTypes'
import codegenNativeComponent from 'react-native/Libraries/Utilities/codegenNativeComponent'

export interface NativeProps extends ViewProps {
  onViewReady?: DirectEventHandler<null>
  text?: string
}

export type RTNTestViewType = HostComponent<NativeProps>

export default codegenNativeComponent<NativeProps>('RTNTestView') as RTNTestViewType
