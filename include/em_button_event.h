#ifndef EM_BUTTON_EVENT_H
#define EM_BUTTON_EVENT_H

#include "em_timeout.h"
#include "em_button_defs.h"

// The base abstract button event class
class EmButtonEvent {
public:
    EmButtonEvent(const EmButtonEventCallback callback,
                  bool enabled=true,
                  void* callbackUserData=NULL)
     : m_callback(callback),
       m_callbackUserData(callbackUserData),
       m_isEnabled(enabled) {}

    bool isEnabled() const {
        return m_isEnabled;
    }

    void setEnabled(bool enabled) {
        m_isEnabled = enabled;
    }

    EmButtonEventCallback getCallback() const {
        return m_callback;
    }
    
    void* getCallbackUserData() const {
        return m_callbackUserData;
    }

    void setCallback(EmButtonEventCallback callback,
                     void* callbackUserData=NULL) {
        m_callback = callback;
        m_callbackUserData = callbackUserData;
    }

    // A button calls this method on each 'update' call so that event
    // can react even if state did not change (i.e. oldState == newState)
    virtual void updateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) = 0;

protected:
    EmButtonEventCallback m_callback;
    void* m_callbackUserData;
    bool m_isEnabled;
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
    
    virtual void updateButtonState(EmButton& button,
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

    virtual void updateButtonState(EmButton& button,
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
       m_wasDown(false) {}

    virtual void updateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override;

protected:
    bool m_wasDown;
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
       m_events(events),
       m_eventsCount(eventsCount), 
       m_currentStep(0),
       m_currentStepCallback(m_events[0]->getCallback()),
       m_currentStepCallbackData(m_events[0]->getCallbackUserData()),
       m_stepTimeoutMillis(stepTimeoutMillis) {
        reset();
       }

    // Resets the sequence by awaiting for the first one to be completed
    void reset();

    virtual void updateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override;

    EmBtnSize getCurrentStep() const {
        return m_currentStep;
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
    bool isLast_() {
        return m_currentStep >= m_eventsCount-1; 
    }
    bool isFirst_() {
        return m_currentStep == 0; 
    }
    void moveNext_();
    void moveTo_(EmBtnSize step);
    static void eventCallback_(EmButton& button, 
                               EmButtonEvent& event,
                               EmButtonState state, 
                               uint32_t stateDurationMs,
                               void* pEvent);
    
    // Member vars
    EmButtonEvent** m_events; 
    EmBtnSize m_eventsCount;
    EmBtnSize m_currentStep;
    EmButtonEventCallback m_currentStepCallback;
    void* m_currentStepCallbackData;
    EmTimeout m_stepTimeoutMillis;
};

// The abstract timed button event class
class EmButtonTimedEvent: public EmButtonEvent {
public:
    EmButtonTimedEvent(EmButtonEventCallback callback,
                       uint32_t eventDurationMillis,
                       bool enabled=true,
                       void* callbackUserData=NULL) 
     : EmButtonEvent(callback, enabled, callbackUserData), 
       m_eventTimeout(eventDurationMillis) {}

    void setEnabled(bool enabled, bool restart) {
        m_isEnabled = enabled;
        if (restart) {
            m_eventTimeout.restart();
        }
    }

    void restart() {
        m_eventTimeout.restart();
    }

    virtual void setDuration(uint32_t stateDurationMillis, bool restart=true) {
        m_eventTimeout.setTimeout(stateDurationMillis, restart);
    }

    virtual uint32_t getDurationMillis() {
        return m_eventTimeout.getTimeoutMs();
    }

protected:
    EmTimeout m_eventTimeout;
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
       m_wasEventState(false),
       m_eventRaised(false) {}

    virtual void updateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override {
        if (newState != eventState) {
            m_wasEventState = false;
            m_eventRaised = false;
        } else
        if (!m_wasEventState && oldState != newState) {
            m_wasEventState = true;
            m_eventTimeout.restart();
        } else
        if (!m_eventRaised && m_wasEventState && oldState == newState) {
            if (m_eventTimeout.isElapsed(false)) {
                m_eventRaised = true;
                m_callback(button, *this, oldState, oldStateMillis, m_callbackUserData);
            }
        } 
    }

protected:
    bool m_wasEventState;
    bool m_eventRaised;
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
       m_wasDown(false) {}

    virtual void updateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override {
        if (!m_wasDown && oldState != newState && newState == EmButtonState::down) {
            m_wasDown = true;
            m_eventTimeout.restart();
        } else
        if (m_wasDown && oldState != newState && newState == EmButtonState::up) {
            m_wasDown = false;
            if ((eventType == EmButtonTimeEvent::MoreThan && m_eventTimeout.isElapsed(false)) ||
                (eventType == EmButtonTimeEvent::LessThan && !m_eventTimeout.isElapsed(false))) {
                m_callback(button, *this, oldState, oldStateMillis, m_callbackUserData);
            }
        }
    }

protected:
    bool m_wasDown;
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
       m_eventRaised(false) {}

    virtual void updateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) override;

protected:
    bool m_eventRaised;
};

#endif