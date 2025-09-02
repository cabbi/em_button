#include "em_defs.h"
#include "em_button.h"

// The common callback -> simple print
void evCallback(EmButton& button, 
                EmButtonEvent& event,
                EmButtonState state, 
                uint32_t stateDurationMs, 
                void* text) {
    Serial.println((const char*)text);
}

// A simple push button
EmButtonPushed evPushed(evCallback, true, (void*)"Button Pushed!");
EmButtonEvent* pushBtnEvents[] = {&evPushed};
EmGpioDebounceButton pushBtn(10, pushBtnEvents, SIZE_OF(pushBtnEvents), 30, false, HIGH);

// A simple push button
EmButtonUp evUp(evCallback, true, (void*)"Button Up!");
EmButtonDown evDown(evCallback, true, (void*)"Button Down!");
EmButtonEvent* upDownBtnEvents[] = {&evUp, &evDown};
EmGpioDebounceButton upDownBtn(11, upDownBtnEvents, SIZE_OF(upDownBtnEvents), 30, false, HIGH);

// Events for sequence definition
EmButtonPushedMoreThan ev1(evCallback, 2000, true, (void*)"Long Button Push!");
EmButtonPushedLessThan ev2(evCallback, 500, true, (void*)"Short Button Push!");
EmButtonEvent* seqEvents[] = {&ev1, &ev2, &ev2};
EmButtonEventsSequence seqEv(evCallback, 
                             seqEvents, SIZE_OF(seqEvents), 
                             2000,
                             true, (void*)"Sequence completed!");

// A sequence button
EmButtonEvent* seqBtnEvents[] = {&seqEv};
EmGpioDebounceButton sequenceBtn(12, seqBtnEvents, SIZE_OF(seqBtnEvents), 30, false, HIGH);

// The updater object used to simply update all objects at once
EmUpdatable* updatableObjs[] = {&pushBtn, &upDownBtn, &sequenceBtn};
EmUpdater<updatableObjs, SIZE_OF(updatableObjs)> updater;


void setup() {
    Serial.begin(9600);
}

void loop() {
    // Updates the buttons status
    updater.update();
}
