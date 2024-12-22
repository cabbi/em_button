#ifndef EM_BUTTON_EVENT_H
#define EM_BUTTON_EVENT_H

#include "em_timeout.h"
#include "em_button_defs.h"

// The base abstract button event class
class EmButtonEvent {
public:
    EmButtonEvent(EmButtonEventCallback callback,
                  bool enabled=true,
                  void* callbackUserData=NULL)
     : m_Callback(callback),
       m_CallbackUserData(callbackUserData),
       m_IsEnabled(enabled) {}

    bool IsEnabled() const {
        return m_IsEnabled;
    }

    void SetEnabled(bool enabled) {
        m_IsEnabled = enabled;
    }

    EmButtonEventCallback GetCallback() const {
        return m_Callback;
    }
    
    void* GetCallbackUserData() const {
        return m_CallbackUserData;
    }

    void SetCallback(EmButtonEventCallback callback,
                     void* callbackUserData=NULL) { 
        m_Callback = callback;
        m_CallbackUserData = callbackUserData;
    }

    // A button calls this method on each 'Update' call so that event
    // can react even if state did not change (i.e. oldState == newState)
    virtual void UpdateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) = 0;

protected:
    EmButtonEventCallback m_Callback;
    void* m_CallbackUserData;
    bool m_IsEnabled;
};

// The button down event class
//
// This event is raised when the button moves from 'up' to 'down' state
class EmButtonDown: public EmButtonEvent {
public:
    EmButtonDown(EmButtonEventCallback callback,
                 bool enabled=true,
                 void* callbackUserData=NULL) 
     : EmButtonEvent(callback, enabled, callbackUserData) {}
    
    virtual void UpdateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override;
};

// The button up event class
//
// This event is raised when the button moves from 'down' to 'up' state
class EmButtonUp: public EmButtonEvent {
public:
    EmButtonUp(EmButtonEventCallback callback,
               bool enabled=true,
               void* callbackUserData=NULL) 
     : EmButtonEvent(callback, enabled, callbackUserData) {}

    virtual void UpdateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override;
};

// The button pushed event class
//
// This event is raised when the button has beeing pressed 'up' to 'down' and back to 'up' state
class EmButtonPushed: public EmButtonEvent {
public:
    EmButtonPushed(EmButtonEventCallback callback,
                   bool enabled=true,
                   void* callbackUserData=NULL) 
     : EmButtonEvent(callback, enabled, callbackUserData),
       m_WasDown(false) {}

    virtual void UpdateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override;

protected:
    bool m_WasDown;
};

// The events sequence class.
//
// A specified sequence of events should be meet in order to raise the callback.
// if intermediate callbacks are not needed, sequence events can have NULL or
//  'NullButtonEventCallback' as callback. 
//
// Once the first event is meet then 'stepTimeoutMillis' for each of the oder steps is applied.
// This means that if a step is not met within this timeout, the sequence is reset waiting
// for the first event in the sequence to be met.
class EmButtonEventsSequence: public EmButtonEvent {
public:
    EmButtonEventsSequence(EmButtonEventCallback callback,
                           EmButtonEvent* events[],
                           EmBtnSize eventsCount,
                           uint32_t stepTimeoutMillis,
                           bool enabled=true,
                           void* callbackUserData=NULL) 
     : EmButtonEvent(callback, enabled, callbackUserData),
       m_Events(events),
       m_EventsCount(eventsCount), 
       m_CurrentStep(0),
       m_CurrentStepCallback(m_Events[0]->GetCallback()),
       m_CurrentStepCallbackData(m_Events[0]->GetCallbackUserData()),
       m_StepTimeoutMillis(stepTimeoutMillis) {
        Reset();
       }

    // Resets the sequence by awaiting for the first one to be completed
    void Reset();

    virtual void UpdateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override;

    EmBtnSize GetCurrentStep() const {
        return m_CurrentStep;
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
    bool _isLast() {
        return m_CurrentStep >= m_EventsCount-1; 
    }
    bool _isFirst() {
        return m_CurrentStep == 0; 
    }
    void _moveNext();
    void _moveTo(EmBtnSize step);
    static void _eventCallback(EmButton& button, 
                               EmButtonEvent& event,
                               EmButtonState state, 
                               uint32_t stateDurationMs,
                               void* pEvent);
    
    // Member vars
    EmButtonEvent** m_Events; 
    EmBtnSize m_EventsCount;
    EmBtnSize m_CurrentStep;
    EmButtonEventCallback m_CurrentStepCallback;
    void* m_CurrentStepCallbackData;
    EmTimeout m_StepTimeoutMillis;
};

// The abstract timed button event class
class EmButtonTimedEvent: public EmButtonEvent {
public:
    EmButtonTimedEvent(EmButtonEventCallback callback,
                       uint32_t eventDurationMillis,
                       bool enabled=true,
                       void* callbackUserData=NULL) 
     : EmButtonEvent(callback, enabled, callbackUserData), 
       m_EventTimeout(eventDurationMillis) {}

    void SetEnabled(bool enabled, bool restart) {
        m_IsEnabled = enabled;
        if (restart) {
            m_EventTimeout.Restart();
        }
    }

    void Restart() {
        m_EventTimeout.Restart();
    }

    virtual void SetDuration(uint32_t stateDurationMillis, bool restart=true) {
        m_EventTimeout.SetTimeout(stateDurationMillis, restart);
    }

    virtual uint32_t GetDurationMillis() {
        return m_EventTimeout.GetTimeoutMs();
    }

protected:
    EmTimeout m_EventTimeout;
};

// The generic timed button event class
//
// This is the base class for the events that react after a certain amount of time
// being on a specified state. For single state events only MoreThan makes sense, 
template <EmButtonState eventState>
class EmButtonStateTimedEvent: public EmButtonTimedEvent {
public:
    EmButtonStateTimedEvent(EmButtonEventCallback callback,
                            uint32_t stateDurationMillis,
                            bool enabled=true,
                            void* callbackUserData=NULL) 
     : EmButtonTimedEvent(callback, stateDurationMillis, enabled, callbackUserData),
       m_WasEventState(false),
       m_EventRaised(false) {}

    virtual void UpdateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override {
        if (newState != eventState) {
            m_WasEventState = false;
            m_EventRaised = false;
        } else
        if (!m_WasEventState && oldState != newState) {
            m_WasEventState = true;
            m_EventTimeout.Restart();
        } else
        if (!m_EventRaised && m_WasEventState && oldState == newState) {
            if (m_EventTimeout.IsElapsed(false)) {
                m_EventRaised = true;
                m_Callback(button, *this, oldState, oldStateMillis);
            }
        } 
    }

protected:
    bool m_WasEventState;
    bool m_EventRaised;
};

class EmButtonDownMoreThan: public EmButtonStateTimedEvent<EmButtonState::down> {
public:
    using EmButtonStateTimedEvent::EmButtonStateTimedEvent;
};

class EmButtonUpMoreThan: public EmButtonStateTimedEvent<EmButtonState::up> {
public:
    using EmButtonStateTimedEvent::EmButtonStateTimedEvent;
};

// The button pushed for "more than" or "less than" than event class.
//
// This event is raised when the button has beeing pressed 'up' to 'down' 
// for "more" or "less" than specified time and then back to 'up' state
template <EmButtonTimeEvent eventType>
class EmButtonPushedTimedEvent: public EmButtonTimedEvent {
public:
    EmButtonPushedTimedEvent(EmButtonEventCallback callback,
                             uint32_t eventDurationMillis,
                             bool enabled=true,
                             void* callbackUserData=NULL) 
     : EmButtonTimedEvent(callback, eventDurationMillis, enabled, callbackUserData),
       m_WasDown(false) {}

    virtual void UpdateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override {
        if (!m_WasDown && oldState != newState && newState == EmButtonState::down) {
            m_WasDown = true;
            m_EventTimeout.Restart();
        } else
        if (m_WasDown && oldState != newState && newState == EmButtonState::up) {
            m_WasDown = false;
            if ((eventType == EmButtonTimeEvent::MoreThan && m_EventTimeout.IsElapsed(false)) ||
                (eventType == EmButtonTimeEvent::LessThan && !m_EventTimeout.IsElapsed(false))) {
                m_Callback(button, *this, oldState, oldStateMillis, m_CallbackUserData);
            }
        }
    }

protected:
    bool m_WasDown;
};

class EmButtonPushedMoreThan: public EmButtonPushedTimedEvent<EmButtonTimeEvent::MoreThan> {
public:
    using EmButtonPushedTimedEvent::EmButtonPushedTimedEvent;
};

class EmButtonPushedLessThan: public EmButtonPushedTimedEvent<EmButtonTimeEvent::LessThan> {
public:
    using EmButtonPushedTimedEvent::EmButtonPushedTimedEvent;
};

// The button steady (i.e. no change) event class
//
// This event is mainly used in application where waiting for a period of time without
// button activity (e.g. exiting from a setup mode after 3 seconds of inactivity)
class EmButtonSteadyMoreThan: public EmButtonTimedEvent {
public:
    EmButtonSteadyMoreThan(EmButtonEventCallback callback,
                           uint32_t inactivityDurationMillis,                           
                           bool enabled=true,
                           void* callbackUserData=NULL) 
     : EmButtonTimedEvent(callback, inactivityDurationMillis, enabled, callbackUserData),
       m_EventRaised(false) {}

    virtual void UpdateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override;

protected:
    bool m_EventRaised;
};

#endif