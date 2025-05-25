#include "message_wrapper.h"
#include <string>
#include <cstring>

extern "C" {
#include <ft8/message.h>
#include <ft8/crc.h>
}

const char* MessageWrapper::MessageTypeToString(ftx_message_type_t type) {
    switch (type) {
        case FTX_MESSAGE_TYPE_FREE_TEXT: return "FREE_TEXT";
        case FTX_MESSAGE_TYPE_DXPEDITION: return "DXPEDITION";
        case FTX_MESSAGE_TYPE_EU_VHF: return "EU_VHF";
        case FTX_MESSAGE_TYPE_ARRL_FD: return "ARRL_FD";
        case FTX_MESSAGE_TYPE_TELEMETRY: return "TELEMETRY";
        case FTX_MESSAGE_TYPE_CONTESTING: return "CONTESTING";
        case FTX_MESSAGE_TYPE_STANDARD: return "STANDARD";
        case FTX_MESSAGE_TYPE_ARRL_RTTY: return "ARRL_RTTY";
        case FTX_MESSAGE_TYPE_NONSTD_CALL: return "NONSTD_CALL";
        case FTX_MESSAGE_TYPE_WWROF: return "WWROF";
        default: return "UNKNOWN";
    }
}

const char* MessageWrapper::MessageRcToString(ftx_message_rc_t rc) {
    switch (rc) {
        case FTX_MESSAGE_RC_OK: return "OK";
        case FTX_MESSAGE_RC_ERROR_CALLSIGN1: return "ERROR_CALLSIGN1";
        case FTX_MESSAGE_RC_ERROR_CALLSIGN2: return "ERROR_CALLSIGN2";
        case FTX_MESSAGE_RC_ERROR_SUFFIX: return "ERROR_SUFFIX";
        case FTX_MESSAGE_RC_ERROR_GRID: return "ERROR_GRID";
        case FTX_MESSAGE_RC_ERROR_TYPE: return "ERROR_TYPE";
        default: return "UNKNOWN_ERROR";
    }
}

Napi::Function MessageWrapper::IsValidMessage(Napi::Env env) {
    return Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
            Napi::TypeError::New(env, "Expected message string and protocol string").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string message = info[0].As<Napi::String>().Utf8Value();
        std::string protocol = info[1].As<Napi::String>().Utf8Value();
        
        if (protocol != "FT8" && protocol != "FT4") {
            Napi::TypeError::New(env, "Protocol must be 'FT8' or 'FT4'").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        // Try to encode the message to check if it's valid
        ftx_message_t msg;
        ftx_message_init(&msg);
        
        ftx_message_rc_t rc = ftx_message_encode(&msg, nullptr, message.c_str());
        
        return Napi::Boolean::New(env, rc == FTX_MESSAGE_RC_OK);
    });
}

Napi::Function MessageWrapper::GetMessageType(Napi::Env env) {
    return Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected message string").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string message = info[0].As<Napi::String>().Utf8Value();
        
        // Encode the message to determine its type
        ftx_message_t msg;
        ftx_message_init(&msg);
        
        ftx_message_rc_t rc = ftx_message_encode(&msg, nullptr, message.c_str());
        
        if (rc != FTX_MESSAGE_RC_OK) {
            return Napi::String::New(env, "UNKNOWN");
        }
        
        ftx_message_type_t type = ftx_message_get_type(&msg);
        return Napi::String::New(env, MessageTypeToString(type));
    });
}

Napi::Function MessageWrapper::ParseStandardMessage(Napi::Env env) {
    return Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected message string").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        std::string message = info[0].As<Napi::String>().Utf8Value();
        
        // Try to parse as standard message
        ftx_message_t msg;
        ftx_message_init(&msg);
        
        ftx_message_rc_t rc = ftx_message_encode(&msg, nullptr, message.c_str());
        
        if (rc != FTX_MESSAGE_RC_OK) {
            return env.Null();
        }
        
        // Check if it's a standard message type
        ftx_message_type_t type = ftx_message_get_type(&msg);
        if (type != FTX_MESSAGE_TYPE_STANDARD) {
            return env.Null();
        }
        
        // Decode back to get components
        char call_to[32] = {0};
        char call_de[32] = {0};
        char extra[32] = {0};
        
        rc = ftx_message_decode_std(&msg, nullptr, call_to, call_de, extra);
        
        if (rc != FTX_MESSAGE_RC_OK) {
            return env.Null();
        }
        
        Napi::Object result = Napi::Object::New(env);
        result.Set("callTo", Napi::String::New(env, call_to));
        result.Set("callDe", Napi::String::New(env, call_de));
        result.Set("extra", Napi::String::New(env, extra));
        
        return result;
    });
}

Napi::Function MessageWrapper::CalculateCrc14(Napi::Env env) {
    return Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        
        if (info.Length() < 1 || !info[0].IsTypedArray()) {
            Napi::TypeError::New(env, "Expected Uint8Array").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        Napi::Uint8Array data = info[0].As<Napi::Uint8Array>();
        
        // Calculate CRC-14 using the ft8_lib crc function
        uint16_t crc = ftx_compute_crc(data.Data(), data.ElementLength());
        
        return Napi::Number::New(env, crc);
    });
}

Napi::Function MessageWrapper::VerifyCrc14(Napi::Env env) {
    return Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        
        if (info.Length() < 2 || !info[0].IsTypedArray() || !info[1].IsNumber()) {
            Napi::TypeError::New(env, "Expected Uint8Array and number").ThrowAsJavaScriptException();
            return env.Null();
        }
        
        Napi::Uint8Array data = info[0].As<Napi::Uint8Array>();
        uint16_t expected_crc = info[1].As<Napi::Number>().Uint32Value() & 0xFFFF;
        
        // Calculate CRC-14 and compare
        uint16_t calculated_crc = ftx_compute_crc(data.Data(), data.ElementLength());
        
        return Napi::Boolean::New(env, calculated_crc == expected_crc);
    });
}
