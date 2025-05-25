const path = require('path');
const binary = require('@mapbox/node-pre-gyp');

// Try to load precompiled binary first, fallback to local build
let ft8lib;
try {
  const binding_path = binary.find(path.resolve(path.join(__dirname, './package.json')));
  ft8lib = require(binding_path);
} catch (err) {
  // Fallback to locally built version
  try {
    ft8lib = require('./build/Release/ft8_lib.node');
  } catch (err2) {
    throw new Error(`Failed to load ft8_lib native module: ${err.message}. Original error: ${err2.message}`);
  }
}

/**
 * @typedef {import('./index.d.ts').MessageEncoder} MessageEncoder
 * @typedef {import('./index.d.ts').MessageDecoder} MessageDecoder  
 * @typedef {import('./index.d.ts').Utils} Utils
 */

// Export as named exports and default export
module.exports = {
  /** @type {MessageEncoder} */
  MessageEncoder: ft8lib.MessageEncoder,
  /** @type {MessageDecoder} */
  MessageDecoder: ft8lib.MessageDecoder,
  /** @type {Utils} */
  Utils: ft8lib.Utils
};

// Add named exports for ES module compatibility when used with require()
module.exports.MessageEncoder = ft8lib.MessageEncoder;
module.exports.MessageDecoder = ft8lib.MessageDecoder;
module.exports.Utils = ft8lib.Utils;
