/**
 * @type {import('@react-native-community/cli-types').UserDependencyConfig}
 */
module.exports = {
  dependency: {
    platforms: {
      /**
       * @type {import('@react-native-community/cli-types').AndroidDependencyParams}
       */
      android: {
        sourceDir: './android',
        packageImportPath: 'import com.shiki.ShikiPackage;',
        packageInstance: 'new ShikiPackage()',
      },
    },
  },
}
