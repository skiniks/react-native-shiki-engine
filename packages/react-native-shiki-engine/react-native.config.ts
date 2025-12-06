const config = {
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

export default config
