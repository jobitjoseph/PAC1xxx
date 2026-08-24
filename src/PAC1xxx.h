/*!
 * @file PAC1xxx.h
 *
 * Arduino library for the Microchip PAC1xxx single-channel power monitors:
 * PAC1711, PAC1721, PAC1761, PAC1811, PAC1821 and PAC1861 (including the
 * PWRDN variants). The part is identified automatically at begin(), and the
 * full-scale ranges and ADC width are configured to match.
 *
 * Copyright (c) 2026 Jobit Joseph
 * SPDX-License-Identifier: MIT
 *
 * Built on the PAC1711 C HAL published by Microchip Technology Inc.
 * The files under src/hal/ are derived from that HAL and carry Microchip's
 * original copyright notice and disclaimer. See LICENSE-Microchip.txt.
 *
 * https://github.com/jobitjoseph/PAC1xxx
 */

#ifndef PAC1XXX_ARDUINO_H
#define PAC1XXX_ARDUINO_H

#include <Arduino.h>
#include <Wire.h>

#include "hal/PAC1xxx_hal.h"

/** Library version string. */
#define PAC1XXX_VERSION "1.0.0"

/** Default 7-bit I2C address. Override in begin() if your ADDR_SEL pin
 *  strapping selects a different one. */
#define PAC1XXX_DEFAULT_ADDRESS 0x40

/** Default sense resistor, in micro-ohms (10 milliohm). */
#define PAC1XXX_DEFAULT_SHUNT_uOHM 10000UL

/** Settling time required after a REFRESH before the latched registers are
 *  stable, in milliseconds. The datasheet specifies 1 ms. */
#define PAC1XXX_REFRESH_SETTLE_MS 1

/**
 * @brief SAMPLE_MODE values decoded by the library.
 *
 * These are the SAMPLE_MODE codes whose sample rate the library can decode.
 * The remaining codes (7, 8, 9, 12, 13) select further single-shot variants;
 * reach them with setSampleModeRaw() and consult your datasheet.
 */
enum PAC1xxxSampleMode : uint8_t {
    PAC1XXX_SAMPLE_8192SPS       = 0,  /**< Continuous, 8192 samples/s. */
    PAC1XXX_SAMPLE_4096SPS       = 1,  /**< Continuous, 4096 samples/s. */
    PAC1XXX_SAMPLE_1024SPS       = 2,  /**< Continuous, 1024 samples/s. */
    PAC1XXX_SAMPLE_256SPS        = 3,  /**< Continuous, 256 samples/s. */
    PAC1XXX_SAMPLE_64SPS         = 4,  /**< Continuous, 64 samples/s. */
    PAC1XXX_SAMPLE_8SPS          = 5,  /**< Continuous, 8 samples/s. */
    PAC1XXX_SAMPLE_SINGLE_SHOT   = 6,  /**< Single shot. */
    PAC1XXX_SAMPLE_VBUS_16384SPS = 10, /**< 16384 samples/s, VBUS accumulation only. */
    PAC1XXX_SAMPLE_VSENSE_16384SPS = 11, /**< 16384 samples/s, VSENSE accumulation only. */
    PAC1XXX_SAMPLE_SLEEP         = 15  /**< Sleep. */
};

/**
 * @brief Measurement polarity, written to the NEG_PWR_FSR register.
 */
enum PAC1xxxPolarity : uint8_t {
    PAC1XXX_UNIPOLAR      = PAC1xxx_NEGPWRFSR_MODE_UNIPOLAR, /**< Positive only, full range. */
    PAC1XXX_BIPOLAR       = PAC1xxx_NEGPWRFSR_MODE_BIPOLAR,  /**< Signed, full range. */
    PAC1XXX_BIPOLAR_HALF  = PAC1xxx_NEGPWRFSR_MODE_HALFFSR   /**< Signed, half range. */
};

/**
 * @brief What the accumulator sums, written to the CONTROL ACC_CONFIG field.
 */
enum PAC1xxxAccumulate : uint8_t {
    PAC1XXX_ACC_POWER  = PAC1xxx_CONTROL_ACC_CONFIG_VPOWER, /**< Accumulate VPOWER (energy). */
    PAC1XXX_ACC_SENSE  = PAC1xxx_CONTROL_ACC_CONFIG_VSENSE, /**< Accumulate VSENSE (charge). */
    PAC1XXX_ACC_BUS    = PAC1xxx_CONTROL_ACC_CONFIG_VBUS    /**< Accumulate VBUS. */
};

/**
 * @brief Arduino interface to a single PAC1xxx power monitor.
 *
 * Typical use:
 * @code
 * PAC1xxx pac;
 *
 * void setup() {
 *     Wire.begin();
 *     pac.begin();                       // 0x40 on Wire
 *     pac.setShuntMilliOhms(10.0f);
 * }
 *
 * void loop() {
 *     Serial.println(pac.busVoltage());  // volts
 *     Serial.println(pac.current());     // amps
 *     Serial.println(pac.power());       // watts
 *     delay(500);
 * }
 * @endcode
 *
 * Every reading needs a REFRESH first so the device latches a coherent set of
 * values. By default the class issues one automatically before each read and
 * waits the required settling time; see setAutoRefresh() if you would rather
 * drive that yourself.
 */
class PAC1xxx {
public:
    PAC1xxx();

    /* --------------------------------------------------------------------
     * Setup
     * ------------------------------------------------------------------ */

    /**
     * @brief Initialise the device.
     *
     * Does not call Wire.begin() for you: bring the bus up first, so that
     * sketches choosing custom pins or a secondary bus stay in control.
     * Reads and validates the device ID, then configures full-scale ranges to
     * match the part that answered.
     * @param address [in] - 7-bit I2C address. Defaults to 0x40.
     * @param wire    [in] - TwoWire instance. Defaults to Wire.
     * @return true on success. On failure see lastError().
     */
    bool begin(uint8_t address = PAC1XXX_DEFAULT_ADDRESS, TwoWire &wire = Wire);

    /** @brief Initialise on a specific bus at the default address. */
    bool begin(TwoWire &wire) { return begin(PAC1XXX_DEFAULT_ADDRESS, wire); }

    /** @brief True once begin() has succeeded. */
    bool isInitialized();

    /** @brief Set the sense resistor value in milliohms. Default 10.0. */
    void setShuntMilliOhms(float milliOhms);

    /** @brief Set the sense resistor value in microohms. Default 10000. */
    void setShuntMicroOhms(uint32_t microOhms);

    /** @brief Current sense resistor value, in milliohms. */
    float shuntMilliOhms() const;

    /**
     * @brief Choose repeated-start or stop-then-start for register reads.
     *
     * Repeated start (the default) is what the part expects. Turn it off only
     * if you are behind a bus multiplexer or level translator that cannot pass
     * one through.
     */
    void useRepeatedStart(bool enable) { _repeatedStart = enable; }

    /* --------------------------------------------------------------------
     * Identification
     * ------------------------------------------------------------------ */

    /** @brief PRODUCT_ID register value, cached at begin(). */
    uint8_t productID() const { return _device.deviceID.product; }

    /** @brief MANUFACTURER_ID register value. Microchip parts report 0x54. */
    uint8_t manufacturerID() const { return _device.deviceID.manufacturer; }

    /** @brief REVISION_ID register value. */
    uint8_t revisionID() const { return _device.deviceID.revision; }

    /** @brief Part name for the detected PRODUCT_ID, e.g. "PAC1811". */
    const char *partName() const;

    /** @brief True if the detected part has a 16-bit ADC (PAC18xx). */
    bool has16BitADC() const { return !_device.is12bitADCres; }

    /** @brief Bus voltage full scale for the detected part, in millivolts. */
    uint16_t busFullScale_mV() const { return _device.VbusMAX; }

    /* --------------------------------------------------------------------
     * Refresh
     * ------------------------------------------------------------------ */

    /**
     * @brief Latch measurements, reset the accumulators, apply config changes.
     *
     * Blocks for the settling time in synchronous mode.
     */
    bool refresh();

    /**
     * @brief Latch measurements without resetting the accumulators.
     *
     * This is what auto-refresh uses, so that reading voltage or current does
     * not throw away an energy measurement in progress.
     */
    bool refreshV();

    /** @brief REFRESH_G: latch and reset, applied across the bus. */
    bool refreshG();

    /**
     * @brief Enable or disable the automatic refresh before each read.
     *
     * On by default, using REFRESH_V. Turn it off when you are refreshing on
     * your own schedule, or driving several devices from one refreshG().
     */
    void setAutoRefresh(bool enable) { _autoRefresh = enable; }

    /** @brief True if auto-refresh is enabled. */
    bool autoRefresh() const { return _autoRefresh; }

    /* --------------------------------------------------------------------
     * Measurements. Every one returns NAN on failure; check lastError().
     * ------------------------------------------------------------------ */

    float busVoltage();          /**< Bus voltage, volts. */
    float busVoltage_mV();       /**< Bus voltage, millivolts. */
    float busVoltageAverage();   /**< Rolling average bus voltage, volts. */
    float busVoltageMin();       /**< Minimum bus voltage since refresh, volts. */
    float busVoltageMax();       /**< Maximum bus voltage since refresh, volts. */

    float shuntVoltage_mV();     /**< Sense (shunt) voltage, millivolts. */
    float shuntVoltageAverage_mV(); /**< Rolling average sense voltage, millivolts. */

    float current();             /**< Current, amps. */
    float current_mA();          /**< Current, milliamps. */
    float currentAverage();      /**< Rolling average current, amps. */
    float currentMin();          /**< Minimum current since refresh, amps. */
    float currentMax();          /**< Maximum current since refresh, amps. */

    float power();               /**< Power, watts. */
    float power_mW();            /**< Power, milliwatts. */
    float powerMin();            /**< Minimum power since refresh, watts. */
    float powerMax();            /**< Maximum power since refresh, watts. */

    /* --------------------------------------------------------------------
     * Accumulator
     * ------------------------------------------------------------------ */

    /** @brief Accumulated energy since the last refresh(), milliwatt-hours.
     *  Requires ACC_CONFIG set to PAC1XXX_ACC_POWER. */
    float energy_mWh();

    /** @brief Accumulated charge since the last refresh(), milliamp-seconds.
     *  Requires ACC_CONFIG set to PAC1XXX_ACC_SENSE. */
    float charge_mAs();

    /** @brief Accumulated charge since the last refresh(), milliamp-hours. */
    float charge_mAh();

    /** @brief Number of samples in the accumulator. */
    uint32_t accumulatorCount();

    /** @brief Select what the accumulator sums. */
    bool setAccumulateMode(PAC1xxxAccumulate mode);

    /* --------------------------------------------------------------------
     * Configuration
     * ------------------------------------------------------------------ */

    /** @brief Set SAMPLE_MODE from the decoded enum. */
    bool setSampleMode(PAC1xxxSampleMode mode);

    /** @brief Set SAMPLE_MODE from a raw 0..15 code, for the modes the enum
     *  does not name. */
    bool setSampleModeRaw(uint8_t mode);

    /** @brief Effective sample rate in samples/s for the current CONTROL
     *  configuration. Returns 0 for sleep modes and -1 if undecodable. */
    int16_t sampleRate();

    /**
     * @brief Set the AVERAGE field of the CONTROL register.
     *
     * The library core does not decode this field, so it is passed through
     * raw. Codes run 0..7; check the AVERAGE table in the datasheet for your
     * part to see what each one selects.
     */
    bool setAverageCode(uint8_t code);

    /** @brief Current AVERAGE field value, or 0xFF on failure. */
    uint8_t averageCode();

    /** @brief Enable or disable adaptive accumulation (the CONTROL AA bit). */
    bool setAdaptiveAccumulation(bool enable);

    /**
     * @brief Set measurement polarity for the sense and bus channels.
     *
     * Use PAC1XXX_BIPOLAR when current can flow both ways, for example on a
     * battery that both charges and discharges.
     */
    bool setPolarity(PAC1xxxPolarity senseMode, PAC1xxxPolarity busMode);

    /** @brief Shorthand: bipolar full range on both channels, or unipolar. */
    bool setBipolar(bool enable);

    /* --------------------------------------------------------------------
     * Limits and alerts
     * ------------------------------------------------------------------ */

    bool setOverCurrentLimit(float amps);    /**< OC_LIMIT, amps. */
    bool setUnderCurrentLimit(float amps);   /**< UC_LIMIT, amps. */
    bool setOverVoltageLimit(float volts);   /**< OV_LIMIT, volts. */
    bool setUnderVoltageLimit(float volts);  /**< UV_LIMIT, volts. */
    bool setOverPowerLimit(float watts);     /**< OPW_LIMIT, watts. */

    float overCurrentLimit();   /**< OC_LIMIT read back, amps. NAN on failure. */
    float underCurrentLimit();  /**< UC_LIMIT read back, amps. NAN on failure. */
    float overVoltageLimit();   /**< OV_LIMIT read back, volts. NAN on failure. */
    float underVoltageLimit();  /**< UV_LIMIT read back, volts. NAN on failure. */
    float overPowerLimit();     /**< OPW_LIMIT read back, watts. NAN on failure. */

    /** @brief Read the ALERT_STATUS register bit-fields. */
    bool getAlertStatus(PAC1xxx_ALERT_STATUS_REGFIELDS &status);

    /** @brief Write the ALERT_ENABLE register bit-fields. */
    bool setAlertEnable(const PAC1xxx_ALERT_ENABLE_REGFIELDS &enable);

    /** @brief Read the ALERT_ENABLE register bit-fields. */
    bool getAlertEnable(PAC1xxx_ALERT_ENABLE_REGFIELDS &enable);

    /** @brief True if any alert bit is set in ALERT_STATUS. */
    bool anyAlert();

    /* --------------------------------------------------------------------
     * Asynchronous operation
     * ------------------------------------------------------------------ */

    /**
     * @brief Switch between blocking and non-blocking operation.
     *
     * Synchronous (the default) means every call returns with the answer.
     * Asynchronous means calls return immediately with PAC1xxx_REQUEST_PENDING
     * and you must call task() until the request completes, either polling
     * eventStatus() or waiting for the callback set by onEvent().
     *
     * Auto-refresh is meaningless without blocking, so it is switched off when
     * you go asynchronous.
     */
    void setSyncMode(bool sync);

    /** @brief True if operating in blocking mode. */
    bool syncMode() const { return _device.syncMode; }

    /** @brief Drive the state machine. Call repeatedly while busy(). */
    int16_t task();

    /** @brief True while a request is in flight. */
    bool busy();

    /** @brief Abort the request in flight. */
    int16_t abort();

    /** @brief Poll the outcome of the last asynchronous request. */
    int16_t eventStatus(PAC1xxx_EVENT &event, int16_t &processError);

    /** @brief Register a completion callback for asynchronous requests. */
    int16_t onEvent(PAC1xxx_EVENT_HANDLER callback, uintptr_t context);

    /* --------------------------------------------------------------------
     * Errors and escape hatch
     * ------------------------------------------------------------------ */

    /** @brief Result code from the most recent call. PAC1xxx_SUCCESS is 0. */
    int16_t lastError() const { return _lastError; }

    /** @brief Human-readable form of lastError(). */
    const char *lastErrorString() const;

    /**
     * @brief The underlying HAL device context.
     *
     * For the register-level calls this class does not wrap. Pass it to any
     * PAC1xxx_* function from hal/PAC1xxx_hal.h:
     * @code
     * PAC1xxx_SLOW_REGFIELDS slow;
     * PAC1xxx_GetSlow_reg(pac.hal(), &slow);
     * @endcode
     */
    PAC1xxx_DEVICE_CONTEXT_P hal() { return &_device; }

    /* Internal transport hooks. Public because the C trampolines in
     * PAC1xxx.cpp need to reach them; not part of the intended API. */
    bool _transferWriteRead(uint8_t address, uint8_t *writeBuf, size_t writeSize,
                            uint8_t *readBuf, size_t readSize);
    bool _transferWrite(uint8_t address, uint8_t *writeBuf, size_t writeSize);

private:
    PAC1xxx_DEVICE_CONTEXT _device;
    TwoWire  *_wire;
    uint32_t  _shunt_uOhm;
    int16_t   _lastError;
    bool      _autoRefresh;
    bool      _repeatedStart;
    bool      _begun;

    bool prepareRead();
    float readValue(int16_t (*fn)(PAC1xxx_DEVICE_CONTEXT_P, float *), float scale);
    bool readCtrl(PAC1xxx_CONTROL_REGFIELDS &ctrl);
};

#endif /* PAC1XXX_ARDUINO_H */
