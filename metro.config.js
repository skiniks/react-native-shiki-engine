/**
 * Metro configuration for React Native Shiki
 * This file provides the necessary configuration to support cleaner imports for Shiki languages and themes.
 *
 * To use this in your project, merge this configuration with your existing Metro config:
 *
 * ```js
 * const { getDefaultConfig, mergeConfig } = require('@react-native/metro-config')
 * const shikiMetroConfig = require('react-native-shiki/metro.config');
 *
 * module.exports = mergeConfig(
 *   getDefaultConfig(__dirname),
 *   shikiMetroConfig
 * );
 * ```
 */

/**
 * @type {import('metro-config').MetroConfig}
 */
const config = {
  resolver: {
    sourceExts: ['js', 'jsx', 'ts', 'tsx', 'json', 'mjs'],
    resolveRequest: (context, moduleName, platform) => {
      if (moduleName.startsWith('@shikijs/langs/') && !moduleName.includes('dist')) {
        const langName = moduleName.replace('@shikijs/langs/', '')
        const resolvedPath = `@shikijs/langs/dist/${langName}.mjs`

        return context.resolveRequest(
          context,
          resolvedPath,
          platform,
        )
      }

      if (moduleName.startsWith('@shikijs/themes/') && !moduleName.includes('dist')) {
        const themeName = moduleName.replace('@shikijs/themes/', '')
        const resolvedPath = `@shikijs/themes/dist/${themeName}.mjs`

        return context.resolveRequest(
          context,
          resolvedPath,
          platform,
        )
      }

      return context.resolveRequest(context, moduleName, platform)
    },
  },
}

module.exports = config
