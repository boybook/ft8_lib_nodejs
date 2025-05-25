/**
 * FT8/FT4 Digital Mode Library for Node.js
 * 
 * This module provides encoding and decoding capabilities for FT8 and FT4 digital modes
 * used in amateur radio communications. It's a Node.js C++ extension that wraps the
 * ft8_lib C library.
 * 
 * @module ft8-lib
 * @version 1.0.0
 */

export type Protocol = 'FT8' | 'FT4';

export type MessageType = 
  | 'FREE_TEXT'
  | 'DXPEDITION'
  | 'EU_VHF'
  | 'ARRL_FD'
  | 'TELEMETRY'
  | 'CONTESTING'
  | 'STANDARD'
  | 'ARRL_RTTY'
  | 'NONSTD_CALL'
  | 'WWROF'
  | 'UNKNOWN';

export type CallsignHashType = '22_BITS' | '12_BITS' | '10_BITS';

/**
 * Represents a decoded FT8/FT4 message
 */
export interface DecodedMessage {
  /** The decoded message text */
  text: string;
  /** The raw payload bytes */
  payload: Uint8Array;
  /** Message hash for duplicate detection */
  hash: number;
  /** Message type classification */
  type: MessageType;
  /** Signal-to-noise ratio in dB */
  snr?: number;
  /** Frequency offset in Hz */
  frequency?: number;
  /** Time offset in seconds */
  timeOffset?: number;
  /** Decoding confidence score */
  score?: number;
}

/**
 * Represents an encoded message with tones
 */
export interface EncodedMessage {
  /** The original message text */
  text: string;
  /** The raw payload bytes */
  payload: Uint8Array;
  /** Array of tone symbols (0-7 for FT8, 0-3 for FT4) */
  tones: Uint8Array;
  /** Message hash */
  hash: number;
  /** Protocol used for encoding */
  protocol: Protocol;
}

/**
 * Audio buffer containing floating-point samples
 */
export interface AudioBuffer {
  /** Audio samples in range [-1.0, 1.0] */
  samples: Float32Array;
  /** Sample rate in Hz */
  sampleRate: number;
  /** Number of channels (typically 1 for FT8/FT4) */
  channels: number;
}

/**
 * Configuration for the decoder
 */
export interface DecoderConfig {
  /** Protocol to decode (FT8 or FT4) */
  protocol: Protocol;
  /** Minimum sync score threshold for candidates (default: 10) */
  minScore?: number;
  /** Maximum number of candidates to process (default: 140) */
  maxCandidates?: number;
  /** Maximum LDPC iterations (default: 25) */
  maxLdpcIterations?: number;
  /** Maximum decoded messages (default: 50) */
  maxDecodedMessages?: number;
  /** Frequency oversampling rate (default: 2) */
  freqOsr?: number;
  /** Time oversampling rate (default: 2) */
  timeOsr?: number;
  /** Lower frequency bound in Hz (default: 200) */
  frequencyMin?: number;
  /** Upper frequency bound in Hz (default: 3000) */
  frequencyMax?: number;
}

/**
 * Configuration for the encoder
 */
export interface EncoderConfig {
  /** Protocol to use for encoding (FT8 or FT4) */
  protocol: Protocol;
  /** Base frequency in Hz (default: 1000) */
  frequency?: number;
  /** Sample rate for generated audio (default: 12000) */
  sampleRate?: number;
  /** Symbol smoothing filter bandwidth factor (FT8: 2.0, FT4: 1.0) */
  symbolBt?: number;
}

/**
 * Candidate message found during sync detection
 */
export interface MessageCandidate {
  /** Candidate sync score */
  score: number;
  /** Time offset in symbol blocks */
  timeOffset: number;
  /** Frequency offset in bins */
  freqOffset: number;
  /** Time subdivision index */
  timeSub: number;
  /** Frequency subdivision index */
  freqSub: number;
}

/**
 * Decoding status information
 */
export interface DecodeStatus {
  /** Frequency in Hz */
  frequency: number;
  /** Time offset in seconds */
  time: number;
  /** Number of LDPC errors during decoding */
  ldpcErrors: number;
  /** CRC value extracted from message */
  crcExtracted: number;
  /** CRC value calculated over payload */
  crcCalculated: number;
}

/**
 * Callsign hash interface for managing callsign lookups
 */
export interface CallsignHashInterface {
  /**
   * Look up a callsign by its hash
   * @param hashType Type of hash (22, 12, or 10 bits)
   * @param hash Hash value to look up
   * @returns The callsign if found, null otherwise
   */
  lookupHash(hashType: CallsignHashType, hash: number): string | null;
  
  /**
   * Save a callsign with its hash
   * @param callsign The callsign to save
   * @param hash The 22-bit hash value
   */
  saveHash(callsign: string, hash: number): void;
}

/**
 * FT8/FT4 Message encoder class
 */
declare class MessageEncoder {
  /**
   * Create a new message encoder
   * @param config Encoder configuration
   */
  constructor(config?: EncoderConfig);

  /**
   * Encode a text message into FT8/FT4 format
   * @param message The message text to encode
   * @param hashInterface Optional callsign hash interface
   * @returns The encoded message with tones
   */
  encode(message: string, hashInterface?: CallsignHashInterface): EncodedMessage;

  /**
   * Generate audio samples from encoded tones
   * @param tones Array of tone symbols
   * @param config Optional encoder configuration override
   * @returns Audio buffer with generated samples
   */
  generateAudio(tones: Uint8Array, config?: Partial<EncoderConfig>): AudioBuffer;

  /**
   * Encode message and generate audio in one step
   * @param message The message text to encode
   * @param config Optional encoder configuration override
   * @param hashInterface Optional callsign hash interface
   * @returns Audio buffer with generated samples
   */
  encodeToAudio(
    message: string,
    config?: Partial<EncoderConfig>,
    hashInterface?: CallsignHashInterface
  ): AudioBuffer;
}

/**
 * FT8/FT4 Message decoder class
 */
declare class MessageDecoder {
  /**
   * Create a new message decoder
   * @param config Decoder configuration
   */
  constructor(config?: DecoderConfig);

  /**
   * Decode messages from audio buffer
   * @param audio Audio buffer containing FT8/FT4 signals
   * @param hashInterface Optional callsign hash interface
   * @returns Array of decoded messages
   */
  decode(audio: AudioBuffer, hashInterface?: CallsignHashInterface): DecodedMessage[];

  /**
   * Find message candidates in audio
   * @param audio Audio buffer to analyze
   * @returns Array of message candidates
   */
  findCandidates(audio: AudioBuffer): MessageCandidate[];

  /**
   * Decode a specific candidate
   * @param audio Audio buffer containing the signal
   * @param candidate The candidate to decode
   * @param hashInterface Optional callsign hash interface
   * @returns Decoded message and status, or null if decoding failed
   */
  decodeCandidate(
    audio: AudioBuffer,
    candidate: MessageCandidate,
    hashInterface?: CallsignHashInterface
  ): { message: DecodedMessage; status: DecodeStatus } | null;
}

/**
 * Utility functions for FT8/FT4 operations
 */
declare namespace Utils {
  /**
   * Get protocol constants
   * @param protocol The protocol (FT8 or FT4)
   * @returns Object containing protocol-specific constants
   */
  function getProtocolConstants(protocol: Protocol): {
    symbolPeriod: number;
    slotTime: number;
    numTones: number;
    numDataSymbols: number;
    totalSymbols: number;
    syncLength: number;
    numSyncBlocks: number;
    syncOffset: number;
  };

  /**
   * Convert audio samples to/from different formats
   */
  namespace Audio {
    /**
     * Convert 16-bit PCM to float32 samples
     * @param pcm 16-bit PCM data
     * @returns Float32 samples in range [-1.0, 1.0]
     */
    function pcm16ToFloat32(pcm: Int16Array): Float32Array;

    /**
     * Convert float32 samples to 16-bit PCM
     * @param samples Float32 samples in range [-1.0, 1.0]
     * @returns 16-bit PCM data
     */
    function float32ToPcm16(samples: Float32Array): Int16Array;

    /**
     * Load WAV file and return audio buffer
     * @param filePath Path to the WAV file
     * @returns Promise resolving to audio buffer
     */
    function loadWav(filePath: string): Promise<AudioBuffer>;

    /**
     * Save audio buffer as WAV file
     * @param audio Audio buffer to save
     * @param filePath Output file path
     * @returns Promise resolving when file is saved
     */
    function saveWav(filePath: string, audio: AudioBuffer): Promise<void>;
  }

  /**
   * Message utilities
   */
  namespace Message {
    /**
     * Validate if a message can be encoded
     * @param message Message text to validate
     * @param protocol Target protocol
     * @returns true if message is valid for encoding
     */
    function isValidMessage(message: string, protocol: Protocol): boolean;

    /**
     * Get message type from text
     * @param message Message text
     * @returns Detected message type
     */
    function getMessageType(message: string): MessageType;

    /**
     * Parse standard message format
     * @param message Message text
     * @returns Object with parsed components (callTo, callDe, extra)
     */
    function parseStandardMessage(message: string): {
      callTo: string;
      callDe: string;
      extra: string;
    } | null;
  }

  /**
   * CRC utilities
   */
  namespace CRC {
    /**
     * Calculate CRC-14 checksum
     * @param data Data to calculate CRC for
     * @returns CRC-14 value
     */
    function calculateCrc14(data: Uint8Array): number;

    /**
     * Verify CRC-14 checksum
     * @param data Data to verify
     * @param expectedCrc Expected CRC value
     * @returns true if CRC matches
     */
    function verifyCrc14(data: Uint8Array, expectedCrc: number): boolean;
  }
}

// Named exports
export { MessageEncoder, MessageDecoder, Utils };

// Default export interface for CommonJS compatibility
declare const ft8lib: {
  MessageEncoder: typeof MessageEncoder;
  MessageDecoder: typeof MessageDecoder;
  Utils: typeof Utils;
};

export default ft8lib;

// Global module declaration for package name
declare module "ft8-lib" {
  export { MessageEncoder, MessageDecoder, Utils };
  export default ft8lib;
}
