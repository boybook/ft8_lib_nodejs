// Quick Start Example for ft8-lib
const ft8 = require('../');

console.log('🚀 FT8-Lib Quick Start Example\n');

// 1. Simple encode and decode example
console.log('=== Encode and Decode Example ===');

// Create encoder and decoder
const encoder = new ft8.MessageEncoder();
const decoder = new ft8.MessageDecoder();

// Encode a message
const message = "CQ W1ABC FN42";
console.log(`Original message: "${message}"`);

const audioBuffer = encoder.encodeToAudio(message, {
    sampleRate: 12000,
    frequency: 1500,
    protocol: 'FT8',
    amplitude: 0.5
});

console.log(`Generated audio: ${audioBuffer.samples.length} samples at ${audioBuffer.sampleRate}Hz`);

// Decode the audio
const decodedMessages = decoder.decode(audioBuffer);
console.log(`Decoded ${decodedMessages.length} messages:`);

decodedMessages.forEach((msg, i) => {
    console.log(`  ${i+1}. "${msg.text}" (${msg.frequency}Hz, Score: ${msg.score})`);
});

// 2. WAV file example
console.log('\n=== WAV File Example ===');

// Save to WAV file
ft8.Utils.Audio.saveWav('quickstart_example.wav', audioBuffer);
console.log('Saved audio to quickstart_example.wav');

// Load from WAV file
const loadedAudio = ft8.Utils.Audio.loadWav('quickstart_example.wav');
console.log(`Loaded ${loadedAudio.samples.length} samples from WAV file`);

// 3. Multiple message example
console.log('\n=== Multiple Messages Example ===');

const testMessages = [
    "CQ W1ABC FN42",
    "W1ABC K1DEF 73", 
    "K1DEF W1ABC RRR"
];

testMessages.forEach((msg, i) => {
    const isValid = ft8.Utils.Message.isValidMessage(msg, "FT8");
    const type = ft8.Utils.Message.getMessageType(msg, "FT8");
    console.log(`${i+1}. "${msg}" - Valid: ${isValid}, Type: ${type}`);
});

console.log('\n✅ Quick start example completed successfully!');
console.log('Check the generated quickstart_example.wav file.');
