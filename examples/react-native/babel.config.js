const path = require('node:path')

module.exports = {
  presets: ['module:@react-native/babel-preset'],
  plugins: [
    [
      'module-resolver',
      {
        root: ['.'],
        extensions: ['.ios.ts', '.android.ts', '.ts', '.ios.tsx', '.android.tsx', '.tsx', '.jsx', '.js', '.json'],
        alias: {
          'react-native-shiki-engine': path.join(__dirname, '../../packages/react-native-shiki-engine/src/index'),
        },
      },
    ],
  ],
}
