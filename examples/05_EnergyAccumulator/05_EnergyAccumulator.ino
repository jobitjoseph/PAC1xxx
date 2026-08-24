/*
 * PAC1xxx - Energy accumulator and coulomb counting
 *
 * The part sums every sample in hardware, so total energy and charge are exact
 * over the whole window rather than a sum of what the MCU happened to catch.
 * That is the reason to use this part over reading current in a loop.
 *
 * The accumulator sums one quantity at a time, chosen with setAccumulateMode():
 *   PAC1XXX_ACC_POWER -> energy_mWh()
 *   PAC1XXX_ACC_SENSE -> charge_mAs() and charge_mAh()
 *
 * refresh() latches the accumulator and resets it, so the value you read is
 * the total since the previous refresh(). Reading voltage or current in
 * between is safe: auto-refresh uses REFRESH_V, which latches without
 * clearing the accumulator.
 *
 * Copyright (c) 2026 Jobit Joseph - MIT License
 * https://github.com/jobitjoseph/PAC1xxx
 */

#include <PAC1xxx.h>

#define PAC_ADDRESS        0x40
#define SHUNT_MILLIOHMS    10.0f
#define WINDOW_MS          5000UL

PAC1xxx pac;

float totalEnergy_mWh = 0.0f;
unsigned long windowStart = 0;

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

  if (!pac.setAccumulateMode(PAC1XXX_ACC_POWER)) {
    Serial.print(F("Could not set accumulation mode: "));
    Serial.println(pac.lastErrorString());
  }

  // Apply the configuration and start the first window from a clean slate.
  pac.refresh();
  windowStart = millis();

  Serial.println(F("Accumulating energy over 5 second windows..."));
}

void loop() {
  if (millis() - windowStart < WINDOW_MS) {
    delay(50);
    return;
  }

  // Latch and reset: everything read now covers the window just closed.
  uint32_t samples = pac.accumulatorCount();
  float    window  = pac.energy_mWh();

  pac.refresh();
  windowStart = millis();

  if (isnan(window)) {
    Serial.print(F("Accumulator read failed: "));
    Serial.println(pac.lastErrorString());
    return;
  }

  totalEnergy_mWh += window;

  Serial.print(F("samples "));
  Serial.print(samples);
  Serial.print(F("   window "));
  Serial.print(window, 5);
  Serial.print(F(" mWh   total "));
  Serial.print(totalEnergy_mWh, 4);
  Serial.println(F(" mWh"));
}
