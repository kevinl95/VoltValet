/**
 * @brief This code shows how to read the J1772 pilot wave PWM waveform
 */

#include <Arduino.h>
#include <led_ctrl.h>
#include <log.h>

int pin = 19;
unsigned long durationHigh;
unsigned long durationLow;

void setup() {

    LedCtrl.begin();
    LedCtrl.startupCycle();

    Log.begin(115200);
    pinConfigure(PIN_PA4, PIN_DIR_INPUT);
}

void loop() {
  durationHigh = pulseIn(PIN_PA4, HIGH);
  durationLow = pulseIn(PIN_PA4, LOW);
  Log.raw(String(durationHigh));
  Log.raw(String(durationLow));
}
