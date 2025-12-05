const path = require('node:path')
const pkg = require('../../packages/react-native-shiki-engine/package.json')

module.exports = {
  project: {
    ios: {
      automaticPodsInstallation: true,
    },
  },
  dependencies: {
    [pkg.name]: {
      root: path.join(__dirname, '../../packages/react-native-shiki-engine'),
    },
  },
}
