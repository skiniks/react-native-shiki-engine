/**
 * @type {import('@react-native-community/cli-types').UserDependencyConfig}
 */
module.exports = {
  dependency: {
    platforms: {
      android: {
        cxxModuleCMakeListsModuleName: 'react-native-shiki-engine',
        cxxModuleCMakeListsPath: 'CMakeLists.txt',
        cxxModuleHeaderName: 'NativeShikiEngineModule',
      },
    },
  },
}
