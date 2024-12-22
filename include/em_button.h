#ifndef EM_BUTTON_H
#define EM_BUTTON_H

#include "em_defs.h"
#include "em_button_defs.h"
#include "em_button_event.h"

// The base button class
class EmButton: public EmUpdatable {
public:
    EmButton(EmButtonEvent* events[],
             EmBtnSize eventsCount,
             EmButtonState initialState=EmButtonState::up)
     : m_CurrentState(initialState),
       m_CurrentStateMillis(0),
       m_Events(events),
       m_EventsCount(eventsCount) { }

    // Sets the button state.
    // This method will eventually raise the defined events
    void SetState(EmButtonState state);

    // Gets the button state.
    EmButtonState GetState() const {
        return m_CurrentState;
    }

    virtual void Update() override;

    EmButtonState GetCurrentState() const {
        return m_CurrentState;
    }

    uint32_t GetCurrentStateMillis() const {
        return m_CurrentStateMillis;
    }

    EmBtnSize GetEventsCount() const {
        return m_EventsCount;
    }

    EmButtonEvent* GetEvent(EmBtnSize index) const {
        if (index < m_EventsCount) {
            return m_Events[index];
        }
        return NULL;
    }
    
protected:
    EmButtonState m_CurrentState;
    uint32_t m_CurrentStateMillis;
    EmButtonEvent** m_Events; 
    EmBtnSize m_EventsCount;
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

    virtual void Update() override;

protected:
    virtual EmButtonState _getHwState();

    uint8_t m_IoPin;
    uint8_t m_DownValue;
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

    EmTimeout m_DebouncingTimeout;
    EmButtonState m_NewState;
};

#endif