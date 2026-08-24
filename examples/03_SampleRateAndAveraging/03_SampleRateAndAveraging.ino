/*
 * PAC1xxx - Sample rate and averaging
 *
 * Walks through the continuous sample-rate modes and shows the effect on the
 * instantaneous reading versus the rolling average. Slower rates give a longer
 * integration window and a quieter reading; faster rates track transients.
 *
 * The AVERAGE field is passed through as a raw 0..7 code because the vendor
 * HAL does not decode it. Check the AVERAGE table in the datasheet for your
 * part to see what each code selects.
 *
 * Copyright (c) 2026 Jobit Joseph - MIT License
 * https://github.com/jobitjoseph/PAC1xxx
 */

#include <PAC1xxx.h>

#define PAC_ADDRESS       0x40
#define SHUNT_MILLIOHMS   10.0f

PAC1xxx pac;

const PAC1xxxSampleMode MODES[] = {
  PAC1XXX_SAMPLE_8192SPS,
  PAC1XXX_SAMPLE_1024SPS,
  PAC1XXX_SAMPLE_64SPS,
  PAC1XXX_SAMPLE_8SPS
};
const uint8_t MODE_COUNT = sizeof(MODES) / sizeof(MODES[0]);

uint8_t modeIndex = 0;

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

  // Averaging code 0 is no averaging on every part in this family.
  pac.setAverageCode(0);
}

void loop() {
  if (!pac.setSampleMode(MODES[modeIndex])) {
    Serial.print(F("Could not set sample mode: "));
    Serial.println(pac.lastErrorString());
  }

  // A refresh applies the pending CONTROL configuration.
  pac.refresh();
  delay(200);

  Serial.print(F("Sample rate "));
  Serial.print(pac.sampleRate());
  Serial.print(F(" sps -> "));

  for (uint8_t i = 0; i < 5; i++) {
    Serial.print(pac.current_mA(), 2);
    Serial.print(F(" mA "));
    delay(100);
  }

  Serial.print(F("| average "));
  Serial.print(pac.currentAverage() * 1000.0f, 2);
  Serial.println(F(" mA"));

  modeIndex = (modeIndex + 1) % MODE_COUNT;
  delay(1500);
}
