/*
 * PAC1xxx - Several devices at once
 *
 * Each instance carries its own address, bus and shunt value, so monitoring
 * several rails is just several objects. The shunt is per-device: a 100 mOhm
 * resistor on a low-current rail and a 5 mOhm on a high-current one is normal,
 * and getting these values right per rail matters more than anything else in
 * this sketch.
 *
 * If your board only has one bus, give each part a different address with its
 * ADDR_SEL strapping. If you have run out of addresses, put the extras on a
 * second TwoWire instance where your MCU has one, as shown below.
 *
 * Copyright (c) 2026 Jobit Joseph - MIT License
 * https://github.com/jobitjoseph/PAC1xxx
 */

#include <PAC1xxx.h>

PAC1xxx rail3v3;
PAC1xxx rail5v;

// Uncomment for a second bus on an MCU that has one (ESP32, RP2040, ...).
// PAC1xxx railAux;

void report(const char *label, PAC1xxx &pac) {
  float volts = pac.busVoltage();

  Serial.print(label);
  Serial.print(F(" ("));
  Serial.print(pac.partName());
  Serial.print(F("): "));

  if (isnan(volts)) {
    Serial.println(pac.lastErrorString());
    return;
  }

  Serial.print(volts, 3);
  Serial.print(F(" V  "));
  Serial.print(pac.current() * 1000.0f, 2);
  Serial.print(F(" mA  "));
  Serial.print(pac.power() * 1000.0f, 2);
  Serial.println(F(" mW"));
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { }

  Wire.begin();

  if (!rail3v3.begin(0x40, Wire)) {
    Serial.print(F("3V3 monitor not found: "));
    Serial.println(rail3v3.lastErrorString());
  }
  rail3v3.setShuntMilliOhms(10.0f);

  if (!rail5v.begin(0x41, Wire)) {
    Serial.print(F("5V monitor not found: "));
    Serial.println(rail5v.lastErrorString());
  }
  rail5v.setShuntMilliOhms(10.0f);

  // Second bus example:
  // Wire1.begin();
  // railAux.begin(0x40, Wire1);
  // railAux.setShuntMilliOhms(100.0f);
}

void loop() {
  // Refreshing each device separately means their samples are latched a few
  // hundred microseconds apart. If you need the rails sampled together, turn
  // auto-refresh off on both and issue a single refreshG(), which latches
  // every PAC1xxx on the bus at once.
  report("3V3", rail3v3);
  report("5V ", rail5v);
  Serial.println();

  delay(1000);
}
