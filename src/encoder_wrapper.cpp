#include "encoder_wrapper.h"
#include <cmath>
#include <cstring>
#include <algorithm>

extern "C" {
#include <ft8/message.h>
#include <ft8/encode.h>
#include <ft8/constants.h>
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GFSK_CONST_K 5.336446f  // == pi * sqrt(2 / log(2))

// Default configuration values
const float DEFAULT_FREQUENCY = 1000.0f;
const int DEFAULT_SAMPLE_RATE = 12000;
const float FT8_SYMBOL_BT = 2.0f;
const float FT4_SYMBOL_BT = 1.0f;

Napi::Function MessageEncoder::Init(Napi::Env env) {
    Napi::Function func = DefineClass(env, "MessageEncoder", {
        InstanceMethod("encode", &MessageEncoder::Encode),
        InstanceMethod("generateAudio", &MessageEncoder::GenerateAudio),
        InstanceMethod("encodeToAudio", &MessageEncoder::EncodeToAudio)
    });
    
    return func;
}

MessageEncoder::MessageEncoder(const Napi::CallbackInfo& info) : Napi::ObjectWrap<MessageEncoder>(info) {
    Napi::Env env = info.Env();
    
    // Set default values
    protocol_ = FTX_PROTOCOL_FT8;
    frequency_ = DEFAULT_FREQUENCY;
    sample_rate_ = DEFAULT_SAMPLE_RATE;
    symbol_bt_ = FT8_SYMBOL_BT;
    
    // Parse configuration if provided
    if (info.Length() > 0 && info[0].IsObject()) {
        Napi::Object config = info[0].As<Napi::Object>();
        
        if (config.Has("protocol")) {
            std::string protocol = config.Get("protocol").As<Napi::String>().Utf8Value();
            if (protocol == "FT8") {
                protocol_ = FTX_PROTOCOL_FT8;
                symbol_bt_ = FT8_SYMBOL_BT;
            } else if (protocol == "FT4") {
                protocol_ = FTX_PROTOCOL_FT4;
                symbol_bt_ = FT4_SYMBOL_BT;
            } else {
                Napi::TypeError::New(env, "Invalid protocol. Must be 'FT8' or 'FT4'").ThrowAsJavaScriptException();
                return;
            }
        }
        
        if (config.Has("frequency")) {
            frequency_ = config.Get("frequency").As<Napi::Number>().FloatValue();
        }
        
        if (config.Has("sampleRate")) {
            sample_rate_ = config.Get("sampleRate").As<Napi::Number>().Int32Value();
        }
        
        if (config.Has("symbolBt")) {
            symbol_bt_ = config.Get("symbolBt").As<Napi::Number>().FloatValue();
        }
    }
}

Napi::Value MessageEncoder::Encode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected message string").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    std::string message = info[0].As<Napi::String>().Utf8Value();
    
    // Encode the message
    ftx_message_t msg;
    ftx_message_init(&msg);
    
    ftx_message_rc_t rc = ftx_message_encode(&msg, nullptr, message.c_str());
    
    if (rc != FTX_MESSAGE_RC_OK) {
        Napi::Error::New(env, "Failed to encode message").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    // Generate tones
    int num_tones = (protocol_ == FTX_PROTOCOL_FT8) ? FT8_NN : FT4_NN;
    uint8_t* tones = new uint8_t[num_tones];
    
    if (protocol_ == FTX_PROTOCOL_FT8) {
        ft8_encode(msg.payload, tones);
    } else {
        ft4_encode(msg.payload, tones);
    }
    
    // Create result object
    Napi::Object result = Napi::Object::New(env);
    result.Set("text", Napi::String::New(env, message));
    
    // Copy payload
    Napi::Uint8Array payload = Napi::Uint8Array::New(env, FTX_PAYLOAD_LENGTH_BYTES);
    memcpy(payload.Data(), msg.payload, FTX_PAYLOAD_LENGTH_BYTES);
    result.Set("payload", payload);
    
    // Copy tones
    Napi::Uint8Array tonesArray = Napi::Uint8Array::New(env, num_tones);
    memcpy(tonesArray.Data(), tones, num_tones);
    result.Set("tones", tonesArray);
    
    result.Set("hash", Napi::Number::New(env, msg.hash));
    result.Set("protocol", Napi::String::New(env, (protocol_ == FTX_PROTOCOL_FT8) ? "FT8" : "FT4"));
    
    delete[] tones;
    
    return result;
}

void MessageEncoder::GenerateGfskPulse(int n_spsym, float symbol_bt, float* pulse) {
    for (int i = 0; i < 3 * n_spsym; ++i) {
        float t = i / (float)n_spsym - 1.5f;
        float arg1 = GFSK_CONST_K * symbol_bt * (t + 0.5f);
        float arg2 = GFSK_CONST_K * symbol_bt * (t - 0.5f);
        pulse[i] = (erff(arg1) - erff(arg2)) / 2;
    }
}

void MessageEncoder::GenerateGfskSignal(const uint8_t* tones, int num_tones, float frequency,
                                       float symbol_bt, float symbol_period, int sample_rate, float* signal) {
    int n_spsym = (int)(0.5f + sample_rate * symbol_period);
    int n_wave = num_tones * n_spsym;
    float hmod = 1.0f;
    
    // Compute the smoothed frequency waveform
    float dphi_peak = 2 * M_PI * hmod / n_spsym;
    float* dphi = new float[n_wave + 2 * n_spsym];
    
    // Shift frequency up by base frequency
    for (int i = 0; i < n_wave + 2 * n_spsym; ++i) {
        dphi[i] = 2 * M_PI * frequency / sample_rate;
    }
    
    float* pulse = new float[3 * n_spsym];
    GenerateGfskPulse(n_spsym, symbol_bt, pulse);
    
    for (int i = 0; i < num_tones; ++i) {
        int ib = i * n_spsym;
        for (int j = 0; j < 3 * n_spsym; ++j) {
            dphi[j + ib] += dphi_peak * tones[i] * pulse[j];
        }
    }
    
    // Add dummy symbols at beginning and end
    for (int j = 0; j < 2 * n_spsym; ++j) {
        dphi[j] += dphi_peak * pulse[j + n_spsym] * tones[0];
        dphi[j + num_tones * n_spsym] += dphi_peak * pulse[j] * tones[num_tones - 1];
    }
    
    // Calculate and insert the audio waveform
    float phi = 0;
    for (int k = 0; k < n_wave; ++k) {
        signal[k] = sinf(phi);
        phi = fmodf(phi + dphi[k + n_spsym], 2 * M_PI);
    }
    
    // Apply envelope shaping to the first and last symbols
    int n_ramp = n_spsym / 8;
    for (int i = 0; i < n_ramp; ++i) {
        float env = (1 - cosf(2 * M_PI * i / (2 * n_ramp))) / 2;
        signal[i] *= env;
        signal[n_wave - 1 - i] *= env;
    }
    
    delete[] dphi;
    delete[] pulse;
}

Napi::Value MessageEncoder::GenerateAudio(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsTypedArray()) {
        Napi::TypeError::New(env, "Expected Uint8Array of tones").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    Napi::Uint8Array tonesArray = info[0].As<Napi::Uint8Array>();
    
    // Use current config or override from parameter
    float frequency = frequency_;
    int sample_rate = sample_rate_;
    float symbol_bt = symbol_bt_;
    ftx_protocol_t protocol = protocol_;
    
    if (info.Length() > 1 && info[1].IsObject()) {
        Napi::Object config = info[1].As<Napi::Object>();
        
        if (config.Has("frequency")) {
            frequency = config.Get("frequency").As<Napi::Number>().FloatValue();
        }
        if (config.Has("sampleRate")) {
            sample_rate = config.Get("sampleRate").As<Napi::Number>().Int32Value();
        }
        if (config.Has("symbolBt")) {
            symbol_bt = config.Get("symbolBt").As<Napi::Number>().FloatValue();
        }
        if (config.Has("protocol")) {
            std::string protocolStr = config.Get("protocol").As<Napi::String>().Utf8Value();
            if (protocolStr == "FT8") {
                protocol = FTX_PROTOCOL_FT8;
            } else if (protocolStr == "FT4") {
                protocol = FTX_PROTOCOL_FT4;
            }
        }
    }
    
    float symbol_period = (protocol == FTX_PROTOCOL_FT8) ? FT8_SYMBOL_PERIOD : FT4_SYMBOL_PERIOD;
    float slot_time = (protocol == FTX_PROTOCOL_FT8) ? FT8_SLOT_TIME : FT4_SLOT_TIME;
    int num_tones = tonesArray.ElementLength();
    
    // Calculate sample counts
    int num_samples = (int)(0.5f + num_tones * symbol_period * sample_rate);
    int num_silence = (slot_time * sample_rate - num_samples) / 2;
    int num_total_samples = num_silence + num_samples + num_silence;
    
    // Generate signal
    float* signal = new float[num_total_samples];
    
    // Add silence padding
    for (int i = 0; i < num_silence; i++) {
        signal[i] = 0;
        signal[i + num_samples + num_silence] = 0;
    }
    
    // Generate GFSK signal
    GenerateGfskSignal(tonesArray.Data(), num_tones, frequency, symbol_bt, 
                      symbol_period, sample_rate, signal + num_silence);
    
    // Create result object
    Napi::Object result = Napi::Object::New(env);
    
    // Copy samples
    Napi::Float32Array samples = Napi::Float32Array::New(env, num_total_samples);
    memcpy(samples.Data(), signal, num_total_samples * sizeof(float));
    result.Set("samples", samples);
    
    result.Set("sampleRate", Napi::Number::New(env, sample_rate));
    result.Set("channels", Napi::Number::New(env, 1));
    
    delete[] signal;
    
    return result;
}

Napi::Value MessageEncoder::EncodeToAudio(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected message string").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    std::string message = info[0].As<Napi::String>().Utf8Value();
    
    // Use current config or override from parameter
    float frequency = frequency_;
    int sample_rate = sample_rate_;
    float symbol_bt = symbol_bt_;
    ftx_protocol_t protocol = protocol_;
    
    if (info.Length() > 1 && info[1].IsObject()) {
        Napi::Object config = info[1].As<Napi::Object>();
        
        if (config.Has("frequency")) {
            frequency = config.Get("frequency").As<Napi::Number>().FloatValue();
        }
        if (config.Has("sampleRate")) {
            sample_rate = config.Get("sampleRate").As<Napi::Number>().Int32Value();
        }
        if (config.Has("symbolBt")) {
            symbol_bt = config.Get("symbolBt").As<Napi::Number>().FloatValue();
        }
        if (config.Has("protocol")) {
            std::string protocolStr = config.Get("protocol").As<Napi::String>().Utf8Value();
            if (protocolStr == "FT8") {
                protocol = FTX_PROTOCOL_FT8;
            } else if (protocolStr == "FT4") {
                protocol = FTX_PROTOCOL_FT4;
            }
        }
    }
    
    // Encode the message
    ftx_message_t msg;
    ftx_message_init(&msg);
    
    ftx_message_rc_t rc = ftx_message_encode(&msg, nullptr, message.c_str());
    
    if (rc != FTX_MESSAGE_RC_OK) {
        Napi::Error::New(env, "Failed to encode message").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    // Generate tones
    int num_tones = (protocol == FTX_PROTOCOL_FT8) ? FT8_NN : FT4_NN;
    uint8_t* tones = new uint8_t[num_tones];
    
    if (protocol == FTX_PROTOCOL_FT8) {
        ft8_encode(msg.payload, tones);
    } else {
        ft4_encode(msg.payload, tones);
    }
    
    float symbol_period = (protocol == FTX_PROTOCOL_FT8) ? FT8_SYMBOL_PERIOD : FT4_SYMBOL_PERIOD;
    float slot_time = (protocol == FTX_PROTOCOL_FT8) ? FT8_SLOT_TIME : FT4_SLOT_TIME;
    
    // Calculate sample counts
    int num_samples = (int)(0.5f + num_tones * symbol_period * sample_rate);
    int num_silence = (slot_time * sample_rate - num_samples) / 2;
    int num_total_samples = num_silence + num_samples + num_silence;
    
    // Generate signal
    float* signal = new float[num_total_samples];
    
    // Add silence padding
    for (int i = 0; i < num_silence; i++) {
        signal[i] = 0;
        signal[i + num_samples + num_silence] = 0;
    }
    
    // Generate GFSK signal
    GenerateGfskSignal(tones, num_tones, frequency, symbol_bt, 
                      symbol_period, sample_rate, signal + num_silence);
    
    // Create result object
    Napi::Object result = Napi::Object::New(env);
    
    // Copy samples
    Napi::Float32Array samples = Napi::Float32Array::New(env, num_total_samples);
    memcpy(samples.Data(), signal, num_total_samples * sizeof(float));
    result.Set("samples", samples);
    
    result.Set("sampleRate", Napi::Number::New(env, sample_rate));
    result.Set("channels", Napi::Number::New(env, 1));
    
    delete[] signal;
    delete[] tones;
    
    return result;
}
