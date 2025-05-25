#ifndef DECODER_WRAPPER_H
#define DECODER_WRAPPER_H

#include <napi.h>

extern "C" {
#include <ft8/decode.h>
#include <ft8/message.h>
#include <ft8/constants.h>
#include <common/monitor.h>
}

/**
 * MessageDecoder class for decoding FT8/FT4 messages
 * 
 * This class provides a JavaScript interface to decode FT8/FT4 signals
 * from audio buffers and extract transmitted messages.
 */
class MessageDecoder : public Napi::ObjectWrap<MessageDecoder> {
public:
    /**
     * Initialize the MessageDecoder class for Node.js
     * @param env N-API environment
     * @return Constructor function
     */
    static Napi::Function Init(Napi::Env env);
    
    /**
     * Constructor
     * @param info Callback info containing constructor arguments
     */
    MessageDecoder(const Napi::CallbackInfo& info);
    
    /**
     * Destructor - cleanup monitor resources
     */
    ~MessageDecoder();

private:
    /**
     * Decode messages from audio buffer
     * @param info Callback info containing audio buffer and optional hash interface
     * @return Array of decoded messages
     */
    Napi::Value Decode(const Napi::CallbackInfo& info);
    
    /**
     * Find message candidates in audio
     * @param info Callback info containing audio buffer
     * @return Array of message candidates
     */
    Napi::Value FindCandidates(const Napi::CallbackInfo& info);
    
    /**
     * Decode a specific candidate
     * @param info Callback info containing audio, candidate, and hash interface
     * @return Decoded message object or null
     */
    Napi::Value DecodeCandidate(const Napi::CallbackInfo& info);

    // Configuration
    ftx_protocol_t protocol_;
    int min_score_;
    int max_candidates_;
    int max_ldpc_iterations_;
    int max_decoded_messages_;
    int freq_osr_;
    int time_osr_;
    float freq_min_;
    float freq_max_;
    
    // Monitor for signal processing
    monitor_t monitor_;
    bool monitor_initialized_;
    
    /**
     * Initialize the monitor with current configuration
     * @param sample_rate Sample rate of input audio
     */
    void InitializeMonitor(int sample_rate);
    
    /**
     * Process audio buffer through the monitor
     * @param samples Audio samples
     * @param num_samples Number of samples
     * @param sample_rate Sample rate
     */
    void ProcessAudio(const float* samples, int num_samples, int sample_rate);
    
    /**
     * Create a JavaScript object from a decoded message
     * @param env N-API environment
     * @param message The decoded message
     * @param status Decoding status
     * @param message_text Decoded message text
     * @return JavaScript object representing the decoded message
     */
    Napi::Object CreateDecodedMessageObject(Napi::Env env, const ftx_message_t* message,
                                           const ftx_decode_status_t* status, const char* message_text);
    
    /**
     * Create a JavaScript object from a message candidate
     * @param env N-API environment
     * @param candidate The message candidate
     * @return JavaScript object representing the candidate
     */
    Napi::Object CreateCandidateObject(Napi::Env env, const ftx_candidate_t* candidate);
    
    /**
     * Simple callsign hash table for testing
     */
    struct CallsignHashEntry {
        char callsign[12];
        uint32_t hash;
        bool used;
    };
    
    static const int HASH_TABLE_SIZE = 256;
    CallsignHashEntry hash_table_[HASH_TABLE_SIZE];
    
    /**
     * Hash table lookup function
     */
    static bool HashTableLookup(ftx_callsign_hash_type_t hash_type, uint32_t hash, char* callsign);
    
    /**
     * Hash table save function
     */
    static void HashTableSave(const char* callsign, uint32_t hash);
    
    // Static pointer to current instance for C callbacks
    static MessageDecoder* current_instance_;
    
    /**
     * Initialize hash table
     */
    void InitializeHashTable();
};

#endif // DECODER_WRAPPER_H
