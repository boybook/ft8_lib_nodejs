#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#include <napi.h>

/**
 * AudioUtils class provides utility functions for audio processing
 * 
 * This class offers functions for audio format conversion and file I/O
 * operations, making it easier to work with different audio formats in Node.js.
 */
class AudioUtils {
public:
    /**
     * Convert 16-bit PCM to Float32 samples
     * @param env N-API environment
     * @return N-API function that converts PCM16 to Float32
     */
    static Napi::Function Pcm16ToFloat32(Napi::Env env);
    
    /**
     * Convert Float32 samples to 16-bit PCM
     * @param env N-API environment
     * @return N-API function that converts Float32 to PCM16
     */
    static Napi::Function Float32ToPcm16(Napi::Env env);
    
    /**
     * Load WAV file and return audio buffer
     * @param env N-API environment
     * @return N-API function that loads WAV files
     */
    static Napi::Function LoadWav(Napi::Env env);
    
    /**
     * Save audio buffer as WAV file
     * @param env N-API environment
     * @return N-API function that saves WAV files
     */
    static Napi::Function SaveWav(Napi::Env env);

private:
    /**
     * Create an AudioBuffer object from samples and metadata
     * @param env N-API environment
     * @param samples Audio samples
     * @param num_samples Number of samples
     * @param sample_rate Sample rate in Hz
     * @param channels Number of channels
     * @return AudioBuffer object
     */
    static Napi::Object CreateAudioBuffer(Napi::Env env, const float* samples, 
                                         int num_samples, int sample_rate, int channels);
};

#endif // AUDIO_UTILS_H
