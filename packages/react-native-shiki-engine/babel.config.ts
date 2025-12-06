import type { ConfigAPI } from '@babel/core'

export default function (api: ConfigAPI) {
  api.cache.forever()

  return {
    presets: [
      'module:react-native-builder-bob/babel-preset',
    ],
  }
}
