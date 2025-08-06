import { join } from 'node:path'
import antfu from '@antfu/eslint-config'
import oxlint from 'eslint-plugin-oxlint'

export default antfu({
  ...oxlint.buildFromOxlintConfigFile(join(import.meta.dirname, '.oxlintrc.json')),
  ignores: ['build', 'lib', 'android', 'ios', 'cpp', 'example/android', 'example/ios', 'oniguruma', 'thirdparty', 'README.md'],
})
