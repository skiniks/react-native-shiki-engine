import { join } from 'node:path'
import antfu from '@antfu/eslint-config'
import oxlint from 'eslint-plugin-oxlint'

export default antfu({
  ...oxlint.buildFromOxlintConfigFile(join(import.meta.dirname, '.oxlintrc.json')),
  ignores: ['build', 'packages/react-native-shiki-engine/lib', 'packages/react-native-shiki-engine/android', 'packages/react-native-shiki-engine/ios', 'packages/react-native-shiki-engine/cpp', 'examples/react-native/android', 'examples/react-native/ios', 'oniguruma', 'README.md'],
})
