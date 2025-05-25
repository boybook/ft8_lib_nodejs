#include <napi.h>
#include "message_wrapper.h"
#include "encoder_wrapper.h"
#include "decoder_wrapper.h"
#include "audio_utils.h"

extern "C" {
#include <ft8/constants.h>
#include <ft8/message.h>
#include <ft8/encode.h>
#include <ft8/decode.h>
#include <common/wave.h>
}

/**
 * Initialize the FT8/FT4 Library Node.js addon
 * 
 * This function sets up all the classes and utility functions exposed
 * to JavaScript, providing a complete interface to the ft8_lib C library.
 * 
 * @param env The N-API environment
 * @param exports The exports object to populate
 * @return The populated exports object
 */
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Export the main encoder and decoder classes
    exports.Set("MessageEncoder", MessageEncoder::Init(env));
    exports.Set("MessageDecoder", MessageDecoder::Init(env));
    
    // Create Utils namespace object
    Napi::Object utils = Napi::Object::New(env);
    
    // Protocol constants utility
    auto getProtocolConstants = Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected protocol string as first argument").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string protocol = info[0].As<Napi::String>().Utf8Value();
        Napi::Object constants = Napi::Object::New(env);
        
        if (protocol == "FT8") {
            constants.Set("symbolPeriod", Napi::Number::New(env, FT8_SYMBOL_PERIOD));
            constants.Set("slotTime", Napi::Number::New(env, FT8_SLOT_TIME));
            constants.Set("numTones", Napi::Number::New(env, 8));
            constants.Set("numDataSymbols", Napi::Number::New(env, FT8_ND));
            constants.Set("totalSymbols", Napi::Number::New(env, FT8_NN));
            constants.Set("syncLength", Napi::Number::New(env, FT8_LENGTH_SYNC));
            constants.Set("numSyncBlocks", Napi::Number::New(env, FT8_NUM_SYNC));
            constants.Set("syncOffset", Napi::Number::New(env, FT8_SYNC_OFFSET));
        } else if (protocol == "FT4") {
            constants.Set("symbolPeriod", Napi::Number::New(env, FT4_SYMBOL_PERIOD));
            constants.Set("slotTime", Napi::Number::New(env, FT4_SLOT_TIME));
            constants.Set("numTones", Napi::Number::New(env, 4));
            constants.Set("numDataSymbols", Napi::Number::New(env, FT4_ND));
            constants.Set("totalSymbols", Napi::Number::New(env, FT4_NN));
            constants.Set("syncLength", Napi::Number::New(env, FT4_LENGTH_SYNC));
            constants.Set("numSyncBlocks", Napi::Number::New(env, FT4_NUM_SYNC));
            constants.Set("syncOffset", Napi::Number::New(env, FT4_SYNC_OFFSET));
        } else {
            Napi::TypeError::New(env, "Invalid protocol. Must be 'FT8' or 'FT4'").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        return constants;
    });
    utils.Set("getProtocolConstants", getProtocolConstants);
    
    // Audio utilities namespace
    Napi::Object audioUtils = Napi::Object::New(env);
    audioUtils.Set("pcm16ToFloat32", AudioUtils::Pcm16ToFloat32(env));
    audioUtils.Set("float32ToPcm16", AudioUtils::Float32ToPcm16(env));
    audioUtils.Set("loadWav", AudioUtils::LoadWav(env));
    audioUtils.Set("saveWav", AudioUtils::SaveWav(env));
    utils.Set("Audio", audioUtils);
    
    // Message utilities namespace
    Napi::Object messageUtils = Napi::Object::New(env);
    messageUtils.Set("isValidMessage", MessageWrapper::IsValidMessage(env));
    messageUtils.Set("getMessageType", MessageWrapper::GetMessageType(env));
    messageUtils.Set("parseStandardMessage", MessageWrapper::ParseStandardMessage(env));
    utils.Set("Message", messageUtils);
    
    // CRC utilities namespace
    Napi::Object crcUtils = Napi::Object::New(env);
    crcUtils.Set("calculateCrc14", MessageWrapper::CalculateCrc14(env));
    crcUtils.Set("verifyCrc14", MessageWrapper::VerifyCrc14(env));
    utils.Set("CRC", crcUtils);
    
    exports.Set("Utils", utils);
    
    return exports;
}

// Initialize the addon
NODE_API_MODULE(ft8_lib, Init)
