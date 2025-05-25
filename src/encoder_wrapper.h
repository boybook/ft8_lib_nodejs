#ifndef ENCODER_WRAPPER_H
#define ENCODER_WRAPPER_H

#include <napi.h>

extern "C" {
#include <ft8/message.h>
#include <ft8/encode.h>
#include <ft8/constants.h>
}

/**
 * MessageEncoder class for encoding FT8/FT4 messages
 * 
 * This class provides a JavaScript interface to encode text messages into
 * FT8/FT4 tone sequences and generate corresponding audio samples.
 */
class MessageEncoder : public Napi::ObjectWrap<MessageEncoder> {
public:
    /**
     * Initialize the MessageEncoder class for Node.js
     * @param env N-API environment
     * @return Constructor function
     */
    static Napi::Function Init(Napi::Env env);
    
    /**
     * Constructor
     * @param info Callback info containing constructor arguments
     */
    MessageEncoder(const Napi::CallbackInfo& info);

private:
    /**
     * Encode a text message into FT8/FT4 format
     * @param info Callback info containing message and optional hash interface
     * @return Encoded message object with tones
     */
    Napi::Value Encode(const Napi::CallbackInfo& info);
    
    /**
     * Generate audio samples from tone sequence
     * @param info Callback info containing tones and optional config
     * @return AudioBuffer object with generated samples
     */
    Napi::Value GenerateAudio(const Napi::CallbackInfo& info);
    
    /**
     * Encode message and generate audio in one step
     * @param info Callback info containing message, config, and hash interface
     * @return AudioBuffer object with generated samples
     */
    Napi::Value EncodeToAudio(const Napi::CallbackInfo& info);

    // Configuration
    ftx_protocol_t protocol_;
    float frequency_;
    int sample_rate_;
    float symbol_bt_;
    
    /**
     * Generate GFSK modulated signal from tones
     * @param tones Array of tone symbols
     * @param num_tones Number of tones
     * @param frequency Base frequency in Hz
     * @param symbol_bt Symbol smoothing bandwidth factor
     * @param symbol_period Symbol duration in seconds
     * @param sample_rate Sample rate in Hz
     * @param signal Output buffer for signal samples
     */
    void GenerateGfskSignal(const uint8_t* tones, int num_tones, float frequency,
                           float symbol_bt, float symbol_period, int sample_rate, float* signal);
    
    /**
     * Generate GFSK pulse for symbol smoothing
     * @param n_spsym Samples per symbol
     * @param symbol_bt Bandwidth factor
     * @param pulse Output pulse buffer (3*n_spsym samples)
     */
    void GenerateGfskPulse(int n_spsym, float symbol_bt, float* pulse);
};

#endif // ENCODER_WRAPPER_H
