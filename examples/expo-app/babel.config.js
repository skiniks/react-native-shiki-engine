const path = require('node:path')

module.exports = function (api) {
  api.cache.forever()

  return {
    presets: ['babel-preset-expo'],
    plugins: [
      [
        'module-resolver',
        {
          alias: {
            '@': path.join(__dirname, 'src'),
            '@shared': path.join(__dirname, '../shared'),
          },
          extensions: ['.js', '.jsx', '.ts', '.tsx'],
        },
      ],
    ],
  }
}
