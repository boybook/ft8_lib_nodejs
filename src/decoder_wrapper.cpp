#include "decoder_wrapper.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>

extern "C" {
#include <ft8/decode.h>
#include <ft8/message.h>
#include <ft8/constants.h>
#include <common/monitor.h>
}

// Default configuration values
const int DEFAULT_MIN_SCORE = 10;
const int DEFAULT_MAX_CANDIDATES = 140;
const int DEFAULT_MAX_LDPC_ITERATIONS = 25;
const int DEFAULT_MAX_DECODED_MESSAGES = 50;
const int DEFAULT_FREQ_OSR = 2;
const int DEFAULT_TIME_OSR = 2;
const float DEFAULT_FREQ_MIN = 200.0f;
const float DEFAULT_FREQ_MAX = 3000.0f;

// Static instance pointer for C callbacks
MessageDecoder* MessageDecoder::current_instance_ = nullptr;

Napi::Function MessageDecoder::Init(Napi::Env env) {
    Napi::Function func = DefineClass(env, "MessageDecoder", {
        InstanceMethod("decode", &MessageDecoder::Decode),
        InstanceMethod("findCandidates", &MessageDecoder::FindCandidates),
        InstanceMethod("decodeCandidate", &MessageDecoder::DecodeCandidate)
    });
    
    return func;
}

MessageDecoder::MessageDecoder(const Napi::CallbackInfo& info) : Napi::ObjectWrap<MessageDecoder>(info) {
    Napi::Env env = info.Env();
    
    // Set default values
    protocol_ = FTX_PROTOCOL_FT8;
    min_score_ = DEFAULT_MIN_SCORE;
    max_candidates_ = DEFAULT_MAX_CANDIDATES;
    max_ldpc_iterations_ = DEFAULT_MAX_LDPC_ITERATIONS;
    max_decoded_messages_ = DEFAULT_MAX_DECODED_MESSAGES;
    freq_osr_ = DEFAULT_FREQ_OSR;
    time_osr_ = DEFAULT_TIME_OSR;
    freq_min_ = DEFAULT_FREQ_MIN;
    freq_max_ = DEFAULT_FREQ_MAX;
    monitor_initialized_ = false;
    
    // Parse configuration if provided
    if (info.Length() > 0 && info[0].IsObject()) {
        Napi::Object config = info[0].As<Napi::Object>();
        
        if (config.Has("protocol")) {
            std::string protocol = config.Get("protocol").As<Napi::String>().Utf8Value();
            if (protocol == "FT8") {
                protocol_ = FTX_PROTOCOL_FT8;
            } else if (protocol == "FT4") {
                protocol_ = FTX_PROTOCOL_FT4;
            } else {
                Napi::TypeError::New(env, "Invalid protocol. Must be 'FT8' or 'FT4'").ThrowAsJavaScriptException();
                return;
            }
        }
        
        if (config.Has("minScore")) {
            min_score_ = config.Get("minScore").As<Napi::Number>().Int32Value();
        }
        if (config.Has("maxCandidates")) {
            max_candidates_ = config.Get("maxCandidates").As<Napi::Number>().Int32Value();
        }
        if (config.Has("maxLdpcIterations")) {
            max_ldpc_iterations_ = config.Get("maxLdpcIterations").As<Napi::Number>().Int32Value();
        }
        if (config.Has("maxDecodedMessages")) {
            max_decoded_messages_ = config.Get("maxDecodedMessages").As<Napi::Number>().Int32Value();
        }
        if (config.Has("freqOsr")) {
            freq_osr_ = config.Get("freqOsr").As<Napi::Number>().Int32Value();
        }
        if (config.Has("timeOsr")) {
            time_osr_ = config.Get("timeOsr").As<Napi::Number>().Int32Value();
        }
        if (config.Has("frequencyMin")) {
            freq_min_ = config.Get("frequencyMin").As<Napi::Number>().FloatValue();
        }
        if (config.Has("frequencyMax")) {
            freq_max_ = config.Get("frequencyMax").As<Napi::Number>().FloatValue();
        }
    }
    
    InitializeHashTable();
    current_instance_ = this;
}

MessageDecoder::~MessageDecoder() {
    if (monitor_initialized_) {
        monitor_free(&monitor_);
    }
    if (current_instance_ == this) {
        current_instance_ = nullptr;
    }
}

void MessageDecoder::InitializeHashTable() {
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        hash_table_[i].used = false;
        hash_table_[i].callsign[0] = '\0';
        hash_table_[i].hash = 0;
    }
}

bool MessageDecoder::HashTableLookup(ftx_callsign_hash_type_t hash_type, uint32_t hash, char* callsign) {
    if (!current_instance_) {
        callsign[0] = '\0';
        return false;
    }
    
    uint8_t hash_shift = (hash_type == FTX_CALLSIGN_HASH_10_BITS) ? 12 : 
                        (hash_type == FTX_CALLSIGN_HASH_12_BITS ? 10 : 0);
    uint16_t hash10 = (hash >> (12 - hash_shift)) & 0x3FFu;
    int idx_hash = (hash10 * 23) % HASH_TABLE_SIZE;
    
    while (current_instance_->hash_table_[idx_hash].used) {
        if (((current_instance_->hash_table_[idx_hash].hash & 0x3FFFFFu) >> hash_shift) == hash) {
            strcpy(callsign, current_instance_->hash_table_[idx_hash].callsign);
            return true;
        }
        idx_hash = (idx_hash + 1) % HASH_TABLE_SIZE;
    }
    
    callsign[0] = '\0';
    return false;
}

void MessageDecoder::HashTableSave(const char* callsign, uint32_t hash) {
    if (!current_instance_) return;
    
    uint16_t hash10 = (hash >> 12) & 0x3FFu;
    int idx_hash = (hash10 * 23) % HASH_TABLE_SIZE;
    
    while (current_instance_->hash_table_[idx_hash].used) {
        if (((current_instance_->hash_table_[idx_hash].hash & 0x3FFFFFu) == hash) && 
            (strcmp(current_instance_->hash_table_[idx_hash].callsign, callsign) == 0)) {
            return; // Already exists
        }
        idx_hash = (idx_hash + 1) % HASH_TABLE_SIZE;
    }
    
    // Add new entry
    current_instance_->hash_table_[idx_hash].used = true;
    strncpy(current_instance_->hash_table_[idx_hash].callsign, callsign, 11);
    current_instance_->hash_table_[idx_hash].callsign[11] = '\0';
    current_instance_->hash_table_[idx_hash].hash = hash;
}

void MessageDecoder::InitializeMonitor(int sample_rate) {
    if (monitor_initialized_) {
        monitor_free(&monitor_);
    }
    
    monitor_config_t config = {
        .f_min = freq_min_,
        .f_max = freq_max_,
        .sample_rate = sample_rate,
        .time_osr = time_osr_,
        .freq_osr = freq_osr_,
        .protocol = protocol_
    };
    
    monitor_init(&monitor_, &config);
    monitor_initialized_ = true;
}

void MessageDecoder::ProcessAudio(const float* samples, int num_samples, int sample_rate) {
    if (!monitor_initialized_ || monitor_.wf.max_blocks == 0) {
        InitializeMonitor(sample_rate);
    }
    
    monitor_reset(&monitor_);
    
    // Process audio in block_size chunks as done in the original demo
    // Each call to monitor_process expects exactly block_size samples
    int chunk_size = monitor_.block_size;
    
    for (int offset = 0; offset < num_samples; offset += chunk_size) {
        int remaining = std::min(chunk_size, num_samples - offset);
        
        // If we don't have enough samples for a full chunk, pad with zeros
        if (remaining < chunk_size) {
            std::vector<float> padded_chunk(chunk_size, 0.0f);
            std::copy(samples + offset, samples + offset + remaining, padded_chunk.begin());
            monitor_process(&monitor_, padded_chunk.data());
        } else {
            monitor_process(&monitor_, samples + offset);
        }
    }
}

Napi::Object MessageDecoder::CreateDecodedMessageObject(Napi::Env env, const ftx_message_t* message,
                                                       const ftx_decode_status_t* status, const char* message_text) {
    Napi::Object result = Napi::Object::New(env);
    
    result.Set("text", Napi::String::New(env, message_text));
    result.Set("hash", Napi::Number::New(env, message->hash));
    
    // Copy payload
    Napi::Uint8Array payload = Napi::Uint8Array::New(env, FTX_PAYLOAD_LENGTH_BYTES);
    memcpy(payload.Data(), message->payload, FTX_PAYLOAD_LENGTH_BYTES);
    result.Set("payload", payload);
    
    // Determine message type
    ftx_message_type_t type = ftx_message_get_type(message);
    const char* type_str;
    switch (type) {
        case FTX_MESSAGE_TYPE_FREE_TEXT: type_str = "FREE_TEXT"; break;
        case FTX_MESSAGE_TYPE_DXPEDITION: type_str = "DXPEDITION"; break;
        case FTX_MESSAGE_TYPE_EU_VHF: type_str = "EU_VHF"; break;
        case FTX_MESSAGE_TYPE_ARRL_FD: type_str = "ARRL_FD"; break;
        case FTX_MESSAGE_TYPE_TELEMETRY: type_str = "TELEMETRY"; break;
        case FTX_MESSAGE_TYPE_CONTESTING: type_str = "CONTESTING"; break;
        case FTX_MESSAGE_TYPE_STANDARD: type_str = "STANDARD"; break;
        case FTX_MESSAGE_TYPE_ARRL_RTTY: type_str = "ARRL_RTTY"; break;
        case FTX_MESSAGE_TYPE_NONSTD_CALL: type_str = "NONSTD_CALL"; break;
        case FTX_MESSAGE_TYPE_WWROF: type_str = "WWROF"; break;
        default: type_str = "UNKNOWN"; break;
    }
    result.Set("type", Napi::String::New(env, type_str));
    
    if (status) {
        result.Set("frequency", Napi::Number::New(env, status->freq));
        result.Set("timeOffset", Napi::Number::New(env, status->time));
    }
    
    return result;
}

Napi::Object MessageDecoder::CreateCandidateObject(Napi::Env env, const ftx_candidate_t* candidate) {
    Napi::Object result = Napi::Object::New(env);
    
    result.Set("score", Napi::Number::New(env, candidate->score));
    result.Set("timeOffset", Napi::Number::New(env, candidate->time_offset));
    result.Set("freqOffset", Napi::Number::New(env, candidate->freq_offset));
    result.Set("timeSub", Napi::Number::New(env, candidate->time_sub));
    result.Set("freqSub", Napi::Number::New(env, candidate->freq_sub));
    
    return result;
}

Napi::Value MessageDecoder::Decode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected AudioBuffer object").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Object audioBuffer = info[0].As<Napi::Object>();
    
    if (!audioBuffer.Has("samples") || !audioBuffer.Has("sampleRate")) {
        Napi::TypeError::New(env, "AudioBuffer must have 'samples' and 'sampleRate' properties").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Float32Array samples = audioBuffer.Get("samples").As<Napi::Float32Array>();
    int sample_rate = audioBuffer.Get("sampleRate").As<Napi::Number>().Int32Value();
    
    // Process audio
    ProcessAudio(samples.Data(), samples.ElementLength(), sample_rate);
    
    // Find candidates
    ftx_candidate_t* candidates = new ftx_candidate_t[max_candidates_];
    int num_candidates = ftx_find_candidates(&monitor_.wf, max_candidates_, candidates, min_score_);
    
    // Setup hash interface
    ftx_callsign_hash_interface_t hash_if = {
        .lookup_hash = HashTableLookup,
        .save_hash = HashTableSave
    };
    
    // Decode messages
    std::vector<Napi::Object> decoded_messages;
    
    for (int i = 0; i < num_candidates && decoded_messages.size() < max_decoded_messages_; ++i) {
        ftx_message_t message;
        ftx_decode_status_t status;
        
        if (ftx_decode_candidate(&monitor_.wf, &candidates[i], max_ldpc_iterations_, &message, &status)) {
            char message_text[FTX_MAX_MESSAGE_LENGTH] = {0};
            
            if (ftx_message_decode(&message, &hash_if, message_text) == FTX_MESSAGE_RC_OK) {
                float freq_hz = (monitor_.min_bin + candidates[i].freq_offset + 
                               (float)candidates[i].freq_sub / monitor_.wf.freq_osr) / 
                               ((protocol_ == FTX_PROTOCOL_FT8) ? FT8_SYMBOL_PERIOD : FT4_SYMBOL_PERIOD);
                float time_sec = (candidates[i].time_offset + 
                                (float)candidates[i].time_sub / monitor_.wf.time_osr) * 
                                ((protocol_ == FTX_PROTOCOL_FT8) ? FT8_SYMBOL_PERIOD : FT4_SYMBOL_PERIOD);
                
                status.freq = freq_hz;
                status.time = time_sec;
                
                Napi::Object decoded = CreateDecodedMessageObject(env, &message, &status, message_text);
                decoded.Set("score", Napi::Number::New(env, candidates[i].score));
                decoded_messages.push_back(decoded);
            }
        }
    }
    
    delete[] candidates;
    
    // Create result array
    Napi::Array result = Napi::Array::New(env, decoded_messages.size());
    for (size_t i = 0; i < decoded_messages.size(); ++i) {
        result.Set(i, decoded_messages[i]);
    }
    
    return result;
}

Napi::Value MessageDecoder::FindCandidates(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "Expected AudioBuffer object").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Object audioBuffer = info[0].As<Napi::Object>();
    
    if (!audioBuffer.Has("samples") || !audioBuffer.Has("sampleRate")) {
        Napi::TypeError::New(env, "AudioBuffer must have 'samples' and 'sampleRate' properties").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Float32Array samples = audioBuffer.Get("samples").As<Napi::Float32Array>();
    int sample_rate = audioBuffer.Get("sampleRate").As<Napi::Number>().Int32Value();
    
    // Process audio
    ProcessAudio(samples.Data(), samples.ElementLength(), sample_rate);
    
    // Find candidates
    ftx_candidate_t* candidates = new ftx_candidate_t[max_candidates_];
    int num_candidates = ftx_find_candidates(&monitor_.wf, max_candidates_, candidates, min_score_);
    
    // Create result array
    Napi::Array result = Napi::Array::New(env, num_candidates);
    for (int i = 0; i < num_candidates; ++i) {
        result.Set(i, CreateCandidateObject(env, &candidates[i]));
    }
    
    delete[] candidates;
    
    return result;
}

Napi::Value MessageDecoder::DecodeCandidate(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsObject() || !info[1].IsObject()) {
        Napi::TypeError::New(env, "Expected AudioBuffer and candidate objects").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Object audioBuffer = info[0].As<Napi::Object>();
    Napi::Object candidateObj = info[1].As<Napi::Object>();
    
    if (!audioBuffer.Has("samples") || !audioBuffer.Has("sampleRate")) {
        Napi::TypeError::New(env, "AudioBuffer must have 'samples' and 'sampleRate' properties").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Float32Array samples = audioBuffer.Get("samples").As<Napi::Float32Array>();
    int sample_rate = audioBuffer.Get("sampleRate").As<Napi::Number>().Int32Value();
    
    // Parse candidate
    ftx_candidate_t candidate;
    candidate.score = candidateObj.Get("score").As<Napi::Number>().Int32Value();
    candidate.time_offset = candidateObj.Get("timeOffset").As<Napi::Number>().Int32Value();
    candidate.freq_offset = candidateObj.Get("freqOffset").As<Napi::Number>().Int32Value();
    candidate.time_sub = candidateObj.Get("timeSub").As<Napi::Number>().Uint32Value();
    candidate.freq_sub = candidateObj.Get("freqSub").As<Napi::Number>().Uint32Value();
    
    // Process audio
    ProcessAudio(samples.Data(), samples.ElementLength(), sample_rate);
    
    // Setup hash interface
    ftx_callsign_hash_interface_t hash_if = {
        .lookup_hash = HashTableLookup,
        .save_hash = HashTableSave
    };
    
    // Decode the specific candidate
    ftx_message_t message;
    ftx_decode_status_t status;
    
    if (!ftx_decode_candidate(&monitor_.wf, &candidate, max_ldpc_iterations_, &message, &status)) {
        return env.Null();
    }
    
    char message_text[FTX_MAX_MESSAGE_LENGTH] = {0};
    
    if (ftx_message_decode(&message, &hash_if, message_text) != FTX_MESSAGE_RC_OK) {
        return env.Null();
    }
    
    // Calculate frequency and time
    float freq_hz = (monitor_.min_bin + candidate.freq_offset + 
                    (float)candidate.freq_sub / monitor_.wf.freq_osr) / 
                    ((protocol_ == FTX_PROTOCOL_FT8) ? FT8_SYMBOL_PERIOD : FT4_SYMBOL_PERIOD);
    float time_sec = (candidate.time_offset + 
                     (float)candidate.time_sub / monitor_.wf.time_osr) * 
                     ((protocol_ == FTX_PROTOCOL_FT8) ? FT8_SYMBOL_PERIOD : FT4_SYMBOL_PERIOD);
    
    status.freq = freq_hz;
    status.time = time_sec;
    
    // Create result object
    Napi::Object result = Napi::Object::New(env);
    result.Set("message", CreateDecodedMessageObject(env, &message, &status, message_text));
    
    Napi::Object statusObj = Napi::Object::New(env);
    statusObj.Set("frequency", Napi::Number::New(env, status.freq));
    statusObj.Set("time", Napi::Number::New(env, status.time));
    statusObj.Set("ldpcErrors", Napi::Number::New(env, status.ldpc_errors));
    statusObj.Set("crcExtracted", Napi::Number::New(env, status.crc_extracted));
    statusObj.Set("crcCalculated", Napi::Number::New(env, status.crc_calculated));
    result.Set("status", statusObj);
    
    return result;
}
