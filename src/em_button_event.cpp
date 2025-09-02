#include "em_defs.h"
#include "em_button.h"
    
void EmButtonDown::updateButtonState(EmButton& button,
                                     uint32_t oldStateMillis,
                                     EmButtonState oldState,
                                     EmButtonState newState) 
{
    if (oldState != newState && newState == EmButtonState::down) {
        m_callback(button, *this, newState, oldStateMillis, m_callbackUserData);
    }
}

void EmButtonUp::updateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) 
{
    if (oldState != newState && newState == EmButtonState::up) {
        m_callback(button, *this, newState, oldStateMillis, m_callbackUserData);
    }
}

void EmButtonPushed::updateButtonState(EmButton& button,
                                       uint32_t oldStateMillis,
                                       EmButtonState oldState,
                                       EmButtonState newState) 
{
    if (oldState != newState && newState == EmButtonState::down) {
        m_wasDown = true;
    }
    if (m_wasDown  && oldState != newState && newState == EmButtonState::up) {
        m_wasDown = false;
        m_callback(button, *this, newState, oldStateMillis, m_callbackUserData);
    }
}

void EmButtonSteadyMoreThan::updateButtonState(EmButton& button,
                                               uint32_t oldStateMillis,
                                               EmButtonState oldState,
                                               EmButtonState newState) 
{
    if (oldState == newState) {
        if (m_eventTimeout.isElapsed(true)) {
            m_eventRaised = true;
            m_callback(button, *this, oldState, oldStateMillis, m_callbackUserData);
        }
    } else {
        m_eventRaised = false;
        m_eventTimeout.restart();
    }
}

void EmButtonEventsSequence::reset() {
    // Reset by restarting the sequence
    moveTo_(0);
}

void EmButtonEventsSequence::updateButtonState(EmButton& button,
                                               uint32_t oldStateMillis,
                                               EmButtonState oldState,
                                               EmButtonState newState) 
{
    // Check if step timeout is elapsed (not valid for first step!)
    if (!isFirst_() && m_stepTimeoutMillis.isElapsed(false)) {
        reset();
    }
    // Update the current step event state
    m_events[m_currentStep]->updateButtonState(button, 
                                               oldStateMillis,
                                               oldState,
                                               newState);
}

void EmButtonEventsSequence::moveNext_()
{
    // Set next step index    
    moveTo_(isLast_() ? 0 : m_currentStep+1);
}

void EmButtonEventsSequence::moveTo_(EmBtnSize step)
{
    // Restore current event to original callback if any
    m_events[m_currentStep]->setCallback(m_currentStepCallback, m_currentStepCallbackData);

    // Disable all events 
    // NOTE: we enable current only after this loop in case the  
    //       sequence contains same event instance multiple times 
    for (EmBtnSize i=0; i < m_eventsCount; i++) {
        m_events[i]->setEnabled(false);
    }
    
    // Set and enable current step (see NOTE above)
    m_currentStep = MIN(step, m_eventsCount-1);
    m_events[m_currentStep]->setEnabled(true);

    // Replace the user defined callback if this is the current event
    m_currentStepCallback = m_events[m_currentStep]->getCallback();
    m_currentStepCallbackData = m_events[m_currentStep]->getCallbackUserData();
    m_events[m_currentStep]->setCallback(&EmButtonEventsSequence::eventCallback_, this);

    // Restart the step timeout
    m_stepTimeoutMillis.restart();
}

void EmButtonEventsSequence::eventCallback_(EmButton& button, 
                                            EmButtonEvent& event,
                                            EmButtonState state, 
                                            uint32_t stateDurationMs,
                                            void* pEvent)
{
    EmButtonEventsSequence* thisEvent = static_cast<EmButtonEventsSequence*>(pEvent);
    // Any defined event callback
    if (thisEvent->m_currentStepCallback != NULL) {
        thisEvent->m_currentStepCallback(button, 
                                         event, 
                                         state, 
                                         stateDurationMs, 
                                         thisEvent->m_currentStepCallbackData);
    }
    // Sequence is completed, lets call final event callback
    if (thisEvent->isLast_()) {
        thisEvent->m_callback(button, 
                              event, 
                              state, 
                              stateDurationMs, 
                              thisEvent->m_callbackUserData);
    }
    // Move to next event in the sequence
    thisEvent->moveNext_();
}
