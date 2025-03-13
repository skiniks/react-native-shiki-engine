import React from 'react'
import { View } from 'react-native'

export function BackArrow({ color = '#88C0D0', size = 16 }) {
  return (
    <View style={{
      width: size,
      height: size,
      justifyContent: 'center',
      alignItems: 'center',
    }}
    >
      <View
        style={{
          width: size * 0.5,
          height: size * 0.5,
          borderLeftWidth: 2,
          borderBottomWidth: 2,
          borderColor: color,
          transform: [
            { rotate: '45deg' },
            { translateX: size * 0.1 },
          ],
        }}
      />
    </View>
  )
}
