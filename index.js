const ft8lib = require('./build/Release/ft8_lib.node');

module.exports = {
  MessageEncoder: ft8lib.MessageEncoder,
  MessageDecoder: ft8lib.MessageDecoder,
  Utils: ft8lib.Utils
};
