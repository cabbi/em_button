#include <Arduino.h>

#include "em_defs.h"

#include "em_button.h"
#include "em_button_event.h"


void EmButton::SetState(EmButtonState state)
{
    uint32_t nowMillis = millis();
    uint32_t elapsedMillis = static_cast<uint32_t>(nowMillis - m_CurrentStateMillis);
    for (int8_t i=0; i < m_EventsCount; i++) {
        if (m_Events[i]->IsEnabled()) {
            m_Events[i]->UpdateButtonState(*this,
                                           elapsedMillis, 
                                           m_CurrentState,
                                           state);
        }
    }
    if (m_CurrentState != state) {
        m_CurrentState = state;
        m_CurrentStateMillis = nowMillis;
    }
}

void EmButton::Update() 
{
    // Simply update this button events with the current state
    SetState(m_CurrentState);
}

EmGpioButton::EmGpioButton(uint8_t ioPin,
                           EmButtonEvent* events[],
                           EmBtnSize eventsCount,
                           bool inputBuildinPullUp, 
                           uint8_t downValue)
 : EmButton(events, eventsCount, EmButtonState::up),
   m_IoPin(ioPin),
   m_DownValue(downValue)
{
    pinMode(m_IoPin, inputBuildinPullUp ? INPUT_PULLUP : INPUT); 
}

void EmGpioButton::Update() {
    SetState(_getHwState());
    EmButton::Update();
}

EmButtonState EmGpioButton::_getHwState() {
    return digitalRead(m_IoPin) == m_DownValue
           ? EmButtonState::down : EmButtonState::up;
}

EmGpioDebounceButton::EmGpioDebounceButton(uint8_t ioPin,
                                           EmButtonEvent* events[],
                                           EmBtnSize eventsCount,
                                           uint16_t debouncingMillis,
                                           bool inputBuildinPullUp, 
                                           uint8_t downValue)
 : EmGpioButton(ioPin, events, eventsCount, inputBuildinPullUp, downValue),
   m_DebouncingTimeout(debouncingMillis),
   m_NewState(EmButtonState::up)
{
}

EmButtonState EmGpioDebounceButton::_getHwState() {
    EmButtonState newState = EmGpioButton::_getHwState();
    if (m_NewState != newState) {
        m_NewState = newState;
        m_DebouncingTimeout.Restart();
    }
    return m_DebouncingTimeout.IsElapsed(false) ? m_NewState : m_CurrentState;
}
