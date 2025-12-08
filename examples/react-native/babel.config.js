const path = require('node:path')

module.exports = {
  presets: ['module:@react-native/babel-preset'],
  plugins: [
    [
      'module-resolver',
      {
        alias: {
          '@shared': path.join(__dirname, '../shared'),
        },
        extensions: ['.js', '.jsx', '.ts', '.tsx'],
      },
    ],
  ],
}
