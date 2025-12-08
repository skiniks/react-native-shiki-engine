const path = require('node:path')
const { getDefaultConfig, mergeConfig } = require('@react-native/metro-config')
const pak = require('../../packages/react-native-shiki-engine/package.json')

const projectRoot = __dirname
const workspaceRoot = path.resolve(projectRoot, '../..')
const modules = Object.keys({ ...pak.peerDependencies })

const escapeRegExp = str => str.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')

/**
 * Metro configuration
 * https://facebook.github.io/metro/docs/configuration
 *
 * @type {import('metro-config').MetroConfig}
 */
const config = {
  projectRoot,
  watchFolders: [workspaceRoot],

  // We need to make sure that only one version is loaded for peerDependencies
  // So we block them at the root, and alias them to the versions in example's node_modules
  resolver: {
    unstable_enableSymlinks: true,

    nodeModulesPaths: [
      path.resolve(projectRoot, 'node_modules'),
      path.resolve(workspaceRoot, 'node_modules'),
    ],

    blockList: modules.map(
      m => new RegExp(`^${escapeRegExp(path.join(workspaceRoot, 'node_modules', m))}\\/.*$`),
    ),

    extraNodeModules: {
      ...modules.reduce((acc, name) => {
        acc[name] = path.join(projectRoot, 'node_modules', name)
        return acc
      }, {}),
      'react-native-shiki-engine': path.join(workspaceRoot, 'packages/react-native-shiki-engine'),
      '@shared': path.join(workspaceRoot, 'examples/shared'),
    },
  },

  transformer: {
    getTransformOptions: async () => ({
      transform: {
        experimentalImportSupport: false,
        inlineRequires: true,
      },
    }),
  },
}

module.exports = mergeConfig(getDefaultConfig(__dirname), config)
