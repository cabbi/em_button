#ifndef EM_BUTTON_DEFS_H
#define EM_BUTTON_DEFS_H

#include <stdint.h>

// Forward declarations
class EmButton;
class EmButtonEvent;

// The button & events size type
typedef uint8_t EmBtnSize;

// Button states
enum class EmButtonState: uint8_t {    
    up = 0,
    down = 1,
};

// The time event type definition
enum class EmButtonTimeEvent: uint8_t {
    LessThan = 0,
    MoreThan = 1,
};

// The event function callback 
typedef void (*EmButtonEventCallback)(EmButton& button, 
                                      EmButtonEvent& event,
                                      EmButtonState state, 
                                      uint32_t stateDurationMs,
                                      void* pUserData);

// The null callback (might be used to define events used in 'EmButtonEventsSequence')
inline void NullButtonEventCallback(EmButton& button, 
                                    EmButtonEvent& event,
                                    EmButtonState state, 
                                    uint32_t stateDurationMs,
                                    void* pUserData) {}

#endif