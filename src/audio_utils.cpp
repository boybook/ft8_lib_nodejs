#include "audio_utils.h"
#include <cstring>
#include <algorithm>

extern "C" {
#include <common/wave.h>
}

Napi::Object AudioUtils::CreateAudioBuffer(Napi::Env env, const float* samples, 
                                          int num_samples, int sample_rate, int channels) {
    Napi::Object audioBuffer = Napi::Object::New(env);
    
    // Copy samples
    Napi::Float32Array samplesArray = Napi::Float32Array::New(env, num_samples);
    memcpy(samplesArray.Data(), samples, num_samples * sizeof(float));
    
    audioBuffer.Set("samples", samplesArray);
    audioBuffer.Set("sampleRate", Napi::Number::New(env, sample_rate));
    audioBuffer.Set("channels", Napi::Number::New(env, channels));
    
    return audioBuffer;
}

Napi::Function AudioUtils::Pcm16ToFloat32(Napi::Env env) {
    return Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsTypedArray()) {
            Napi::TypeError::New(env, "Expected Int16Array").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        Napi::Int16Array pcm = info[0].As<Napi::Int16Array>();
        int num_samples = pcm.ElementLength();
        
        Napi::Float32Array result = Napi::Float32Array::New(env, num_samples);
        
        // Convert 16-bit PCM to float32 in range [-1.0, 1.0]
        for (int i = 0; i < num_samples; ++i) {
            result[i] = pcm[i] / 32768.0f;
        }
        
        return result;
    });
}

Napi::Function AudioUtils::Float32ToPcm16(Napi::Env env) {
    return Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsTypedArray()) {
            Napi::TypeError::New(env, "Expected Float32Array").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        Napi::Float32Array samples = info[0].As<Napi::Float32Array>();
        int num_samples = samples.ElementLength();
        
        Napi::Int16Array result = Napi::Int16Array::New(env, num_samples);
        
        // Convert float32 to 16-bit PCM with clipping
        for (int i = 0; i < num_samples; ++i) {
            float sample = samples[i];
            // Clamp to [-1.0, 1.0] range
            sample = std::max(-1.0f, std::min(1.0f, sample));
            result[i] = (int16_t)(sample * 32767.0f);
        }
        
        return result;
    });
}

Napi::Function AudioUtils::LoadWav(Napi::Env env) {
    return Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected file path string").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string filePath = info[0].As<Napi::String>().Utf8Value();
        
        // Load WAV file using ft8_lib's wave functions
        int num_samples = 0;
        int sample_rate = 0;
        
        // First, get the file size to allocate buffer
        // We'll use a reasonable maximum size for FT8/FT4 files
        const int MAX_SAMPLES = 15 * 48000; // 15 seconds at 48kHz max
        float* samples = new float[MAX_SAMPLES];
        num_samples = MAX_SAMPLES;
        
        int result = load_wav(samples, &num_samples, &sample_rate, filePath.c_str());
        
        if (result != 0) {
            delete[] samples;
            Napi::Error::New(env, "Failed to load WAV file").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        // Create AudioBuffer object
        Napi::Object audioBuffer = CreateAudioBuffer(env, samples, num_samples, sample_rate, 1);
        
        delete[] samples;
        
        return audioBuffer;
    });
}

Napi::Function AudioUtils::SaveWav(Napi::Env env) {
    return Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsObject()) {
            Napi::TypeError::New(env, "Expected file path string and AudioBuffer object").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string filePath = info[0].As<Napi::String>().Utf8Value();
        Napi::Object audioBuffer = info[1].As<Napi::Object>();
        
        if (!audioBuffer.Has("samples") || !audioBuffer.Has("sampleRate")) {
            Napi::TypeError::New(env, "AudioBuffer must have 'samples' and 'sampleRate' properties").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        Napi::Float32Array samples = audioBuffer.Get("samples").As<Napi::Float32Array>();
        int sample_rate = audioBuffer.Get("sampleRate").As<Napi::Number>().Int32Value();
        int num_samples = samples.ElementLength();
        
        // Save WAV file using ft8_lib's wave functions
        int result = save_wav(samples.Data(), num_samples, sample_rate, filePath.c_str());
        
        if (result != 0) {
            Napi::Error::New(env, "Failed to save WAV file").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        return env.Undefined();
    });
}
