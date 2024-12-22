#include "em_defs.h"
#include "em_button.h"
    
void EmButtonDown::UpdateButtonState(EmButton& button,
                                     uint32_t oldStateMillis,
                                     EmButtonState oldState,
                                     EmButtonState newState) 
{
    if (oldState != newState && newState == EmButtonState::down) {
        m_Callback(button, *this, newState, oldStateMillis, m_CallbackUserData);
    }
}

void EmButtonUp::UpdateButtonState(EmButton& button,
                                   uint32_t oldStateMillis,
                                   EmButtonState oldState,
                                   EmButtonState newState) 
{
    if (oldState != newState && newState == EmButtonState::up) {
        m_Callback(button, *this, newState, oldStateMillis, m_CallbackUserData);
    }
}

void EmButtonPushed::UpdateButtonState(EmButton& button,
                                       uint32_t oldStateMillis,
                                       EmButtonState oldState,
                                       EmButtonState newState) 
{
    if (oldState != newState && newState == EmButtonState::down) {
        m_WasDown = true;
    }
    if (m_WasDown  && oldState != newState && newState == EmButtonState::up) {
        m_WasDown = false;
        m_Callback(button, *this, newState, oldStateMillis, m_CallbackUserData);
    }
}

void EmButtonSteadyMoreThan::UpdateButtonState(EmButton& button,
                                               uint32_t oldStateMillis,
                                               EmButtonState oldState,
                                               EmButtonState newState) 
{
    if (oldState == newState) {
        if (m_EventTimeout.IsElapsed(true)) {
            m_EventRaised = true;
            m_Callback(button, *this, oldState, oldStateMillis, m_CallbackUserData);
        }
    } else {
        m_EventRaised = false;
        m_EventTimeout.Restart();
    }
}

void EmButtonEventsSequence::Reset() {
    // Reset by restarting the sequence
    _moveTo(0);
}

void EmButtonEventsSequence::UpdateButtonState(EmButton& button,
                                               uint32_t oldStateMillis,
                                               EmButtonState oldState,
                                               EmButtonState newState) 
{
    // Check if step timeout is elapsed (not valid for first step!)
    if (!_isFirst() && m_StepTimeoutMillis.IsElapsed(false)) {
        Reset();
    }
    // Update the current step event state
    m_Events[m_CurrentStep]->UpdateButtonState(button, 
                                               oldStateMillis,
                                               oldState,
                                               newState);
}

void EmButtonEventsSequence::_moveNext()
{
    // Set next step index    
    _moveTo(_isLast() ? 0 : m_CurrentStep+1);
}

void EmButtonEventsSequence::_moveTo(EmBtnSize step)
{
    // Restore current event to original callback if any
    m_Events[m_CurrentStep]->SetCallback(m_CurrentStepCallback, m_CurrentStepCallbackData);

    // Disable all events 
    // NOTE: we enable current only after this loop in case the  
    //       sequence contains same event instance multiple times 
    for (EmBtnSize i=0; i < m_EventsCount; i++) {
        m_Events[i]->SetEnabled(false);
    }
    
    // Set and enable current step (see NOTE above)
    m_CurrentStep = MIN(step, m_EventsCount-1);
    m_Events[m_CurrentStep]->SetEnabled(true);

    // Replace the user defined callback if this is the current event
    m_CurrentStepCallback = m_Events[m_CurrentStep]->GetCallback();
    m_CurrentStepCallbackData = m_Events[m_CurrentStep]->GetCallbackUserData();
    m_Events[m_CurrentStep]->SetCallback(&EmButtonEventsSequence::_eventCallback, this);

    // Restart the step timeout
    m_StepTimeoutMillis.Restart();
}

void EmButtonEventsSequence::_eventCallback(EmButton& button, 
                                            EmButtonEvent& event,
                                            EmButtonState state, 
                                            uint32_t stateDurationMs,
                                            void* pEvent)
{
    EmButtonEventsSequence* thisEvent = static_cast<EmButtonEventsSequence*>(pEvent);
    // Any defined event callback
    if (thisEvent->m_CurrentStepCallback != NULL) {
        thisEvent->m_CurrentStepCallback(button, 
                                         event, 
                                         state, 
                                         stateDurationMs, 
                                         thisEvent->m_CurrentStepCallbackData);
    }
    // Sequence is completed, lets call final event callback
    if (thisEvent->_isLast()) {
        thisEvent->m_Callback(button, 
                              event, 
                              state, 
                              stateDurationMs, 
                              thisEvent->m_CallbackUserData);
    }
    // Move to next event in the sequence
    thisEvent->_moveNext();
}
