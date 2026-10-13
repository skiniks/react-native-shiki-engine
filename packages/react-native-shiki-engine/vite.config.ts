import { fmt, lint } from '@rari/lint/vite'
import { defineConfig } from 'vite-plus'

export default defineConfig({
  fmt,
  lint: {
    ...lint,
    ignorePatterns: [...(lint.ignorePatterns ?? []), 'third_party/oniguruma/**', '**/lib/'],
    rules: {
      ...lint.rules,
      'typescript/prefer-readonly-parameter-types': 'off',
    },
    overrides: [
      ...(lint.overrides ?? []),
      {
        files: ['src/NativeShikiEngine.ts'],
        rules: {
          'typescript/no-empty-object-type': 'off',
        },
      },
    ],
  },
  test: {
    environment: 'node',
  },
})
