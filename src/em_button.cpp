#include <Arduino.h>

#include "em_defs.h"

#include "em_button.h"
#include "em_button_event.h"


void EmButton::setState(EmButtonState state)
{
    uint32_t nowMillis = millis();
    uint32_t elapsedMillis = static_cast<uint32_t>(nowMillis - m_currentStateMillis);
    for (int8_t i=0; i < m_eventsCount; i++) {
        if (m_events[i]->isEnabled()) {
            m_events[i]->updateButtonState(*this,
                                           elapsedMillis, 
                                           m_currentState,
                                           state);
        }
    }
    if (m_currentState != state) {
        m_currentState = state;
        m_currentStateMillis = nowMillis;
    }
}

void EmButton::update() 
{
    // Simply update this button events with the current state
    setState(m_currentState);
}

EmGpioButton::EmGpioButton(uint8_t ioPin,
                           EmButtonEvent* events[],
                           EmBtnSize eventsCount,
                           bool inputBuildinPullUp, 
                           uint8_t downValue)
 : EmButton(events, eventsCount, EmButtonState::up),
   m_ioPin(ioPin),
   m_downValue(downValue)
{
    pinMode(m_ioPin, inputBuildinPullUp ? INPUT_PULLUP : INPUT); 
}

void EmGpioButton::update() {
    setState(_getHwState());
    EmButton::update();
}

EmButtonState EmGpioButton::_getHwState() {
    return digitalRead(m_ioPin) == m_downValue
           ? EmButtonState::down : EmButtonState::up;
}

EmGpioDebounceButton::EmGpioDebounceButton(uint8_t ioPin,
                                           EmButtonEvent* events[],
                                           EmBtnSize eventsCount,
                                           uint16_t debouncingMillis,
                                           bool inputBuildinPullUp, 
                                           uint8_t downValue)
 : EmGpioButton(ioPin, events, eventsCount, inputBuildinPullUp, downValue),
   m_debouncingTimeout(debouncingMillis),
   m_newState(EmButtonState::up)
{
}

EmButtonState EmGpioDebounceButton::_getHwState() {
    EmButtonState newState = EmGpioButton::_getHwState();
    if (m_newState != newState) {
        m_newState = newState;
        m_debouncingTimeout.restart();
    }
    return m_debouncingTimeout.isElapsed(false) ? m_newState : m_currentState;
}
