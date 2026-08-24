/*
 * PAC1xxx - Basic readings
 *
 * Prints bus voltage, current and power once a second.
 *
 * Wiring (any board with I2C):
 *   VDD  -> 3.3V
 *   GND  -> GND
 *   SDA  -> SDA
 *   SCL  -> SCL
 *   SENSE+ / SENSE- across your shunt resistor
 *   VBUS -> the rail you want to measure
 *
 * Set SHUNT_MILLIOHMS to the value of the resistor actually fitted on your
 * board. Getting this wrong scales every current and power reading.
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

  Serial.print(F("Found "));
  Serial.print(pac.partName());
  Serial.print(F(" with a "));
  Serial.print(pac.shuntMilliOhms(), 2);
  Serial.println(F(" mOhm shunt"));
}

void loop() {
  float volts = pac.busVoltage();
  float amps  = pac.current();
  float watts = pac.power();

  if (isnan(volts)) {
    Serial.print(F("Read failed: "));
    Serial.println(pac.lastErrorString());
  } else {
    Serial.print(volts, 3);  Serial.print(F(" V   "));
    Serial.print(amps, 4);   Serial.print(F(" A   "));
    Serial.print(watts, 4);  Serial.println(F(" W"));
  }

  delay(1000);
}
