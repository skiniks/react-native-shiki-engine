import antfu from '@antfu/eslint-config'
import deMorgan from 'eslint-plugin-de-morgan'

export default antfu({
  ignores: ['build', 'lib', 'android', 'ios', 'cpp', 'example/android', 'example/ios', 'thirdparty', 'README.md'],
  plugins: { deMorgan },
  react: true,
})
