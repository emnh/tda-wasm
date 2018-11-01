const path = require('path');

const UglifyJsPlugin = require('uglifyjs-webpack-plugin');

module.exports = {
  //mode: 'development',
  mode: 'production',
  entry: './src/ui.js',
  output: {
    path: path.resolve(__dirname, 'dist'),
    filename: 'ui.js'
  },
  devServer: {
    index: 'default.html',
    contentBase: path.join(__dirname, "."),
    compress: true,
    port: 8080
  },
  plugins: [
    /* new UglifyJsPlugin()*/
  ]
};
