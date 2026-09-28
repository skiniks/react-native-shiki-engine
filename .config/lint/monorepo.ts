import type { Linter } from 'eslint'
import type { OxfmtConfig } from 'vite-plus/fmt'
import type { OxlintConfig } from 'vite-plus/lint'
import { fmt as rariFmt, lint as rariLint } from '@rari/lint/vite'

const monorepoIgnorePatterns: string[] = [
  '**/.expo/',
  '**/.gradle',
  '**/.cxx/',
  '**/.kotlin/',
  '**/.metro-health-check*',
  '**/Pods/',
  '**/DerivedData',
  '**/lib/',
  '**/build/',
  'vendor/bundle/',
  '**/xcuserdata',
  '**/*.jsbundle',
  '**/*.keystore',
  '!**/debug.keystore',
  '**/*.xcuserstate',
  'examples/expo-app/ios',
  'examples/expo-app/android',
  'packages/react-native-shiki-engine/third_party/oniguruma/**',
]

export const monorepoEslintConfigs: Linter.Config[] = [
  {
    ignores: monorepoIgnorePatterns,
  },
  {
    files: ['**/scripts/**/*.{ts,tsx,mts,cts}'],
    rules: {
      'no-console': 'off',
    },
  },
]

export const monorepoFmt: OxfmtConfig = {
  ...rariFmt,
  ignorePatterns: [...(rariFmt.ignorePatterns ?? []), ...monorepoIgnorePatterns],
}

export const monorepoLint: OxlintConfig = {
  ...rariLint,
  ignorePatterns: [...(rariLint.ignorePatterns ?? []), ...monorepoIgnorePatterns],
  rules: {
    ...rariLint.rules,
    'typescript/prefer-readonly-parameter-types': 'off',
  },
  overrides: [
    ...(rariLint.overrides ?? []),
    {
      files: ['**/scripts/**/*.{ts,tsx,mts,cts}'],
      rules: {
        'no-console': 'off',
      },
    },
    {
      files: ['packages/react-native-shiki-engine/src/NativeShikiEngine.ts'],
      rules: {
        'typescript/no-empty-object-type': 'off',
      },
    },
  ],
}
