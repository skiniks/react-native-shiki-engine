import { join } from 'node:path'
import antfu from '@antfu/eslint-config'
import oxlint from 'eslint-plugin-oxlint'

export default antfu(
  { react: true },
  {
    rules: {
      'pnpm/yaml-enforce-settings': 'off',
    },
  },
  ...oxlint.buildFromOxlintConfigFile(join(import.meta.dirname, '.oxlintrc.json')),
)
