#ifndef MESSAGE_WRAPPER_H
#define MESSAGE_WRAPPER_H

#include <napi.h>

extern "C" {
#include <ft8/message.h>
#include <ft8/crc.h>
}

/**
 * MessageWrapper class provides utilities for FT8/FT4 message handling
 * 
 * This class wraps the ft8_lib message functions and provides JavaScript-friendly
 * interfaces for message validation, type detection, parsing, and CRC operations.
 */
class MessageWrapper {
public:
    /**
     * Check if a message is valid for encoding in the specified protocol
     * @param env N-API environment
     * @return N-API function that validates messages
     */
    static Napi::Function IsValidMessage(Napi::Env env);
    
    /**
     * Determine the message type from message text
     * @param env N-API environment
     * @return N-API function that returns message type
     */
    static Napi::Function GetMessageType(Napi::Env env);
    
    /**
     * Parse a standard message format into components
     * @param env N-API environment
     * @return N-API function that parses messages
     */
    static Napi::Function ParseStandardMessage(Napi::Env env);
    
    /**
     * Calculate CRC-14 checksum
     * @param env N-API environment
     * @return N-API function that calculates CRC
     */
    static Napi::Function CalculateCrc14(Napi::Env env);
    
    /**
     * Verify CRC-14 checksum
     * @param env N-API environment
     * @return N-API function that verifies CRC
     */
    static Napi::Function VerifyCrc14(Napi::Env env);

private:
    /**
     * Convert ftx_message_type_t enum to string
     * @param type The message type enum value
     * @return String representation of the message type
     */
    static const char* MessageTypeToString(ftx_message_type_t type);
    
    /**
     * Convert ftx_message_rc_t enum to string
     * @param rc The message return code enum value
     * @return String representation of the return code
     */
    static const char* MessageRcToString(ftx_message_rc_t rc);
};

#endif // MESSAGE_WRAPPER_H
