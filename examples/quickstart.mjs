// Quick Start Example for ft8-lib
import {
    MessageEncoder,
    MessageDecoder,
    Utils
} from 'ft8-lib';

console.log('🚀 FT8-Lib Quick Start Example\n');

// 1. Simple encode and decode example
console.log('=== Encode and Decode Example ===');

// Create encoder and decoder
const encoder = new MessageEncoder();
const decoder = new MessageDecoder();

// Encode a message
const message = "CQ W1ABC FN42";
console.log(`Original message: "${message}"`);

const audioBuffer = encoder.encodeToAudio(message, {
    sampleRate: 12000,
    frequency: 1500,
    protocol: 'FT8',
    symbolBt: 2.0
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
Utils.Audio.saveWav('quickstart_example.wav', audioBuffer);
console.log('Saved audio to quickstart_example.wav');

// Load from WAV file
const loadedAudio = await Utils.Audio.loadWav('quickstart_example.wav');
console.log(`Loaded ${loadedAudio.samples.length} samples from WAV file`);

// 3. Multiple message example
console.log('\n=== Multiple Messages Example ===');

const testMessages = [
    "CQ W1ABC FN42",
    "W1ABC K1DEF 73", 
    "K1DEF W1ABC RRR"
];

testMessages.forEach((msg, i) => {
    const isValid = Utils.Message.isValidMessage(msg, "FT8");
    const type = Utils.Message.getMessageType(msg);
    console.log(`${i+1}. "${msg}" - Valid: ${isValid}, Type: ${type}`);
});

console.log('\n✅ Quick start example completed successfully!');
console.log('Check the generated quickstart_example.wav file.');
