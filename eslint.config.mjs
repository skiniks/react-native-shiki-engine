import antfu from '@antfu/eslint-config'

export default antfu({
  ignores: ['build', 'lib', 'android', 'ios', 'cpp', 'example/android', 'example/ios', 'oniguruma', 'README.md'],
})
