/*!
 * @file PAC1xxx.cpp
 *
 * Arduino wrapper implementation for the Microchip PAC1xxx power monitors.
 *
 * Copyright (c) 2026 Jobit Joseph
 * SPDX-License-Identifier: MIT
 *
 * Built on the PAC1711 C HAL published by Microchip Technology Inc.
 * See LICENSE-Microchip.txt.
 *
 * https://github.com/jobitjoseph/PAC1xxx
 */

#include "PAC1xxx.h"

/* --------------------------------------------------------------------------
 * Transport trampolines
 *
 * The HAL calls out through C function pointers. These forward to the right
 * PAC1xxx instance using the context pointer handed to PAC1xxx_I2C_Initialize,
 * which is what lets several devices sit on different TwoWire buses at once.
 * They are declared extern "C" so the pointer types match the ones the HAL
 * declares inside its own extern "C" block.
 * ------------------------------------------------------------------------ */

extern "C" {

static bool pac1xxx_wire_write_read(void *ctx, uint8_t address,
                                    uint8_t *writeBuf, size_t writeSize,
                                    uint8_t *readBuf, size_t readSize)
{
    if (ctx == NULL) {
        return false;
    }
    return static_cast<PAC1xxx *>(ctx)->_transferWriteRead(address, writeBuf, writeSize,
                                                           readBuf, readSize);
}

static bool pac1xxx_wire_write(void *ctx, uint8_t address,
                               uint8_t *writeBuf, size_t writeSize)
{
    if (ctx == NULL) {
        return false;
    }
    return static_cast<PAC1xxx *>(ctx)->_transferWrite(address, writeBuf, writeSize);
}

} /* extern "C" */

/* --------------------------------------------------------------------------
 * Construction and setup
 * ------------------------------------------------------------------------ */

PAC1xxx::PAC1xxx()
    : _wire(&Wire),
      _shunt_uOhm(PAC1XXX_DEFAULT_SHUNT_uOHM),
      _lastError(PAC1xxx_SUCCESS),
      _autoRefresh(true),
      _repeatedStart(true),
      _begun(false)
{
    memset(&_device, 0, sizeof(_device));
}

bool PAC1xxx::begin(uint8_t address, TwoWire &wire)
{
    _wire  = &wire;
    _begun = false;

    PAC1xxx_DEVICE_INIT init;
    memset(&init, 0, sizeof(init));

    init.syncMode = true;
    init.rsense   = _shunt_uOhm;

    init.i2c_init.i2cAddress   = address;
    init.i2c_init.i2cContext   = this;
    init.i2c_init.i2cWriteRead = pac1xxx_wire_write_read;
    init.i2c_init.i2cWrite     = pac1xxx_wire_write;

    _lastError = PAC1xxx_Device_Initialize(&_device, init);
    if (_lastError != PAC1xxx_SUCCESS) {
        return false;
    }

    _begun = true;
    return true;
}

bool PAC1xxx::isInitialized()
{
    return _begun && PAC1xxx_Device_IsInitialized(&_device);
}

void PAC1xxx::setShuntMicroOhms(uint32_t microOhms)
{
    _shunt_uOhm    = microOhms;
    _device.rsense = microOhms;
}

void PAC1xxx::setShuntMilliOhms(float milliOhms)
{
    if (milliOhms <= 0.0f) {
        _lastError = PAC1xxx_INVALID_SHUNT_VALUE;
        return;
    }
    setShuntMicroOhms((uint32_t)(milliOhms * 1000.0f + 0.5f));
}

float PAC1xxx::shuntMilliOhms() const
{
    return (float)_shunt_uOhm / 1000.0f;
}

/* --------------------------------------------------------------------------
 * Transport
 * ------------------------------------------------------------------------ */

bool PAC1xxx::_transferWriteRead(uint8_t address, uint8_t *writeBuf, size_t writeSize,
                                 uint8_t *readBuf, size_t readSize)
{
    if ((_wire == NULL) || (readBuf == NULL) || (readSize == 0)) {
        return false;
    }

    if (writeSize > 0) {
        _wire->beginTransmission(address);
        if (_wire->write(writeBuf, writeSize) != writeSize) {
            _wire->endTransmission(true);
            return false;
        }
        /* false keeps the bus held for a repeated start, which is what the
         * device expects between the register pointer and the data phase. */
        if (_wire->endTransmission(_repeatedStart ? false : true) != 0) {
            return false;
        }
    }

    size_t got = _wire->requestFrom((uint8_t)address, (uint8_t)readSize);
    if (got != readSize) {
        /* Drain whatever did arrive so the next transfer starts clean. */
        while (_wire->available()) {
            (void)_wire->read();
        }
        return false;
    }

    for (size_t i = 0; i < readSize; i++) {
        readBuf[i] = (uint8_t)_wire->read();
    }

    return true;
}

bool PAC1xxx::_transferWrite(uint8_t address, uint8_t *writeBuf, size_t writeSize)
{
    if ((_wire == NULL) || (writeBuf == NULL) || (writeSize == 0)) {
        return false;
    }

    _wire->beginTransmission(address);
    if (_wire->write(writeBuf, writeSize) != writeSize) {
        _wire->endTransmission(true);
        return false;
    }

    return (_wire->endTransmission(true) == 0);
}

/* --------------------------------------------------------------------------
 * Identification
 * ------------------------------------------------------------------------ */

const char *PAC1xxx::partName() const
{
    switch (_device.deviceID.product) {
        case PAC1711_PRODUCT_ID:     return "PAC1711";
        case PAC1711PDN_PRODUCT_ID:  return "PAC1711 (PWRDN)";
        case PAC1711TEST_PRODUCT_ID: return "PAC1711 (test)";
        case PAC1721_PRODUCT_ID:     return "PAC1721";
        case PAC1721PDN_PRODUCT_ID:  return "PAC1721 (PWRDN)";
        case PAC1761_PRODUCT_ID:     return "PAC1761";
        case PAC1811_PRODUCT_ID:     return "PAC1811";
        case PAC1811PDN_PRODUCT_ID:  return "PAC1811 (PWRDN)";
        case PAC1821_PRODUCT_ID:     return "PAC1821";
        case PAC1821PDN_PRODUCT_ID:  return "PAC1821 (PWRDN)";
        case PAC1861_PRODUCT_ID:     return "PAC1861";
        default:                     return "unknown";
    }
}

/* --------------------------------------------------------------------------
 * Refresh
 * ------------------------------------------------------------------------ */

bool PAC1xxx::refresh()
{
    _lastError = PAC1xxx_Refresh(&_device);
    if (_lastError != PAC1xxx_SUCCESS) {
        return false;
    }
    if (_device.syncMode) {
        delay(PAC1XXX_REFRESH_SETTLE_MS);
    }
    return true;
}

bool PAC1xxx::refreshV()
{
    _lastError = PAC1xxx_RefreshV(&_device);
    if (_lastError != PAC1xxx_SUCCESS) {
        return false;
    }
    if (_device.syncMode) {
        delay(PAC1XXX_REFRESH_SETTLE_MS);
    }
    return true;
}

bool PAC1xxx::refreshG()
{
    _lastError = PAC1xxx_RefreshG(&_device);
    if (_lastError != PAC1xxx_SUCCESS) {
        return false;
    }
    if (_device.syncMode) {
        delay(PAC1XXX_REFRESH_SETTLE_MS);
    }
    return true;
}

bool PAC1xxx::prepareRead()
{
    if (!_autoRefresh || !_device.syncMode) {
        return true;
    }
    return refreshV();
}

/* --------------------------------------------------------------------------
 * Measurements
 * ------------------------------------------------------------------------ */

float PAC1xxx::readValue(int16_t (*fn)(PAC1xxx_DEVICE_CONTEXT_P, float *), float scale)
{
    float value = NAN;

    if (!prepareRead()) {
        return NAN;
    }

    _lastError = fn(&_device, &value);
    if (_lastError != PAC1xxx_SUCCESS) {
        return NAN;
    }

    return value * scale;
}

float PAC1xxx::busVoltage_mV()        { return readValue(PAC1xxx_GetVBUS_mV, 1.0f); }
float PAC1xxx::busVoltage()           { return readValue(PAC1xxx_GetVBUS_mV, 0.001f); }
float PAC1xxx::busVoltageAverage()    { return readValue(PAC1xxx_GetVBUS_AVG_mV, 0.001f); }
float PAC1xxx::busVoltageMin()        { return readValue(PAC1xxx_GetVBUSmin_mV, 0.001f); }
float PAC1xxx::busVoltageMax()        { return readValue(PAC1xxx_GetVBUSmax_mV, 0.001f); }

float PAC1xxx::shuntVoltage_mV()        { return readValue(PAC1xxx_GetVSENSE_mV, 1.0f); }
float PAC1xxx::shuntVoltageAverage_mV() { return readValue(PAC1xxx_GetVSENSE_AVG_mV, 1.0f); }

float PAC1xxx::current_mA()      { return readValue(PAC1xxx_GetISENSE_mA, 1.0f); }
float PAC1xxx::current()         { return readValue(PAC1xxx_GetISENSE_mA, 0.001f); }
float PAC1xxx::currentAverage()  { return readValue(PAC1xxx_GetISENSE_AVG_mA, 0.001f); }
float PAC1xxx::currentMin()      { return readValue(PAC1xxx_GetISENSEmin_mA, 0.001f); }
float PAC1xxx::currentMax()      { return readValue(PAC1xxx_GetISENSEmax_mA, 0.001f); }

float PAC1xxx::power_mW()  { return readValue(PAC1xxx_GetVPOWER_mW, 1.0f); }
float PAC1xxx::power()     { return readValue(PAC1xxx_GetVPOWER_mW, 0.001f); }
float PAC1xxx::powerMin()  { return readValue(PAC1xxx_GetVPOWERmin_mW, 0.001f); }
float PAC1xxx::powerMax()  { return readValue(PAC1xxx_GetVPOWERmax_mW, 0.001f); }

/* --------------------------------------------------------------------------
 * Accumulator
 * ------------------------------------------------------------------------ */

float PAC1xxx::energy_mWh()
{
    float value = NAN;
    _lastError = PAC1xxx_GetEnergy_mWh(&_device, &value);
    return (_lastError == PAC1xxx_SUCCESS) ? value : NAN;
}

float PAC1xxx::charge_mAs()
{
    float value = NAN;
    _lastError = PAC1xxx_GetCoulomb_mAs(&_device, &value);
    return (_lastError == PAC1xxx_SUCCESS) ? value : NAN;
}

float PAC1xxx::charge_mAh()
{
    float mAs = charge_mAs();
    return isnan(mAs) ? NAN : (mAs / 3600.0f);
}

uint32_t PAC1xxx::accumulatorCount()
{
    uint32_t count = 0;
    _lastError = PAC1xxx_GetAccumulatorCount(&_device, &count);
    return (_lastError == PAC1xxx_SUCCESS) ? count : 0;
}

bool PAC1xxx::setAccumulateMode(PAC1xxxAccumulate mode)
{
    PAC1xxx_CONTROL_REGFIELDS ctrl;
    if (!readCtrl(ctrl)) {
        return false;
    }
    ctrl.ACC_CONFIG = (uint8_t)mode;
    _lastError = PAC1xxx_SetCtrl_reg(&_device, ctrl);
    return (_lastError == PAC1xxx_SUCCESS);
}

/* --------------------------------------------------------------------------
 * Configuration
 * ------------------------------------------------------------------------ */

bool PAC1xxx::readCtrl(PAC1xxx_CONTROL_REGFIELDS &ctrl)
{
    /* reg_select 1 == CONTROL, the register holding the pending configuration */
    _lastError = PAC1xxx_GetCtrl_reg(&_device, 1, &ctrl);
    return (_lastError == PAC1xxx_SUCCESS);
}

bool PAC1xxx::setSampleModeRaw(uint8_t mode)
{
    if (mode > 15) {
        _lastError = PAC1xxx_INVALID_PARAMETER;
        return false;
    }

    PAC1xxx_CONTROL_REGFIELDS ctrl;
    if (!readCtrl(ctrl)) {
        return false;
    }
    ctrl.SAMPLE_MODE = mode;
    _lastError = PAC1xxx_SetCtrl_reg(&_device, ctrl);
    return (_lastError == PAC1xxx_SUCCESS);
}

bool PAC1xxx::setSampleMode(PAC1xxxSampleMode mode)
{
    return setSampleModeRaw((uint8_t)mode);
}

int16_t PAC1xxx::sampleRate()
{
    PAC1xxx_CONTROL_REGFIELDS ctrl;
    /* reg_select 3 == CONTROL_LAT, the configuration actually in effect */
    _lastError = PAC1xxx_GetCtrl_reg(&_device, 3, &ctrl);
    if (_lastError != PAC1xxx_SUCCESS) {
        return -1;
    }
    return PAC1xxx_DecodeCTRLtoSampleRate(ctrl);
}

bool PAC1xxx::setAverageCode(uint8_t code)
{
    if (code > 7) {
        _lastError = PAC1xxx_INVALID_PARAMETER;
        return false;
    }

    PAC1xxx_CONTROL_REGFIELDS ctrl;
    if (!readCtrl(ctrl)) {
        return false;
    }
    ctrl.AVERAGE = code;
    _lastError = PAC1xxx_SetCtrl_reg(&_device, ctrl);
    return (_lastError == PAC1xxx_SUCCESS);
}

uint8_t PAC1xxx::averageCode()
{
    PAC1xxx_CONTROL_REGFIELDS ctrl;
    if (!readCtrl(ctrl)) {
        return 0xFF;
    }
    return ctrl.AVERAGE;
}

bool PAC1xxx::setAdaptiveAccumulation(bool enable)
{
    PAC1xxx_CONTROL_REGFIELDS ctrl;
    if (!readCtrl(ctrl)) {
        return false;
    }
    ctrl.AA = enable ? 1 : 0;
    _lastError = PAC1xxx_SetCtrl_reg(&_device, ctrl);
    return (_lastError == PAC1xxx_SUCCESS);
}

bool PAC1xxx::setPolarity(PAC1xxxPolarity senseMode, PAC1xxxPolarity busMode)
{
    PAC1xxx_NEGPWRFSR_REGFIELDS neg;

    /* reg_select 1 == NEG_PWR_FSR */
    _lastError = PAC1xxx_GetNegPwrFsr_reg(&_device, 1, &neg);
    if (_lastError != PAC1xxx_SUCCESS) {
        return false;
    }

    neg.CFG_VS = (uint8_t)senseMode;
    neg.CFG_VB = (uint8_t)busMode;

    _lastError = PAC1xxx_SetNegPwrFsr_reg(&_device, neg);
    return (_lastError == PAC1xxx_SUCCESS);
}

bool PAC1xxx::setBipolar(bool enable)
{
    PAC1xxxPolarity mode = enable ? PAC1XXX_BIPOLAR : PAC1XXX_UNIPOLAR;
    return setPolarity(mode, mode);
}

/* --------------------------------------------------------------------------
 * Limits and alerts
 * ------------------------------------------------------------------------ */

bool PAC1xxx::setOverCurrentLimit(float amps)
{
    _lastError = PAC1xxx_SetOClimit_mA(&_device, amps * 1000.0f);
    return (_lastError == PAC1xxx_SUCCESS);
}

bool PAC1xxx::setUnderCurrentLimit(float amps)
{
    _lastError = PAC1xxx_SetUClimit_mA(&_device, amps * 1000.0f);
    return (_lastError == PAC1xxx_SUCCESS);
}

bool PAC1xxx::setOverVoltageLimit(float volts)
{
    _lastError = PAC1xxx_SetOVlimit_mV(&_device, volts * 1000.0f);
    return (_lastError == PAC1xxx_SUCCESS);
}

bool PAC1xxx::setUnderVoltageLimit(float volts)
{
    _lastError = PAC1xxx_SetUVlimit_mV(&_device, volts * 1000.0f);
    return (_lastError == PAC1xxx_SUCCESS);
}

bool PAC1xxx::setOverPowerLimit(float watts)
{
    _lastError = PAC1xxx_SetOPWlimit_mW(&_device, watts * 1000.0f);
    return (_lastError == PAC1xxx_SUCCESS);
}

float PAC1xxx::overCurrentLimit()  { return readValue(PAC1xxx_GetOClimit_mA, 0.001f); }
float PAC1xxx::underCurrentLimit() { return readValue(PAC1xxx_GetUClimit_mA, 0.001f); }
float PAC1xxx::overVoltageLimit()  { return readValue(PAC1xxx_GetOVlimit_mV, 0.001f); }
float PAC1xxx::underVoltageLimit() { return readValue(PAC1xxx_GetUVlimit_mV, 0.001f); }
float PAC1xxx::overPowerLimit()    { return readValue(PAC1xxx_GetOPWlimit_mW, 0.001f); }

bool PAC1xxx::getAlertStatus(PAC1xxx_ALERT_STATUS_REGFIELDS &status)
{
    _lastError = PAC1xxx_GetAlertStatus_reg(&_device, &status);
    return (_lastError == PAC1xxx_SUCCESS);
}

bool PAC1xxx::setAlertEnable(const PAC1xxx_ALERT_ENABLE_REGFIELDS &enable)
{
    _lastError = PAC1xxx_SetAlertEnable_reg(&_device, enable);
    return (_lastError == PAC1xxx_SUCCESS);
}

bool PAC1xxx::getAlertEnable(PAC1xxx_ALERT_ENABLE_REGFIELDS &enable)
{
    _lastError = PAC1xxx_GetAlertEnable_reg(&_device, &enable);
    return (_lastError == PAC1xxx_SUCCESS);
}

bool PAC1xxx::anyAlert()
{
    PAC1xxx_ALERT_STATUS_REGFIELDS status;
    if (!getAlertStatus(status)) {
        return false;
    }

    return status.RV || status.FV || status.RC || status.FC ||
           status.OC || status.UC || status.OV || status.UV ||
           status.OPC || status.OPW || status.ACC_OVF || status.ACC_COUNT;
}

/* --------------------------------------------------------------------------
 * Asynchronous operation
 * ------------------------------------------------------------------------ */

void PAC1xxx::setSyncMode(bool sync)
{
    _device.syncMode = sync;
    if (!sync) {
        /* Auto-refresh depends on being able to wait for the settling time,
         * which a non-blocking caller cannot do here. */
        _autoRefresh = false;
    }
}

int16_t PAC1xxx::task()
{
    return PAC1xxx_LibTask(&_device);
}

bool PAC1xxx::busy()
{
    return PAC1xxx_Device_IsBusy(&_device);
}

int16_t PAC1xxx::abort()
{
    _lastError = PAC1xxx_AbortRequest(&_device);
    return _lastError;
}

int16_t PAC1xxx::eventStatus(PAC1xxx_EVENT &event, int16_t &processError)
{
    return PAC1xxx_GetEventStatus(&_device, &event, &processError);
}

int16_t PAC1xxx::onEvent(PAC1xxx_EVENT_HANDLER callback, uintptr_t context)
{
    _lastError = PAC1xxx_SetUserCallback(&_device, callback, context);
    return _lastError;
}

/* --------------------------------------------------------------------------
 * Errors
 * ------------------------------------------------------------------------ */

const char *PAC1xxx::lastErrorString() const
{
    switch (_lastError) {
        case PAC1xxx_SUCCESS:                      return "success";
        case PAC1xxx_BUSY:                         return "busy";
        case PAC1xxx_REQUEST_ABORT:                return "request aborted";
        case PAC1xxx_REQUEST_PENDING:              return "request pending";
        case PAC1xxx_I2C_FAIL:                     return "I2C communication failed";
        case PAC1xxx_MUTEX_FAIL:                   return "mutex creation failed";
        case PAC1xxx_INVALID_PARAMETER:            return "invalid parameter";
        case PAC1xxx_INVALID_SHUNT_VALUE:          return "shunt resistance is zero";
        case PAC1xxx_INVALID_DEVICE:               return "unrecognised device ID";
        case PAC1xxx_INVALID_SAMPLE_MODE:          return "invalid sample mode";
        case PAC1xxx_INVALID_ACCUMULATION_MODE:    return "invalid accumulation mode";
        case PAC1xxx_DIFFERENT_ACCUMULATION_MODE:  return "accumulator configured for a different quantity";
        case PAC1xxx_LIBTASK_FAIL:                 return "library task failed";
        default:                                   return "unknown error";
    }
}
