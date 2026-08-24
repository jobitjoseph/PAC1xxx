# PAC1xxx

Arduino library for the Microchip PAC1xxx family of single-channel I²C power monitors.

These parts measure bus voltage and the drop across a sense resistor, compute power in hardware, and accumulate energy or charge across every sample taken. That last part is the reason to reach for one instead of an ADC and a loop: the accumulator sums *all* samples, so a load transient between two of your reads still shows up in the energy total.
<p align="center">
  <a href="https://www.paypal.com/paypalme/jobitjoseph">
    <img src="https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif" alt="Donate with PayPal" />
  </a>
</p>

[![Arduino Lint](https://img.shields.io/badge/arduino--lint-passing-brightgreen)](https://github.com/arduino/arduino-lint)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Supported parts

The part is identified from its `PRODUCT_ID` register at `begin()`, and the full-scale ranges and ADC width are configured to match. You do not tell the library which chip you have.

| Part | ADC | Bus full scale | Sense full scale |
|---|---|---|---|
| PAC1711 | 12-bit | 42 V | 100 mV |
| PAC1721 | 12-bit | 9 V | 100 mV |
| PAC1761 | 12-bit | 65 V | 100 mV |
| PAC1811 | 16-bit | 42 V | 100 mV |
| PAC1821 | 16-bit | 9 V | 100 mV |
| PAC1861 | 16-bit | 65 V | 100 mV |

The PWRDN variants of the PAC1711, PAC1721, PAC1811 and PAC1821 are recognised and behave identically for measurement purposes.

> This library does **not** cover the multi-channel PAC193x/PAC195x parts, nor the older PAC1710/PAC1720/PAC1921. Those are different silicon with a different register map.

## Installation

**Arduino IDE** — Library Manager, search for `PAC1xxx`. Or download this repository as a ZIP and use *Sketch → Include Library → Add .ZIP Library*.

**PlatformIO** — add to `platformio.ini`:

```ini
lib_deps = jobitjoseph/PAC1xxx
```

or point it straight at the repository:

```ini
lib_deps = https://github.com/jobitjoseph/PAC1xxx.git
```

## Wiring

| PAC1xxx | MCU |
|---|---|
| VDD | 3.3 V |
| GND | GND |
| SDA | SDA (with a pull-up, typically 4.7 kΩ) |
| SCL | SCL (with a pull-up, typically 4.7 kΩ) |
| ALERT | any interrupt-capable input, open drain so it needs a pull-up (optional) |

`SENSE+` and `SENSE-` go across the sense resistor, high side of the load. `VBUS` connects to the rail being measured.

## Quick start

```cpp
#include <PAC1xxx.h>

PAC1xxx pac;

void setup() {
  Serial.begin(115200);
  Wire.begin();                     // bring the bus up yourself

  if (!pac.begin(0x40)) {           // address defaults to 0x40
    Serial.println(pac.lastErrorString());
    while (1);
  }

  pac.setShuntMilliOhms(10.0f);     // defaults to 10 mΩ
  Serial.println(pac.partName());   // e.g. "PAC1811"
}

void loop() {
  Serial.print(pac.busVoltage());   Serial.print(" V  ");
  Serial.print(pac.current());      Serial.print(" A  ");
  Serial.println(pac.power());      // W
  delay(1000);
}
```

### Two settings you must get right

**Address.** Defaults to `0x40`. Your actual address depends on how `ADDR_SEL` is strapped on your board — check the datasheet table for your part and pass it to `begin()`.

**Shunt resistor.** Defaults to 10 mΩ. This value scales every current and power figure the library produces, so an incorrect value gives readings that look plausible and are wrong. Set it with `setShuntMilliOhms()` (or `setShuntMicroOhms()` for finer values) to whatever is actually fitted.

## Refresh, and why it matters

The device latches a coherent snapshot of its measurements on a `REFRESH` command; the readable registers do not change until you issue one. The datasheet requires 1 ms of settling before the latched data is stable.

By default the library handles this for you: every measurement call issues a `REFRESH_V` first and waits. `REFRESH_V` latches *without* clearing the accumulator, so reading voltage in a loop never destroys an energy measurement in progress.

Turn it off with `setAutoRefresh(false)` when you want to control the timing yourself — for example when driving several devices from one `refreshG()` so their samples line up, or when you want a fixed accumulation window.

| Call | Latches | Resets accumulator | Applies pending config |
|---|---|---|---|
| `refresh()` | yes | yes | yes |
| `refreshV()` | yes | no | yes |
| `refreshG()` | yes | yes | yes |

Configuration changes only take effect at the next refresh. If you call `setSampleMode()` or `setPolarity()` and the reading does not change, that is why — issue a `refresh()`.

## API

### Setup

```cpp
bool  begin(uint8_t address = 0x40, TwoWire &wire = Wire);
bool  begin(TwoWire &wire);
bool  isInitialized();
void  setShuntMilliOhms(float milliOhms);
void  setShuntMicroOhms(uint32_t microOhms);
float shuntMilliOhms() const;
void  useRepeatedStart(bool enable);
```

`begin()` does not call `Wire.begin()`, so sketches using custom pins or a secondary bus stay in control. `useRepeatedStart(false)` falls back to stop-then-start for the register read, which you only need behind a bus multiplexer or level translator that cannot pass a repeated start through.

### Identification

```cpp
uint8_t     productID();
uint8_t     manufacturerID();   // 0x54 for Microchip
uint8_t     revisionID();
const char *partName();         // "PAC1811", etc.
bool        has16BitADC();
uint16_t    busFullScale_mV();
```

### Measurements

Every one returns `NAN` on failure; call `lastError()` or `lastErrorString()` to find out why.

```cpp
float busVoltage();               // V
float busVoltage_mV();
float busVoltageAverage();        // V, rolling average
float busVoltageMin();            // V, since last refresh
float busVoltageMax();

float shuntVoltage_mV();
float shuntVoltageAverage_mV();

float current();                  // A
float current_mA();
float currentAverage();
float currentMin();
float currentMax();

float power();                    // W
float power_mW();
float powerMin();
float powerMax();
```

### Accumulator

```cpp
bool     setAccumulateMode(PAC1xxxAccumulate mode);
float    energy_mWh();            // needs PAC1XXX_ACC_POWER
float    charge_mAs();            // needs PAC1XXX_ACC_SENSE
float    charge_mAh();
uint32_t accumulatorCount();
```

Modes: `PAC1XXX_ACC_POWER`, `PAC1XXX_ACC_SENSE`, `PAC1XXX_ACC_BUS`. The accumulator sums one quantity at a time, so asking for energy while it is configured for charge returns an error rather than a wrong number.

### Configuration

```cpp
bool    setSampleMode(PAC1xxxSampleMode mode);
bool    setSampleModeRaw(uint8_t mode);      // 0..15
int16_t sampleRate();                        // samples/s, 0 = sleep, -1 = undecodable
bool    setAverageCode(uint8_t code);        // 0..7, raw
uint8_t averageCode();
bool    setAdaptiveAccumulation(bool enable);
bool    setPolarity(PAC1xxxPolarity sense, PAC1xxxPolarity bus);
bool    setBipolar(bool enable);
```

Sample modes: `PAC1XXX_SAMPLE_8192SPS`, `_4096SPS`, `_1024SPS`, `_256SPS`, `_64SPS`, `_8SPS`, `_SINGLE_SHOT`, `_VBUS_16384SPS`, `_VSENSE_16384SPS`, `_SLEEP`. The remaining `SAMPLE_MODE` codes (7, 8, 9, 12, 13) select further single-shot variants — reach them with `setSampleModeRaw()`.

Polarity: `PAC1XXX_UNIPOLAR`, `PAC1XXX_BIPOLAR`, `PAC1XXX_BIPOLAR_HALF`. Bipolar mode is what you want for a battery that both charges and discharges. Full-range bipolar spends a code bit on the sign, halving resolution in each direction; `PAC1XXX_BIPOLAR_HALF` keeps the resolution and halves the range instead.

> **`setAverageCode()` takes a raw value.** The vendor HAL this library is built on does not decode the `AVERAGE` field, so the library passes codes 0–7 through untouched rather than inventing names for them. Check the `AVERAGE` table in the datasheet for your part. Code 0 is no averaging.

### Limits and alerts

```cpp
bool  setOverCurrentLimit(float amps);
bool  setUnderCurrentLimit(float amps);
bool  setOverVoltageLimit(float volts);
bool  setUnderVoltageLimit(float volts);
bool  setOverPowerLimit(float watts);

float overCurrentLimit();   // and the matching readbacks
float underCurrentLimit();
float overVoltageLimit();
float underVoltageLimit();
float overPowerLimit();

bool  getAlertStatus(PAC1xxx_ALERT_STATUS_REGFIELDS &status);
bool  setAlertEnable(const PAC1xxx_ALERT_ENABLE_REGFIELDS &enable);
bool  getAlertEnable(PAC1xxx_ALERT_ENABLE_REGFIELDS &enable);
bool  anyAlert();
```

Limits are quantised to the register resolution, so read them back rather than assuming your value landed exactly. Comparison happens in hardware against every sample, which is how a spike shorter than your loop interval still gets caught.

### Non-blocking operation

```cpp
void    setSyncMode(bool sync);   // true (blocking) by default
bool    syncMode() const;
int16_t task();                   // call repeatedly while busy()
bool    busy();
int16_t abort();
int16_t eventStatus(PAC1xxx_EVENT &event, int16_t &processError);
int16_t onEvent(PAC1xxx_EVENT_HANDLER callback, uintptr_t context);
```

Most sketches should stay synchronous. Asynchronous mode exists for the case where you cannot afford to block on I²C at all; requests return `PAC1xxx_REQUEST_PENDING` immediately and you drive them with `task()`. Auto-refresh switches itself off when you go asynchronous, since the settling wait becomes yours to manage. See `07_AsyncNonBlocking`.

### Errors and the escape hatch

```cpp
int16_t     lastError();          // PAC1xxx_SUCCESS == 0
const char *lastErrorString();
PAC1xxx_DEVICE_CONTEXT_P hal();
```

The C++ class wraps the common ground. The vendor HAL underneath exposes roughly a hundred register-level functions, and `hal()` hands you the context to call any of them:

```cpp
#include <PAC1xxx.h>

PAC1xxx_SLOW_REGFIELDS slow;
PAC1xxx_GetSlow_reg(pac.hal(), &slow);
```

Everything in `src/hal/PAC1xxx_hal.h` is fair game.

## Examples

| Sketch | What it shows |
|---|---|
| `01_BasicReadings` | Voltage, current and power in a loop |
| `02_DeviceInfo` | Which part answered, IDs, ranges — first bring-up check |
| `03_SampleRateAndAveraging` | Sample rate versus noise, instantaneous versus average |
| `04_BidirectionalCurrent` | Bipolar mode for charge/discharge measurement |
| `05_EnergyAccumulator` | Hardware energy totals over fixed windows |
| `06_AlertsAndLimits` | Limit registers and the alert status flags |
| `07_AsyncNonBlocking` | Driving the state machine without blocking |
| `08_MultipleDevices` | Several rails, several addresses, several buses |

## Compatibility

`architectures=*`. Verified to compile against Adafruit SAMD (Feather M0), Arduino AVR (Uno) and ESP32. The library uses `float` and one `uint64_t` accumulator, which costs some flash on 8-bit parts — around 15 kB for a typical sketch on an Uno — but it fits and runs.

Multiple devices and multiple `TwoWire` buses are supported: each instance holds its own address, bus and shunt value.

## Credits

This library is built on the **PAC1711 C HAL published by Microchip Technology Inc.** All of the measurement scaling, register handling and state-machine logic comes from that HAL. The work here is the Arduino integration: a Wire-based transport layer, a C++ interface, and the family-wide naming.

The files under `src/hal/` are derived from Microchip's code and keep their original copyright notice and disclaimer, as Microchip's terms require. See [LICENSE-Microchip.txt](LICENSE-Microchip.txt).

Changes made to the HAL are listed in the header of each modified file. In summary: the symbol prefix was renamed `PAC1711_` → `PAC1xxx_` (identifiers naming an actual part keep their part number), the dead PIC32/Harmony transport machinery was removed, and a caller-supplied context pointer was threaded through the transfer function pointers so several devices can share one program.

## License

MIT for this library's own code — see [LICENSE](LICENSE). The `src/hal` files remain subject to Microchip's terms.

Copyright © 2026 Jobit Joseph

## Author

**Jobit Joseph**
[github.com/jobitjoseph](https://github.com/jobitjoseph) · jobitjoseph1@gmail.com

Issues and pull requests welcome at [github.com/jobitjoseph/PAC1xxx](https://github.com/jobitjoseph/PAC1xxx).
