# FT8/FT4 Node.js Library

A high-performance Node.js C++ extension providing complete FT8 and FT4 digital mode encoding and decoding capabilities for amateur radio applications.

[![npm version](https://badge.fury.io/js/ft8-lib.svg)](https://badge.fury.io/js/ft8-lib)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Overview

This library provides a comprehensive Node.js interface to the FT8/FT4 digital mode protocols used in amateur radio communication. Built as a native C++ extension wrapping the excellent [ft8_lib](https://github.com/kgoba/ft8_lib) by Karlis Goba, it offers:

- **Message Encoding**: Convert text messages to FT8/FT4 tone sequences
- **Audio Generation**: Generate audio signals with GFSK modulation
- **Signal Decoding**: Decode FT8/FT4 signals from audio data
- **Message Parsing**: Extract and validate amateur radio messages
- **WAV File Support**: Load and save audio in WAV format
- **TypeScript Support**: Complete type definitions included

## Features

- ✅ **Full FT8/FT4 Protocol Support** - Encode and decode both protocols
- ✅ **High Performance** - Native C++ implementation with minimal overhead
- ✅ **Cross-Platform** - Supports Windows, macOS, and Linux
- ✅ **Audio I/O** - Built-in WAV file loading and saving
- ✅ **Message Validation** - Automatic validation of callsigns and message formats
- ✅ **Multiple Sample Rates** - Support for 8kHz and 12kHz audio
- ✅ **TypeScript Ready** - Complete type definitions included
- ✅ **Memory Efficient** - Minimal memory footprint and no memory leaks

## Installation

### Prerequisites

- Node.js 14+ (16+ recommended)
- Python 3.x (for node-gyp)
- C++ compiler:
  - **Windows**: Visual Studio 2019+ or Build Tools for Visual Studio
  - **macOS**: Xcode command line tools (`xcode-select --install`)
  - **Linux**: GCC 7+ or Clang 8+

### Install from npm

```bash
npm install ft8-lib
```

### Build from Source

```bash
git clone https://github.com/your-repo/ft8_lib_nodejs.git
cd ft8_lib_nodejs
git submodule update --init --recursive
npm install
npm run build
```

## Quick Start

### Basic Message Encoding

```javascript
const ft8 = require('ft8-lib');

// Create encoder
const encoder = new ft8.MessageEncoder();

// Encode a CQ message
const message = "CQ W1ABC FN42";
const encoded = encoder.encode(message);
console.log(`Encoded ${encoded.tones.length} tones`);

// Generate audio
const audioConfig = {
    sampleRate: 12000,
    frequency: 1500,    // Hz
    protocol: 'FT8',
    amplitude: 0.5
};

const audioBuffer = encoder.generateAudio(encoded.tones, audioConfig);
console.log(`Generated ${audioBuffer.samples.length} samples`);

// Save to WAV file
ft8.Utils.Audio.saveWav('output.wav', audioBuffer);
```

### Basic Message Decoding

```javascript
const ft8 = require('ft8-lib');

// Create decoder
const decoder = new ft8.MessageDecoder();

// Load audio from WAV file
const audioBuffer = ft8.Utils.Audio.loadWav('input.wav');

// Decode messages
const messages = decoder.decode(audioBuffer);

messages.forEach((msg, i) => {
    console.log(`Message ${i + 1}:`);
    console.log(`  Text: "${msg.text}"`);
    console.log(`  Frequency: ${msg.frequency.toFixed(1)} Hz`);
    console.log(`  Time: ${msg.timeOffset.toFixed(3)} s`);
    console.log(`  Score: ${msg.score}`);
});
```

## API Reference

### MessageEncoder

#### Constructor
```javascript
const encoder = new ft8.MessageEncoder(config);
```

**Config options:**
- `protocol` (string): "FT8" or "FT4" (default: "FT8")

#### Methods

##### `encode(message)`
Encode a text message to tone sequence.

```javascript
const encoded = encoder.encode("CQ W1ABC FN42");
// Returns: { 
//   isValid: boolean,
//   tones: number[],      // Array of tone indices (0-7)
//   payload: Uint8Array   // Raw payload bytes
// }
```

##### `generateAudio(tones, config)`
Generate audio signal from tone sequence.

```javascript
const audioBuffer = encoder.generateAudio(tones, {
    sampleRate: 12000,    // 8000 or 12000
    frequency: 1500,      // Center frequency in Hz
    protocol: 'FT8',      // 'FT8' or 'FT4'
    amplitude: 0.5        // 0.0 to 1.0
});
// Returns: { samples: Float32Array, sampleRate: number }
```

##### `encodeToAudio(message, config)`
One-step encoding from message to audio.

```javascript
const audioBuffer = encoder.encodeToAudio("CQ W1ABC FN42", {
    sampleRate: 12000,
    frequency: 1500,
    protocol: 'FT8',
    amplitude: 0.5
});
```

### MessageDecoder

#### Constructor
```javascript
const decoder = new ft8.MessageDecoder(config);
```

**Config options:**
- `protocol` (string): "FT8" or "FT4" (default: "FT8")
- `minScore` (number): Minimum decoding score (default: 10)
- `maxCandidates` (number): Maximum candidates to process (default: 140)
- `maxLdpcIterations` (number): LDPC decoder iterations (default: 25)
- `frequencyMin` (number): Minimum frequency in Hz (default: 200)
- `frequencyMax` (number): Maximum frequency in Hz (default: 3000)

#### Methods

##### `decode(audioBuffer)`
Decode all messages from audio buffer.

```javascript
const messages = decoder.decode(audioBuffer);
// Returns array of:
// {
//   text: string,           // Decoded message text
//   frequency: number,      // Frequency in Hz
//   timeOffset: number,     // Time offset in seconds
//   score: number,          // Decoding confidence score
//   type: string,           // Message type (e.g., "STANDARD")
//   hash: number,           // Message hash
//   payload: Uint8Array     // Raw payload
// }
```

##### `findCandidates(audioBuffer)`
Find signal candidates without full decoding.

```javascript
const candidates = decoder.findCandidates(audioBuffer);
// Returns array of candidate objects with score, time, and frequency info
```

### Utils

#### Audio Utilities

##### `loadWav(filename)`
Load audio from WAV file.

```javascript
const audioBuffer = ft8.Utils.Audio.loadWav('input.wav');
// Returns: { samples: Float32Array, sampleRate: number }
```

##### `saveWav(filename, audioBuffer)`
Save audio to WAV file.

```javascript
ft8.Utils.Audio.saveWav('output.wav', audioBuffer);
```

##### `convertPcm16ToFloat32(pcmData)`
Convert PCM16 to Float32 audio data.

##### `convertFloat32ToPcm16(floatData)`
Convert Float32 to PCM16 audio data.

#### Message Utilities

##### `isValidMessage(text, protocol)`
Validate FT8/FT4 message format.

```javascript
const isValid = ft8.Utils.Message.isValidMessage("CQ W1ABC FN42", "FT8");
```

##### `getMessageType(text, protocol)`
Get the type of a message.

```javascript
const type = ft8.Utils.Message.getMessageType("CQ W1ABC FN42", "FT8");
// Returns: "STANDARD", "FREE_TEXT", etc.
```

## Examples

### Generate Test Signals

```javascript
const ft8 = require('ft8-lib');

const encoder = new ft8.MessageEncoder();
const testMessages = [
    "CQ W1ABC FN42",
    "W1ABC K1DEF 73",
    "K1DEF W1ABC RRR"
];

testMessages.forEach((message, i) => {
    const audioBuffer = encoder.encodeToAudio(message, {
        sampleRate: 12000,
        frequency: 1500,
        protocol: 'FT8',
        amplitude: 0.5
    });
    
    ft8.Utils.Audio.saveWav(`test_${i}.wav`, audioBuffer);
    console.log(`Generated test_${i}.wav: "${message}"`);
});
```

### Batch Decode WAV Files

```javascript
const ft8 = require('ft8-lib');
const fs = require('fs');
const path = require('path');

const decoder = new ft8.MessageDecoder();

// Process all WAV files in a directory
const wavDir = './wav_files';
const files = fs.readdirSync(wavDir).filter(f => f.endsWith('.wav'));

files.forEach(filename => {
    const filepath = path.join(wavDir, filename);
    
    try {
        const audioBuffer = ft8.Utils.Audio.loadWav(filepath);
        const messages = decoder.decode(audioBuffer);
        
        console.log(`\n${filename}:`);
        if (messages.length > 0) {
            messages.forEach(msg => {
                console.log(`  "${msg.text}" (${msg.frequency.toFixed(1)}Hz, Score: ${msg.score})`);
            });
        } else {
            console.log('  No messages decoded');
        }
    } catch (error) {
        console.log(`  Error: ${error.message}`);
    }
});
```

### Multi-frequency Signal Generation

```javascript
const ft8 = require('ft8-lib');

const encoder = new ft8.MessageEncoder();
const frequencies = [800, 1200, 1500, 2000, 2500];
const message = "CQ TEST FN42";

// Create mixed signal with multiple frequencies
const sampleRate = 12000;
const duration = 15.0; // seconds
const mixedSamples = new Float32Array(duration * sampleRate);

frequencies.forEach((freq, i) => {
    const audioBuffer = encoder.encodeToAudio(message, {
        sampleRate: sampleRate,
        frequency: freq,
        protocol: 'FT8',
        amplitude: 0.2  // Lower amplitude to avoid clipping
    });
    
    // Add to mixed signal
    for (let j = 0; j < mixedSamples.length && j < audioBuffer.samples.length; j++) {
        mixedSamples[j] += audioBuffer.samples[j];
    }
});

// Save mixed signal
const mixedBuffer = { samples: mixedSamples, sampleRate: sampleRate };
ft8.Utils.Audio.saveWav('multi_frequency_test.wav', mixedBuffer);

// Decode the mixed signal
const decoder = new ft8.MessageDecoder();
const decodedMessages = decoder.decode(mixedBuffer);

console.log(`Generated ${frequencies.length} signals, decoded ${decodedMessages.length} messages:`);
decodedMessages.forEach(msg => {
    console.log(`  "${msg.text}" at ${msg.frequency.toFixed(0)}Hz`);
});
```

## Performance Considerations

- **Memory Usage**: Each decoder instance uses ~50MB for waterfall processing
- **CPU Usage**: Decoding is CPU-intensive; consider worker threads for real-time applications
- **Sample Rate**: 12kHz provides better frequency resolution than 8kHz
- **Audio Duration**: Standard FT8 slots are 15 seconds, FT4 slots are 7.5 seconds

## Error Handling

```javascript
const ft8 = require('ft8-lib');

try {
    const encoder = new ft8.MessageEncoder();
    const encoded = encoder.encode("INVALID MESSAGE FORMAT");
    
    if (!encoded.isValid) {
        console.log('Message encoding failed - invalid format');
        return;
    }
    
    // Continue with valid encoding...
} catch (error) {
    console.error('Encoder error:', error.message);
}

try {
    const audioBuffer = ft8.Utils.Audio.loadWav('nonexistent.wav');
} catch (error) {
    console.error('WAV loading error:', error.message);
}
```

## Troubleshooting

### Build Issues

**Windows**: If you encounter build errors, ensure you have Visual Studio Build Tools installed:
```bash
npm install --global windows-build-tools
```

**macOS**: Install Xcode command line tools:
```bash
xcode-select --install
```

**Linux**: Install build dependencies:
```bash
# Ubuntu/Debian
sudo apt-get install build-essential python3-dev

# CentOS/RHEL
sudo yum groupinstall "Development Tools"
```

### Runtime Issues

**No messages decoded**: Check that:
- Audio sample rate is 8kHz or 12kHz
- Signal frequency is within the configured range (200-3000Hz default)
- Audio contains valid FT8/FT4 signals
- Signal strength is adequate (score > 10)

**Memory issues**: 
- Limit the number of concurrent decoder instances
- Use `decoder.findCandidates()` for analysis without full decoding
- Process audio in chunks for very long recordings

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

The underlying ft8_lib library is also MIT licensed.

## Acknowledgments

- [Karlis Goba (kgoba)](https://github.com/kgoba) for the excellent ft8_lib C library
- The WSJT-X development team for FT8/FT4 protocol specifications
- The amateur radio community for digital mode innovations

## Related Projects

- [ft8_lib](https://github.com/kgoba/ft8_lib) - Original C library
- [WSJT-X](https://physics.princeton.edu/pulsar/k1jt/wsjtx.html) - Reference implementation
- [js8call](http://js8call.com/) - JS8 digital mode for weak signal communication
