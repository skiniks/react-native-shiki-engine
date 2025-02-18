import type { ColorValue, HostComponent, ViewProps } from 'react-native'
import type { DirectEventHandler, WithDefault } from 'react-native/Libraries/Types/CodegenTypes'
import codegenNativeComponent from 'react-native/Libraries/Utilities/codegenNativeComponent'

type FontStyle = 'normal' | 'italic'

export interface NativeProps extends ViewProps {
  onViewReady?: DirectEventHandler<null>
  text?: string
  textColor?: ColorValue
  fontStyle?: WithDefault<FontStyle, 'normal'>
}

export type RTNTestViewType = HostComponent<NativeProps>

export default codegenNativeComponent<NativeProps>('RTNTestView') as RTNTestViewType
