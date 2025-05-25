// ES Module wrapper for ft8-lib
// This file provides ES module exports by re-exporting from the CommonJS index.js

// Import the CommonJS module (Node.js allows this)
import ft8lib from './index.js';

// Re-export as named exports for ES modules
export const MessageEncoder = ft8lib.MessageEncoder;
export const MessageDecoder = ft8lib.MessageDecoder;
export const Utils = ft8lib.Utils;

// Re-export the default export for compatibility
export default ft8lib;