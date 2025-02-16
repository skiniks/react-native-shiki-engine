import antfu from '@antfu/eslint-config'

export default antfu({
  react: true,
  ignores: ['build', 'lib', 'android', 'ios', 'cpp', 'example/android', 'example/ios', 'thirdparty', 'README.md'],
})
