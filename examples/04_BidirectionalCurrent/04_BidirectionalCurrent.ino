/*
 * PAC1xxx - Bidirectional (bipolar) current
 *
 * By default the part measures current in one direction only. A battery that
 * both charges and discharges needs bipolar mode, where the sense channel is
 * signed and readings can go negative.
 *
 * The trade-off: bipolar full range spends one code bit on the sign, so the
 * resolution in each direction halves. PAC1XXX_BIPOLAR_HALF keeps the
 * resolution and halves the range instead, which is the better pick when you
 * know the current never approaches full scale.
 *
 * Copyright (c) 2026 Jobit Joseph - MIT License
 * https://github.com/jobitjoseph/PAC1xxx
 */

#include <PAC1xxx.h>

#define PAC_ADDRESS       0x40
#define SHUNT_MILLIOHMS   10.0f

PAC1xxx pac;

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

  // Sense channel signed, bus channel left unipolar: a bus rail does not go
  // negative, so there is no reason to give up a bit there.
  if (!pac.setPolarity(PAC1XXX_BIPOLAR, PAC1XXX_UNIPOLAR)) {
    Serial.print(F("Could not set polarity: "));
    Serial.println(pac.lastErrorString());
  }

  // Apply the pending configuration.
  pac.refresh();

  Serial.println(F("Positive current = discharge, negative = charge"));
}

void loop() {
  float amps = pac.current();

  if (isnan(amps)) {
    Serial.print(F("Read failed: "));
    Serial.println(pac.lastErrorString());
  } else {
    Serial.print(pac.busVoltage(), 3);
    Serial.print(F(" V   "));
    Serial.print(amps * 1000.0f, 2);
    Serial.print(F(" mA   "));
    Serial.println(amps >= 0 ? F("(discharging)") : F("(charging)"));
  }

  delay(500);
}
