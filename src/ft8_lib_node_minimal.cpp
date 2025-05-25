#include <napi.h>

/**
 * Minimal test addon to debug hanging issues
 */
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // Just export a simple function to test
    auto testFunction = Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        return Napi::String::New(env, "Test function working");
    });
    
    exports.Set("test", testFunction);
    
    // Test Utils object
    Napi::Object utils = Napi::Object::New(env);
    
    auto simpleUtil = Napi::Function::New(env, [](const Napi::CallbackInfo& info) -> Napi::Value {
        Napi::Env env = info.Env();
        return Napi::Boolean::New(env, true);
    });
    
    utils.Set("simpleUtil", simpleUtil);
    exports.Set("Utils", utils);
    
    return exports;
}

// Initialize the addon
NODE_API_MODULE(ft8_lib, Init)
