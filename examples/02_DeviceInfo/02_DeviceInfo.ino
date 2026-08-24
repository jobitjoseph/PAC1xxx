/*
 * PAC1xxx - Device information
 *
 * Reports which part answered, its ID registers, ADC width, full-scale bus
 * voltage and the configured sample rate. Useful as a first bring-up check on
 * a new board.
 *
 * Copyright (c) 2026 Jobit Joseph - MIT License
 * https://github.com/jobitjoseph/PAC1xxx
 */

#include <PAC1xxx.h>

#define PAC_ADDRESS  0x40

PAC1xxx pac;

void printHex(uint8_t value) {
  if (value < 0x10) Serial.print('0');
  Serial.print(value, HEX);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { }

  Wire.begin();

  if (!pac.begin(PAC_ADDRESS)) {
    Serial.print(F("PAC1xxx not found: "));
    Serial.println(pac.lastErrorString());
    Serial.println(F("Check wiring, pull-ups, and that the address matches"));
    Serial.println(F("your ADDR_SEL strapping."));
    while (1) { delay(1000); }
  }

  Serial.println(F("--- PAC1xxx ---"));

  Serial.print(F("Library version : "));
  Serial.println(F(PAC1XXX_VERSION));

  Serial.print(F("Part            : "));
  Serial.println(pac.partName());

  Serial.print(F("Product ID      : 0x"));   printHex(pac.productID());      Serial.println();
  Serial.print(F("Manufacturer ID : 0x"));   printHex(pac.manufacturerID()); Serial.println();
  Serial.print(F("Revision ID     : 0x"));   printHex(pac.revisionID());     Serial.println();

  Serial.print(F("ADC resolution  : "));
  Serial.print(pac.has16BitADC() ? 16 : 12);
  Serial.println(F(" bit"));

  Serial.print(F("Bus full scale  : "));
  Serial.print(pac.busFullScale_mV() / 1000.0f, 1);
  Serial.println(F(" V"));

  Serial.print(F("Sample rate     : "));
  Serial.print(pac.sampleRate());
  Serial.println(F(" samples/s"));

  Serial.print(F("Shunt resistor  : "));
  Serial.print(pac.shuntMilliOhms(), 3);
  Serial.println(F(" mOhm"));
}

void loop() {
}
