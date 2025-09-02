#ifndef EM_BUTTON_H
#define EM_BUTTON_H

#include "em_defs.h"
#include "em_threading.h"
#include "em_button_defs.h"
#include "em_button_event.h"

// The base button class
class EmButton: public EmUpdatable {
public:
    EmButton(EmButtonEvent* events[],
             EmBtnSize eventsCount,
             EmButtonState initialState=EmButtonState::up)
     : m_currentState(initialState),
       m_currentStateMillis(0),
       m_events(events),
       m_eventsCount(eventsCount) { }

    // Sets the button state.
    // This method will eventually raise the defined events
    void setState(EmButtonState state);

    // Gets the button state.
    EmButtonState getState() const {
        return m_currentState;
    }

    virtual void update() override;

    EmButtonState getCurrentState() const {
        return m_currentState;
    }

    uint32_t getCurrentStateMillis() const {
        return m_currentStateMillis;
    }

    EmBtnSize getEventsCount() const {
        return m_eventsCount;
    }

    EmButtonEvent* getEvent(EmBtnSize index) const {
        if (index < m_eventsCount) {
            return m_events[index];
        }
        return NULL;
    }
    
protected:
    EmButtonState m_currentState;
    ts_uint32 m_currentStateMillis;
    EmButtonEvent** m_events; 
    EmBtnSize m_eventsCount;
};

// The button linked to an hardware gpio port.
//
// If your button circuit is not using a debouncing technic you 
// better use the 'EmGpioDebounceButton' class.
class EmGpioButton: public EmButton {
public:
    EmGpioButton(uint8_t ioPin,
                 EmButtonEvent* events[],
                 EmBtnSize eventsCount,
                 bool inputBuildinPullUp = true, 
                 uint8_t downValue = LOW);

    virtual void update() override;

protected:
    virtual EmButtonState _getHwState();

    uint8_t m_ioPin;
    uint8_t m_downValue;
};

// The button linked to an hardware GPIO port using software debouncing.
class EmGpioDebounceButton: public EmGpioButton {
public:
    EmGpioDebounceButton(uint8_t ioPin,
                         EmButtonEvent* events[],
                         EmBtnSize eventsCount,
                         uint16_t debouncingMillis = 50,
                         bool inputBuildinPullUp = true, 
                         uint8_t downValue = LOW);

protected:
    virtual EmButtonState _getHwState() override;

    EmTimeout m_debouncingTimeout;
    EmButtonState m_newState;
};

#endif