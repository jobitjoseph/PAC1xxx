/*
  Portions (c) 2026 Microchip Technology Inc. and its subsidiaries

  Subject to your compliance with these terms, you may use this Microchip 
  software and any derivatives of this software. You must retain the above
  copyright notice with any redistribution of this software and the following
  disclaimers. It is your responsibility to comply with third party license
  terms applicable to your use of third party software (including open source
  software) that may accompany this Microchip software. THIS SOFTWARE IS
  SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR
  STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF 
  NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
  IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
  INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
  WHATSOEVER RELATED TO THIS SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
  BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
  FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS
  IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF
  ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

  ---------------------------------------------------------------------------
  Modifications (c) 2026 Jobit Joseph, released under the MIT License.

  Changes from the original Microchip PAC1711 C HAL:
    - Symbol prefix renamed PAC1711_ -> PAC1xxx_ throughout. Identifiers that
      name an actual part (PAC1711_PRODUCT_ID, PAC1711_VBUS_MAX_mV and the
      like) deliberately keep their part number.
    - Files renamed to match the PAC1xxx library layout.
    - No functional change to the measurement, scaling or state-machine logic.

  This file is the vendor HAL. Sketches should use the PAC1xxx C++ class in
  ../PAC1xxx.h instead; reach in here only for the advanced register-level
  calls the wrapper does not surface.
  ---------------------------------------------------------------------------
*/

#ifndef PAC1xxx_HAL_H
#define PAC1xxx_HAL_H

// PAC1xxx generic library version
#define PAC1xxx_LIBVER "1.0"

/*
 * Included Files
 */
#include "PAC1xxx_i2c.h"
#include "PAC1xxx_definitions.h"
#include "PAC1xxx_mutex.h"

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*
 * PAC1xxx library error codes
 */
//Return codes from the public API
#define PAC1xxx_SUCCESS                               0 /**< The API request or function call is completed with no error. */
#define PAC1xxx_BUSY                                  1 /**< The new request is rejected while the library is busy processing the previous request. */    
#define PAC1xxx_REQUEST_ABORT                         2 /**< The API return code indicates that the pending request processing has been aborted. */
#define PAC1xxx_REQUEST_PENDING                       3 /**< The API return code indicates that the request processing has been started. 
                                                             The processing completion will be signaled (asynchronously) by the user callback function
                                                             registerd with PAC1xxx_SetUserCallback(), 
                                                             or determined by polling the processing status with PAC1xxx_GetEventStatus() */

#define PAC1xxx_I2C_FAIL                              4 /**< The API request processing failed because of I2C communication failure. */
#define PAC1xxx_MUTEX_FAIL                            5 /**< PAC1xxx_Device_Initialize() failed to create the mutex. */
#define PAC1xxx_INVALID_PARAMETER                     6 /**< The request is rejected because of incorect or invalid parameter. */
#define PAC1xxx_INVALID_SHUNT_VALUE                   7 /**< The request is rejected because the shunt value is set to 0 (zero). */
#define PAC1xxx_INVALID_DEVICE                        8 /**< PAC1xxx_Device_Initialize() failed to recognize the PAC17xx device ID. */
#define PAC1xxx_INVALID_SAMPLE_MODE                   9 /**< The API request processing failed because the current SAMPLE_MODE configuration
                                                             is not supported by the API function or the SAMPLE_MODE reading is incorrect. */
#define PAC1xxx_INVALID_ACCUMULATION_MODE            10 /**< The API request processing failed because the current ACC_CONFIG configuration
                                                             is not set for Vpower, Vsense or Vbus accumulation */
#define PAC1xxx_DIFFERENT_ACCUMULATION_MODE          11 /**< The API request processing failed because the current ACC_CONFIG configuration
                                                             is not compatible with the selected API. */

#define PAC1xxx_LIBTASK_DONE                          0 /**< The PAC1xxx_LibTask() function call is completed with no error. */
#define PAC1xxx_LIBTASK_FAIL                         12 /**< The PAC1xxx_LibTask() function call failed. */
    
/*
 * Constants
 */
#define ENERGY_UNIT_CONVERSION  (float)1/3600           /**< Conversion constant from 1Ws to 1Wh */        

//PAC1xxx maximum transmission and reception data byte-count
#define PAC1xxx_I2C_TX_MAXSIZE                       3  /**< The maximum data size to write TO device in one i2c transfer. */
#define PAC1xxx_I2C_RX_MAXSIZE  (PAC1xxx_VACC_SZ + SMBUS_BYTECNT_SZ)    
                                                        /**< The maximum data size to read FROM device in one I2C transfer. */
/*
 * Type definitions
 */

    
/** @struct _PAC1xxx_DEVICE_INIT
 * @brief _PAC1xxx_DEVICE_INIT structure type parameter for the PAC1xxx_Device_Initialize() function.
 * 
 * The structure must contain the initialization and configuration values for various attributes and members
 * of the _PAC1xxx_DEVICE_CONTEXT structure type.
 * - syncMode:
 *      + Configuration of the library synchronous/asynchronous operation mode.
 *          + *true* = The library API calls are blocking, 
 *                      they return when the request processing is completed.
 *          + *false* = The library API calls return immediately,
 *                       and the completion of the request will be signaled (asynchronously)
 *                       by the user callback function registered with PAC1xxx_SetUserCallback(), 
 *                       or determined by polling the processing status with PAC1xxx_GetEventStatus().
 * - rsense:
 *      + Sense resistor (shunt) value, in micro-Ohms.
 * - i2c_init:
 *      + _PAC1xxx_I2C_INIT structure type parameter containing the initialization values for various I2C attributes 
 *        and members of the _PAC1xxx_I2C_CONTEXT structure. It is used as parameter for the PAC1xxx_I2C_Initialize() function.
 */    
typedef struct _PAC1xxx_DEVICE_INIT{
    bool syncMode;              /**< if *true* ("synchronous mode") the library API returns after the request processing is completed, 
                                     otherwise the function calls return immediately and the request processing 
                                     is signaled by user callback or by status polling */
    uint32_t rsense;            /**< Sense resistor (shunt) value, in micro-Ohms. 
                                     @warning If the value is set to 0, the API calls related to Energy, Power or Current
                                     computation are rejected. */
    PAC1xxx_I2C_INIT i2c_init;  /**< initialization values for various I2C attributes and members of 
                                     the _PAC1xxx_I2C_CONTEXT structure type. */
} PAC1xxx_DEVICE_INIT;    


/** PAC1xxx_EVENT enum defines the PAC1xxx library event identifiers returned 
 * by the PAC1xxx_GetEventStatus() function or by the event handler function 
 * registered with PAC1xxx_SetUserCallback().
 */
typedef enum {
    PAC1xxx_EVENT_NONE             = -1,    /**< No library event occured (yet) for the latest API request. */
    PAC1xxx_EVENT_REQUEST_SUCCESS  = 0,     /**< The latest API request was completed succesfully. */
    PAC1xxx_EVENT_REQUEST_FAIL     = 1,     /**< The latest API request failed. 
                                                 The reason is indicated by the _PAC1xxx_DEVICE_CONTEXT::processError. */
    PAC1xxx_EVENT_REQUEST_ABORT    = 2,     /**< The latest asynchronous API processing was aborted 
                                                 as a result of the PAC1xxx_AbortRequest() API call. */
}PAC1xxx_EVENT, *PAC1xxx_EVENT_P;


/**
 * @brief Library event handler function prototype.
 * 
 * Function prototype for the library events handler that a user application can
 * register as "callback function", using the PAC1xxx_SetUserCallback() API.
 * The library executes the "callback function" when the processing of asynchronous 
 * library API requests is finished, passing down to the user call-back
 * function the processing completion event type and the user "context" data pointer parameters.
 * @param event [in] - @ref PAC1xxx_EVENT event type indicating how the processing request was completed.
 * @param context [in] - "Transparent" pointer to a user variable of any datatype, 
 *                       as registered by the user application and returned back to the user application.
 */
typedef void ( *PAC1xxx_EVENT_HANDLER )( PAC1xxx_EVENT event, uintptr_t context );

/** @cond */
/**
 * PAC1xxx_procState enum defines the IDs associated with the various library processing stages 
 * that occur during processing the library requests. 
 * @note They are not part of the public API, being used only by the library internal logic.
 */
typedef enum _PAC1xxx_procState{
    Uninitialized = 0,
    Idle          = 1,
    Sync          = 2,

    RefreshReq                  = 10,
    GetDeviceIDReq              = 20,
    SetRegisterReq              = 30,
    GetRegister8bitReq          = 40,
    GetRegister16bitReq         = 50,
    GetRegister56bitReq         = 60,            
    GetRegister32bitReq         = 70,
           
    GetCtrlRegisterReq          = 80,
    GetCtrlLatRegisterReq       = 81,
    SetCtrlRegisterReq          = 85,
    GetNegPWRFSRRegisterReq     = 90,
    GetNegPWRFSRLatRegisterReq  = 91,            
    SetNegPWRFSRRegisterReq     = 95,
    GetSMBusRegisterReq         = 100,
    SetSMBusRegisterReq         = 110,
    GetSlowRegisterReq          = 120,
            
    GetAccCountReq                 = 130,
    GetVACCValueReq                = 140,
    GetVACCValueReq_polarityUpdate = 141,
    GetVACCValueReq_ctrlUpdate     = 142,
    GetVACCValueReq_AccCntUpdate   = 143,
            
    GetVBUSvalueReq                = 150,
    GetVBUSValueReq_polarityUpdate = 151,
            
    GetVSENSEvalueReq                = 160,
    GetVSENSEValueReq_polarityUpdate = 161,
            
    GetISENSEvalueReq                = 170,
    GetISENSEValueReq_polarityUpdate = 171,
            
    GetVPOWERValueReq                = 180,
    GetVPOWERValueReq_polarityUpdate = 181,
            
    GetAlertStatusRegisterReq   = 200,
    GetAlertConfigRegisterReq   = 210,
    GetAccFullnessRegisterReq   = 220,
    GetLimitNsamplesRegisterReq = 230,
    GetCurrentLimitValueReq     = 240,
    GetPowerLimitValueReq       = 250,
    GetVoltageLimitValueReq     = 260,
    GetStepLimitRegisterReq     = 270,
} PAC1xxx_procState;
/** @endcond */

/** @cond */
/**  
 * PAC1xxx_ProcessMode enum defines the IDs used to differentiate between various 
 * modes to process the same data registers.
 * @note They are are not part of the public API, being used only by the library internal logic.
 */
typedef enum _PAC1xxx_ProcessMode{
    ProcessNone = 0,
    ProcessVACCget = 1,          //mW or mV
    ProcessVACCenergy  = 2,      //mWh
    ProcessVACCcoulomb = 3,      //mAh
    ProcessVACCtimedEnergy  = 4, //mWh
    ProcessVACCtimedCoulomb = 5, //mAh
    ProcessCTRLlat = 6,          //CONTROL_LAT cache
    ProcessNEGPWRlat = 7,        //NEG_PWR_FSR_LAT cache
} PAC1xxx_ProcessMode;
/** @endcond */


/** @struct _PAC1xxx_DEVICE_CONTEXT 
 * @brief _PAC1xxx_DEVICE_CONTEXT structure type holds the attributes, the status and 
 * the memory variables assigned to one PAC17xx device.
 * 
 * The structure must be allocated by the user application and must be initialized
 * by calling the PAC1xxx_Device_Initialize() library function. 
 * @attention The user application must not access directly the structure members but use the 
 * library API for controlling the PAC17xx device and for reporting the data. 
 * A pointer to this structure type is provided as parameter to most of the library 
 * public API functions.   
 */
typedef struct _PAC1xxx_DEVICE_CONTEXT
{
    PAC1xxx_MUTEX mutexProcState;               /**< Mutex which helps avoiding to process 
                                                     concurrent API requests. */
    volatile PAC1xxx_procState processingState; /**< Processing state machine */
    PAC1xxx_EVENT deviceEventStatus;            /**< Keeps the processing events status.
                                                     It is reported by PAC1xxx_GetEventStatus().
                                                     It is reset every time a new request is initiated. */
    int16_t processError;                       /**< Contains the error code at the end of the request processing, 
                                                     except for the API requests rejected upfront due to:
                                                     - PAC1xxx_BUSY
                                                     - PAC1xxx_MUTEX_FAIL
                                                     - PAC1xxx_INVALID_PARAMETER
                                                     It is reported by PAC1xxx_GetEventStatus(). */
    
    PAC1xxx_I2C_CONTEXT i2c_context;            /**< I2C attributes */
    PAC1xxx_I2C_TRANSFER_EVENT i2cCommStatus;   /**< i2c communication status tracking flag. 
                                                     It is reset before each new I2C transfer. */

    PAC1xxx_EVENT_HANDLER userCallback;         /**< Pointer to the registered user call-back function
                                                     for the library events. */
    uintptr_t             userContext;          /**< pointer to the registered user variable of any datatype, 
                                                     returned back to the user application by the call-back 
                                                     function for the library events. */ 
    bool syncMode;                              /**< if *true* the library API is blocking */
    
    uint8_t i2cTxBuffer[PAC1xxx_I2C_TX_MAXSIZE];
    uint8_t i2cRxBuffer[PAC1xxx_I2C_RX_MAXSIZE];
    uint8_t i2cRxBuffer_ctrl[PAC1xxx_CONTROL_SZ + SMBUS_BYTECNT_SZ];
    uint8_t i2cRxBuffer_negPwr[PAC1xxx_NEGPWRFSR_SZ + SMBUS_BYTECNT_SZ];
    uint8_t i2cRxBuffer_accCount[PAC1xxx_ACC_COUNT_SZ + SMBUS_BYTECNT_SZ];

    bool is12bitADCres;                         /**< ADC resolution: *true* for 12-bit (PAC17xx), *false* for 16-bit (PAC18xx) */
    uint16_t VbusMAX;                           /**< Device Vbus max voltage, determined by PAC1xxx_Device_Initialize() */
    uint16_t VsenseMAX;                         /**< Device Vsense max voltage, determined by PAC1xxx_Device_Initialize() */
    uint16_t VPowerMAX;                         /**< Device Vpower max product, determined by PAC1xxx_Device_Initialize() */

    uint32_t rsense;                            /**< Sense resistor values, expressed in micro-Ohm units. */
    
    bool ENABLE_BYTE_COUNT_FLAG;                /**< Cached BYTE_COUNT flag from the SMBUS_SETTINGS register.
                                                     The flag is updated by PAC1xxx_GetSMBusSettings_reg()
                                                     and PAC1xxx_SetSMBusSettings_reg(). */

    // Computed scale values
    // "sign" depends on negPwr configuration
    uint16_t VbusScaleRange;                    /**< Computed Vbus scale range, according to NEG_PWR_FSR_LAT   */
    uint16_t VsenseScaleRange;                  /**< Computed Vsense scale range, according to NEG_PWR_FSR_LAT */
    uint16_t VPowerScaleRange;                  /**< Computed Vpower scale range, according to NEG_PWR_FSR_LAT */
    bool     IsSignedVbus;                      /**< Vbus is signed */
    bool     IsSignedVsense;                    /**< Vsense is signed */
    bool     IsSignedPower;                     /**< Power is signed */

    //cached device registers   
    PAC1xxx_deviceID deviceID;                  /**< Updated by the PAC1xxx_GetDeviceID() as soon as 
                                                     called by PAC1xxx_Device_Initialize(); */
    bool deviceID_cached;                       /**< true if *deviceID* is valid */ 
    
    PAC1xxx_CONTROL_REGFIELDS ctrl_LAT;            /**< Updated by PAC1xxx_GetCtrl_reg() and PAC1xxx_SetCtrl_reg() */
    bool ctrl_LAT_cached;                       /**< true if *ctrl_LAT* is valid */
    bool ctrl_change_pending;                   /**< true if the next refresh applies pending CONTROL configuration change */
    
    PAC1xxx_NEGPWRFSR_REGFIELDS negPwrFsr_LAT;  /**< Updated by the PAC1xxx_GetNegPwrFsr_reg() and PAC1xxx_SetNegPwrFsr_reg() */
    bool negPwr_LAT_cached;                     /**< true if *negPwrFsr_LAT* is valid */
    bool negPwr_change_pending;                 /**< true if the next refresh applies pending NEG_PWR_FSR configuration change */

    uint32_t accCount;                          /**< Updated by the PAC1xxx_GetVACC(), PAC1xxx_GetEnergy_mWh(), PAC1xxx_GetTimedEnergy_mWh()
                                                     PAC1xxx_GetCoulomb_mAs(), PAC1xxx_GetTimedCoulomb_mAs() */
    bool accCount_cached;                       /**< true if *accCount* is valid */

    PAC1xxx_ProcessMode regProcMode;            /**< Variable used internally by the library to indicate in which way to
                                                     process the read register data. */ 
    uint32_t time;                              /**< Variable to store the "time" parameter for 
                                                     PAC1xxx_GetTimedEnergy_mWh() and PAC1xxx_GetTimedCoulomb_mAs() */
    bool ABORT_REQUESTED_FLAG;                  /**< true while a processing abort request is pending. */
    
    void *outData;                              /**< Pointer to the user variable to which is copied 
                                                     the result of the API processing. */    
    uint8_t *accMode;                           /**< Pointer to the user variable to which is copied
                                                     the VACC mode indication while PAC1xxx_GetVACC() is processed. */  
} PAC1xxx_DEVICE_CONTEXT, *PAC1xxx_DEVICE_CONTEXT_P;


/** @cond */
/**  
 * PAC1xxx_REFRESH_MODE enum defines the IDs used to differentiate between various 
 * REFRESH command types.
 * @note They are are not part of the public API, bing used only by the library internal logic.
 */
typedef enum {
    PAC1xxx_REFRESH         = 0,
    PAC1xxx_REFRESH_G       = 1,
    PAC1xxx_REFRESH_V       = 2
}PAC1xxx_REFRESH_MODE, *PAC1xxx_REFRESH_MODE_P;
/** @endcond */

/* 
 * PAC1xxx library public interface (API)
 */


/**
 * @brief Register the user handler function for the library processing events.
 * 
 * The routine registers the user function that PAC1xxx library runtime  calls back 
 * every time the library generates a processing event of @ref PAC1xxx_EVENT type.
 * For instance, the user call-back function can be useful to determine the 
 * processing completion of the latest API request.
 * @param pdevice  [in,out] - PAC1xxx device context data.
 * @param userCallback [in] - The call-back function reference. 
 * @param userContext  [in] - Pointer to a variable of user defined datatype that 
 *                            the called-back function will receive as parameter.
 *                            For instance, this may be the pointer to a device 
 *                            context variable (of @ref PAC1xxx_DEVICE_CONTEXT type)
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetUserCallback(pPACdevice, appCallback, (uintptr_t)&user_data);
 * @endcode
 */
int16_t PAC1xxx_SetUserCallback(
    const PAC1xxx_DEVICE_CONTEXT_P pdevice,
    const PAC1xxx_EVENT_HANDLER userCallback,
    const uintptr_t userContext
);


/**
 * @brief Request to abort ongoing API request processing.
 * 
 * The routine can be called from user application or from timer event handler  
 * function in order to terminate the pending library request and reset the library state to "Idle".
 *  
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS* - if the abort request has been registered 
 *      - *PAC1xxx_INVALID_PARAMETER* - if *pdevice* parameter is NULL
 * @par Processing event and error code at the request completion:
 *      - *PAC1xxx_EVENT_REQUEST_ABORT* event, *PAC1xxx_REQUEST_ABORT* error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_AbortRequest(pPACdevice);
 *  //wait
 *  errorCode = PAC1xxx_GetEventStatus(pPACdevice, &pEvent, &pProcessError);
 *  
 * @endcode
 */
int16_t PAC1xxx_AbortRequest(PAC1xxx_DEVICE_CONTEXT_P pdevice);


/**
 * @brief Provides the latest library event and the processing error which generated the event.
 * 
 * This function may be used to determine the completion status of the last API 
 * request processing, especially when the library is configured in asynchronous mode (syncMode = false)).
 * @param pdevice       [in]  - PAC1xxx device context data.
 * @param pevent        [out] - The storage address for the latest reported library event value.
 * @param pProcessError [out] - The storage address for the reported API processing error
 *                              which generated the latest library event.
 * @attention *pProcessError* value must be ignored if the *pevent* value is *PAC1xxx_EVENT_NONE*
 * 
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_INVALID_PARAMETER*
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetEventStatus(pPACdevice, &pEvent, &pProcessError);
 * @endcode
*/
int16_t PAC1xxx_GetEventStatus(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_EVENT_P pevent, int16_t* pProcessError);


/**
 * @brief Initializes the PAC1xxx instance.
 * 
 * This routine initializes the PAC1xxx_DEVICE_CONTEXT device context data and 
 * the PAC1xxx device to the default configuration.
 * It also reads the device ID registers and caches their values in the device context data. 
 * 
 * @warning 
 * - The PAC1xxx_Device_Initialize() function must be executed before any other library function.
 * - The user must ensure that the platform I2C Driver initialization is completed 
 *   before the PAC1xxx_Device_Initialize() function call.
 * - The user must ensure that *deviceInit* parameter contains valid values.
 * - PAC1xxx_Device_Initialize() function is a synchronous (blocking) API, regardless the user 
 *   choice for the *syncMode* library initialization parameter.
 * @note 1ms delay must be allowed between the PAC1xxx_Device_Initialize() function call 
 *       and the following function calls. 
 * @param pdevice   [out] - Reference to the memory variable of type PAC1xxx_DEVICE_CONTEXT
 *                          which holds the context data for one PAC1xxx device instance.
 * @param deviceInit [in] - Structure that contains the initialization parameters 
 *                          for one PAC1xxx device instance such as:
 *                          - i2c communication settings 
 *                          - library sync/async operation mode
 *                          - shunt resistor value
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_MUTEX_FAIL*
 *      - *PAC1xxx_INVALID_DEVICE*
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_Device_Initialize(pPACdevice, deviceInit);
 * @endcode
 */
int16_t PAC1xxx_Device_Initialize(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_DEVICE_INIT deviceInit);


/**
 * @brief PAC1xxx device instance initialization status test.
 * 
 * The function returns the initialization status of the PAC1xxx device instance.
 * @param pdevice [in] - PAC1xxx device context data.
 * @return 
 *  - *true* - if the PAC1xxx instance was initialized 
 *  - "false" - if the PAC1xxx instance was NOT initialized
 *  - *false* - if pdevice is NULL.
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_Device_IsInitialized(pPACdevice);
 * @endcode
 */
bool PAC1xxx_Device_IsInitialized(PAC1xxx_DEVICE_CONTEXT_P pdevice);


/**
 * @brief Reports the ID values of the PAC1xxx device instance.
 * 
 * This method provides the content of the ID registers:
 * - PRODUCT_ID, 
 * - MANUFACTURER_ID and 
 * - REVISION_ID.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *   
 * @remark The function caches the ID values in the device context data. Subsequent 
 * function calls are not doing I2C transfers. 
 *
 * @param pdevice   [in,out]  - PAC1xxx device context data.
 * @param pdeviceID [out] - PAC1xxx_deviceID_P type variable to be updated with 
 *                          the content of the ID registers.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetDeviceID(pPACdevice, &deviceIDs);
 * @endcode
 */
int16_t PAC1xxx_GetDeviceID(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_deviceID_P pdeviceID);


/**
 * @brief Executes a device 'REFRESH' command.
 * 
 * This method executes the device 'REFRESH' command. 
 * The accumulator registers (power products, sample count) and the Vbus, 
 * Vsense measurements are latched into the device readable registers, 
 * the accumulators are reset and the configuration changes are applied. 
 * The latched data is stable and can be read after 1ms.
 *
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *  
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 * 
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_Refresh(pPACdevice);
 * @endcode
 */
int16_t PAC1xxx_Refresh(PAC1xxx_DEVICE_CONTEXT_P pdevice);


/**
 * @brief Executes a 'REFRESH_G' command.
 * 
 * This method executes the device 'REFRESH_G' command, using i2c 
 * General Call command. In this case, for all PAC1xxx devices connected at the
 * same i2c bus, the accumulator registers (power products, sample count) and the 
 * Vbus, Vsense measurements are latched into the devices readable registers,  
 * the accumulators are reset and the configuration changes are applied. 
 * The latched data is stable and can be read after 1ms.
 *
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *  
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 * 
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_RefreshG(pPACdevice);
 * @endcode
 */
int16_t PAC1xxx_RefreshG(PAC1xxx_DEVICE_CONTEXT_P pdevice);


/**
 * @brief Executes a device 'REFRESH_V' command.
 * 
 * This method executes the device 'REFRESH_V' command. 
 * The accumulator registers (power products, sample count) and the 
 * Vbus, Vsense measurements are latched into the device readable registers,  
 * and the configuration changes are applied but the accumulators are not reset. 
 * The latched data is stable and can be read after 1ms.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *  
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_RefreshV(pPACdevice);
 * @endcode
 */
int16_t PAC1xxx_RefreshV(PAC1xxx_DEVICE_CONTEXT_P pdevice);


/**
 * @brief Reports the *CONTROL*, *CONTROL_ACT* or *CONTROL_LAT* register value.
 * 
 * This method reports the CONTROL (target configuration), or the CONTROL_ACT (active configuration) or 
 * the CONTROL_LAT (latched configuration) register value, as indicated by the *reg_select* parameter.
 * If CONTROL_LAT is selected, the function caches the register value in the device context data.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param reg_select [in]: - The register selector parameter:
 *                           - 1- CONTROL 
 *                           - 2- CONTROL_ACT 
 *                           - 3- CONTROL_LAT
 * @param pCtrl_reg [out] - The storage address for the register value,
 *                          of @ref PAC1xxx_CONTROL_REGFIELDS type.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *   
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetCtrl_reg(pPACdevice, 1, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetCtrl_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t reg_select, PAC1xxx_CONTROL_REGFIELDS_P pCtrl_reg);


/**
 * @brief Sets the *CONTROL* register value.
 * 
 * This method sets the control register value. Followed by the REFRESH, 
 * REFRESH_V or REFRESH_G command, the new value of register will be activated.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param  pdevice  [in,out] - PAC1xxx device context data.
 * @param  Ctrl_reg [in]     - PAC1xxx_CONTROL_REGFIELDS structure holding the register bitfield values.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetCtrl_reg(pPACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetCtrl_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_CONTROL_REGFIELDS Ctrl_reg);


/**
 * @brief Reports the *ACC_COUNT* register value.
 * 
 * This method provides the count of samples that have been summed in the accumulator. 
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data.
 * @param pregister_val [out] - The storage address for the reported  
 *                              accumulator count register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetAccumulatorCount(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetAccumulatorCount(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint32_t* pregister_val);


/**
 * @brief Reports the *Accumulator Count PRESET* register (25H) value.
 * 
 * This method returns the value of the Accumulator Count PRESET register (25H) value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*. 
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data.
 * @param pregister_val [out] - The storage address for the reported  
 *                              accumulator count preset register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetAccumulatorCountPreset_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetAccumulatorCountPreset_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Sets the *Accumulator Count PRESET* register (25H) value.
 * 
 * This method sets the Accumulator Count PRESET (25H) register value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*. 
 * 
 * @param pdevice  [in,out] - PAC1xxx device context data.
 * @param register_val [in] - the register value to be set.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetAccumulatorCountPreset_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetAccumulatorCountPreset_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t register_val);


/**
 * @brief Reports the *VACC* accumulator register value.
 * 
 * This method reports the *VACC* accumulator register value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*. 
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data.
 * @param pregister_val [out] - The storage address for the reported accumulator register value.
 *
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVACCn_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVACC_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint64_t* pregister_val);


/**
 * @brief Computes the Power, the Coulomb count or the Voltage accumulated in 
 * *VACC* register.
 * 
 * This method computes the accumulated power, or the Coulomb count or 
 * the accumulated voltage depending on the VACC accumulator operation mode.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*. 
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param pvalue [out] - The storage address for the reported Power, Coulomb count or
 *                       Voltage.
 *                       The measurement unit is indicated by the *pmode* output value.
 * @param pmode  [out] - The storage address for the reported Accumulation mode (Vpower, 
 *                       Vsense or Vbus mode), as configured in the Accum Config register:
 *                       - Power accumulator - mode 0 => *pvalue* unit is milli-Watt
 *                       - Coulomb count     - mode 1 => *pvalue* unit is milli-Amp*sec
 *                       - Vbus accumulator  - mode 2 => *pvalue* unit is milli-Volt
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *      - *PAC1xxx_INVALID_ACCUMULATION_MODE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with 
 *      *PAC1xxx_I2C_FAIL* or *PAC1xxx_INVALID_ACCUMULATION_MODE* error codes
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVACCn(pPACdevice, &value, &mode);
 * @endcode
 */
int16_t PAC1xxx_GetVACC(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue, uint8_t* pmode);


/**
 * @brief Reports the *Accumulator PRESET* register (26H) value.
 * 
 * This method returns the value of the *Accumulator PRESET* register (26H) value.
 *
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *
 * @param pdevice    [in,out] - PAC1xxx device context data.
 * @param pregister_val [out] - The storage address for the reported accumulator preset register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVACCPreset_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVACCPreset_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Sets the *Accumulator PRESET* register (26H) value.
 * 
 * This method sets the *Accumulator PRESET* (26H) register value.
 *
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice  [in,out] - PAC1xxx device context data.
 * @param register_val [in] - The register value to be set.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetAccumulatorCount(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetVACCPreset_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t register_val);


/**
 * @brief Computes the energy value accumulated by *VACC* register.
 * 
 * This method computes the energy value from the power product samples accumulated
 * by the VACC register. The value unit is milli-Watt-hour.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param pvalue     [out] - Storage address for the reported energy value, in milli-Watt-hour.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *      - *PAC1xxx_INVALID_SAMPLE_MODE*
 *      - *PAC1xxx_DIFFERENT_ACCUMULATION_MODE*
 *      - *PAC1xxx_INVALID_ACCUMULATION_MODE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with 
 *      *PAC1xxx_I2C_FAIL*, or *PAC1xxx_INVALID_SAMPLE_MODE*, or 
 *      *PAC1xxx_DIFFERENT_ACCUMULATION_MODE*, or *PAC1xxx_INVALID_ACCUMULATION_MODE* error codes.
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetEnergy_mWh(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetEnergy_mWh(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Computes the energy value for the power product accumulation in *VACC*
 * during the user measured time interval.
 * 
 * This method computes the energy value accumulated by VACC in the measured time interval
 * lapsed between the accumulator reset and the last refresh command.
 * The value unit is milli-Watt-hour.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param pvalue     [out] - The storage address for the reported energy value, in milli-Watt-hour.
 * @param time        [in]  - The accumulation time, in milli-seconds.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *      - *PAC1xxx_DIFFERENT_ACCUMULATION_MODE*
 *      - *PAC1xxx_INVALID_ACCUMULATION_MODE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with 
 *      *PAC1xxx_I2C_FAIL*, or 
 *      *PAC1xxx_DIFFERENT_ACCUMULATION_MODE*, or *PAC1xxx_INVALID_ACCUMULATION_MODE* error codes.
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetTimedEnergy_mWh(pPACdevice, &value, time);
 * @endcode
 */
int16_t PAC1xxx_GetTimedEnergy_mWh(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue, uint32_t time);


/**
 * @brief Computes the Coulomb count value accumulated by *VACC*.
 * 
 * This method computes the Coulomb count value for the VSENSE accumulation in VACC. 
 * The value unit is milli-Amp-sec.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param pvalue     [out] - The storage address for the reported Coulomb count value, in milli-Amp-sec.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *      - *PAC1xxx_INVALID_SAMPLE_MODE*
 *      - *PAC1xxx_DIFFERENT_ACCUMULATION_MODE*
 *      - *PAC1xxx_INVALID_ACCUMULATION_MODE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with 
 *      *PAC1xxx_I2C_FAIL*, or *PAC1xxx_INVALID_SAMPLE_MODE*, or 
 *      *PAC1xxx_DIFFERENT_ACCUMULATION_MODE*, or *PAC1xxx_INVALID_ACCUMULATION_MODE* error codes.
 *
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetCoulomb_mAs(PACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetCoulomb_mAs(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Computes the Coulomb count value for the user measured time 
 * interval of VSENSE accumulation in *VACC*.
 * 
 * This method computes the Coulomb count value for the accumulated VSENSE
 * in the measured time interval lapsed between the accumulator reset and the last refresh command.
 * The value unit is milli-Amp-sec.
 * 
  * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param pvalue     [out] - The storage address for the reported Coulomb count value, in milli-Amp-sec
 * @param time        [in] - The accumulation time, in milli-sec 
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *      - *PAC1xxx_DIFFERENT_ACCUMULATION_MODE*
 *      - *PAC1xxx_INVALID_ACCUMULATION_MODE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with 
 *      *PAC1xxx_I2C_FAIL*, or
 *      *PAC1xxx_DIFFERENT_ACCUMULATION_MODE*, or *PAC1xxx_INVALID_ACCUMULATION_MODE* error codes.
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetTimedCoulomb_mAs(PACdevice, &value, time);
 * @endcode
 */
int16_t PAC1xxx_GetTimedCoulomb_mAs(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue, uint32_t time);


/**
 * @brief Reports the *VBUS* register value.
 * 
 * This method provides the register value of the latched bus voltage sample.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *  
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the reported bus voltage register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVBUSn_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVBUS_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Computes the voltage value measured by *VBUS*.
 * 
 * This method computes the voltage value of the VBUS sample, expressed in milli-Volts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated bus voltage value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVBUSn_mV(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetVBUS_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Reports the *VSENSE* register digital value.
 * 
 * This method gets the digital value of the sense voltage sample latched in the *VSENSE* register.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the reported register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVSENSEn_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVSENSE_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Computes the *VSENSE* voltage value.
 * 
 * This method computes the Voltage value of the sense voltage sample latched in 
 * the *VSENSE* register. The value unit is milli-Volt.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated voltage value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVSENSEn_mV(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetVSENSE_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Computes the Amperage value measured by *VSENSE*.
 * 
 * This method computes the Amperage of the current flowing through the 
 * sense resistor, measured by VSENSE and expressed in mA.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated sense current value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetISENSEn_mA(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetISENSE_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Reports the *VBUS_AVG* register value.
 * 
 * This method gets the VBUS_AVG register value, a rolling average of 
 * the 8 most recent VBUS samples.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the reported VBUS_AVG register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVBUSn_AVG_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVBUS_AVG_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Computes the average voltage value measured by *VBUS_AVG* register.
 * 
 * This method computes the voltage value reported by *VBUS_AVG*, a rolling average of 
 * the 8 most recent bus voltage measurements.
 * The unit value is milli-Volt.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the computed bus voltage average value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVBUSn_AVG_mV(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetVBUS_AVG_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Reports the *VSENSE_AVG* register value.
 * 
 * This method reports the VSENSE_AVG register value, which is a rolling average of 
 * the 8 most recent sense voltage measurements.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address of the VSENSE_AVG register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVSENSEn_AVG_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVSENSE_AVG_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Computes the voltage average value measured by *VSENSE_AVG*.
 * 
 * This method reports the voltage value of a rolling average of 
 * the 8 most recent VSENSE samples.
 * The value unit is milli-Volt.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated voltage value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVSENSEn_AVG_mV(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetVSENSE_AVG_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Computes the Amperage average value measured by *VSENSE_AVG*.
 * 
 * This method computes the average Amperage value measured by VSENSE_AVG, 
 * which is the rolling average of the 8 most recent VSENSE voltage samples.
 * The value unit is milli-Amp.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated Amperage average.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetISENSEn_AVG_mA(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetISENSE_AVG_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Reports the *VPOWER* register digital value.
 * 
 * This method returns the register value of the proportional power product - *VPOWER*.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the VPOWER register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVPOWERn_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVPOWER_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint32_t* pregister_val);


/**
 * @brief Computes the power value from the *VPOWER* power product value.
 * 
 * This method computes the power from the power product latched by the VPOWER register,
 * expressed in mW.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated value of power.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVPOWERn_mW(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetVPOWER_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief  Reports the *VBUS Minimum* register value.
 * 
 * This method provides the digital value of the VBUS_minimum register.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *  
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVBUSmin_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVBUSmin_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Computes the voltage value of the *VBUS_Minimum* register.
 * 
 * This method computes the voltage value of the sample latched in the *VBUS_Minimum* register, 
 * expressed in milli-Volts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *  
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated voltage value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVBUSmin_mV(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetVBUSmin_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief  Reports the *VBUS Maximum* register value.
 * 
 * This method provides the digital value of the VBUS_maximum register.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *  
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVBUSmax_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVBUSmax_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Computes the voltage value of the *VBUS_Maximum* register.
 * 
 * This method computes the voltage value of the sample latched in the *VBUS_Maximum* register, 
 * expressed in milli-Volts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *  
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated voltage value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVBUSmax_mV(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetVBUSmax_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Reports the *VSENSE_Minimum* register digital value.
 * 
 * This method provides the register digital value of the *VSENSE_Minimum* register.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVSENSEmin_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVSENSEmin_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Computes the voltage value of the *VSENSE_Minimum* register.
 * 
 * This method computes the voltage value of the sample latched in the 
 * *VSENSE_Minimum* register, expressed in milli-Volts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *  
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated voltage value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVSENSEmin_mV(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetVSENSEmin_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Reports the *VSENSE_Maximum* register digital value.
 * 
 * This method provides the register digital value of the *VSENSE_Maximum* register.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVSENSEmax_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVSENSEmax_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Computes the voltage value of the *VSENSE_Maximum* register.
 * 
 * This method computes the voltage value of the sample latched in the 
 * *VSENSE_Maximum* register, expressed in milli-Volts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *  
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated voltage value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVSENSEmax_mV(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetVSENSEmax_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Computes the minimum Amperage value latched by *VSENSE_Minimum* register.
 * 
 * This method computes the minimum Amperage of the current flowing through the 
 * sense resistor, as sampled by *VSENSE_minimum* register and expressed in mA.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated Amperage value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetISENSEmin_mA(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetISENSEmin_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Computes the maximum Amperage value latched by *VSENSE_Maximum* register.
 * 
 * This method computes the maximum Amperage of the current flowing through the 
 * sense resistor, as sampled by *VSENSE_maximum* register and expressed in mA.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated Amperage value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetISENSEmax_mA(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetISENSEmax_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Reports the *VPOWER_Minimum* register digital value.
 * 
 * This method returns the digital value of the minimum proportional power product
 * latched by the *VPOWER_Minimum* register.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register digital value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVPOWERmin_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVPOWERmin_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint32_t* pregister_val);


/**
 * @brief Computes the minimum power value latched in the *VPOWER* power product value.
 * 
 * This method computes the minimum power from the power product latched by 
 * the *VPOWER_Minimum* register, expressed in mW.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated value of power.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVPOWERmin_mW(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetVPOWERmin_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Reports the *VPOWER_Maximum* register digital value.
 * 
 * This method returns the digital value of the maximum proportional power product,
 * latched by the *VPOWER_Maximum* register.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice   [in,out]  - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register digital value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVPOWERmax_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetVPOWERmax_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint32_t* pregister_val);


/**
 * @brief Computes the maximum power value, latched by the *VPOWER_Maximum* register.
 * 
 * This method computes the maximum power from the power product latched by 
 * the *VPOWER_Maximum* register, expressed in mW.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated value of power.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *      - *PAC1xxx_INVALID_SHUNT_VALUE*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetVPOWERmax_mW(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetVPOWERmax_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Reports the *SMBus_Settings* register value.
 * 
 * This method returns the register value of the *SMBus_Settings*.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pSMBus_reg [out] - The storage address for the register bit-field values, 
 *                           of PAC1xxx_SMBUS_SETTINGS_REGFIELDS type.  
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetSMBusSettings_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetSMBusSettings_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_SMBUS_SETTINGS_REGFIELDS_P pSMBus_reg);


/**
 * @brief Sets the *SMBus_Settings* register value.
 * 
 * This method sets the *SMBus_Settings* register value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param  pdevice   [in,out] - PAC1xxx device context data.
 * @param  SMBus_reg     [in] - PAC1xxx_SMBUS_SETTINGS_REGFIELDS structure holding the register bit-field values.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetSMBusSettings_reg(pPACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetSMBusSettings_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_SMBUS_SETTINGS_REGFIELDS SMBus_reg);


/**
 * @brief Reports the *NEG_PWR_FSR*, *NEG_PWR_FSR_ACT* or *NEG_PWR_FSR_LAT* register value.
 * 
 * This method reports the NEG_PWR_FSR (target configuration), or the NEG_PWR_FSR_ACT (active configuration) or 
 * the NEG_PWR_FSR_LAT (latched configuration) register value, as indicated by the *reg_select* parameter.
 * If NEG_PWR_FSR_LAT is selected, the function caches the register value in the device context data.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 *
 * @param pdevice    [in,out] - PAC1xxx device context data.
 * @param reg_select [in]     - The register selector parameter:
 *                              - 1 - NEG_PWR_FSR, 
 *                              - 2 - NEG_PWR_FSR_ACT, 
 *                              - 3 - NEG_PWR_FSR_LAT
 * @param pNegPwrFsr_reg [out] - The storage address for the register bit-field values,
 *                               of PAC1xxx_NEGPWRFSR_REGFIELDS type.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetNegPwrFsr_reg(PACdevice, 1, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetNegPwrFsr_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t reg_select, PAC1xxx_NEGPWRFSR_REGFIELDS_P pNegPwrFsr_reg);


/**
 * @brief Sets the *NEG_PWR_FSR* register value.
 * 
 * This method sets the current NEG_PWR_FSR register value. Followed by the REFRESH, 
 * REFRESH_V or REFRESH_G command, the new value of register will be activated.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice   [in,out] - PAC1xxx device context data.
 * @param NegPwrFsr_reg [in] - PAC1xxx_NEGPWRFSR_REGFIELDS structure holding the register bit-field values.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetNegPwrFsr_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetNegPwrFsr_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_NEGPWRFSR_REGFIELDS NegPwrFsr_reg);


/**
 * @brief Reports the *SLOW* register value.
 * 
 * This method reports the SLOW register bit-field values.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice   [in,out] - PAC1xxx device context data.
 * @param pSlow_reg    [out] - The storage address for the register bit-field values,
 *                             of PAC1xxx_SLOW_REGFIELDS type.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetSlow_reg(PACdevice, &reg_value);
 * @endcode 
 */
int16_t PAC1xxx_GetSlow_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_SLOW_REGFIELDS_P pSlow_reg);


/**
 * @brief Sets the *SLOW* register value.
 * 
 * This method sets the SLOW register value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice  [in,out] - PAC1xxx device context data.
 * @param Slow_reg     [in] - PAC1xxx_SLOW_REGFIELDS structure holding the register bit-field values.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetSlow_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetSlow_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_SLOW_REGFIELDS Slow_reg);


/**
 * @brief Reports the *ALERT_STATUS* register value.
 * 
 * This method reports the *ALERT_STATUS* register bit-field values.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice       [in,out] - PAC1xxx device context data.
 * @param pAlertStatus_reg [out] - The storage address for the register bit-field values,
 *                                 of PAC1xxx_ALERT_STATUS_REGFIELDS type.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetAlertStatus_reg(PACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetAlertStatus_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_STATUS_REGFIELDS_P pAlertStatus_reg);


/**
 * @brief Reports the *ALERT_ENABLE* register value.
 * 
 * This method reports the *ALERT_ENABLE* register bit-field values.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice        in,out] - PAC1xxx device context data.
 * @param pAlertEnable_reg [out] - The storage address for the register bit-field values,
 *                                 of PAC1xxx_ALERT_ENABLE_REGFIELDS type.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetAlertEnable_reg(PACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetAlertEnable_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS_P pAlertEnable_reg);


/**
 * @brief Sets the *ALERT_ENABLE* register value.
 * 
 * This method sets the *ALERT_ENABLE* register value. Followed by the REFRESH, 
 * REFRESH_V or REFRESH_G command, the new value of register will be activated.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice     [in,out] - PAC1xxx device context data.
 * @param AlertEnable_reg [in] - PAC1xxx_ALERT_ENABLE_REGFIELDS structure holding 
 *                               the register bit-field values.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetAlertEnable_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetAlertEnable_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS AlertEnable_reg);


/**
 * @brief Reports the *SLOW_ALERT0* register value.
 * 
 * This method reports the *SLOW_ALERT0* register bit-field values.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice      [in,out] - PAC1xxx device context data.
 * @param pSlowAlert0_reg [out] - The storage address for the register bit-field values,
 *                                of PAC1xxx_ALERT_ENABLE_REGFIELDS type.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetSlowAlert0_reg(PACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetSlowAlert0_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS_P pSlowAlert0_reg);


/**
 * @brief Sets the *SLOW_ALERT0* register value.
 * 
 * This method sets the *SLOW_ALERT0* register value. Followed by the REFRESH, 
 * REFRESH_V or REFRESH_G command, the new value of register will be activated.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice     [in,out] - PAC1xxx device context data.
 * @param AlertEnable_reg [in] - PAC1xxx_ALERT_ENABLE_REGFIELDS structure holding 
 *                               the register bit-field values.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetSlowAlert0_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetSlowAlert0_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS AlertEnable_reg);


/**
 * @brief Reports the *GPIO_ALERT1* register value.
 * 
 * This method reports the *GPIO_ALERT1* register bit-field values.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice      [in,out] - PAC1xxx device context data.
 * @param pGpioAlert1_reg [out] - The storage address for the register bit-field values,
 *                                of PAC1xxx_ALERT_ENABLE_REGFIELDS type.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetGpioAlert1_reg(PACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetGpioAlert1_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS_P pGpioAlert1_reg);


/**
 * @brief Sets the *GPIO_ALERT1* register value.
 * 
 * This method sets the *GPIO_ALERT1* register value. Followed by the REFRESH, 
 * REFRESH_V or REFRESH_G command, the new value of register will be activated.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice     [in,out] - PAC1xxx device context data.
 * @param AlertEnable_reg [in] - PAC1xxx_ALERT_ENABLE_REGFIELDS structure holding 
 *                               the register bit-field values.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetGpioAlert1_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetGpioAlert1_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS AlertEnable_reg);


/**
 * @brief Reports the *ACC_Fullness_limits* register value.
 * 
 * This method reports the *ACC_Fullness_limits* register bit-field values.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice             [in,out] - PAC1xxx device context data.
 * @param pAccFullnessLimits_reg [out] - The storage address for the register bit-field values,
 *                                       of PAC1xxx_ACCUM_LIMITS_REGFIELDS type.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetAccFullness_reg(PACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetAccFullness_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ACCUM_LIMITS_REGFIELDS_P pAccFullnessLimits_reg);


/**
 * @brief Sets the *ACC_Fullness_limits* register value.
 * 
 * This method sets the *ACC_Fullness_limits* register value. Followed by the REFRESH, 
 * REFRESH_V or REFRESH_G command, the new value of register will be activated.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice           [in,out] - PAC1xxx device context data.
 * @param AccFullnessLimits_reg [in] - PAC1xxx_ACCUM_LIMITS_REGFIELDS structure holding 
 *                                     the register bit-field values.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetAccFullness_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetAccFullness_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ACCUM_LIMITS_REGFIELDS AccFullnessLimits_reg);


/**
 * @brief Reports the *OC_limit* register value.
 * 
 * This method returns the *OC_limit* (Over Current limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetOClimit_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetOClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t* pregister_val);


/**
 * @brief Sets the *OC_limit* register value.
 * 
 * This method sets the *OC_limit* (Over Current limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice  [in,out] - PAC1xxx device context data.
 * @param register_val [in] - The register value to be set.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetOClimit_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetOClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t register_val);


/**
 * @brief Reports the Amperage limit value set in the *OC_limit* register.
 * 
 * This method reports the Amperage value set in the *OC_limit* (Over Current limit)
 * register, expressed in milli-Amps.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated Amperage limit value.
 * @warning If the shunt resistor value is set to 0 at library initialization 
 * (see PAC1xxx_Device_Initialize()) the returned Amperage limit value is 0. 
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetOClimit_mA(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetOClimit_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Sets the Amperage limit value in the *OC_limit* register.
 * 
 * This method sets the Amperage value in the *OC_limit* (Over Current limit) register,
 * expressed in milli-Amps.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param value       [in] - The Amperage value to be set.
 * @warning 
 *  - If the sense (shunt) resistor value is set to 0 at library initialization 
 *    (see PAC1xxx_Device_Initialize()) the register value is set to 0. 
 *  - If the value exceeds the Amperage that the device can measure 
 *    with the configured sense (shunt) resistor, the function writes 
 *    the maximum positive (0x7f) or the minimum negative (0x80) 8-bit digital value
 *    into the limit register.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetOClimit_mA(pPACdevice, limitValue);
 * @endcode
 */
int16_t PAC1xxx_SetOClimit_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value);


/**
 * @brief Reports the *UC_limit* register value.
 * 
 * This method returns the *UC_limit* (Under Current limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice   [in,out]  - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetUClimit_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetUClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t* pregister_val);


/**
 * @brief Sets the *UC_limit* register value.
 * 
 * This method sets the *UC_limit* (Under Current limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice  [in,out] - PAC1xxx device context data.
 * @param register_val [in] - The register value to be set.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetUClimit_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetUClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t register_val);


/**
 * @brief Reports the Amperage limit value set in the *UC_limit* register.
 * 
 * This method reports the Amperage value set in the *UC_limit* (Under Current limit)
 * register, expressed in milli-Amps.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated Amperage limit value.
 * @warning If the shunt resistor value is set to 0 at library initialization 
 * (see PAC1xxx_Device_Initialize()) the returned Amperage limit value is 0. 
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetUClimit_mA(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetUClimit_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Sets the Amperage limit value in the *UC_limit* register.
 * 
 * This method sets the Amperage value in the *UC_limit* (Under Current limit) register,
 * expressed in milli-Amps.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param value       [in] - The Amperage value to be set.
 * @warning 
 *  - If the sense (shunt) resistor value is set to 0 at library initialization 
 *    (see PAC1xxx_Device_Initialize()) the register value is set to 0. 
 *  - If the value exceeds the Amperage that the device can measure 
 *    with the configured sense (shunt) resistor, the function writes 
 *    the maximum positive (0x7f) or the minimum negative (0x80) 8-bit digital value
 *    into the limit register.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetUClimit_mA(pPACdevice, limitValue);
 * @endcode
 */
int16_t PAC1xxx_SetUClimit_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value);


/**
 * @brief Reports the *OP_Warning_limit* register value.
 * 
 * This method returns the *OP_Warning_limit* (Over Power Warning limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetOPWlimit_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetOPWlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Reports the *OP_Critical_limit* register value.
 * 
 * This method returns the *OP_Critical_limit* (Over Power Critical limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetOPClimit_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetOPClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val);


/**
 * @brief Sets the *OP_Warning_limit* register value.
 * 
 * This method sets the *OP_Warning_limit* (Over Power Warning limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice  [in,out] - PAC1xxx device context data.
 * @param register_val [in] - The register value to be set.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetOPWlimit_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetOPWlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t register_val);


/**
 * @brief Sets the *OP_Critical_limit* register value.
 * 
 * This method sets the *OP_Critical_limit* (Over Power Critical limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice  [in,out] - PAC1xxx device context data.
 * @param register_val [in] - The register value to be set.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetOPClimit_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetOPClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t register_val);


/**
 * @brief Reports the Power limit value set in the *OP_Warning_limit* register.
 * 
 * This method reports the Power value set in the *OP_Warning_limit* (Over Power Warning limit)
 * register, expressed in milli-Watts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated power limit value.
 * @warning If the shunt resistor value is set to 0 at library initialization 
 * (see PAC1xxx_Device_Initialize()) the returned Power limit value is 0. 
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetOPWlimit_mW(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetOPWlimit_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Reports the Power limit value set in the *OP_Critical_limit* register.
 * 
 * This method reports the Power value set in the *OP_Critical_limit* (Over Power Critical limit)
 * register, expressed in milli-Watts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated power limit value.
 * @warning If the shunt resistor value is set to 0 at library initialization 
 * (see PAC1xxx_Device_Initialize()) the returned Power limit value is 0. 
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetOPClimit_mW(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetOPClimit_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Sets the Power limit value in the *OP_Warning_limit* register.
 * 
 * This method sets the Power value in the *OP_Warning_limit* (Over Power Warning limit) register,
 * expressed in milli-Watts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param value       [in] - The Power value to be set.
 * @warning
 *  - If the sense (shunt) resistor value is set to 0 at library initialization 
 *    (see PAC1xxx_Device_Initialize()) the register value is set to 0. 
 *  - If the value exceeds the Power that the device can measure 
 *    with the configured sense (shunt) resistor, the function writes 
 *    the maximum positive (0x7fff) or the minimum negative (0x8000) 16-bit digital value
 *    into the limit register.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetOPWlimit_mW(pPACdevice, limitValue);
 * @endcode
 */
int16_t PAC1xxx_SetOPWlimit_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value);


/**
 * @brief Sets the Power limit value in the *OP_Critical_limit* register.
 * 
 * This method sets the Power value in the *OP_Critical_limit* (Over Power Critical limit) register,
 * expressed in milli-Watts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param value       [in] - The Power value to be set.
 * @warning
 *  - If the sense (shunt) resistor value is set to 0 at library initialization 
 *    (see PAC1xxx_Device_Initialize()) the register value is set to 0. 
 *  - If the value exceeds the Power that the device can measure 
 *    with the configured sense (shunt) resistor, the function writes 
 *    the maximum positive (0x7fff) or the minimum negative (0x8000) 16-bit digital value
 *    into the limit register.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetOPClimit_mW(pPACdevice, limitValue);
 * @endcode
 */
int16_t PAC1xxx_SetOPClimit_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value);


/**
 * @brief Reports the *OV_limit* register value.
 * 
 * This method returns the *OV_limit* (Over Voltage limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetOVlimit_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetOVlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t* pregister_val);


/**
 * @brief Sets the *OV_limit* register value.
 * 
 * This method sets the *OV_limit* (Over Voltage limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice  [in,out] - PAC1xxx device context data.
 * @param register_val [in] - The register value to be set.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetOVlimit_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetOVlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t register_val);


/**
 * @brief Reports the Voltage limit value set in the *OV_limit* register.
 * 
 * This method reports the Voltage value set in the *OV_limit* (Over Voltage limit)
 * register, expressed in milli-Volts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated Voltage limit value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetOVlimit_mV(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetOVlimit_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Sets the Voltage limit value in the *OV_limit* register.
 * 
 * This method sets the Voltage value in the *OV_limit* (Over Voltage limit) register,
 * expressed in milli-Volts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param value       [in] - The Voltage value to be set.
 * @warning
 *  - If the value exceeds the Voltage that the device can measure 
 *    the function writes the maximum positive (0x7f) or 
 *    the minimum negative (0x80) 8-bit digital value into the limit register.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetOVlimit_mV(pPACdevice, limitValue);
 * @endcode
 */
int16_t PAC1xxx_SetOVlimit_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value);


/**
 * @brief Reports the *UV_limit* register value.
 * 
 * This method returns the *UV_limit* (Under Voltage limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice    [in,out] - PAC1xxx device context data
 * @param pregister_val [out] - The storage address for the register value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetUVlimit_reg(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetUVlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t* pregister_val);


/**
 * @brief Sets the *UV_limit* register value.
 * 
 * This method sets the *UV_limit* (Under Voltage limit) register digital value.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice  [in,out] - PAC1xxx device context data.
 * @param register_val [in] - The register value to be set.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetUVlimit_reg(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetUVlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t register_val);


/**
 * @brief Reports the Voltage limit value set in the *UV_limit* register.
 * 
 * This method reports the Voltage value set in the *UV_limit* (Under Voltage limit)
 * register, expressed in milli-Volts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data
 * @param pvalue     [out] - The storage address for the calculated Voltage limit value.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetUVlimit_mV(pPACdevice, &value);
 * @endcode
 */
int16_t PAC1xxx_GetUVlimit_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue);


/**
 * @brief Sets the Voltage limit value in the *UV_limit* register.
 * 
 * This method sets the Voltage value in the *UV_limit* (Under Voltage limit) register,
 * expressed in milli-Volts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @param value       [in] - The Voltage value to be set.
 * @warning
 *  - If the value exceeds the Voltage that the device can measure 
 *    the function writes the maximum positive (0x7f) or 
 *    the minimum negative (0x80) 8-bit digital value into the limit register.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 *  
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetUVlimit_mV(pPACdevice, limitValue);
 * @endcode
 */
int16_t PAC1xxx_SetUVlimit_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value);


/**
 * @brief Reports the *STEP_limit* register value.
 * 
 * This method reports the *STEP_limit* register bit-field values. 
 * The device detects the step changes in the average value of voltage (*VBUS_AVG*)
 * or current (*VSENSE_AVG*) and triggers the related alerts 
 * if the step changes exceed the set limits.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice     [in,out] - PAC1xxx device context data.
 * @param pStepLimit_reg [out] - The storage address for the register bit-field values,
 *                               of PAC1xxx_STEP_LIMIT_REGFIELDS type.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetStepLimit(pPACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetStepLimit(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_STEP_LIMIT_REGFIELDS_P pStepLimit_reg);


/**
 * @brief Sets the *STEP_limit* register value.
 * 
 * This method sets the *STEP_limit* register bit-field values.
 * The device detects the step changes in the average value of voltage (*VBUS_AVG*)
 * or current (*VSENSE_AVG*) and triggers the related alerts 
 * if the step changes exceed the set limits.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*. 
 * 
 * @param pdevice   [in,out] - PAC1xxx device context data.
 * @param StepLimit_reg [in] - PAC1xxx_STEP_LIMIT_REGFIELDS structure 
 *                             holding the register bit-field values.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetStepLimit(pPACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetStepLimit(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_STEP_LIMIT_REGFIELDS StepLimit_reg);


/**
 * @brief Reports the *Limit Nsamples* register value.
 * 
 * This method reports the *Limit Nsamples* register bit-field values, 
 * which indicate the number of consecutive samples exceeding the limits required
 * to trigger the OPC, OPW, OC, UC, OV or UV alerts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*.
 * 
 * @param pdevice         [in,out] - PAC1xxx device context data.
 * @param pLimitNsamples_reg [out] - The storage address for the register bit-field values,
 *                                   of PAC1xxx_LIMIT_NSAMPLES_REGFIELDS type.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_GetLimitNsamples_reg(PACdevice, &reg_value);
 * @endcode
 */
int16_t PAC1xxx_GetLimitNsamples(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_LIMIT_NSAMPLES_REGFIELDS_P pLimitNsamples_reg);


/**
 * @brief Sets the *Limit Nsamples* register value.
 * 
 * This method sets the *Limit Nsamples* register bit-field values, which indicate 
 * the number of consecutive samples exceeding the limits required to trigger 
 * the OPC, OPW, OC, UC, OV or UV alerts.
 * 
 * @warning
 * - If the library is configured in synchronous mode (syncMode = true) 
 *   the function call returns after the request processing is completed. 
 *   The returned code indicates if the request succeeded or it failed.
 * - If the library is configured in asynchronous mode (syncMode = false) 
 *   the function call returns immediately and the request processing is performed 
 *   by the library runtime function - see *PAC1xxx_LibTask()*.
 *   The returned error code indicates if the request is pending or if it failed.
 *   The user application can determine the request processing completion in two ways:  
 *   + status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + register a library event handler function, using *PAC1xxx_SetUserCallback()*. 
 * 
 * @param pdevice       [in,out] - PAC1xxx device context data.
 * @param LimitNsamples_reg [in] - PAC1xxx_LIMIT_NSAMPLES_REGFIELDS structure 
 *                                 holding the register bit-field values.
 * @return The error code:
 *      - *PAC1xxx_SUCCESS*
 *      - *PAC1xxx_REQUEST_PENDING*
 *      - *PAC1xxx_REQUEST_ABORT*
 *      - *PAC1xxx_I2C_FAIL*
 *      - *PAC1xxx_BUSY*
 *      - *PAC1xxx_INVALID_PARAMETER*
 *
 * @par The asynchronous API processing events and the errors which generates them are:
 * - *PAC1xxx_EVENT_REQUEST_SUCCESS* event with *PAC1xxx_SUCCESS*       error code
 * - *PAC1xxx_EVENT_REQUEST_ABORT*   event with *PAC1xxx_REQUEST_ABORT* error code
 * - *PAC1xxx_EVENT_REQUEST_FAIL*    event with *PAC1xxx_I2C_FAIL*      error code
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_SetLimitNsamples(PACdevice, reg_value);
 * @endcode
 */
int16_t PAC1xxx_SetLimitNsamples(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_LIMIT_NSAMPLES_REGFIELDS LimitNsamples_reg);


/**
 * @brief Indicates if the PAC1xxx device instance is busy.
 * 
 * The method returns the indication if the PAC1xxx instance is already busy processing 
 * an API request or if it can accept a new request.
 * @param pdevice [in] - PAC1xxx device context data.
 * @return
 *  - *true* - if the PAC1xxx instance is busy
 *  - *false* - if the PAC1xxx instance is NOT busy.
 *  - *false* - if *pdevice* parameter is NULL
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_Device_IsBusy(pPACdevice);
 * @endcode
 */
bool PAC1xxx_Device_IsBusy(PAC1xxx_DEVICE_CONTEXT_P pdevice);



/**
 * @brief Library runtime function for API request processing.
 * 
 * This is the library runtime function which monitors the I2C communication status
 * and continues the processing of the current API request when the data transfers are completed.
 * It initiates successive I2C transfers as needed in the case of complex API requests
 * which need data from multiple device registers.
 * The runtime function indicates the success or the failure of the pending API processing 
 * by recording an "event" of @ref PAC1xxx_EVENT type and an error code into the device context data.
 * Also, the runtime function executes the event handler callback function if 
 * there is one registered by the user application - see PAC1xxx_SetUserCallback().
 * 
 * The returned error code indicates if the function call was completed or if some
 * error condition forced the unexpected function exit.
 *    
 * @warning
 * The function implements a state machine and needs to be executed periodically in order to 
 * execute all the processing steps required by a certain API request:
 * - If the library is configured in *synchronous* mode (syncMode = true) 
 *   the function call is executed in a "busy-waiting" loop by the pending API request function, 
 *   until the API processing is completed with success or failure. The error code returned by the
 *   API function call indicates the result status.
 * - If the library is configured in *asynchronous* mode (syncMode = false) 
 *   the function call must be included by the user in a periodic application task or thread.
 *   For example, PAC1xxx_LibTask() may be registered as a periodic timer expiration event handler 
 *   or included on the list of tasks executed by the application main loop.  
 *   The user application can determine if the pending API request processing was completed
 *   in two ways:  
 *   + by status polling,  using PAC1xxx_GetEventStatus() API, or
 *   + by callback function, using *PAC1xxx_SetUserCallback()* to register a 
 *   library event handler function,.
 * 
 * @param pdevice [in,out] - PAC1xxx device context data.
 * @return The error code:
 *      - *PAC1xxx_LIBTASK_DONE*
 *      - *PAC1xxx_LIBTASK_FAIL*
 * 
 * Example:
 * @code
 *  errorCode = PAC1xxx_LibTask(pPACdevice);
 * @endcode
 */
int16_t PAC1xxx_LibTask(PAC1xxx_DEVICE_CONTEXT_P pdevice);

//L2 API following
/////////////////////////////////////

/**
 * @brief Returns as sample rate value the *SAMPLE_MODE* bit-field.
 * 
 * The method decodes and returns as sample rate value the *SAMPLE_MODE* bit-field value
 * of the *PAC1xxx_CONTROL_REGFIELDS* structure.
 * 
 * @param ctrlReg [in] - PAC1xxx_CONTROL_REGFIELDS structure holding the register bit-field values.
 * @return
 *  - sampling rate values: 8, 64, 256, 1024, 4096, 8192 or 16384
 *  - single-shot value: 1
 *  - sleep mode value (no sampling): 0
 *  - -1: invalid bit-field value
 * 
 * Example:
 * @code
 *  sampleRate = PAC1xxx_DecodeCTRLtoSampleRate(ctrlReg);
 * @endcode
 */
int16_t PAC1xxx_DecodeCTRLtoSampleRate(PAC1xxx_CONTROL_REGFIELDS ctrlReg);


/**
 * @brief Copy a 16-bit unsigned value to a memory buffer, as 2-byte BIG-ENDIAN byte-stream.
 * 
 * The method makes a copy of a 16-bit unsigned value into a memory buffer, 
 * in BIG-ENDIAN byte order.
 * The function can be used to write a 16-bit value into a I2C Tx buffer,
 * in order to transfer it into a 16-bit device register.
 * 
 * @warning
 * The buffer address parameter must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param regVal    [in]  - The integer value.
 * @param pRawBytes [out] - The address of the memory buffer.
 * 
 * Example:
 * @code
 *  PAC1xxx_Reg16bitToRawBytes(regValue, pBuffer);
 * @endcode
 */
void PAC1xxx_Reg16bitToRawBytes(uint16_t regVal, uint8_t* pRawBytes);


/**
 * @brief Returns a 2-byte BIG-ENDIAN byte-stream as a 16-bit unsigned value.
 * 
 * The method returns a 2-byte BIG-ENDIAN byte-stream as a 16-bit unsigned value.
 * The function can be used to read the 16-bit value from the I2C Rx buffer after 
 * the data transfer from a 16-bit device register.
 * 
 * @warning
 * The buffer address parameter must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pRawBytes [in] - The address of the memory buffer containing 
 *                         the BIG-ENDIAN byte-stream.
 * @return
 *  16-bit unsigned value
 * 
 * Example:
 * @code
 *  regValue = PAC1xxx_RawBytestoReg16bit(pBuffer);
 * @endcode
 */
uint16_t PAC1xxx_RawBytestoReg16bit(uint8_t* pRawBytes);


/**
 * @brief Returns a 4-byte BIG-ENDIAN byte-stream as a 32-bit unsigned value.
 * 
 * The method returns a 4-byte BIG-ENDIAN byte-stream as a 32-bit unsigned value.
 * The function can be used to read a 32-bit value from the I2C Rx buffer after 
 * the data transfer from a 32-bit device register.
 * 
 * @warning
 * The buffer address parameter must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pRawBytes [in] - The address of the memory buffer containing 
 *                         the BIG-ENDIAN byte-stream.
 * @return
 *  32-bit unsigned value
 * 
 * Example:
 * @code
 *  regValue = PAC1xxx_RawBytestoReg32bit(pBuffer);
 * @endcode
 */
uint32_t PAC1xxx_RawBytestoReg32bit(uint8_t* pRawBytes);


/**
 * @brief Returns a 7-byte BIG-ENDIAN byte-stream as a 64-bit unsigned value.
 * 
 * The method returns a 7-byte BIG-ENDIAN byte-stream as a 64-bit unsigned value.
 * The function can be used to read 56-bit value from the I2C Rx buffer after 
 * the data transfer from a 56-bit device register.
 * 
 * @warning
 * The buffer address parameter must be no-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pRawBytes [in] - The address of the memory buffer containing 
 *                         the BIG-ENDIAN byte-stream.
 * @return
 *  64-bit unsigned value
 * 
 * Example:
 * @code
 *  regValue = PAC1xxx_RawBytesToReg64bit(pBuffer);
 * @endcode
 */
uint64_t PAC1xxx_RawBytesToReg64bit(uint8_t* pRawBytes);


/**
 * @brief Copies the bit-fields of the *CONTROL*, *CONTROL_ACT* or *CONTROL_LAT* register raw value
 * into a memory variable of PAC1xxx_CONTROL_REGFIELDS type.
 * 
 * The method copies the bit-fields of the *CONTROL*, *CONTROL_ACT* or *CONTROL_LAT* register 
 * raw value into a memory variable of PAC1xxx_CONTROL_REGFIELDS type.
 * The function can be used to read into PAC1xxx_CONTROL_REGFIELDS type variable 
 * from the I2C Rx buffer after the data transfer from a *CONTROL*, *CONTROL_ACT* or *CONTROL_LAT*
 * device register.
 * The register raw value is a 2-byte BIG-ENDIAN byte-stream.
 * 
 * @warning
 * The pointer type function parameters must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pCtrlBytes     [in]  - The memory buffer address containing the raw register byte-stream.
 * @param pCtrlRegfields [out] - The storage address for the register bit-field values,
 *                               of PAC1xxx_CONTROL_REGFIELDS type.
 * 
 * Example:
 * @code
 *  PAC1xxx_CtrlBytesToRegfields(pBuffer, &ctrlReg);
 * @endcode
 */
void PAC1xxx_CtrlBytesToRegfields(uint8_t* pCtrlBytes, PAC1xxx_CONTROL_REGFIELDS_P pCtrlRegfields);


/**
 * @brief Copies the bit-fields of a memory variable of PAC1xxx_CONTROL_REGFIELDS type
 * into a register raw byte-stream for *CONTROL*, *CONTROL_ACT* or *CONTROL_LAT* register.
 * 
 * The method copies the bit-fields of a memory variable of PAC1xxx_CONTROL_REGFIELDS type
 * into a register raw byte-stream buffer.
 * The function can be used to write the PAC1xxx_CONTROL_REGFIELDS type variable 
 * into the I2C Tx buffer in order to transfer the raw byte-stream to a 
 * *CONTROL*, *CONTROL_ACT* or *CONTROL_LAT* device register.
 * The register raw value is a 2-byte BIG-ENDIAN byte-stream.
 * 
 * @warning
 * The pointer type function parameter must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param CtrlRegfields [in]  - PAC1xxx_CONTROL_REGFIELDS structure holding the register bit-field values.
 * @param pCtrlBytes    [out] - The memory buffer address for the raw register byte-stream.
 * 
 * Example:
 * @code
 *  PAC1xxx_CtrlRegfieldsToBytes(ctrlReg, pBuffer);
 * @endcode
 */
void PAC1xxx_CtrlRegfieldsToBytes(PAC1xxx_CONTROL_REGFIELDS CtrlRegfields, uint8_t* pCtrlBytes);


/**
 * @brief Copies the bit-fields of the *NEG_PWR_FSR*, *NEG_PWR_FSR_ACT* or *NEG_PWR_FSR_LAT*
 * register raw value into a memory variable of PAC1xxx_NEGPWRFSR_REGFIELDS type.
 * 
 * The method copies the bit-fields of the *NEG_PWR_FSR*, *NEG_PWR_FSR_ACT* or *NEG_PWR_FSR_LAT*
 * register raw value into a memory variable of PAC1xxx_NEGPWRFSR_REGFIELDS type.
 * The function can be used to read into PAC1xxx_NEGPWRFSR_REGFIELDS type variable 
 * from the I2C Rx buffer after the data transfer from a *NEG_PWR_FSR*, *NEG_PWR_FSR_ACT* or 
 * *NEG_PWR_FSR_LAT* device register.
 * The register raw value is 1-byte long.
 * 
 * @warning
 * The pointer type function parameters must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pNegPwrFsrBytes     [in]  - The memory buffer address containing the raw register byte-stream.
 * @param pNegPwrFsrRegfields [out] - The storage address for the register bit-field values,
 *                                    of PAC1xxx_NEGPWRFSR_REGFIELDS type.
 * 
 * Example:
 * @code
 *  PAC1xxx_NegPwrFsrBytesToRegfields(pBuffer, &negPwrFsrReg);
 * @endcode
 */
void PAC1xxx_NegPwrFsrBytesToRegfields(uint8_t* pNegPwrFsrBytes, PAC1xxx_NEGPWRFSR_REGFIELDS_P pNegPwrFsrRegfields);


/**
 * @brief Copies the bit-fields of a memory variable of PAC1xxx_NEGPWRFSR_REGFIELDS type
 * into a register raw byte-stream for *NEG_PWR_FSR*, *NEG_PWR_FSR_ACT* or *NEG_PWR_FSR_LAT* register.
 * 
 * The method copies the bit-fields of a memory variable of PAC1xxx_NEGPWRFSR_REGFIELDS type
 * into a register raw byte-stream buffer.
 * The function can be used to write the PAC1xxx_NEGPWRFSR_REGFIELDS type variable 
 * into the I2C Tx buffer in order to transfer the raw byte-stream to a 
 * *NEG_PWR_FSR*, *NEG_PWR_FSR_ACT* or *NEG_PWR_FSR_LAT* device register.
 * The register raw value is 1-byte long.
 * 
 * @warning
 * The pointer type function parameter must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param NegPwrFsrRegfields [in]  - PAC1xxx_NEGPWRFSR_REGFIELDS structure holding the register bit-field values.
 * @param pNegPwrFsrBytes    [out] - The memory buffer address for the raw register byte-stream.
 * 
 * Example:
 * @code
 *  PAC1xxx_NegPwrFsrRegfieldsToBytes(negPwrFsrReg, pBuffer);
 * @endcode
 */
void PAC1xxx_NegPwrFsrRegfieldsToBytes(PAC1xxx_NEGPWRFSR_REGFIELDS NegPwrFsrRegfields, uint8_t* pNegPwrFsrBytes);


/**
 * @brief Copies the bit-fields of the *SMBUS_SETTINGS* register raw value
 * into a memory variable of PAC1xxx_SMBUS_SETTINGS_REGFIELDS type.
 * 
 * The method copies the bit-fields of the *SMBUS_SETTINGS* register 
 * raw value into a memory variable of PAC1xxx_SMBUS_SETTINGS_REGFIELDS type.
 * The function can be used to read into PAC1xxx_SMBUS_SETTINGS_REGFIELDS type variable 
 * from the I2C Rx buffer after the data transfer from a *SMBUS_SETTINGS*
 * device register.
 * The register raw value is 1-byte long.
 * 
 * @warning
 * The pointer type function parameters must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pSMBusBytes     [in]  - The memory buffer address containing the raw register byte-stream.
 * @param pSMBusRegfields [out] - The storage address for the register bit-field values,
 *                               of PAC1xxx_SMBUS_SETTINGS_REGFIELDS type.
 * 
 * Example:
 * @code
 *  PAC1xxx_SMBusBytesToRegfields(pBuffer, &smbusReg);
 * @endcode
 */
void PAC1xxx_SMBusBytesToRegfields(uint8_t* pSMBusBytes, PAC1xxx_SMBUS_SETTINGS_REGFIELDS_P pSMBusRegfields);


/**
 * @brief Copies the bit-fields of a memory variable of PAC1xxx_SMBUS_SETTINGS_REGFIELDS type
 * into a register raw byte-stream for *SMBUS_SETTINGS* register.
 * 
 * The method copies the bit-fields of a memory variable of PAC1xxx_SMBUS_SETTINGS_REGFIELDS type
 * into a register raw byte-stream buffer.
 * The function can be used to write the PAC1xxx_SMBUS_SETTINGS_REGFIELDS type variable 
 * into the I2C Tx buffer in order to transfer the raw byte-stream to a 
 * *SMBUS_SETTINGS* device register.
 * The register raw value is 1-byte long.
 * 
 * @warning
 * The pointer type function parameter must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param SMBusRegfields [in]  - PAC1xxx_SMBUS_SETTINGS_REGFIELDS structure holding the register bit-field values.
 * @param pSMBusBytes    [out] - The memory buffer address for the raw register byte-stream.
 * 
 * Example:
 * @code
 *  PAC1xxx_SMBusRegfieldsToBytes(smbusReg, pBuffer);
 * @endcode
 */
void PAC1xxx_SMBusRegfieldsToBytes(PAC1xxx_SMBUS_SETTINGS_REGFIELDS SMBusRegfields, uint8_t* pSMBusBytes);


/**
 * @brief Copies the bit-fields of the *SLOW* register raw value
 * into a memory variable of PAC1xxx_SLOW_REGFIELDS type.
 * 
 * The method copies the bit-fields of the *SLOW* register 
 * raw value into a memory variable of PAC1xxx_SLOW_REGFIELDS type.
 * The function can be used to read into PAC1xxx_SLOW_REGFIELDS type variable 
 * from the I2C Rx buffer after the data transfer from a *SLOW*
 * device register.
 * The register raw value is 1-byte long.
 * 
 * @warning
 * The pointer type function parameters must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pSlowBytes     [in]  - The memory buffer address containing the raw register byte-stream.
 * @param pSlowRegfields [out] - The storage address for the register bit-field values,
 *                               of PAC1xxx_SLOW_REGFIELDS type.
 * 
 * Example:
 * @code
 *  PAC1xxx_SlowBytesToRegfields(pBuffer, &slowReg);
 * @endcode
 */
void PAC1xxx_SlowBytesToRegfields(uint8_t* pSlowBytes, PAC1xxx_SLOW_REGFIELDS_P pSlowRegfields);


/**
 * @brief Copies the bit-fields of a memory variable of PAC1xxx_SLOW_REGFIELDS type
 * into a register raw byte-stream for *SLOW* register.
 * 
 * The method copies the bit-fields of a memory variable of PAC1xxx_SLOW_REGFIELDS type
 * into a register raw byte-stream buffer.
 * The function can be used to write the PAC1xxx_SLOW_REGFIELDS type variable 
 * into the I2C Tx buffer in order to transfer the raw byte-stream to a 
 * *SLOW* device register.
 * The register raw value is 1-byte long.
 * 
 * @warning
 * The pointer type function parameter must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param SlowRegfields [in]  - PAC1xxx_SLOW_REGFIELDS structure holding the register bit-field values.
 * @param pSlowBytes    [out] - The memory buffer address for the raw register byte-stream.
 * 
 * Example:
 * @code
 *  PAC1xxx_SlowRegfieldsToBytes(slowReg, pBuffer);
 * @endcode
 */
void PAC1xxx_SlowRegfieldsToBytes(PAC1xxx_SLOW_REGFIELDS SlowRegfields, uint8_t* pSlowBytes);


/**
 * @brief Copies the bit-fields of the *ALERT_STATUS* register raw value
 * into a memory variable of PAC1xxx_ALERT_STATUS_REGFIELDS type.
 * 
 * The method copies the bit-fields of the *ALERT_STATUS* register 
 * raw value into a memory variable of PAC1xxx_ALERT_STATUS_REGFIELDS type.
 * The function can be used to read into PAC1xxx_ALERT_STATUS_REGFIELDS type variable 
 * from the I2C Rx buffer after the data transfer from a *ALERT_STATUS*
 * device register.
 * The register raw value is a 2-byte BIG-ENDIAN byte-stream.
 * 
 * @warning
 * The pointer type function parameters must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pAlertBytes           [in]  - The memory buffer address containing the raw register byte-stream.
 * @param pAlertStatusRegfields [out] - The storage address for the register bit-field values,
 *                                      of PAC1xxx_ALERT_STATUS_REGFIELDS type.
 * 
 * Example:
 * @code
 *  PAC1xxx_AlertStatusBytesToRegfields(pBuffer, &alertStatusReg);
 * @endcode
 */
void PAC1xxx_AlertStatusBytesToRegfields(uint8_t* pAlertBytes, PAC1xxx_ALERT_STATUS_REGFIELDS_P pAlertStatusRegfields);


/**
 * @brief Copies the bit-fields of the *ALERT_ENABLE* register raw value
 * into a memory variable of PAC1xxx_ALERT_ENABLE_REGFIELDS type.
 * 
 * The method copies the bit-fields of the *ALERT_ENABLE* register 
 * raw value into a memory variable of PAC1xxx_ALERT_ENABLE_REGFIELDS type.
 * The function can be used to read into PAC1xxx_ALERT_ENABLE_REGFIELDS type variable 
 * from the I2C Rx buffer after the data transfer from a *ALERT_ENABLE*
 * device register.
 * The register raw value is a 2-byte BIG-ENDIAN byte-stream.
 * 
 * @warning
 * The pointer type function parameters must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pAlertBytes           [in]  - The memory buffer address containing the raw register byte-stream.
 * @param pAlertEnableRegfields [out] - The storage address for the register bit-field values,
 *                                      of PAC1xxx_ALERT_ENABLE_REGFIELDS type.
 * 
 * Example:
 * @code
 *  PAC1xxx_AlertEnableBytesToRegfields(pBuffer, &alertEnableReg);
 * @endcode
 */
void PAC1xxx_AlertEnableBytesToRegfields(uint8_t* pAlertBytes, PAC1xxx_ALERT_ENABLE_REGFIELDS_P pAlertEnableRegfields);


/**
 * @brief Copies the bit-fields of a memory variable of PAC1xxx_ALERT_ENABLE_REGFIELDS type
 * into a register raw byte-stream for *ALERT_ENABLE* register.
 * 
 * The method copies the bit-fields of a memory variable of PAC1xxx_ALERT_ENABLE_REGFIELDS type
 * into a register raw byte-stream buffer.
 * The function can be used to write the PAC1xxx_ALERT_ENABLE_REGFIELDS type variable 
 * into the I2C Tx buffer in order to transfer the raw byte-stream to a 
 * *ALERT_ENABLE* device register.
 * The register raw value is a 2-byte BIG-ENDIAN byte-stream.
 * 
 * @warning
 * The pointer type function parameter must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param AlertEnableRegfields [in]  - PAC1xxx_ALERT_ENABLE_REGFIELDS structure holding the register bit-field values.
 * @param pAlertBytes          [out] - The memory buffer address for the raw register byte-stream.
 * 
 * Example:
 * @code
 *  PAC1xxx_AlertEnableRegfieldsToBytes(alertEnableReg, pBuffer);
 * @endcode
 */
void PAC1xxx_AlertEnableRegfieldsToBytes(PAC1xxx_ALERT_ENABLE_REGFIELDS AlertEnableRegfields, uint8_t* pAlertBytes);


/**
 * @brief Copies the bit-fields of the *ACC_Fullness_limits* register raw value
 * into a memory variable of PAC1xxx_ACCUM_LIMITS_REGFIELDS type.
 * 
 * The method copies the bit-fields of the *ACC_Fullness_limits* register 
 * raw value into a memory variable of PAC1xxx_ACCUM_LIMITS_REGFIELDS type.
 * The function can be used to read into PAC1xxx_ACCUM_LIMITS_REGFIELDS type variable 
 * from the I2C Rx buffer after the data transfer from a *ACC_Fullness_limits*
 * device register.
 * The register raw value is 1-byte long.
 * 
 * @warning
 * The pointer type function parameters must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pAccFullnessBytes     [in]  - The memory buffer address containing the raw register byte-stream.
 * @param pAccFullnessRegfields [out] - The storage address for the register bit-field values,
 *                                      of PAC1xxx_ACCUM_LIMITS_REGFIELDS type.
 * 
 * Example:
 * @code
 *  PAC1xxx_AccFullnessBytesToRegfields(pBuffer, &accFullnessReg);
 * @endcode
 */
void PAC1xxx_AccFullnessBytesToRegfields(uint8_t* pAccFullnessBytes, PAC1xxx_ACCUM_LIMITS_REGFIELDS_P pAccFullnessRegfields);


/**
 * @brief Copies the bit-fields of a memory variable of PAC1xxx_ACCUM_LIMITS_REGFIELDS type
 * into a register raw byte-stream for *ACC_Fullness_limits* register.
 * 
 * The method copies the bit-fields of a memory variable of PAC1xxx_ACCUM_LIMITS_REGFIELDS type
 * into a register raw byte-stream buffer.
 * The function can be used to write the PAC1xxx_ACCUM_LIMITS_REGFIELDS type variable 
 * into the I2C Tx buffer in order to transfer the raw byte-stream to a 
 * *ACC_Fullness_limits* device register.
 * The register raw value is 1-byte long.
 * 
 * @warning
 * The pointer type function parameter must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param AccFullnessRegfields [in]  - PAC1xxx_ACCUM_LIMITS_REGFIELDS structure holding the register bit-field values.
 * @param pAccFullnessBytes    [out] - The memory buffer address for the raw register byte-stream.
 * 
 * Example:
 * @code
 *  PAC1xxx_AccFullnessRegfieldsToBytes(accFullnessReg, pBuffer);
 * @endcode
 */
void PAC1xxx_AccFullnessRegfieldsToBytes(PAC1xxx_ACCUM_LIMITS_REGFIELDS AccFullnessRegfields, uint8_t* pAccFullnessBytes);


/**
 * @brief Copies the bit-fields of the *STEP_limit* register raw value
 * into a memory variable of PAC1xxx_STEP_LIMIT_REGFIELDS type.
 * 
 * The method copies the bit-fields of the *STEP_limit* register 
 * raw value into a memory variable of PAC1xxx_STEP_LIMIT_REGFIELDS type.
 * The function can be used to read into PAC1xxx_STEP_LIMIT_REGFIELDS type variable 
 * from the I2C Rx buffer after the data transfer from a *STEP_limit* device register.
 * The register raw value is 1-byte long.
 * 
 * @warning
 * The pointer type function parameters must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pStepLimitBytes     [in]  - The memory buffer address containing the raw register byte-stream.
 * @param pStepLimitRegfields [out] - The storage address for the register bit-field values,
 *                                    of PAC1xxx_STEP_LIMIT_REGFIELDS type.
 * 
 * Example:
 * @code
 *  PAC1xxx_StepLimitBytesToRegfields(pBuffer, &stepLimitReg);
 * @endcode
 */
void PAC1xxx_StepLimitBytesToRegfields(uint8_t* pStepLimitBytes, PAC1xxx_STEP_LIMIT_REGFIELDS_P pStepLimitRegfields);


/**
 * @brief Copies the bit-fields of a memory variable of PAC1xxx_STEP_LIMIT_REGFIELDS type
 * into a register raw byte-stream for *STEP_limit* register.
 * 
 * The method copies the bit-fields of a memory variable of PAC1xxx_STEP_LIMIT_REGFIELDS type
 * into a register raw byte-stream buffer.
 * The function can be used to write the PAC1xxx_STEP_LIMIT_REGFIELDS type variable 
 * into the I2C Tx buffer in order to transfer the raw byte-stream to a 
 * *STEP_limit* device register.
 * The register raw value is 1-byte long.
 * 
 * @warning
 * The pointer type function parameter must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param StepLimitRegfields [in]  - PAC1xxx_STEP_LIMIT_REGFIELDS structure holding the register bit-field values.
 * @param pStepLimitBytes    [out] - The memory buffer address for the raw register byte-stream.
 * 
 * Example:
 * @code
 *  PAC1xxx_StepLimitRegfieldsToBytes(stepLimitReg, pBuffer);
 * @endcode
 */
void PAC1xxx_StepLimitRegfieldsToBytes(PAC1xxx_STEP_LIMIT_REGFIELDS StepLimitRegfields, uint8_t* pStepLimitBytes);


/**
 * @brief Copies the bit-fields of the *Limit_Nsamples* register raw value
 * into a memory variable of PAC1xxx_LIMIT_NSAMPLES_REGFIELDS type.
 * 
 * The method copies the bit-fields of the *Limit_Nsamples* register 
 * raw value into a memory variable of PAC1xxx_LIMIT_NSAMPLES_REGFIELDS type.
 * The function can be used to read into PAC1xxx_LIMIT_NSAMPLES_REGFIELDS type variable 
 * from the I2C Rx buffer after the data transfer from a *Limit_Nsamples* device register.
 * The register raw value is a 2-byte BIG-ENDIAN byte-stream.
 * 
 * @warning
 * The pointer type function parameters must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param pNsamplesBytes     [in]  - The memory buffer address containing the raw register byte-stream.
 * @param pNsamplesRegfields [out] - The storage address for the register bit-field values,
 *                                   of PAC1xxx_LIMIT_NSAMPLES_REGFIELDS type.
 * 
 * Example:
 * @code
 *  PAC1xxx_NsamplesBytesToRegfields(pBuffer, &nsamplesReg);
 * @endcode
 */
void PAC1xxx_NsamplesBytesToRegfields(uint8_t* pNsamplesBytes, PAC1xxx_LIMIT_NSAMPLES_REGFIELDS_P pNsamplesRegfields);


/**
 * @brief Copies the bit-fields of a memory variable of PAC1xxx_LIMIT_NSAMPLES_REGFIELDS type
 * into a register raw byte-stream for *Limit_Nsamples* register.
 * 
 * The method copies the bit-fields of a memory variable of PAC1xxx_LIMIT_NSAMPLES_REGFIELDS type
 * into a register raw byte-stream buffer.
 * The function can be used to write the PAC1xxx_LIMIT_NSAMPLES_REGFIELDS type variable 
 * into the I2C Tx buffer in order to transfer the raw byte-stream to a 
 * *Limit_Nsamples* device register.
 * The register raw value is a 2-byte BIG-ENDIAN byte-stream.
 * 
 * @warning
 * The pointer type function parameter must be non-NULL.
 * The function makes no NULL-pointer check.
 * 
 * @param NsamplesRegfields [in]  - PAC1xxx_LIMIT_NSAMPLES_REGFIELDS structure holding the register bit-field values.
 * @param pNsamplesBytes    [out] - The memory buffer address for the raw register byte-stream.
 * 
 * Example:
 * @code
 *  PAC1xxx_NsamplesRegfieldsToBytes(nsamplesReg, pBuffer);
 * @endcode
 */
void PAC1xxx_NsamplesRegfieldsToBytes(PAC1xxx_LIMIT_NSAMPLES_REGFIELDS NsamplesRegfields, uint8_t* pNsamplesBytes);


/**
 * @brief Sign check of the *VBUS* value, for the input PAC1xxx_NEGPWRFSR_REGFIELDS value.
 * 
 * This method checks the sign of the *VBUS* digital value for the given PAC1xxx_NEGPWRFSR_REGFIELDS
 * input value. For example, it can be used to determine the sign of the latched *VBUS* sample, 
 * providing as input the PAC1xxx_NEGPWRFSR_REGFIELDS value of the *NEG_PWR_FSR_LAT* register.   
 * 
 * @warning The function assumes that CFG_VB bit-field input value is a valid one. 
 * If the value is "Reserved", the function returns *false*.
 * 
 * @param negpwrfsr [in] - PAC1xxx_NEGPWRFSR_REGFIELDS structure holding the "FSR" register bit-field values.
 * @return 
 *      - *true*  if VBUS measurement is signed (bipolar configuration).
 *      - *false* if VBUS measurement is unsigned (unipolar configuration).
 *      - *false* if *CFG_VB* bit-field input value is "Reserved" (0b11).
 *
 * Example:
 * @code
 *  signed = PAC1xxx_IsSignedVbus(negpwrReg);
 * @endcode
 */
bool PAC1xxx_IsSignedVbus(PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr);


/**
 * @brief Sign check of the *VSENSE* value for the input PAC1xxx_NEGPWRFSR_REGFIELDS value.
 * 
 * This method checks the sign of the *VSENSE* digital value for the given PAC1xxx_NEGPWRFSR_REGFIELDS
 * input value. For example, it can be used to determine the sign of the latched *VSENSE* sample, 
 * providing as input the PAC1xxx_NEGPWRFSR_REGFIELDS value of the *NEG_PWR_FSR_LAT* register.   
 *
 * @warning The function assumes that CFG_VS bit-field input value is a valid one. 
 * If the value is "Reserved", the function returns *false*.
 *  
 * @param negpwrfsr [in] - PAC1xxx_NEGPWRFSR_REGFIELDS structure holding the "FSR" register bit-field values.
 * @return 
 *      - *true*  if VSENSE measurement is signed (bipolar configuration).
 *      - *false* if VSENSE measurement is unsigned (unipolar configuration).
 *      - *false* if *CFG_VS* bit-field input value is "Reserved" (0b11).
 *
 * Example:
 * @code
 *  signed = PAC1xxx_IsSignedVsense(negpwrReg);
 * @endcode
 */
bool PAC1xxx_IsSignedVsense(PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr);


/**
 * @brief Sign check of the *VPOWER* value for the input PAC1xxx_NEGPWRFSR_REGFIELDS value.
 * 
 * This method checks the sign of the *VPOWER* digital value for the given PAC1xxx_NEGPWRFSR_REGFIELDS
 * input value. For example, it can be used to determine the sign of the latched *VPOWER* sample, 
 * providing as input the PAC1xxx_NEGPWRFSR_REGFIELDS value of the *NEG_PWR_FSR_LAT* register.   
 *
 * @warning The function assumes that CFG_VB and CFG_VS bit-field input values are valid ones. 
 * If the value of any of them is "Reserved", the function returns *false*.
 *  
 * @param negpwrfsr [in] - PAC1xxx_NEGPWRFSR_REGFIELDS structure holding the "FSR" register bit-field values.
 * @return 
 *      - *true*  if VPOWER measurement is signed (bipolar configuration).
 *      - *false* if VPOWER measurement is unsigned (unipolar configuration).
 *      - *false* if *CFG_VB* or *CFG_VS* bit-field input value is "Reserved" (0b11).
 *
 * Example:
 * @code
 *  signed = PAC1xxx_IsSignedVpower(negpwrReg);
 * @endcode
 */
bool PAC1xxx_IsSignedVpower(PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr);


/**
 * @brief Returns the *VBUS* scale range.
 * 
 * This method computes the *VBUS* scale range value to be used in the conversion of the 
 * *VBUS* digital code into a Voltage value. For example, the computed VBUS scale range value is used 
 * as input parameter for the PAC1xxx_VoltageReg16bitToVoltage_mV() function.
 * The *VBUS* scale range is derived from the device maximum supported (peak) *VBUS* voltage value
 * and the VBUS FSR configuration - CFG_VB - bit-field value of the input PAC1xxx_NEGPWRFSR_REGFIELDS 
 * data structure.
 * 
 * The input maximum voltage and the returned *VBUS* scale range voltage are positive values,
 * expressed in milli-Volts.
 
 * @param VbusMAX   [in] - *VBUS* maximum supported value, expressed in milli-Volts.
 * @param negpwrfsr [in] - PAC1xxx_NEGPWRFSR_REGFIELDS structure holding the "FSR" register bit-field values.
 * @return The computed *VBUS* scale range value, expressed in milli-Volts.
 *
 * Example:
 * @code
 *  signed = PAC1xxx_VbusScaleRange(PAC1711_VBUS_MAX_mV, negpwrReg);
 * @endcode
 */
uint16_t PAC1xxx_VbusScaleRange(uint16_t VbusMAX, PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr);


/**
 * @brief Returns the *VSENSE* scale range.
 * 
 * This method computes the *VSENSE* scale range value to be used in the conversion of the 
 * *VSENE* digital code into a Voltage value. For example, the computed *VSENSE* scale range value is used 
 * as input parameter for the PAC1xxx_VpowerReg32bitToPower_mW() or 
 * PAC1xxx_VoltageReg16bitToCurrent_mA() functions.
 * The *VSENSE* scale range is derived from the device maximum supported (peak) *VSENSE* voltage value
 * and the VSENSE FSR configuration - CFG_VS - bit-field value of the input PAC1xxx_NEGPWRFSR_REGFIELDS 
 * data structure.
 * 
 * The input maximum voltage and the returned *VSENSE* scale range voltage are positive values,
 * expressed in milli-Volts.
 
 * @param VsenseMax [in] - *VSENSE* maximum supported value, expressed in milli-Volts.
 * @param negpwrfsr [in] - PAC1xxx_NEGPWRFSR_REGFIELDS structure holding the "FSR" register bit-field values.
 * @return The computed *VSENSE* scale range value, expressed in milli-Volts.
 *
 * Example:
 * @code
 *  signed = PAC1xxx_VsenseScaleRange(PAC1xxx_VSENSE_MAX_mV, negpwrReg);
 * @endcode
 */
uint16_t PAC1xxx_VsenseScaleRange(uint16_t VsenseMax, PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr);


/**
 * @brief Returns the *VPOWER* scale range.
 * 
 * This method computes the *VPOWER* scale range value to be used in the conversion of the 
 * *VPOWER* digital code into a Power value. For example, the computed *VPOWER* scale range value is used 
 * as input parameter for the PAC1xxx_VpowerReg32bitToPower_mW() or 
 * PAC1xxx_VaccReg64bitToPower_mW() functions.
 * The *VPOWER* scale range is derived from the device maximum supported (peak) *VPOWER* value,
 * the VBUS FSR configuration - CFG_VB - and the VSENSE FSR configuration - CFG_VS - bit-field
 * values of the input PAC1xxx_NEGPWRFSR_REGFIELDS data structure.
 * 
 * The input maximum *VPOWER* and the returned *VPOWER* scale range are positive values,
 * expressed in milli-Volts^2.
 
 * @param VPowerMax [in] - *VPOWER* maximum supported value, expressed in milli-Volts^2.
 * @param negpwrfsr [in] - PAC1xxx_NEGPWRFSR_REGFIELDS structure holding the "FSR" register bit-field values.
 * @return The computed *VPOWER* scale range value, expressed in milli-Volts^2.
 *
 * Example:
 * @code
 *  signed = PAC1xxx_VpowerScaleRange(PAC1711_VPOWER_MAX_mV2, negpwrReg);
 * @endcode
 */
uint16_t PAC1xxx_VpowerScaleRange(uint16_t VPowerMax, PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr);


/**
 * @brief Converts to Voltage the accumulated digital value from *VACC* register.
 * 
 * This method computes the Voltage value from the digital value accumulated by the 
 * *VACC* register, provided that it is configured to accumulate *VBUS* or *VSENSE* samples. 
 * The result is expressed in milli-Volts.
 * 
 * @param VAccReg           [in] - *VACC* register digital value, 
 *                                 acquired with I2C read protocol or 
 *                                 with PAC1xxx_GetVACC_reg() function.
 * @param IsSignedVoltage   [in] - *VACC* sign indication, 
 *                                 as reported by PAC1xxx_IsSignedVbus() or
 *                                 by PAC1xxx_IsSignedVsense(). 
 * @param VoltageScaleRange [in] - The *VBUS* voltage scale range or 
 *                                 the *VSENSE* voltage scale range,
 *                                 as reported by PAC1xxx_VbusScaleRange() or
 *                                 by PAC1xxx_VsenseScaleRange()
 * @param is12bitADCres    [in] 
 *                               - *true* if the device ADC resolution is 12bit (PAC17xx family).
 *                               - *false* if the device ADC resolution is 16 bit (PAC18xx family).
 * @return The accumulated Voltage value, expressed in milli-Volts.
 * 
 * Example:
 * @code
 *  // If *VACC* is configured to accumulate VBUS //
 *  negpwrLatReg = PAC1xxx_NegPwrFsrBytesToRegfields(pi2cRxNegPwrFsrLatBytes);
 *  vaccReg = PAC1xxx_RawBytesToReg64bit(pi2cRxVACCbytes);
 *  vbusVoltageSum = PAC1xxx_VaccReg64bitToVoltage_mV(
 *                           vaccReg, 
 *                           PAC1xxx_IsSignedVbus(negpwrLatReg), 
 *                           PAC1xxx_VbusScaleRange(PAC1711_VBUS_MAX_mV, negpwrLatReg),
 *                           true);
 * 
 * // If *VACC* is configured to accumulate VSENSE //
 * negpwrLatReg = PAC1xxx_NegPwrFsrBytesToRegfields(pi2cRxNegPwrFsrLatBytes); 
 * vaccReg = PAC1xxx_RawBytesToReg64bit(pi2cRxVACCBytes);
 * vsenseVoltageSum = PAC1xxx_VaccReg64bitToVoltage_mV(
 *                          vaccReg, 
 *                          PAC1xxx_IsSignedVsense(negpwrLatReg), 
 *                          PAC1xxx_VsenseScaleRange(PAC1xxx_VSENSE_MAX_mV, negpwrLatReg),
 *                          true);
 * @endcode
 */
float PAC1xxx_VaccReg64bitToVoltage_mV(uint64_t VAccReg, bool IsSignedVoltage, uint16_t VoltageScaleRange, bool is12bitADCres);

/**
 * @brief Converts to Power the accumulated digital value from *VACC* register.
 * 
 * This method computes the Power value from the digital value accumulated by the 
 * *VACC* register, provided that it is configured to accumulate *VPOWER* samples. 
 * The result is expressed in milli-Watts.
 * 
 * @param VAccReg          [in] - *VACC* register digital value, 
 *                                acquired with I2C read protocol or 
 *                                with PAC1xxx_GetVACC_reg() function.
 * @param IsSignedPower    [in] - *VACC* sign indication, 
 *                                as reported by PAC1xxx_IsSignedVpower()
 * @param VPowerScaleRange [in] - The *VPOWER* scale range
 *                                as reported by PAC1xxx_VpowerScaleRange()
 * @param is12bitADCres    [in] 
 *                              - *true* if the device ADC resolution is 12bit (PAC17xx family).
 *                              - *false* if the device ADC resolution is 16 bit (PAC18xx family).
 * @param rsense           [in] - The sense (shunt) resistor value, 
 *                                expressed in micro-Ohms.
 * @return The accumulated Power value, expressed in milli-Watts.
 * @warning If rsense value is 0, the returned value is 0.
 * 
 * Example:
 * @code
 *  negpwrLatReg = PAC1xxx_NegPwrFsrBytesToRegfields(pi2cRxNegPwrFsrLatBytes);
 *  vaccReg = PAC1xxx_RawBytesToReg64bit(pi2cRxVACCbytes);
 *  vpowerSum = PAC1xxx_VaccReg64bitToPower_mW(
 *                          vaccReg, 
 *                          PAC1xxx_IsSignedVpower(negpwrLatReg), 
 *                          PAC1xxx_VpowerScaleRange(PAC1711_VPOWER_MAX_mV2, negpwrLatReg),
 *                          true,
 *                          rsense_uOhm);
 * @endcode
 */
float PAC1xxx_VaccReg64bitToPower_mW(uint64_t VAccReg, bool IsSignedPower, uint16_t VPowerScaleRange, bool is12bitADCres, uint32_t rsense);

/**
 * @brief Converts to Power the digital value from the *VPOWER* register.
 * 
 * This method computes the Power value from the digital value reported by the 
 * *VPOWER*, *VPOWER_Minimum* or "VPOWER_Maximum* register. 
 * The result is expressed in milli-Watts.
 * 
 * @param VpowerReg        [in] - *VPOWER* register digital value, 
 *                                acquired with I2C read protocol or 
 *                                with PAC1xxx_GetVPOWER_reg() function.
 * @param IsSignedPower    [in] - *VPOWER* sign indication, 
 *                                as reported by PAC1xxx_IsSignedVpower()
 * @param VPowerScaleRange [in] - The *VPOWER* scale range
 *                                as reported by PAC1xxx_VpowerScaleRange()
 * @param rsense           [in] - The sense (shunt) resistor value, 
 *                                expressed in micro-Ohms.
 * @return The Power value, expressed in milli-Watts.
 * @warning If rsense value is 0, the returned value is 0.
 * 
 * Example:
 * @code
 *  negpwrLatReg = PAC1xxx_NegPwrFsrBytesToRegfields(pi2cRxNegPwrFsrLatBytes);
 *  vpowerReg = PAC1xxx_RawBytesToReg32bit(pi2cRxVPOWERbytes);
 *  vpowerSum = PAC1xxx_VpowerReg32bitToPower_mW(
 *                          vpowerReg, 
 *                          PAC1xxx_IsSignedVpower(negpwrLatReg), 
 *                          PAC1xxx_VpowerScaleRange(PAC1711_VPOWER_MAX_mV2, negpwrLatReg),
 *                          rsense_uOhm);
 * @endcode
 */
float PAC1xxx_VpowerReg32bitToPower_mW(uint32_t VpowerReg, bool IsSignedPower, uint16_t VPowerScaleRange, uint32_t rsense);


/**
 * @brief Converts the Voltage the digital value from the  *VBUS* or *VSENSE* type registers.
 * 
 * This method computes the Voltage value from the digital value reported by the 
 * *VBUS* or *VSENSE* register and their siblings ("AVG", "Minimum" and "Maximum"). 
 * The result is expressed in milli-Volts.
 * 
 * @param VoltageReg        [in] - Voltage register digital value acquired with I2C read protocol or 
 *                                 with PAC1xxx_GetVBUS_reg() or PAC1xxx_GetVSENSE_reg() functions and
 *                                 their "AVG_reg", "min_reg" or "max_reg" siblings.
 * @param IsSignedVoltage   [in] - Voltage register sign indication as reported by PAC1xxx_IsSignedVbus() or
 *                                 PAC1xxx_IsSignedVsense()
 * @param VoltageScaleRange [in] - Voltage register scale range as reported by PAC1xxx_VbusScaleRange() or
 *                                 PAC1xxx_VsenseScaleRange()
 * @return The Voltage value, expressed in milli-Volts.
 * 
 * Example:
 * @code
 *  negpwrLatReg = PAC1xxx_NegPwrFsrBytesToRegfields(pi2cRxNegPwrFsrLatBytes);
 *  voltageReg = PAC1xxx_RawBytesToReg16bit(pi2cRxVBUSBytes);
 *  voltage = PAC1xxx_VoltageReg16bitToVoltage_mV(
 *                          voltageReg, 
 *                          PAC1xxx_IsSignedVbus(negpwrLatReg), 
 *                          PAC1xxx_VbusScaleRange(PAC1711_VBUS_MAX_mV, negpwrLatReg));
 * @endcode
 */
float PAC1xxx_VoltageReg16bitToVoltage_mV(uint16_t VoltageReg, bool IsSignedVoltage, uint16_t VoltageScaleRange);


/**
 * @brief Converts to Amperage the digital value from the *VSENSE* type registers.
 * 
 * This method computes the Amperage value from the digital value reported by the 
 * *VSENSE* register and its siblings ("AVG", "Minimum" and "Maximum"). 
 * The result is expressed in milli-Amps.
 * 
 * @param VsenseReg        [in] - VSENSE type register digital value acquired with I2C read protocol or 
 *                                with PAC1xxx_GetVSENSE_reg() function and
 *                                their "AVG_reg", "min_reg" or "max_reg" siblings.
 * @param IsSignedVoltage  [in] - VSENSE register sign indication as reported by PAC1xxx_IsSignedVsense()
 * @param VsenseScaleRange [in] - VSENSE register scale range as reported by PAC1xxx_VsenseScaleRange()
 * @param rsense           [in] - The sense (shunt) resistor value, expressed in micro-Ohms.
 * @return The Voltage value, expressed in milli-Volts.
 * @warning If rsense value is 0, the returned value is 0.
 * 
 * Example:
 * @code
 *  negpwrLatReg = PAC1xxx_NegPwrFsrBytesToRegfields(pi2cRxNegPwrFsrLatBytes);
 *  voltageReg = PAC1xxx_RawBytesToReg16bit(pi2cRxVSENSEBytes);
 *  current = PAC1xxx_VoltageReg16bitToCurrent_mA(
 *                          voltageReg, 
 *                          PAC1xxx_IsSignedVsense(negpwrLatReg), 
 *                          PAC1xxx_VsenseScaleRange(PAC1711_VBUS_MAX_mV, negpwrLatReg),
 *                          rsense_uOhm);
 * @endcode
 */
float PAC1xxx_VoltageReg16bitToCurrent_mA(uint16_t VsenseReg, bool IsSignedVoltage, uint16_t VsenseScaleRange, uint32_t rsense);


/**
 * @brief Converts to Energy the accumulated Power at the configured sample rate.
 * 
 * This method computes the Energy value from the Power value accumulated by the 
 * *VACC* register at the configured sample rate. 
 * The result is expressed in milli-Watt-hours.
 * 
 * @param accumulatedPower_mW [in] - the accumulated Power value returned by PAC1xxx_VaccReg64bitToPower_mW() 
 * @param sampleRate          [in] - the sample rate value returned by PAC1xxx_DecodeCTRLtoSampleRate()
 * @return The accumulated Energy value, expressed in milli-Watt-hours.
 * @warning 
 *  - If sampleRate value is <=1, the returned value is 0. 
 *  - For sampleRate = 1 (single-shot mode) consider using PAC1xxx_VaccPowerTimedToEnergy_mWh()  
 * 
 * Example:
 * @code
 *  powerSum = PAC1xxx_VaccReg64bitToPower_mW(
 *                          vaccReg, 
 *                          PAC1xxx_IsSignedVpower(negpwrLatReg), 
 *                          PAC1xxx_VpowerScaleRange(PAC1711_VPOWER_MAX_mV2, negpwrLatReg),
 *                          rsense_uOhm);
 *  sampleRate = PAC1xxx_DecodeCTRLtoSampleRate(ctrlLatReg);
 *  energy = PAC1xxx_VaccPowerToEnergy_mWh(powerSum, sampleRate);
 * @endcode
 */
float PAC1xxx_VaccPowerToEnergy_mWh(float accumulatedPower_mW, int16_t sampleRate);


/**
 * @brief Converts to Energy the accumulated Power during the timed acquisition interval.
 * 
 * This method computes the Energy value from the Power value accumulated by the 
 * *VACC* register during the measured time interval. 
 * The result is expressed in milli-Watt-hours.
 * 
 * @param accumulatedPower_mW [in] - The accumulated Power value returned by PAC1xxx_VaccReg64bitToPower_mW() 
 * @param sampleCount         [in] - The number of accumulated samples reported by *ACC_COUNT* register 
 *                                   by using I2C read protocol or by using PAC1xxx_GetAccumulatorCount() function.
 * @param time_ms             [in] - The duration of the acquisition interval, in milli-seconds.
 * @return The accumulated Energy value, expressed in milli-Watt-hours.
 * @warning If the sampleCount value or the time_ms value is 0, the returned Energy value is 0. 
 * 
 * Example:
 * @code
 *  powerSum = PAC1xxx_VaccReg64bitToPower_mW(
 *                          vaccReg, 
 *                          PAC1xxx_IsSignedVpower(negpwrLatReg), 
 *                          PAC1xxx_VpowerScaleRange(PAC1711_VPOWER_MAX_mV2, negpwrLatReg),
 *                          rsense_uOhm);
 *  energy = PAC1xxx_VaccPowerTimedToEnergy_mWh(powerSum, sampleCountReg, timeMs);
 * @endcode
 */
float PAC1xxx_VaccPowerTimedToEnergy_mWh(float accumulatedPower_mW, uint32_t sampleCount, uint32_t time_ms);


/**
 * @brief Converts to Coulomb count the accumulated VSENSE at the configured sample rate.
 * 
 * This method computes the Coulomb count value from the VSENSE value accumulated by the 
 * *VACC* register at the configured sample rate. 
 * The result is expressed in milli-Coulombs (milli-Amps-second).
 * 
 * @param accumulatedVoltage_mV [in] - the accumulated VSENSE value returned by PAC1xxx_VaccReg64bitToVoltage_mV() 
 * @param sampleRate            [in] - the sample rate value returned by PAC1xxx_DecodeCTRLtoSampleRate()
 * @param rsense                [in] - The sense (shunt) resistor value, expressed in micro-Ohms.
 * @return The accumulated Coulomb count value, expressed in milli-Coulombs (milli-Amps-second).
 * @warning 
 *  - If rsense value is 0, the returned value is 0.
 *  - If sampleRate value is <=1, the returned value is 0. 
 *  - For sampleRate = 1 (single-shot mode) consider using PAC1xxx_VaccVoltageTimedToCoulombCnt()  
 * 
 * Example:
 * @code
 * vsenseVoltageSum = PAC1xxx_VaccReg64bitToVoltage_mV(
 *                          vaccReg, 
 *                          PAC1xxx_IsSignedVsense(negpwrLatReg), 
 *                          PAC1xxx_VsenseScaleRange(PAC1xxx_VSENSE_MAX_mV, negpwrLatReg));
 *  sampleRate = PAC1xxx_DecodeCTRLtoSampleRate(ctrlLatReg);
 *  coulombCnt = PAC1xxx_VaccVoltageToCoulombCnt(vsenseVoltageSum, sampleRate, rsense_uOhm);
 * @endcode
 */
float PAC1xxx_VaccVoltageToCoulombCnt(float accumulatedVoltage_mV, int16_t sampleRate, uint32_t rsense);


/**
 * @brief Converts to Coulomb count the accumulated VSENSE during the timed acquisition interval.
 * 
 * This method computes the Coulomb count value from the VSENSE value accumulated by the 
 * *VACC* register during the measured time interval. 
 * The result is expressed in milli-Coulombs (milli-Amps-second).
 * 
 * @param accumulatedVoltage_mV [in] - the accumulated VSENSE value returned by PAC1xxx_VaccReg64bitToVoltage_mV() 
 * @param sampleCount           [in] - The number of accumulated samples reported by *ACC_COUNT* register 
 *                                     by using I2C read protocol or by using PAC1xxx_GetAccumulatorCount() function.
 * @param time_ms               [in] - The duration of the acquisition interval, in milli-seconds.
 * @param rsense                [in] - The sense (shunt) resistor value, expressed in micro-Ohms.
 * @return The accumulated Coulomb count value, expressed in milli-Coulombs (milli-Amps-second).
 * @warning 
 *  - If rsense value is 0, the returned value is 0.
 *  - If the sampleCount value or the time_ms value is 0, the returned Energy value is 0. 
 * 
 * Example:
 * @code
 * vsenseVoltageSum = PAC1xxx_VaccReg64bitToVoltage_mV(
 *                          vaccReg, 
 *                          PAC1xxx_IsSignedVsense(negpwrLatReg), 
 *                          PAC1xxx_VsenseScaleRange(PAC1xxx_VSENSE_MAX_mV, negpwrLatReg));
 *  coulombCnt = PAC1xxx_VaccVoltageTimedToCoulombCnt(vsenseVoltageSum, sampleCountReg, timeMs, rsense_uOhm);
 * @endcode
 */
float PAC1xxx_VaccVoltageTimedToCoulombCnt(float accumulatedVoltage_mV, uint32_t sampleCount, uint32_t time_ms, uint32_t rsense);


/**
 * @brief Converts to Amperage the digital value from the *OC_Limit* or *UC_Limit* register.
 * 
 * This method computes the Amperage value from the digital value reported by the 
 * *OC_Limit* or *UC_Limit* registers. 
 * The result is expressed in milli-Amps.
 * 
 * @param limitRegister [in] - *OC_Limit* or *UC_Limit* register digital value acquired with I2C read protocol or 
 *                             with PAC1xxx_GetOClimit_reg() or with PAC1xxx_GetUClimit_reg functions.
 * @param VsenseMAX     [in] - *VSENSE* maximum supported value, expressed in milli-Volts.
 * @param rsense        [in] - The sense (shunt) resistor value, expressed in micro-Ohms.
 * @return The Amperage value, expressed in milli-Amps.
 * @warning If rsense value is 0, the returned value is 0.
 * 
 * Example:
 * @code
 *  OClimitmA = PAC1xxx_CLimitRegisterToCurrent_mA(OClimitReg, PAC1xxx_VSENSE_MAX_mV, rsense);
 * @endcode
 */
float PAC1xxx_CLimitRegisterToCurrent_mA(uint8_t limitRegister, uint16_t VsenseMAX, uint32_t rsense);


/**
 * @brief Converts to Power value the digital value from the *OP_Warning_Limit* or *OP_Critical_Limit* register.
 * 
 * This method computes the Power value from the digital value reported by the 
 * *OP_Warning_Limit* or *OP_Critical_Limit* registers. 
 * The result is expressed in milli-Watts.
 * 
 * @param limitRegister [in] - *OP_Warning_Limit* or *OP_CRitical_Limit* register digital value 
 *                             acquired with I2C read protocol or 
 *                             with PAC1xxx_GetOPWlimit_reg() or with PAC1xxx_GetOPClimit_reg() functions.
 * @param VPowerMAX     [in] - *VPOWER* maximum supported value, expressed in milli-Volt^2.
 * @param rsense        [in] - The sense (shunt) resistor value, expressed in micro-Ohms.
 * @return The Power value, expressed in milli-Watts.
 * @warning If rsense value is 0, the returned value is 0.
 * 
 * Example:
 * @code
 *  OPWlimitmW = PAC1xxx_PLimitRegisterToPower_mW(OPWlimitReg, PAC1711_VPOWER_MAX_mV2, rsense);
 * @endcode
 */
float PAC1xxx_PLimitRegisterToPower_mW(uint16_t limitRegister, uint16_t VPowerMAX, uint32_t rsense);


/**
 * @brief Converts to Voltage the digital value from the *OV_Limit* or *UV_Limit* register.
 * 
 * This method computes the Voltage value from the digital value reported by the 
 * *OV_Limit* or *UV_Limit* registers. 
 * The result is expressed in milli-Volts.
 * 
 * @param limitRegister [in] - *OV_Limit* or *UV_Limit* register digital value acquired with I2C read protocol or 
 *                             with PAC1xxx_GetOVlimit_reg() or with PAC1xxx_GetUVlimit_reg functions.
 * @param VbusMAX       [in] - *VBUS* maximum supported value, expressed in milli-Volts.
 * @return The Voltage value, expressed in milli-Volts.
 * 
 * Example:
 * @code
 *  OVlimitmV = PAC1xxx_VLimitRegisterToVoltage_mV(OVlimitReg, PAC1711_VBUS_MAX_mV);
 * @endcode
 */
float PAC1xxx_VLimitRegisterToVoltage_mV(uint8_t limitRegister, uint16_t VbusMAX);


/**
 * @brief Converts a Power value to the digital value to be set into the 
 * *OP_Warning_Limit* or the *OP_Critical_Limit* register.
 * 
 * This method converts the input Power value to the digital value to be written into the 
 * *OP_Warning_Limit* or *OP_Critical_Limit* registers. 
 * 
 * @param PowerLimit [in] - The desired Power limit value, expressed in milli-Watts.
 * @param VPowerMAX  [in] - *VPOWER* maximum supported value, expressed in milli-Volt^2.
 * @param rsense     [in] - The sense (shunt) resistor value, expressed in micro-Ohms.
 * @return The digital value to be written into power limit alert register.
 * @warning 
 *  - If the PowerLimit value exceeds the power that the device can measure for 
 *    the given VPowerMAX and sense (shunt) resistor parameters, the function returns 
 *    the maximum positive (0x7fff) or the minimum negative (0x8000) 16-bit digital value 
 *    that can be written to the power limit registers.
 *  - If VPowerMAX value is 0, the returned value is 0.
 *  - If rsense value is 0, the returned value is 0.
 * 
 * Example:
 * @code
 *  OPWlimitReg = PAC1xxx_Plimit_mWtoRegisterVal(OPWlimitmW, PAC1711_VPOWER_MAX_mV2, rsense);
 * @endcode
 */
uint16_t PAC1xxx_Plimit_mWtoRegisterVal(float PowerLimit, uint16_t VPowerMAX, uint32_t rsense);


/**
 * @brief Converts a current Amperage value to the digital value to be set into the 
 * *OC_Limit* or the *UC_Limit* register.
 * 
 * This method converts the input Amperage value to the digital value to be written into the 
 * *OC_Limit* or *UC_Limit* registers. 
 * 
 * @param CurrentLimit [in] - The desired Amperage limit value, expressed in milli-Amps.
 * @param VsenseMAX    [in] - *VSENSE* maximum supported value, expressed in milli-Volts.
 * @param rsense       [in] - The sense (shunt) resistor value, expressed in micro-Ohms.
 * @return The digital value to be written into Current limit alert register.
 * @warning 
 *  - If the CurrentLimit value exceeds the Amperage that the device can measure for 
 *    the given VsenseMAX and sense (shunt) resistor parameters, the function returns 
 *    the maximum positive (0x7f) or the minimum negative (0x80) 8-bit digital value 
 *    that can be written to the Current limit registers.
 *  - If VsenseMAX value is 0, the returned value is 0.
 *  - If rsense value is 0, the returned value is 0.
 * 
 * Example:
 * @code
 *  OClimitReg = PAC1xxx_Climit_mAtoRegisterVal(OClimitmA, PAC1xxx_VSENSE_MAX_mV, rsense);
 * @endcode
 */
uint8_t PAC1xxx_Climit_mAtoRegisterVal(float CurrentLimit, uint16_t VsenseMAX, uint32_t rsense);


/**
 * @brief Converts a Voltage value to the digital value to be set into the 
 * *OV_Limit* or the *UV_Limit* register.
 * 
 * This method converts the input Voltage value to the digital value to be written into the 
 * *OV_Limit* or *UV_Limit* registers. 
 * 
 * @param VoltageLimit [in] - The desired Voltage limit value, expressed in milli-Volts.
 * @param VbusMax      [in] - *VBUS* maximum supported value, expressed in milli-Volts.
 * @return The digital value to be written into Voltage limit alert register.
 * @warning 
 *  - If the VoltageLimit value exceeds the Voltage that the device can measure for 
 *    the given VbusMAX parameter, the function returns 
 *    the maximum positive (0x7f) or the minimum negative (0x80) 8-bit digital value 
 *    that can be written to the Voltage limit registers.
 *  - If VbusMAX value is 0, the returned value is 0.
 * 
 * Example:
 * @code
 *  OVlimitReg = PAC1xxx_Vlimit_mVtoRegisterVal(OVlimitmV, PAC1711_VBUS_MAX_mV);
 * @endcode
 */
uint8_t PAC1xxx_Vlimit_mVtoRegisterVal(float VoltageLimit, uint16_t VbusMax);


#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	// _PAC1xxx_H

/**
  End of File
*/
