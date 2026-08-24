/*
 * PAC1xxx - Limits and alerts
 *
 * The part compares every sample against the limit registers in hardware and
 * latches a flag when one is crossed, so a spike shorter than your loop
 * interval is still caught.
 *
 * This sketch polls ALERT_STATUS. To use the ALERT pin instead, wire it to an
 * interrupt-capable input (it is open drain, so it needs a pull-up) and read
 * the status register from the handler. Reading ALERT_STATUS clears it.
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

  // Limits are quantised to the register resolution, so read them back to see
  // what the part actually stored rather than assuming your value landed.
  pac.setOverCurrentLimit(1.5f);     // amps
  pac.setOverVoltageLimit(5.5f);     // volts
  pac.setUnderVoltageLimit(3.0f);    // volts
  pac.setOverPowerLimit(7.5f);       // watts

  Serial.print(F("OC limit set to "));
  Serial.print(pac.overCurrentLimit(), 3);
  Serial.println(F(" A"));

  Serial.print(F("OV limit set to "));
  Serial.print(pac.overVoltageLimit(), 3);
  Serial.println(F(" V"));

  // Enable the alerts we care about.
  PAC1xxx_ALERT_ENABLE_REGFIELDS enable;
  memset(&enable, 0, sizeof(enable));
  enable.OC  = 1;
  enable.OV  = 1;
  enable.UV  = 1;
  enable.OPW = 1;

  if (!pac.setAlertEnable(enable)) {
    Serial.print(F("Could not enable alerts: "));
    Serial.println(pac.lastErrorString());
  }

  pac.refresh();
  Serial.println(F("Watching for limit violations..."));
}

void loop() {
  PAC1xxx_ALERT_STATUS_REGFIELDS status;

  if (!pac.getAlertStatus(status)) {
    Serial.print(F("Alert read failed: "));
    Serial.println(pac.lastErrorString());
    delay(1000);
    return;
  }

  if (status.OC)  Serial.println(F("ALERT: over current"));
  if (status.UC)  Serial.println(F("ALERT: under current"));
  if (status.OV)  Serial.println(F("ALERT: over voltage"));
  if (status.UV)  Serial.println(F("ALERT: under voltage"));
  if (status.OPW) Serial.println(F("ALERT: over power (average)"));
  if (status.OPC) Serial.println(F("ALERT: over power (peak)"));

  Serial.print(pac.busVoltage(), 3);
  Serial.print(F(" V   "));
  Serial.print(pac.current(), 4);
  Serial.println(F(" A"));

  delay(500);
}
