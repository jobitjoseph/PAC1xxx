/*
 * PAC1xxx - Non-blocking operation
 *
 * In synchronous mode every call waits for the I2C transfer to finish. At
 * 100 kHz that is a few hundred microseconds, which is fine for most sketches
 * but not if you are servicing something time-critical in the same loop.
 *
 * In asynchronous mode a request returns PAC1xxx_REQUEST_PENDING immediately
 * and you drive it forward by calling task() until busy() goes false. This
 * example uses the raw HAL calls, because the C++ convenience getters are
 * built around returning a finished value.
 *
 * Note that auto-refresh is switched off when you go asynchronous: you own the
 * refresh timing, including the 1 ms settling wait before reading.
 *
 * Copyright (c) 2026 Jobit Joseph - MIT License
 * https://github.com/jobitjoseph/PAC1xxx
 */

#include <PAC1xxx.h>

#define PAC_ADDRESS       0x40
#define SHUNT_MILLIOHMS   10.0f

PAC1xxx pac;

enum State { IDLE, REFRESHING, SETTLING, READING };
State state = IDLE;

unsigned long settleStart = 0;
unsigned long nextSample  = 0;
float voltage_mV = 0.0f;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { }

  Wire.begin();

  if (!pac.begin(PAC_ADDRESS)) {
    Serial.print(F("PAC1xxx not found: "));
    Serial.println(pac.lastErrorString());
    while (1) { delay(1000); }
  }

  pac.setShuntMilliOhms(SHUNT_MILLIOHMS);
  pac.setSyncMode(false);

  Serial.println(F("Running non-blocking. The loop never waits on I2C."));
}

void loop() {
  // Whatever else your sketch needs to do stays responsive here.

  // Keep the library state machine turning.
  if (pac.busy()) {
    pac.task();
  }

  switch (state) {
    case IDLE:
      if (millis() >= nextSample) {
        PAC1xxx_RefreshV(pac.hal());
        state = REFRESHING;
      }
      break;

    case REFRESHING:
      if (!pac.busy()) {
        settleStart = millis();
        state = SETTLING;
      }
      break;

    case SETTLING:
      // The datasheet asks for 1 ms before the latched data is stable.
      if (millis() - settleStart >= 2) {
        PAC1xxx_GetVBUS_mV(pac.hal(), &voltage_mV);
        state = READING;
      }
      break;

    case READING:
      if (!pac.busy()) {
        PAC1xxx_EVENT event;
        int16_t processError;
        pac.eventStatus(event, processError);

        if (event == PAC1xxx_EVENT_REQUEST_SUCCESS) {
          Serial.print(voltage_mV / 1000.0f, 3);
          Serial.println(F(" V"));
        } else {
          Serial.print(F("Request failed, error "));
          Serial.println(processError);
        }

        nextSample = millis() + 1000;
        state = IDLE;
      }
      break;
  }
}
