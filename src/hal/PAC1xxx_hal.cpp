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

#include "PAC1xxx_hal.h"

//Non-public return codes from non-public API (used internally)
#define PAC1xxx_ALREADY_CACHED                     1000 /**< Internal only return code used by non-public API. 
                                                             Not returned to user appication by the public API. */

void PAC1xxx_I2CEventHandler(PAC1xxx_I2C_TRANSFER_EVENT event,  uintptr_t context);

static void PAC1xxx_GetDeviceIDProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetCtrl_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ProcessMode procMode);
static void PAC1xxx_GetAccumulatorCountProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static int16_t PAC1xxx_GetVACCProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetVBUS_mVProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetVSENSE_mVProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetISENSE_mAProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetVPOWER_mWProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetSMBusSettings_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_SetSMBusSettings_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetNegPwrFsr_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ProcessMode procMode);
static void PAC1xxx_GetSlow_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetAlertStatus_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetAlert_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetAccFullness_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetCurrentLimit_mAProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetPowerLimit_mWProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetVoltageLimit_mVProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetStepLimitProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_GetLimitNsamplesProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static int16_t PAC1xxx_UpdateContext_ChannelPolarity(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_UpdateContext_ChannelPolarityProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static int16_t PAC1xxx_UpdateContext_Ctrl(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_UpdateContext_CtrlProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static int16_t PAC1xxx_UpdateContext_AccumulatorCount(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_UpdateContext_AccumulatorCountProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static int16_t PAC1xxx_Get_Register(PAC1xxx_DEVICE_CONTEXT_P pdevice, 
                                    void* pregister_val, uint8_t registerAddr, size_t regSize, 
                                    PAC1xxx_procState processingState);
static int16_t PAC1xxx_Get_VACCRegister(PAC1xxx_DEVICE_CONTEXT_P pdevice, 
                                        void* pregister_val, uint8_t* pmode, uint32_t time,
                                        PAC1xxx_ProcessMode procMode);
static void PAC1xxx_Get_Reg8bitProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_Get_Reg16bitProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_Get_Reg32bitProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static void PAC1xxx_Get_Reg56bitProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice);
static int16_t PAC1xxx_Set_Register(PAC1xxx_DEVICE_CONTEXT_P pdevice, 
                                    uint8_t *pregisterBytes, uint8_t registerAddr, size_t regSize, 
                                    PAC1xxx_procState processingState);

static inline void PAC1xxx_UpdateContext_ScaleValues(PAC1xxx_DEVICE_CONTEXT_P pdevice);
///////////////////////////////////////////////////////////////////////////////

/*
 * Library functions, target platform independent 
 */

int16_t PAC1xxx_AbortRequest(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    if (pdevice != NULL){
        pdevice->ABORT_REQUESTED_FLAG = true;
        return PAC1xxx_SUCCESS;
    }else{
        return PAC1xxx_INVALID_PARAMETER;
    }
}

static void inline callUserCallback(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    // call the user call-back if there is one registered
    if(pdevice->userCallback != NULL){
        PAC1xxx_EVENT_HANDLER userCallback;
        PAC1xxx_EVENT event;
        uintptr_t userContext; 
        
        userCallback = pdevice->userCallback;
        event = pdevice->deviceEventStatus;
        userContext = pdevice->userContext;
        userCallback(event, userContext);
    }
}


//pdevice_context is the device context structure, PAC1xxx_PDEVICE_CONTEXT
void PAC1xxx_I2CEventHandler(PAC1xxx_I2C_TRANSFER_EVENT event, uintptr_t pdevice_context)
{
    if(pdevice_context == 0){
        return;
    }
    switch(event){
        case PAC1xxx_I2C_TRANSFER_EVENT_HANDLE_EXPIRED:
        case PAC1xxx_I2C_TRANSFER_EVENT_ERROR:
        case PAC1xxx_I2C_TRANSFER_EVENT_HANDLE_INVALID:
            ((PAC1xxx_DEVICE_CONTEXT_P)pdevice_context)->i2cCommStatus = PAC1xxx_I2C_TRANSFER_EVENT_ERROR;
            break;
        default:
            ((PAC1xxx_DEVICE_CONTEXT_P)pdevice_context)->i2cCommStatus = event;
    }
}


static void RequestCompletion(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_EVENT deviceEvent, int16_t errorCode){
    pdevice->deviceEventStatus = deviceEvent;
    pdevice->processError = errorCode;
    //call the user callback
    callUserCallback(pdevice);
    if ( pdevice->syncMode == true){
        pdevice->processingState = Sync;
    }else{
        pdevice->processingState = Idle;
    } 
}

int16_t PAC1xxx_LibTask(PAC1xxx_DEVICE_CONTEXT_P pdevice)
{
    int16_t errorCode = PAC1xxx_SUCCESS;
    PAC1xxx_EVENT libEvent;
    
    if(pdevice == NULL) return PAC1xxx_LIBTASK_FAIL;
    
    switch(pdevice->processingState){
        case Sync:
            if ( pdevice->syncMode != true) pdevice->processingState = Idle;
        case Idle:
        case Uninitialized:
            pdevice->ABORT_REQUESTED_FLAG = false;
            return PAC1xxx_LIBTASK_DONE;
        default:
            break;
    }
    
    //if requested, abort the current processing
    if(pdevice->ABORT_REQUESTED_FLAG == true){
        pdevice->ABORT_REQUESTED_FLAG = false;
        RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_ABORT, PAC1xxx_REQUEST_ABORT);
        return PAC1xxx_LIBTASK_DONE;
    }
    
    if(pdevice->i2c_context.i2cEventCallbackRegistered == false){
        // if the communication events are not already reported via call-back function then 
        // we check for the communication status here.
        PAC1xxx_I2C_TRANSFER_EVENT i2cEvent;
        i2cEvent = PAC1xxx_I2C_TransferStatusGet(pdevice->i2c_context);
        PAC1xxx_I2CEventHandler(i2cEvent, (uintptr_t)pdevice);
    }
    
    // if we are servicing an API request and we got a communication error
    // we terminate the request and signal the error
    if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_ERROR){
        RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_FAIL, PAC1xxx_I2C_FAIL);
        return PAC1xxx_LIBTASK_DONE;    
    }

    // if we are servicing an API request, continue the processing
    switch(pdevice->processingState)
    {
        case RefreshReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;

        case GetDeviceIDReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetDeviceIDProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }            
            return PAC1xxx_LIBTASK_DONE;

        // GET register request for
        // UClimit, OClimit UVlimit and OVlimit:
        case GetRegister8bitReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_Get_Reg8bitProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }             
            return PAC1xxx_LIBTASK_DONE;
        
        // GET register request for
        // OPWlimit, OPClimit, VBUS, VBUSAVG, VBUSmin, VBUSmax, 
        // VSENSE, VSENSEAVG, VSENSEmin, VSENSEmax, AccCountPreset, VACC
        case GetRegister16bitReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_Get_Reg16bitProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }            
            return PAC1xxx_LIBTASK_DONE;

        // GET register request for
        // VPOWER, VPOWERmin, VPOWERmax
        case GetRegister32bitReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_Get_Reg32bitProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            } 
            return PAC1xxx_LIBTASK_DONE;

        //GET register request for VACC
        case GetRegister56bitReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_Get_Reg56bitProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;
            
        case GetCtrlRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetCtrl_regProcess(pdevice, ProcessNone);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }             
            return PAC1xxx_LIBTASK_DONE;

        case GetCtrlLatRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetCtrl_regProcess(pdevice, ProcessCTRLlat);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }             
            return PAC1xxx_LIBTASK_DONE;        
            
        case GetSMBusRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetSMBusSettings_regProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;    

        case GetNegPWRFSRRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetNegPwrFsr_regProcess(pdevice, ProcessNone);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;    

        case GetNegPWRFSRLatRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetNegPwrFsr_regProcess(pdevice, ProcessNEGPWRlat);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;               
            
        case GetAccCountReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetAccumulatorCountProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE; 
            
        case GetSlowRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetSlow_regProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;             

        case GetAlertStatusRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetAlertStatus_regProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;             

        // GET alert configuration register request for
        // GpioAlert1, SlowAlert0, AlertEnable
        case GetAlertConfigRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetAlert_regProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE; 

        case GetAccFullnessRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetAccFullness_regProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE; 

        case GetStepLimitRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetStepLimitProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE; 
            
        case GetLimitNsamplesRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetLimitNsamplesProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE; 

        // SET configuration register request for:
        // UVlimit, OVlimit, OPWlimit, OPClimit, SetUClimit, OClimit, 
        // VACCPreset, AccCountPreset, LimitNsamples, UVlimit, OVlimit,
        // OPClimit, SetOPWlimit, UClimit, OClimit, AccFullness, 
        // GpioAlert1, SlowAlert0, AlertEnable, Slow, NegPWRFSR, Control
        case SetRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;

        case SetCtrlRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                pdevice->ctrl_change_pending = true; //next refresh will apply the configuration change
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;

        case SetNegPWRFSRRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                pdevice->negPwr_change_pending = true; //next refresh will apply the configuration change
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;
            
        case SetSMBusRegisterReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_SetSMBusSettings_regProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;            

        // GET value request for 
        // UClimit, OClimit
        case GetCurrentLimitValueReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetCurrentLimit_mAProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE;             

        // GET value request for 
        // OPWlimit, OPClimit
        case GetPowerLimitValueReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetPowerLimit_mWProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE; 

        // GET value request for 
        // UVlimit, OVlimit
        case GetVoltageLimitValueReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                PAC1xxx_GetVoltageLimit_mVProcess(pdevice);
                //complete the request and go to Sync state
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            }
            return PAC1xxx_LIBTASK_DONE; 

        // GET mV value request for 
        // VBUS, VBUSAVG, VBUSmin, VBUSmax
        case GetVBUSvalueReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                //we got the register value
                //check the channel polarity    
                errorCode = PAC1xxx_UpdateContext_ChannelPolarity(pdevice);
                if ((errorCode != PAC1xxx_SUCCESS) && 
                    (errorCode != PAC1xxx_ALREADY_CACHED)){
                    RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_FAIL, errorCode);
                    return PAC1xxx_LIBTASK_DONE;
                }                
                pdevice->processingState = GetVBUSValueReq_polarityUpdate;
            }else{ 
                return PAC1xxx_LIBTASK_DONE;
            }
        case GetVBUSValueReq_polarityUpdate:
            if(errorCode != PAC1xxx_ALREADY_CACHED){
                if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                    //we got the NEG_PWR_FSR_LAT register value
                    PAC1xxx_UpdateContext_ChannelPolarityProcess(pdevice);
                }else{
                    return PAC1xxx_LIBTASK_DONE;
                }
            }
            PAC1xxx_GetVBUS_mVProcess(pdevice);
            //complete the request and go to Sync state
            RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            return PAC1xxx_LIBTASK_DONE;

        // GET mV value request for 
        // VSENSE, VSENSEAVG, VSENSEmin, VSENSEmax
        case GetVSENSEvalueReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                //we got the register value
                //check the channel polarity    
                errorCode = PAC1xxx_UpdateContext_ChannelPolarity(pdevice);
                if ((errorCode != PAC1xxx_SUCCESS) && 
                    (errorCode != PAC1xxx_ALREADY_CACHED)){
                    RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_FAIL, errorCode);
                    return PAC1xxx_LIBTASK_DONE;
                }
                pdevice->processingState = GetVSENSEValueReq_polarityUpdate;
            }else{ 
                return PAC1xxx_LIBTASK_DONE;
            }
        case GetVSENSEValueReq_polarityUpdate:
            if(errorCode != PAC1xxx_ALREADY_CACHED){
                if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                    //we got the NEG_PWR_FSR_LAT register value
                    PAC1xxx_UpdateContext_ChannelPolarityProcess(pdevice);
                }else{
                    return PAC1xxx_LIBTASK_DONE;
                }
            }
            PAC1xxx_GetVSENSE_mVProcess(pdevice);
            //complete the request and go to Sync state
            RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            return PAC1xxx_LIBTASK_DONE;

        // GET mA value request for 
        // ISENSE, ISENSEAVG, ISENSEmin, ISENSEmax
        case GetISENSEvalueReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                //we got the register value
                //check the channel polarity    
                errorCode = PAC1xxx_UpdateContext_ChannelPolarity(pdevice);
                if ((errorCode != PAC1xxx_SUCCESS) && 
                    (errorCode != PAC1xxx_ALREADY_CACHED)){
                    RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_FAIL, errorCode);
                    return PAC1xxx_LIBTASK_DONE;
                }
                pdevice->processingState = GetISENSEValueReq_polarityUpdate;
            }else{ 
                return PAC1xxx_LIBTASK_DONE;
            }
        case GetISENSEValueReq_polarityUpdate:
            if(errorCode != PAC1xxx_ALREADY_CACHED){
                if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                    //we got the NEG_PWR_FSR_LAT register value
                    PAC1xxx_UpdateContext_ChannelPolarityProcess(pdevice);
                }else{
                    return PAC1xxx_LIBTASK_DONE;
                }
            }
            PAC1xxx_GetISENSE_mAProcess(pdevice);
            //complete the request and go to Sync state
            RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            return PAC1xxx_LIBTASK_DONE;

        // GET mW value request for 
        // VPOWER, VPOWERmin, VPOWERmax
        case GetVPOWERValueReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                //we got the register value
                //check the channel polarity    
                errorCode = PAC1xxx_UpdateContext_ChannelPolarity(pdevice);
                if ((errorCode != PAC1xxx_SUCCESS) && 
                    (errorCode != PAC1xxx_ALREADY_CACHED)){
                    RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_FAIL, errorCode);
                    return PAC1xxx_LIBTASK_DONE;
                }
                pdevice->processingState = GetVPOWERValueReq_polarityUpdate;
            }else{ 
                return PAC1xxx_LIBTASK_DONE;
            }
        case GetVPOWERValueReq_polarityUpdate:
            if(errorCode != PAC1xxx_ALREADY_CACHED){
                if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                    //we got the NEG_PWR_FSR_LAT register value
                    PAC1xxx_UpdateContext_ChannelPolarityProcess(pdevice);
                }else{
                    return PAC1xxx_LIBTASK_DONE;
                }
            }
            PAC1xxx_GetVPOWER_mWProcess(pdevice);
            //complete the request and go to Sync state
            RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_SUCCESS, PAC1xxx_SUCCESS);
            return PAC1xxx_LIBTASK_DONE;

        // GET VACC value according to the configured accumulation mode
        // Power sum - mW
        // Voltage sum - mV (VSENSE or VBUS)
        // Energy accumulation - mWh (using sps or timed interval)
        // Coulomb Count - mAh  (using sps or timed interval)
        case GetVACCValueReq:
            if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                //we got the register value
                //check the channel polarity    
                errorCode = PAC1xxx_UpdateContext_ChannelPolarity(pdevice);
                if ((errorCode != PAC1xxx_SUCCESS) && 
                    (errorCode != PAC1xxx_ALREADY_CACHED)){
                    RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_FAIL, errorCode);
                    return PAC1xxx_LIBTASK_DONE;
                }
                pdevice->processingState = GetVACCValueReq_polarityUpdate;
            }else{
                return PAC1xxx_LIBTASK_DONE;
            }
        case GetVACCValueReq_polarityUpdate:
            if(errorCode != PAC1xxx_ALREADY_CACHED){
                if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                    //we got the NEG_PWR_FSR_LAT register value
                    PAC1xxx_UpdateContext_ChannelPolarityProcess(pdevice);
                }else{
                    return PAC1xxx_LIBTASK_DONE;
                }
            }
            errorCode = PAC1xxx_UpdateContext_Ctrl((PAC1xxx_DEVICE_CONTEXT_P)pdevice);
            if ((errorCode != PAC1xxx_SUCCESS) && 
                (errorCode != PAC1xxx_ALREADY_CACHED)){
                RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_FAIL, errorCode);
                return PAC1xxx_LIBTASK_DONE;
            }
            pdevice->processingState = GetVACCValueReq_ctrlUpdate;
        case GetVACCValueReq_ctrlUpdate:
            if(errorCode != PAC1xxx_ALREADY_CACHED){
                if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                    //we got the CTRL_LAT register value
                    PAC1xxx_UpdateContext_CtrlProcess(pdevice);
                }else{
                    return PAC1xxx_LIBTASK_DONE;
                }
            }
            if((pdevice->regProcMode == ProcessVACCtimedEnergy) || 
               (pdevice->regProcMode == ProcessVACCtimedCoulomb) ){
                //get the sample count - read ACC_COUNT
                errorCode = PAC1xxx_UpdateContext_AccumulatorCount(pdevice);
                if ((errorCode != PAC1xxx_SUCCESS) && 
                    (errorCode != PAC1xxx_ALREADY_CACHED)){
                    RequestCompletion(pdevice, PAC1xxx_EVENT_REQUEST_FAIL, errorCode);
                    return PAC1xxx_LIBTASK_DONE;
                }            
            }
            pdevice->processingState = GetVACCValueReq_AccCntUpdate;
        case GetVACCValueReq_AccCntUpdate:
            if((pdevice->regProcMode == ProcessVACCtimedEnergy) || 
               (pdevice->regProcMode == ProcessVACCtimedCoulomb) ){
                if(errorCode != PAC1xxx_ALREADY_CACHED){
                    if(pdevice->i2cCommStatus == PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE){
                        //we got the CTRL_LAT register value
                        PAC1xxx_UpdateContext_AccumulatorCountProcess(pdevice);
                    }else{
                        return PAC1xxx_LIBTASK_DONE;
                    }
                }
            }
            //process the VACC register 
            errorCode = PAC1xxx_GetVACCProcess(pdevice);
            if (errorCode != PAC1xxx_SUCCESS){
                libEvent = PAC1xxx_EVENT_REQUEST_FAIL;
            }else{
                //complete the request and go to Sync state
                libEvent = PAC1xxx_EVENT_REQUEST_SUCCESS;
            }
            RequestCompletion(pdevice, libEvent, errorCode);
            return PAC1xxx_LIBTASK_DONE;

        default:
            return PAC1xxx_LIBTASK_DONE;
    }
}


bool PAC1xxx_Device_IsInitialized(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    if (pdevice == NULL){ 
        return false;   // return "false" if pdevice is NULL
    }else{
        return (pdevice->processingState != Uninitialized);
    }
}


bool PAC1xxx_Device_IsBusy(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    bool deviceIsBusy = false;
    if (pdevice != NULL){
        if( pdevice->processingState != Idle )  deviceIsBusy = true; 
    }
    return deviceIsBusy;
}


int16_t PAC1xxx_Device_Initialize(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_DEVICE_INIT deviceInit) {
    int16_t errorCode = PAC1xxx_SUCCESS;
    
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    
    //step 1 - initialize the device context
    memset(pdevice, 0, sizeof(PAC1xxx_DEVICE_CONTEXT));

    // Create the processingState mutex
    if( PAC1xxx_MUTEX_Create(&(pdevice->mutexProcState)) == false )
    {
        return PAC1xxx_MUTEX_FAIL;
    }
    
    // lock the processingState mutex
    if( PAC1xxx_MUTEX_Lock(&(pdevice->mutexProcState)) == false )
    {
        return PAC1xxx_BUSY;
    }
    
    pdevice->processingState = Uninitialized;
    pdevice->deviceEventStatus = PAC1xxx_EVENT_NONE;
    pdevice->ABORT_REQUESTED_FLAG = false;
    
    //step2 - configure the I2C communication
    if (false == PAC1xxx_I2C_Initialize(&(pdevice->i2c_context), deviceInit.i2c_init)){
        pdevice->processError = PAC1xxx_I2C_FAIL;
        errorCode = PAC1xxx_I2C_FAIL;
        goto initialize_error;    
    }
    
    if(false == PAC1xxx_I2C_TransferEventHandlerSet(&(pdevice->i2c_context), 
                                                    PAC1xxx_I2CEventHandler, 
                                                    (uintptr_t)pdevice)){        
        pdevice->i2c_context.i2cEventCallbackRegistered = false;
    }else{
        pdevice->i2c_context.i2cEventCallbackRegistered = true;
    }

    //step3 - set the initial device configuration
    
    //NOTE: use synchronous communication for the device initial configuration.
    //reset the device configuration (similar to POR)
    pdevice->syncMode = true;
    pdevice->processingState = Idle; // set the state machine to Idle to allow function calls
    
    //unlock the processingState mutex
    PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));
    
    pdevice->ENABLE_BYTE_COUNT_FLAG = false;
    //initialize the cached registers in the device context
    pdevice->deviceID_cached = false;
    pdevice->negPwr_change_pending = false;
    pdevice->negPwr_LAT_cached = false;
    pdevice->ctrl_change_pending = false;
    pdevice->ctrl_LAT_cached = false;
    pdevice->accCount_cached = false;
    
    // make sure that the I2C protocol is set to default configuration: I2C version (not SMBUS)
    // Configure SMBUS_SETTINGS - 0x10
    PAC1xxx_SMBUS_SETTINGS_REGFIELDS SMBus;
    SMBus.GPIO_DATA1 = 0b0;
    SMBus.GPIO_DATA0 = 0b0;
    SMBus.ANY_ALERT = 0b0;
    SMBus.POR = 0b1;    //keep the flag value as is
    SMBus.TIMEOUT = 0b0;
    SMBus.BYTE_COUNT = 0b0;
    SMBus.I2C_HISPEED = 0b0;
    errorCode = PAC1xxx_SetSMBusSettings_reg(pdevice, SMBus);
    if(errorCode != PAC1xxx_SUCCESS) goto initialize_error;

    //read and store the device IDs in the device context
    errorCode = PAC1xxx_GetDeviceID(pdevice, &(pdevice->deviceID)); 
    if(errorCode != PAC1xxx_SUCCESS) goto initialize_error;
    
    //set the device specific properties in the device context
    pdevice->VsenseMAX = PAC1xxx_VSENSE_MAX_mV;                 //milliVolts
    
    switch (pdevice->deviceID.product){
        
        //PAC17xx family
        case PAC1711PDN_PRODUCT_ID:{                            //PAC1711 w/ PWRDN
            pdevice->is12bitADCres = true;
            pdevice->VbusMAX     = PAC1711_VBUS_MAX_mV;         //milliVolts
            pdevice->VPowerMAX   = PAC1711_VPOWER_MAX_mV2;      //milli-Volt^2
            break;
        }
        case PAC1711_PRODUCT_ID:{                               //PAC1711 w/o PWRDN
            pdevice->is12bitADCres = true;
            pdevice->VbusMAX     = PAC1711_VBUS_MAX_mV;         //milliVolts
            pdevice->VPowerMAX   = PAC1711_VPOWER_MAX_mV2;      //milli-Volt^2
            break;
        }
        case PAC1721PDN_PRODUCT_ID:{                            //PAC1721 w/ PWRDN
            pdevice->is12bitADCres = true;
            pdevice->VbusMAX     = PAC1721_VBUS_MAX_mV;         //milliVolts
            pdevice->VPowerMAX   = PAC1721_VPOWER_MAX_mV2;      //milli-Volt^2
            break;
        }
        case PAC1721_PRODUCT_ID:{                               //PAC1721 w/o PWRDN
            pdevice->is12bitADCres = true;
            pdevice->VbusMAX     = PAC1721_VBUS_MAX_mV;         //milliVolts
            pdevice->VPowerMAX   = PAC1721_VPOWER_MAX_mV2;      //milli-Volt^2
            break;
        }
        case PAC1761_PRODUCT_ID:{                               //PAC1761 w/o PWRDN
            pdevice->is12bitADCres = true;
            pdevice->VbusMAX     = PAC1761_VBUS_MAX_mV;         //milliVolts
            pdevice->VPowerMAX   = PAC1761_VPOWER_MAX_mV2;      //milli-Volt^2
            break;
        }
        
        //PAC18xx family
        case PAC1811PDN_PRODUCT_ID:{                            //PAC1811 w/ PWRDN
            pdevice->is12bitADCres = false;
            pdevice->VbusMAX     = PAC1811_VBUS_MAX_mV;         //milliVolts
            pdevice->VPowerMAX   = PAC1811_VPOWER_MAX_mV2;      //milli-Volt^2
            break;
        }
        case PAC1811_PRODUCT_ID:{                               //PAC1811 w/o PWRDN
            pdevice->is12bitADCres = false;
            pdevice->VbusMAX     = PAC1811_VBUS_MAX_mV;         //milliVolts
            pdevice->VPowerMAX   = PAC1811_VPOWER_MAX_mV2;      //milli-Volt^2
            break;
        }
        case PAC1821PDN_PRODUCT_ID:{                            //PAC1821 w/ PWRDN
            pdevice->is12bitADCres = false;
            pdevice->VbusMAX     = PAC1821_VBUS_MAX_mV;         //milliVolts
            pdevice->VPowerMAX   = PAC1821_VPOWER_MAX_mV2;      //milli-Volt^2
            break;
        }
        case PAC1821_PRODUCT_ID:{                               //PAC1821 w/o PWRDN
            pdevice->is12bitADCres = false;
            pdevice->VbusMAX     = PAC1821_VBUS_MAX_mV;         //milliVolts
            pdevice->VPowerMAX   = PAC1821_VPOWER_MAX_mV2;      //milli-Volt^2
            break;
        }
        case PAC1861_PRODUCT_ID:{                               //PAC1861 w/o PWRDN
            pdevice->is12bitADCres = false;
            pdevice->VbusMAX     = PAC1861_VBUS_MAX_mV;         //milliVolts
            pdevice->VPowerMAX   = PAC1861_VPOWER_MAX_mV2;      //milli-Volt^2
            break;
        }
        
        default:
            errorCode = PAC1xxx_INVALID_DEVICE;
            goto initialize_error;
    }
    pdevice->deviceID_cached = true;    //the device was recognized => mark the deviceID as cached
    
    pdevice->rsense = deviceInit.rsense;        
        
    // Configure CTRL - 0x2520
    PAC1xxx_CONTROL_REGFIELDS Ctrl;
    Ctrl.SAMPLE_MODE   = 0b0010;
    Ctrl.GPIO_ALERT1   = 0b01;
    Ctrl.SLOW_ALERT0   = 0b01;
    Ctrl.AVERAGE       = 0b001;
    Ctrl.AA            = 0b0;
    Ctrl.ACC_CONFIG    = 0b00;
    Ctrl.AUTO_REFRESH  = 0b00;
    errorCode = PAC1xxx_SetCtrl_reg(pdevice, Ctrl);    
    if(errorCode != PAC1xxx_SUCCESS) goto initialize_error;
  
    // Configure NEG_PWR_FSR - 0x0000
    PAC1xxx_NEGPWRFSR_REGFIELDS NegPwr;
    NegPwr.CFG_VS = PAC1xxx_NEGPWRFSR_MODE_UNIPOLAR;    
    NegPwr.CFG_VB = PAC1xxx_NEGPWRFSR_MODE_UNIPOLAR;
    errorCode = PAC1xxx_SetNegPwrFsr_reg(pdevice, NegPwr);    
    if(errorCode != PAC1xxx_SUCCESS) goto initialize_error;   

    // Configure SLOW - 0x00
    PAC1xxx_SLOW_REGFIELDS Slow;
    Slow.RefreshRise  = 0b0;
    Slow.RefreshVRise = 0b0;
    Slow.RefreshFall  = 0b0;
    Slow.RefreshVFall = 0b0;
    errorCode = PAC1xxx_SetSlow_reg(pdevice, Slow);
    if(errorCode != PAC1xxx_SUCCESS) goto initialize_error;
        
    // Configure ACC_Fullness_limits - 0x01
    PAC1xxx_ACCUM_LIMITS_REGFIELDS AccLimits;
    AccLimits.ACC_COUNT_FULL = PAC1xxx_ACCLIMITS_COUNT_15BY16;
    errorCode = PAC1xxx_SetAccFullness_reg(pdevice, AccLimits);
    if(errorCode != PAC1xxx_SUCCESS) goto initialize_error;
    
    // Configure ALERT_ENABLE - 0x000000
    PAC1xxx_ALERT_ENABLE_REGFIELDS AlertEnable;
    memset(&AlertEnable, 0, sizeof(PAC1xxx_ALERT_ENABLE_REGFIELDS));
    errorCode = PAC1xxx_SetAlertEnable_reg(pdevice, AlertEnable);
    if(errorCode != PAC1xxx_SUCCESS) goto initialize_error;
        
    errorCode = PAC1xxx_Refresh(pdevice);    // the REFRESH ensures that the configuration
                                             // changes are applied
//  if(errorCode != PAC1xxx_SUCCESS) goto initialize_error; 

initialize_error:
    if(errorCode != PAC1xxx_SUCCESS){        
        pdevice->processingState = Uninitialized;
    }
    pdevice->syncMode = deviceInit.syncMode;            // set the user sync mode selection
    PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));   //no issue if the mutex was already un-locked 
    return errorCode;
}


int16_t PAC1xxx_SetUserCallback(
    const PAC1xxx_DEVICE_CONTEXT_P pdevice,
    const PAC1xxx_EVENT_HANDLER userCallback,
    const uintptr_t userContext
){
    if( (pdevice == NULL) || (userCallback == NULL) ){
        return PAC1xxx_INVALID_PARAMETER;
    }

    //check if new device request is allowed
    // lock the processingState mutex
    if( PAC1xxx_MUTEX_Lock(&(pdevice->mutexProcState)) == false )    
    {
        return PAC1xxx_BUSY;
    }    
    
    if(pdevice->processingState == Idle){
        // set the callback
        pdevice->userCallback = userCallback;
        pdevice->userContext = userContext;
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));        
        return PAC1xxx_SUCCESS;
    }else
    {
        // reject new request
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));
        return PAC1xxx_BUSY;
    }  
}

int16_t PAC1xxx_GetEventStatus(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_EVENT_P pevent, int16_t* pProcessError){
    if ( (pdevice == NULL) || (pevent == NULL) || (pProcessError == NULL) ) return PAC1xxx_INVALID_PARAMETER;
    *pevent = pdevice->deviceEventStatus;
    *pProcessError = pdevice->processError;
    return PAC1xxx_SUCCESS;
}

static int16_t RequestReturn(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    //wait here for the request processing completion if the library is in SYNC mode
    int16_t retcode = PAC1xxx_REQUEST_PENDING;
    if(pdevice->syncMode == true){ 
        while (pdevice->processingState != Sync){
            if (PAC1xxx_LibTask(pdevice) == PAC1xxx_LIBTASK_FAIL){
                return PAC1xxx_LIBTASK_FAIL;
            }
        }
        retcode = pdevice->processError;
        pdevice->processingState = Idle;     //the request is fully completed here
    }
    return retcode;
}

int16_t PAC1xxx_GetDeviceID(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_deviceID_P pdeviceID){
    int16_t retcode;
    bool bSuccess;
    if ((pdevice == NULL) || (pdeviceID == NULL)) return PAC1xxx_INVALID_PARAMETER;
    
    //check if new device request is allowed
    // lock the processingState mutex
    if( PAC1xxx_MUTEX_Lock(&(pdevice->mutexProcState)) == false )    
    {
        return PAC1xxx_BUSY;
    }
    
    if(pdevice->processingState == Idle){
        // start new request 
        pdevice->processingState = GetDeviceIDReq;
        pdevice->deviceEventStatus = PAC1xxx_EVENT_NONE;
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));
    }else{
        // reject new request
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));
        return PAC1xxx_BUSY;
    }

    pdevice->outData = (void*)pdeviceID;
   
    pdevice->i2cTxBuffer[0] = PAC1xxx_PRODUCT_ID_ADDR;
    unsigned int i2cRxSize = PAC1xxx_ID_REGS_SZ;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) i2cRxSize += 3; //there is one byteCount reported before each ID register value
    pdevice->i2cCommStatus = PAC1xxx_I2C_TRANSFER_NO_EVENT;
    bSuccess = PAC1xxx_I2C_WriteRead(&(pdevice->i2c_context), (void*) pdevice->i2cTxBuffer, 1, 
                                               (void*) pdevice->i2cRxBuffer, i2cRxSize); 
    if(bSuccess == false){
        pdevice->processingState = Idle;
        pdevice->processError = PAC1xxx_I2C_FAIL;
        return PAC1xxx_I2C_FAIL;
    }
    
    //wait here for the request processing completion if the library is in SYNC mode
    retcode = RequestReturn(pdevice);
    return retcode;
}


static void PAC1xxx_GetDeviceIDProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    int idx = 0;

    pRawValue = pdevice->i2cRxBuffer;
    //Product ID
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) idx++;
    pdevice->deviceID.product = pRawValue[idx];
    ((PAC1xxx_deviceID_P)pdevice->outData)->product = pRawValue[idx++];
    
    //Manufacturer ID    
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) idx++;
    pdevice->deviceID.manufacturer = pRawValue[idx];
    ((PAC1xxx_deviceID_P)pdevice->outData)->manufacturer = pRawValue[idx++];

    //Revision ID
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) idx++;
    pdevice->deviceID.revision = pRawValue[idx];
    ((PAC1xxx_deviceID_P)pdevice->outData)->revision = pRawValue[idx];
}



int16_t PAC1xxx_RefreshReq(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_REFRESH_MODE refreshMode){
    int16_t retcode;
    bool bSuccess;
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    
    // check if new device request is allowed
    // lock the processingState mutex
    if( PAC1xxx_MUTEX_Lock(&(pdevice->mutexProcState)) == false )    
    {
        return PAC1xxx_BUSY;
    }
    
    if(pdevice->processingState == Idle){
        // start new request 
        pdevice->processingState = RefreshReq;
        pdevice->deviceEventStatus = PAC1xxx_EVENT_NONE;
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));        
    }else
    {
        // reject new request
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));        
        return PAC1xxx_BUSY;
    }
    
    // invalidate the device context cache flags
    if(pdevice->negPwr_change_pending == true){
        pdevice->negPwr_LAT_cached = false;
        pdevice->negPwr_change_pending = false;
    }
    if(pdevice->ctrl_change_pending == true){
        pdevice->ctrl_LAT_cached = false;
        pdevice->ctrl_change_pending = false;
    }
    pdevice->accCount_cached = false;
    
    switch (refreshMode){
        case PAC1xxx_REFRESH_G:
            pdevice->i2cTxBuffer[0] = PAC1xxx_REFRESH_G_CMD_ADDR;
            break;
        case PAC1xxx_REFRESH_V:
            pdevice->i2cTxBuffer[0] = PAC1xxx_REFRESH_V_CMD_ADDR;
            break;
        case PAC1xxx_REFRESH:
        default:
            pdevice->i2cTxBuffer[0] = PAC1xxx_REFRESH_CMD_ADDR;
            break;
    }
    
    pdevice->i2cCommStatus = PAC1xxx_I2C_TRANSFER_NO_EVENT;    
    bSuccess = PAC1xxx_I2C_Write(&(pdevice->i2c_context), (void*) pdevice->i2cTxBuffer, 1);    
    if(bSuccess == false){
        pdevice->processingState = Idle;
        pdevice->processError = PAC1xxx_I2C_FAIL;
        return PAC1xxx_I2C_FAIL;
    }    
    
    // wait here for the request processing completion if the library is in SYNC mode
    retcode = RequestReturn(pdevice);
    return retcode;
}

int16_t PAC1xxx_Refresh(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    return PAC1xxx_RefreshReq(pdevice, PAC1xxx_REFRESH);
}

int16_t PAC1xxx_RefreshG(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    return PAC1xxx_RefreshReq(pdevice, PAC1xxx_REFRESH_G);
}

int16_t PAC1xxx_RefreshV(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    return PAC1xxx_RefreshReq(pdevice, PAC1xxx_REFRESH_V);
}


int16_t PAC1xxx_GetCtrl_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t reg_select, PAC1xxx_CONTROL_REGFIELDS_P pCtrl_reg){
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    switch(reg_select){
        case 1:
            return PAC1xxx_Get_Register(pdevice, (void*)pCtrl_reg, PAC1xxx_CONTROL_ADDR, PAC1xxx_CONTROL_SZ, GetCtrlRegisterReq);
        case 2:
            return PAC1xxx_Get_Register(pdevice, (void*)pCtrl_reg, PAC1xxx_CONTROL_ACT_ADDR, PAC1xxx_CONTROL_SZ, GetCtrlRegisterReq);
        case 3:
            return PAC1xxx_Get_Register(pdevice, (void*)pCtrl_reg, PAC1xxx_CONTROL_LAT_ADDR, PAC1xxx_CONTROL_SZ, GetCtrlLatRegisterReq);
        default:
            return PAC1xxx_INVALID_PARAMETER;      
    }
}


static void PAC1xxx_GetCtrl_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ProcessMode procMode){
    uint8_t* pRawValue;
    pRawValue = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    PAC1xxx_CtrlBytesToRegfields(pRawValue, (PAC1xxx_CONTROL_REGFIELDS_P)pdevice->outData);
   
    // cache the LAT register value in context 
    if(procMode == ProcessCTRLlat){
        pdevice->ctrl_LAT = *((PAC1xxx_CONTROL_REGFIELDS_P)pdevice->outData);
        pdevice->ctrl_LAT_cached = true;
    }
}


int16_t PAC1xxx_SetCtrl_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_CONTROL_REGFIELDS Ctrl_reg){
    uint8_t pregisterBytes[PAC1xxx_CONTROL_SZ];
    
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    
    PAC1xxx_CtrlRegfieldsToBytes(Ctrl_reg, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_CONTROL_ADDR, PAC1xxx_CONTROL_SZ, 
                                SetCtrlRegisterReq);
}


int16_t PAC1xxx_GetAccumulatorCount(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint32_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_ACC_COUNT_ADDR, PAC1xxx_ACC_COUNT_SZ, GetAccCountReq);
}


static void PAC1xxx_GetAccumulatorCountProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    PAC1xxx_Get_Reg32bitProcess(pdevice);
    pdevice->accCount = *((uint32_t*)pdevice->outData);
    pdevice->accCount_cached = true;
}


int16_t PAC1xxx_SetAccumulatorCountPreset_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t register_val){
    uint8_t pregisterBytes[PAC1xxx_ACC_PRESET_SZ];

    PAC1xxx_Reg16bitToRawBytes(register_val, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_ACC_COUNT_PRESET_ADDR, PAC1xxx_ACC_PRESET_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_GetAccumulatorCountPreset_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_ACC_COUNT_PRESET_ADDR, PAC1xxx_ACC_PRESET_SZ, 
                                GetRegister16bitReq);
}


int16_t PAC1xxx_GetVACC_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint64_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VACC_ADDR, PAC1xxx_VACC_SZ, 
                                GetRegister56bitReq);
}


int16_t PAC1xxx_SetVACCPreset_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t register_val){
    uint8_t pregisterBytes[PAC1xxx_ACC_PRESET_SZ];

    PAC1xxx_Reg16bitToRawBytes(register_val, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_ACC_PRESET_ADDR, PAC1xxx_ACC_PRESET_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_GetVACCPreset_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_ACC_PRESET_ADDR, PAC1xxx_ACC_PRESET_SZ, 
                                GetRegister16bitReq);
}


int16_t PAC1xxx_GetVACC(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue, uint8_t* pmode){
    if ((pdevice == NULL) || (pvalue == NULL) || (pmode == NULL)) return PAC1xxx_INVALID_PARAMETER;
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;
    return PAC1xxx_Get_VACCRegister(pdevice, 
                                    (void*)pvalue, pmode, 0,
                                    ProcessVACCget);
}


int16_t PAC1xxx_DecodeCTRLtoSampleRate(PAC1xxx_CONTROL_REGFIELDS ctrlReg){
    
    if( (ctrlReg.AA == 1) && (ctrlReg.SAMPLE_MODE < 6)){
        //adaptive accumulation modes
        return 8192; 
    }else{
        switch(ctrlReg.SAMPLE_MODE){
            case 0:
                return 8192;
            case 1:
                return 4096;
            case 2:
                return 1024;
            case 3:
                return 256;
            case 4:
                return 64;
            case 5:
                return 8;
            case 6:
            case 7:
            case 8:
            case 9:
            case 12:
            case 13:
                return 1;   //Single shot modes
            case 10:
            case 11:
                return 16384; //Vbus or Vsense accumulation only
            case 14:
            case 15:
                return 0;   //SLEEP modes
            default:
                return -1;  // invalid sample mode number
        }
    }
}


static int16_t PAC1xxx_GetVACCProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawBytes;
    uint64_t regVal;
    uint8_t mode;
    int16_t sampleRate;
    int16_t errorCode = PAC1xxx_SUCCESS;
    float result = 0.0;
    
    pRawBytes = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawBytes++;
    regVal = PAC1xxx_RawBytesToReg64bit(pRawBytes);
    
    mode = pdevice->ctrl_LAT.ACC_CONFIG;
    
    if(pdevice->regProcMode == ProcessVACCget){
        *((uint8_t*)pdevice->accMode) = mode;
    }
    
    switch(mode){
        case PAC1xxx_CONTROL_ACC_CONFIG_VPOWER: //Power accumulator - mode 0 - milliWatt
            result = PAC1xxx_VaccReg64bitToPower_mW(regVal, pdevice->IsSignedPower,
                                                    pdevice->VPowerScaleRange, 
                                                    pdevice->is12bitADCres,
                                                    pdevice->rsense);
            
            switch(pdevice->regProcMode){
                case ProcessVACCget:
                    break;
                case ProcessVACCenergy:
                    sampleRate = PAC1xxx_DecodeCTRLtoSampleRate(pdevice->ctrl_LAT);
                    //single-shot requires the timed interval
                    if(sampleRate <= 1) errorCode = PAC1xxx_INVALID_SAMPLE_MODE;
                    result = PAC1xxx_VaccPowerToEnergy_mWh(result, sampleRate);
                    break;
                case ProcessVACCtimedEnergy:
                    result = PAC1xxx_VaccPowerTimedToEnergy_mWh(result, pdevice->accCount, pdevice->time);
                    break;
                default:
                    errorCode = PAC1xxx_DIFFERENT_ACCUMULATION_MODE;
                    result = 0.0;
            }
            break;
            
        case PAC1xxx_CONTROL_ACC_CONFIG_VSENSE: //Vsense accumulator - mode 1 - milliVolt            
            result = PAC1xxx_VaccReg64bitToVoltage_mV(regVal, pdevice->IsSignedVsense,
                                                      pdevice->VsenseScaleRange,
                                                      pdevice->is12bitADCres);
            switch(pdevice->regProcMode){
                case ProcessVACCget:
                    break;
                case ProcessVACCcoulomb:
                    sampleRate = PAC1xxx_DecodeCTRLtoSampleRate(pdevice->ctrl_LAT);
                    //single-shot requires the timed interval
                    if(sampleRate <= 1) errorCode = PAC1xxx_INVALID_SAMPLE_MODE;
                    result = PAC1xxx_VaccVoltageToCoulombCnt(result, sampleRate, pdevice->rsense);
                    break;
                case ProcessVACCtimedCoulomb:
                    result = PAC1xxx_VaccVoltageTimedToCoulombCnt(result, pdevice->accCount, pdevice->time, pdevice->rsense);
                    break;
                default:
                    errorCode = PAC1xxx_DIFFERENT_ACCUMULATION_MODE;
                    result = 0.0;
            }
            break;
            
        case PAC1xxx_CONTROL_ACC_CONFIG_VBUS: //Vbus accumulator  - mode 2 - milliVolt
            if (pdevice->regProcMode == ProcessVACCget){
                result = PAC1xxx_VaccReg64bitToVoltage_mV(regVal, pdevice->IsSignedVbus,
                                                          pdevice->VbusScaleRange,
                                                          pdevice->is12bitADCres);

            }else{
                errorCode = PAC1xxx_DIFFERENT_ACCUMULATION_MODE;
                result = 0.0;
            }
            break;

        default: //PAC1xxx_CTRL_ACC_MODE_RESERVED
            errorCode = PAC1xxx_INVALID_ACCUMULATION_MODE;
    }
    *((float*)pdevice->outData) = result;
    return errorCode;
}


//reports energy in mWh
int16_t PAC1xxx_GetEnergy_mWh(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    if ((pdevice == NULL) || (pvalue == NULL)) return PAC1xxx_INVALID_PARAMETER;
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;
    return PAC1xxx_Get_VACCRegister(pdevice, 
                                    (void*)pvalue, 0, 0, 
                                    ProcessVACCenergy);    
}


//reports energy in mWh
int16_t PAC1xxx_GetTimedEnergy_mWh(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue, uint32_t time){
    if ((pdevice == NULL) || (pvalue == NULL)) return PAC1xxx_INVALID_PARAMETER;    
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;
    return PAC1xxx_Get_VACCRegister(pdevice, 
                                    (void*)pvalue, 0, time,
                                    ProcessVACCtimedEnergy);    
}


//reports energy in mAs
int16_t PAC1xxx_GetCoulomb_mAs(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    if ((pdevice == NULL) || (pvalue == NULL)) return PAC1xxx_INVALID_PARAMETER;    
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;
    return PAC1xxx_Get_VACCRegister(pdevice, 
                                    (void*)pvalue, 0, 0,
                                    ProcessVACCcoulomb);
}


//reports energy in mAs
int16_t PAC1xxx_GetTimedCoulomb_mAs(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue, uint32_t time){
    if ((pdevice == NULL) || (pvalue == NULL)) return PAC1xxx_INVALID_PARAMETER;    
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;
    return PAC1xxx_Get_VACCRegister(pdevice, 
                                    (void*)pvalue, 0, time,
                                    ProcessVACCtimedCoulomb);
}


int16_t PAC1xxx_GetVBUS_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VBUS_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetRegister16bitReq);
}

int16_t PAC1xxx_GetVBUS_AVG_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VBUS_AVG_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetRegister16bitReq);
}


int16_t PAC1xxx_GetVBUSmin_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VBUS_MIN_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetRegister16bitReq);
}


int16_t PAC1xxx_GetVBUSmax_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VBUS_MAX_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetRegister16bitReq);
}


int16_t PAC1xxx_GetVBUS_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VBUS_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetVBUSvalueReq);
}


int16_t PAC1xxx_GetVBUS_AVG_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VBUS_AVG_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetVBUSvalueReq);
}


int16_t PAC1xxx_GetVBUSmin_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VBUS_MIN_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetVBUSvalueReq);
}


int16_t PAC1xxx_GetVBUSmax_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VBUS_MAX_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetVBUSvalueReq);    
}


static void PAC1xxx_GetVBUS_mVProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawBytes;
    uint16_t VbusReg;
    float VbusReal;
    
    pRawBytes = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawBytes++;
    VbusReg = PAC1xxx_RawBytestoReg16bit(pRawBytes);
    VbusReal = PAC1xxx_VoltageReg16bitToVoltage_mV(VbusReg, pdevice->IsSignedVbus, pdevice->VbusScaleRange);
    *((float*)pdevice->outData) = VbusReal;
}

int16_t PAC1xxx_GetVSENSE_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VSENSE_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetRegister16bitReq);
}

int16_t PAC1xxx_GetVSENSE_AVG_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VSENSE_AVG_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetRegister16bitReq);
}


int16_t PAC1xxx_GetVSENSEmin_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VSENSE_MIN_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetRegister16bitReq);
}


int16_t PAC1xxx_GetVSENSEmax_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VSENSE_MAX_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetRegister16bitReq);
}

int16_t PAC1xxx_GetVSENSE_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VSENSE_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetVSENSEvalueReq);
}


int16_t PAC1xxx_GetVSENSE_AVG_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VSENSE_AVG_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetVSENSEvalueReq);
}


int16_t PAC1xxx_GetVSENSEmin_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VSENSE_MIN_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetVSENSEvalueReq);    
}


int16_t PAC1xxx_GetVSENSEmax_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VSENSE_MAX_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetVSENSEvalueReq);    
}


static void PAC1xxx_GetVSENSE_mVProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawBytes;
    uint16_t VsenseReg;
    float VsenseReal;
    
    pRawBytes = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawBytes++;
    VsenseReg = PAC1xxx_RawBytestoReg16bit(pRawBytes);
    VsenseReal = PAC1xxx_VoltageReg16bitToVoltage_mV(VsenseReg, pdevice->IsSignedVsense, pdevice->VsenseScaleRange);
    *((float*)pdevice->outData) = VsenseReal;
}


int16_t PAC1xxx_GetISENSE_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;    
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VSENSE_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetISENSEvalueReq);
}


int16_t PAC1xxx_GetISENSE_AVG_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VSENSE_AVG_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetISENSEvalueReq);
}


int16_t PAC1xxx_GetISENSEmin_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VSENSE_MIN_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetISENSEvalueReq);
}


int16_t PAC1xxx_GetISENSEmax_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VSENSE_MAX_ADDR, PAC1xxx_VBUS_VSENSE_SZ, 
                                GetISENSEvalueReq);
}


static void PAC1xxx_GetISENSE_mAProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawBytes;
    uint16_t VsenseReg;
    float IsenseReal;

    pRawBytes = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawBytes++;
    VsenseReg = PAC1xxx_RawBytestoReg16bit(pRawBytes);
    IsenseReal = PAC1xxx_VoltageReg16bitToCurrent_mA(VsenseReg, pdevice->IsSignedVsense, 
                                                     pdevice->VsenseScaleRange, pdevice->rsense);
    *((float*)pdevice->outData) = IsenseReal;
}


int16_t PAC1xxx_GetVPOWER_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint32_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VPOWER_ADDR, PAC1xxx_VPOWER_SZ, 
                                GetRegister32bitReq);
}


int16_t PAC1xxx_GetVPOWERmin_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint32_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VPOWER_MIN_ADDR, PAC1xxx_VPOWER_SZ, 
                                GetRegister32bitReq);
}


int16_t PAC1xxx_GetVPOWERmax_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint32_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_VPOWER_MAX_ADDR, PAC1xxx_VPOWER_SZ, 
                                GetRegister32bitReq);
}


int16_t PAC1xxx_GetVPOWER_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VPOWER_ADDR, PAC1xxx_VPOWER_SZ, 
                                GetVPOWERValueReq);
}


int16_t PAC1xxx_GetVPOWERmin_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VPOWER_MIN_ADDR, PAC1xxx_VPOWER_SZ, 
                                GetVPOWERValueReq);
}


int16_t PAC1xxx_GetVPOWERmax_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    // channel shunt value must be non-zero
    if (pdevice->rsense == 0) return PAC1xxx_INVALID_SHUNT_VALUE;
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_VPOWER_MAX_ADDR, PAC1xxx_VPOWER_SZ, 
                                GetVPOWERValueReq);
}


static void PAC1xxx_GetVPOWER_mWProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawBytes;
    uint32_t regVal;
    float VpowerReal;
    
    pRawBytes = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawBytes++;
    regVal = PAC1xxx_RawBytestoReg32bit(pRawBytes);
    
    VpowerReal = PAC1xxx_VpowerReg32bitToPower_mW(regVal, pdevice->IsSignedPower, pdevice->VPowerScaleRange, pdevice->rsense);
    *((float*)pdevice->outData) = VpowerReal;
}


int16_t PAC1xxx_GetSMBusSettings_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_SMBUS_SETTINGS_REGFIELDS_P pSMBus_reg){
    return PAC1xxx_Get_Register(pdevice, (void*)pSMBus_reg, 
                                PAC1xxx_SMBUS_SETTINGS_ADDR, PAC1xxx_SMBUS_SZ, 
                                GetSMBusRegisterReq);
}


static void PAC1xxx_GetSMBusSettings_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    pRawValue = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    PAC1xxx_SMBusBytesToRegfields(pRawValue, (PAC1xxx_SMBUS_SETTINGS_REGFIELDS_P)pdevice->outData);
    
    // cache the BYTE_COUNT flag into device context
    pdevice->ENABLE_BYTE_COUNT_FLAG = ( ((PAC1xxx_SMBUS_SETTINGS_REGFIELDS_P)(pdevice->outData))->BYTE_COUNT == 1 ) ? true: false;    
}


int16_t PAC1xxx_SetSMBusSettings_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_SMBUS_SETTINGS_REGFIELDS SMBus_reg){
    uint8_t pregisterBytes[PAC1xxx_SMBUS_SZ];
    PAC1xxx_SMBusRegfieldsToBytes(SMBus_reg, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_SMBUS_SETTINGS_ADDR, PAC1xxx_SMBUS_SZ, 
                                SetSMBusRegisterReq);
}    


static void PAC1xxx_SetSMBusSettings_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){    
    // cache the BYTE_COUNT flag into device context
    pdevice->ENABLE_BYTE_COUNT_FLAG = ( ((pdevice->i2cTxBuffer[1] >> PAC1xxx_SMBUS_BYTECOUNT_BITPOS) & PAC1xxx_SMBUS_BITMASK) == 1 ) ? true: false;
}


int16_t PAC1xxx_GetNegPwrFsr_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t reg_select, PAC1xxx_NEGPWRFSR_REGFIELDS_P pNegPwr_reg){
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    switch(reg_select){
        case 1:
            return PAC1xxx_Get_Register(pdevice, (void*)pNegPwr_reg, PAC1xxx_NEG_PWR_FSR_ADDR, PAC1xxx_NEGPWRFSR_SZ, GetNegPWRFSRRegisterReq);
        case 2:
            return PAC1xxx_Get_Register(pdevice, (void*)pNegPwr_reg, PAC1xxx_NEG_PWR_FSR_ACT_ADDR, PAC1xxx_NEGPWRFSR_SZ, GetNegPWRFSRRegisterReq);
        case 3:
            return PAC1xxx_Get_Register(pdevice, (void*)pNegPwr_reg, PAC1xxx_NEG_PWR_FSR_LAT_ADDR, PAC1xxx_NEGPWRFSR_SZ, GetNegPWRFSRLatRegisterReq);            
        default:
            return PAC1xxx_INVALID_PARAMETER;      
    } 
}


static void PAC1xxx_GetNegPwrFsr_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ProcessMode procMode){
uint8_t* pRawValue;
    pRawValue = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    PAC1xxx_NegPwrFsrBytesToRegfields(pRawValue, (PAC1xxx_NEGPWRFSR_REGFIELDS_P)pdevice->outData);
    if(procMode == ProcessNEGPWRlat){    
        pdevice->negPwrFsr_LAT = *((PAC1xxx_NEGPWRFSR_REGFIELDS_P)pdevice->outData);
        pdevice->negPwr_LAT_cached = true;
        PAC1xxx_UpdateContext_ScaleValues(pdevice);
    }
}


int16_t PAC1xxx_SetNegPwrFsr_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_NEGPWRFSR_REGFIELDS NegPwr_reg){
    uint8_t pregisterBytes[PAC1xxx_NEGPWRFSR_SZ];
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    PAC1xxx_NegPwrFsrRegfieldsToBytes(NegPwr_reg, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_NEG_PWR_FSR_ADDR, PAC1xxx_NEGPWRFSR_SZ, 
                                SetNegPWRFSRRegisterReq);
}


int16_t PAC1xxx_GetSlow_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_SLOW_REGFIELDS_P pSlow_reg){
    return PAC1xxx_Get_Register(pdevice, (void*)pSlow_reg, 
                                PAC1xxx_SLOW_ADDR, PAC1xxx_SLOW_SZ, 
                                GetSlowRegisterReq);
}
    

static void PAC1xxx_GetSlow_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    pRawValue = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    PAC1xxx_SlowBytesToRegfields(pRawValue, (PAC1xxx_SLOW_REGFIELDS_P)pdevice->outData);
}


int16_t PAC1xxx_SetSlow_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_SLOW_REGFIELDS Slow_reg){
    uint8_t pregisterBytes[PAC1xxx_SLOW_SZ];
    PAC1xxx_SlowRegfieldsToBytes(Slow_reg, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_SLOW_ADDR, PAC1xxx_SLOW_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_GetAlertStatus_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_STATUS_REGFIELDS_P pAlertStatus_reg){
    return PAC1xxx_Get_Register(pdevice, (void*)pAlertStatus_reg, 
                                PAC1xxx_ALERT_STATUS_ADDR, PAC1xxx_ALERT_SZ, 
                                GetAlertStatusRegisterReq);
}


static void PAC1xxx_GetAlertStatus_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    
    pRawValue = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    PAC1xxx_AlertStatusBytesToRegfields(pRawValue, (PAC1xxx_ALERT_STATUS_REGFIELDS_P)pdevice->outData);
}


int16_t PAC1xxx_GetAlertEnable_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS_P pAlertEnable_reg){
    return PAC1xxx_Get_Register(pdevice, (void*)pAlertEnable_reg, 
                                PAC1xxx_ALERT_ENABLE_ADDR, PAC1xxx_ALERT_SZ, 
                                GetAlertConfigRegisterReq);
}


int16_t PAC1xxx_GetSlowAlert0_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS_P pSlowAlert0_reg){
    return PAC1xxx_Get_Register(pdevice, (void*)pSlowAlert0_reg, 
                                PAC1xxx_SLOW_ALERT0_ADDR, PAC1xxx_ALERT_SZ, 
                                GetAlertConfigRegisterReq);
}  


int16_t PAC1xxx_GetGpioAlert1_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS_P pGpioAlert1_reg){
    return PAC1xxx_Get_Register(pdevice, (void*)pGpioAlert1_reg, 
                                PAC1xxx_GPIO_ALERT1_ADDR, PAC1xxx_ALERT_SZ, 
                                GetAlertConfigRegisterReq);
}


static void PAC1xxx_GetAlert_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    
    pRawValue = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    PAC1xxx_AlertEnableBytesToRegfields(pRawValue, (PAC1xxx_ALERT_ENABLE_REGFIELDS_P)pdevice->outData);
}


int16_t PAC1xxx_SetAlertEnable_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS AlertEnable_reg){
    uint8_t pregisterBytes[PAC1xxx_ALERT_SZ];
    
    PAC1xxx_AlertEnableRegfieldsToBytes(AlertEnable_reg, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_ALERT_ENABLE_ADDR, PAC1xxx_ALERT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_SetSlowAlert0_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS AlertEnable_reg){
    uint8_t pregisterBytes[PAC1xxx_ALERT_SZ];
    
    PAC1xxx_AlertEnableRegfieldsToBytes(AlertEnable_reg, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_SLOW_ALERT0_ADDR, PAC1xxx_ALERT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_SetGpioAlert1_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ALERT_ENABLE_REGFIELDS AlertEnable_reg){
    uint8_t pregisterBytes[PAC1xxx_ALERT_SZ];
    
    PAC1xxx_AlertEnableRegfieldsToBytes(AlertEnable_reg, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_GPIO_ALERT1_ADDR, PAC1xxx_ALERT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_GetAccFullness_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ACCUM_LIMITS_REGFIELDS_P pAccFullnessLimits_reg){
    return PAC1xxx_Get_Register(pdevice, (void*)pAccFullnessLimits_reg, 
                                PAC1xxx_ACC_FULLNESS_LIMITS_ADDR, PAC1xxx_LIMIT_SZ, 
                                GetAccFullnessRegisterReq);
}


static void PAC1xxx_GetAccFullness_regProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    pRawValue = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    PAC1xxx_AccFullnessBytesToRegfields(pRawValue, (PAC1xxx_ACCUM_LIMITS_REGFIELDS_P)pdevice->outData);
}


int16_t PAC1xxx_SetAccFullness_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_ACCUM_LIMITS_REGFIELDS AccFullnessLimits_reg){
    uint8_t pregisterBytes[PAC1xxx_LIMIT_SZ];
    PAC1xxx_AccFullnessRegfieldsToBytes(AccFullnessLimits_reg, pregisterBytes);
    
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_ACC_FULLNESS_LIMITS_ADDR, PAC1xxx_LIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_GetOClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_OC_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                GetRegister8bitReq);
}


int16_t PAC1xxx_GetOClimit_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_OC_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                GetCurrentLimitValueReq);
}


static void PAC1xxx_GetCurrentLimit_mAProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    float limitReal;

    pRawValue = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    limitReal = PAC1xxx_CLimitRegisterToCurrent_mA(pRawValue[0], pdevice->VsenseMAX, pdevice->rsense);
    *((float*)pdevice->outData) = limitReal;
}


int16_t PAC1xxx_SetOClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t register_val){
    return PAC1xxx_Set_Register(pdevice, &register_val, 
                                PAC1xxx_OC_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_SetOClimit_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value){
    uint8_t limitRegister;
    
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    
    limitRegister = PAC1xxx_Climit_mAtoRegisterVal(value, pdevice->VsenseMAX, pdevice->rsense);
    if(limitRegister == PAC1xxx_OVER_CURRENT_BORDER){ 
        limitRegister = PAC1xxx_OVER_CURRENT_BORDER - 1;
    }
    return PAC1xxx_Set_Register(pdevice, &limitRegister, 
                                PAC1xxx_OC_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_GetUClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_UC_LIMIT_ADDR, PAC1xxx_LIMIT_SZ,
                                GetRegister8bitReq);    
}


int16_t PAC1xxx_GetUClimit_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_UC_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                GetCurrentLimitValueReq);
}


int16_t PAC1xxx_SetUClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t register_val){
    return PAC1xxx_Set_Register(pdevice, &register_val, 
                                PAC1xxx_UC_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_SetUClimit_mA(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value){
    uint8_t limitRegister;

    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    
    limitRegister = PAC1xxx_Climit_mAtoRegisterVal(value, pdevice->VsenseMAX, pdevice->rsense);
    if(limitRegister == PAC1xxx_UNDER_CURRENT_BORDER){ 
        limitRegister = PAC1xxx_UNDER_CURRENT_BORDER + 1;
    }    
    return PAC1xxx_Set_Register(pdevice, &limitRegister, 
                                PAC1xxx_UC_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                SetRegisterReq);    
    
}


int16_t PAC1xxx_GetOPWlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_OPW_LIMIT_ADDR, PAC1xxx_OTHERLIMIT_SZ, 
                                GetRegister16bitReq);
}


int16_t PAC1xxx_GetOPWlimit_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_OPW_LIMIT_ADDR, PAC1xxx_OTHERLIMIT_SZ, 
                                GetPowerLimitValueReq);
}


int16_t PAC1xxx_SetOPWlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t register_val){
    uint8_t pregisterBytes[PAC1xxx_OTHERLIMIT_SZ];

    PAC1xxx_Reg16bitToRawBytes(register_val, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_OPW_LIMIT_ADDR, PAC1xxx_OTHERLIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_SetOPWlimit_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value){
    uint16_t limitRegister;
    uint8_t pregisterBytes[PAC1xxx_OTHERLIMIT_SZ];
    
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    
    limitRegister = PAC1xxx_Plimit_mWtoRegisterVal(value, pdevice->VPowerMAX, pdevice->rsense);
    if(limitRegister == PAC1xxx_OVER_POWER_BORDER){ 
        limitRegister = PAC1xxx_OVER_POWER_BORDER - 1;
    }    
    PAC1xxx_Reg16bitToRawBytes(limitRegister, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_OPW_LIMIT_ADDR, PAC1xxx_OTHERLIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_GetOPClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_OPC_LIMIT_ADDR, PAC1xxx_OTHERLIMIT_SZ, 
                                GetRegister16bitReq);
}


int16_t PAC1xxx_GetOPClimit_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_OPC_LIMIT_ADDR, PAC1xxx_OTHERLIMIT_SZ, 
                                GetPowerLimitValueReq);
}


static void PAC1xxx_GetPowerLimit_mWProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawBytes;
    uint16_t limitRegister;
    float limitReal;
    
    pRawBytes = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawBytes++;
    limitRegister = PAC1xxx_RawBytestoReg16bit(pRawBytes);
    limitReal = PAC1xxx_PLimitRegisterToPower_mW(limitRegister, pdevice->VPowerMAX, pdevice->rsense);
    *((float*)pdevice->outData) = limitReal;
}


int16_t PAC1xxx_SetOPClimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint16_t register_val){
    uint8_t pregisterBytes[PAC1xxx_OTHERLIMIT_SZ];

    PAC1xxx_Reg16bitToRawBytes(register_val, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_OPC_LIMIT_ADDR, PAC1xxx_OTHERLIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_SetOPClimit_mW(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value){
    uint16_t limitRegister;
    uint8_t pregisterBytes[PAC1xxx_OTHERLIMIT_SZ];
    
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    
    limitRegister = PAC1xxx_Plimit_mWtoRegisterVal(value, pdevice->VPowerMAX, pdevice->rsense);
    if(limitRegister == PAC1xxx_OVER_POWER_BORDER){ 
        limitRegister = PAC1xxx_OVER_POWER_BORDER - 1;
    }      
    PAC1xxx_Reg16bitToRawBytes(limitRegister, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_OPC_LIMIT_ADDR, PAC1xxx_OTHERLIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_GetOVlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_OV_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                GetRegister8bitReq);
}


int16_t PAC1xxx_GetOVlimit_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_OV_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                GetVoltageLimitValueReq);
}


static void PAC1xxx_GetVoltageLimit_mVProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    float limitReal;
    
    pRawValue = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    limitReal = PAC1xxx_VLimitRegisterToVoltage_mV(pRawValue[0], pdevice->VbusMAX);
    *((float*)pdevice->outData) = limitReal;
}


int16_t PAC1xxx_SetOVlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t register_val){
    return PAC1xxx_Set_Register(pdevice, &register_val, 
                                PAC1xxx_OV_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_SetOVlimit_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value){
    uint8_t limitRegister;

    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    
    limitRegister = PAC1xxx_Vlimit_mVtoRegisterVal(value, pdevice->VbusMAX);
    if(limitRegister == PAC1xxx_OVER_VOLTAGE_BORDER){ 
        limitRegister = PAC1xxx_OVER_VOLTAGE_BORDER - 1;
    }  
    return PAC1xxx_Set_Register(pdevice, &limitRegister, 
                                PAC1xxx_OV_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                SetRegisterReq);    
}


int16_t PAC1xxx_GetUVlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t* pregister_val){
    return PAC1xxx_Get_Register(pdevice, (void*)pregister_val, 
                                PAC1xxx_UV_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                GetRegister8bitReq);
}


//pvalue reported as mV
int16_t PAC1xxx_GetUVlimit_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float* pvalue){
    return PAC1xxx_Get_Register(pdevice, (void*)pvalue, 
                                PAC1xxx_UV_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                GetVoltageLimitValueReq);
}


int16_t PAC1xxx_SetUVlimit_reg(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t register_val){
    return PAC1xxx_Set_Register(pdevice, &register_val, 
                                PAC1xxx_UV_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_SetUVlimit_mV(PAC1xxx_DEVICE_CONTEXT_P pdevice, float value){
    uint8_t limitRegister;
   
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;

    limitRegister = PAC1xxx_Vlimit_mVtoRegisterVal(value, pdevice->VbusMAX);
    if(limitRegister == PAC1xxx_UNDER_VOLTAGE_BORDER){ 
        limitRegister = PAC1xxx_UNDER_VOLTAGE_BORDER + 1;
    }     
    return PAC1xxx_Set_Register(pdevice, &limitRegister, 
                                PAC1xxx_UV_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_GetStepLimit(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_STEP_LIMIT_REGFIELDS_P pStepLimit_reg){
    return PAC1xxx_Get_Register(pdevice, (void*)pStepLimit_reg, 
                                PAC1xxx_STEP_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                GetStepLimitRegisterReq);
}


static void PAC1xxx_GetStepLimitProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    
    pRawValue = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    PAC1xxx_StepLimitBytesToRegfields(pRawValue, (PAC1xxx_STEP_LIMIT_REGFIELDS_P)pdevice->outData);
}


int16_t PAC1xxx_SetStepLimit(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_STEP_LIMIT_REGFIELDS StepLimit_reg){
    uint8_t pregisterBytes[PAC1xxx_LIMIT_SZ];
    PAC1xxx_StepLimitRegfieldsToBytes(StepLimit_reg, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_STEP_LIMIT_ADDR, PAC1xxx_LIMIT_SZ, 
                                SetRegisterReq);
}


int16_t PAC1xxx_GetLimitNsamples(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_LIMIT_NSAMPLES_REGFIELDS_P pLimitNsamples_reg){
    return PAC1xxx_Get_Register(pdevice, (void*)pLimitNsamples_reg, 
                                PAC1xxx_LIMIT_NSAMPLES_ADDR, PAC1xxx_OTHERLIMIT_SZ, 
                                GetLimitNsamplesRegisterReq);
}


static void PAC1xxx_GetLimitNsamplesProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    
    pRawValue = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    PAC1xxx_NsamplesBytesToRegfields(pRawValue, (PAC1xxx_LIMIT_NSAMPLES_REGFIELDS_P)pdevice->outData);
}


int16_t PAC1xxx_SetLimitNsamples(PAC1xxx_DEVICE_CONTEXT_P pdevice, PAC1xxx_LIMIT_NSAMPLES_REGFIELDS LimitNsamples_reg){
    uint8_t pregisterBytes[PAC1xxx_OTHERLIMIT_SZ];
    PAC1xxx_NsamplesRegfieldsToBytes(LimitNsamples_reg, pregisterBytes);
    return PAC1xxx_Set_Register(pdevice, pregisterBytes, 
                                PAC1xxx_LIMIT_NSAMPLES_ADDR, PAC1xxx_OTHERLIMIT_SZ, 
                                SetRegisterReq);
}


static int16_t PAC1xxx_UpdateContext_ChannelPolarity(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t registerAddr;
    int16_t errorCode = PAC1xxx_SUCCESS;
    bool bSuccess;

    // Check if the cached negPwrFsr_LAT is valid
    if(pdevice->negPwr_LAT_cached == false){
        registerAddr = PAC1xxx_NEG_PWR_FSR_LAT_ADDR;
        pdevice->i2cTxBuffer[0] = registerAddr;
        
        unsigned int i2cRxSize = PAC1xxx_NEGPWRFSR_SZ;
        if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) i2cRxSize++;  
        pdevice->i2cCommStatus = PAC1xxx_I2C_TRANSFER_NO_EVENT;
        bSuccess = PAC1xxx_I2C_WriteRead(&(pdevice->i2c_context), (void*) pdevice->i2cTxBuffer, 1, 
                                                   (void*) pdevice->i2cRxBuffer_negPwr, i2cRxSize);            
        if(bSuccess == false){
            return PAC1xxx_I2C_FAIL;
        }        
    }
    else
    {
        errorCode = PAC1xxx_ALREADY_CACHED;
    }
    
   return errorCode; 
}


static void PAC1xxx_UpdateContext_ChannelPolarityProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    pRawValue = pdevice->i2cRxBuffer_negPwr;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    PAC1xxx_NegPwrFsrBytesToRegfields(pRawValue, &(pdevice->negPwrFsr_LAT));
    pdevice->negPwr_LAT_cached = true;
    PAC1xxx_UpdateContext_ScaleValues(pdevice);
}


static int16_t PAC1xxx_UpdateContext_Ctrl(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    int16_t errorCode = PAC1xxx_SUCCESS;
    uint8_t registerAddr;
    bool bSuccess;
         
    //check if cached CTRL_LAT is valid
    //if NOT valid, update the cache
    if(pdevice->ctrl_LAT_cached == false){
        registerAddr = PAC1xxx_CONTROL_LAT_ADDR;
        pdevice->i2cTxBuffer[0] = registerAddr;
        
        unsigned int i2cRxSize = PAC1xxx_CONTROL_SZ;
        if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) i2cRxSize++;
        pdevice->i2cCommStatus = PAC1xxx_I2C_TRANSFER_NO_EVENT;
        bSuccess = PAC1xxx_I2C_WriteRead(&(pdevice->i2c_context), (void*) pdevice->i2cTxBuffer, 1, 
                                                    (void*) pdevice->i2cRxBuffer_ctrl, i2cRxSize);            
        if(bSuccess == false){
            return PAC1xxx_I2C_FAIL;
        }        
    }
    else
    {
        errorCode = PAC1xxx_ALREADY_CACHED;
    }
    
    return errorCode;
}

static void PAC1xxx_UpdateContext_CtrlProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    pRawValue = pdevice->i2cRxBuffer_ctrl;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    PAC1xxx_CtrlBytesToRegfields(pRawValue, &(pdevice->ctrl_LAT));
    pdevice->ctrl_LAT_cached = true;
}


static int16_t PAC1xxx_UpdateContext_AccumulatorCount(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    int16_t errorCode = PAC1xxx_SUCCESS;
    bool bSuccess;
    //check if cached ACC_COUNT is valid
    //if NOT valid, update the cache
    if(pdevice->accCount_cached == false){
        pdevice->i2cTxBuffer[0] = PAC1xxx_ACC_COUNT_ADDR;
        unsigned int i2cRxSize = PAC1xxx_ACC_COUNT_SZ;
        if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) i2cRxSize++;   
        pdevice->i2cCommStatus = PAC1xxx_I2C_TRANSFER_NO_EVENT;
        bSuccess = PAC1xxx_I2C_WriteRead(&(pdevice->i2c_context), (void*) pdevice->i2cTxBuffer, 1, 
                                                    (void*) pdevice->i2cRxBuffer_accCount, i2cRxSize);        
        if(bSuccess == false){
            return PAC1xxx_I2C_FAIL;
        }        
    }
    else
    {
        errorCode = PAC1xxx_ALREADY_CACHED;
    }    
    return errorCode;
}


static void PAC1xxx_UpdateContext_AccumulatorCountProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawBytes;
    pRawBytes = pdevice->i2cRxBuffer_accCount;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawBytes++;
    pdevice->accCount = PAC1xxx_RawBytestoReg32bit(pRawBytes);
    pdevice->accCount_cached = true;
}


static int16_t PAC1xxx_Get_Register(PAC1xxx_DEVICE_CONTEXT_P pdevice, 
                                    void* pregister_val, uint8_t registerAddr, size_t regSize, 
                                    PAC1xxx_procState processingState){
    int16_t retcode;
    bool bSuccess;
    
    if ((pdevice == NULL) || (pregister_val == NULL)) return PAC1xxx_INVALID_PARAMETER;
    
    // check if new device request is allowed
    // lock the processingState mutex
    if( PAC1xxx_MUTEX_Lock(&(pdevice->mutexProcState)) == false )    
    {
        return PAC1xxx_BUSY;
    }
    
    if(pdevice->processingState == Idle){
        // start new request 
        pdevice->processingState = processingState;
        pdevice->deviceEventStatus = PAC1xxx_EVENT_NONE;
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));        
    }else
    {
        // reject new request
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));        
        return PAC1xxx_BUSY;
    }
    
    pdevice->outData = (void*)pregister_val;
    pdevice->i2cTxBuffer[0] = registerAddr;
    
    size_t i2cRxSize = regSize;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) i2cRxSize++; 

    pdevice->i2cCommStatus = PAC1xxx_I2C_TRANSFER_NO_EVENT;
    bSuccess = PAC1xxx_I2C_WriteRead(&(pdevice->i2c_context), (void*) pdevice->i2cTxBuffer, 1, 
                                                (void*) pdevice->i2cRxBuffer, i2cRxSize);        
    if(bSuccess == false){
        pdevice->processingState = Idle;
        pdevice->processError = PAC1xxx_I2C_FAIL;
        return PAC1xxx_I2C_FAIL;
    }
    
    // wait here for the request processing completion if the library is in SYNC mode
    retcode = RequestReturn(pdevice);
    return retcode;
}


static int16_t PAC1xxx_Get_VACCRegister(PAC1xxx_DEVICE_CONTEXT_P pdevice, 
                                        void* pregister_val, uint8_t* pmode, uint32_t time,
                                        PAC1xxx_ProcessMode procMode){
    int16_t retcode;
    bool bSuccess;
    
    // pointer parameters already validated by the caller function:
    // pdevice, pregister_val, pmode
    
    // check if new device request is allowed
    // lock the processingState mutex
    if( PAC1xxx_MUTEX_Lock(&(pdevice->mutexProcState)) == false )    
    {
        return PAC1xxx_BUSY;
    }
    
    if(pdevice->processingState == Idle){
        // start new request 
        pdevice->processingState = GetVACCValueReq;
        pdevice->deviceEventStatus = PAC1xxx_EVENT_NONE;
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));        
    }else
    {
        // reject new request
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));        
        return PAC1xxx_BUSY;
    }
    
    //copy the parameters for post-processing into context data
    pdevice->regProcMode = procMode;
    pdevice->accMode = pmode;
    pdevice->time = time;
    
    pdevice->outData = (void*)pregister_val;
    pdevice->i2cTxBuffer[0] = PAC1xxx_VACC_ADDR;
    
    size_t i2cRxSize = PAC1xxx_VACC_SZ;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) i2cRxSize++; 

    pdevice->i2cCommStatus = PAC1xxx_I2C_TRANSFER_NO_EVENT;
    bSuccess = PAC1xxx_I2C_WriteRead(&(pdevice->i2c_context), (void*) pdevice->i2cTxBuffer, 1, 
                                                (void*) pdevice->i2cRxBuffer, i2cRxSize);        
    if(bSuccess == false){
        pdevice->processingState = Idle;
        pdevice->processError = PAC1xxx_I2C_FAIL;
        return PAC1xxx_I2C_FAIL;
    }
    
    // wait here for the request processing completion if the library is in SYNC mode
    retcode = RequestReturn(pdevice);
    return retcode;
}


static void PAC1xxx_Get_Reg8bitProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawValue;
    
    pRawValue = pdevice->i2cRxBuffer;    
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawValue++;
    
    *((uint8_t*)pdevice->outData) = (uint8_t)pRawValue[0];
}


static void PAC1xxx_Get_Reg16bitProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawBytes;
    uint16_t regVal;
    
    pRawBytes = pdevice->i2cRxBuffer;    
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawBytes++;    
    regVal = PAC1xxx_RawBytestoReg16bit(pRawBytes);
    *((uint16_t*)pdevice->outData) = regVal;
}


static void PAC1xxx_Get_Reg32bitProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawBytes;
    uint32_t regVal;
    
    pRawBytes = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawBytes++;
    regVal = PAC1xxx_RawBytestoReg32bit(pRawBytes);
    *((uint32_t*)pdevice->outData) = regVal;
}


static void PAC1xxx_Get_Reg56bitProcess(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    uint8_t* pRawBytes;
    uint64_t regVal;
    
    pRawBytes = pdevice->i2cRxBuffer;
    if(pdevice->ENABLE_BYTE_COUNT_FLAG == true) pRawBytes++;
    regVal = PAC1xxx_RawBytesToReg64bit(pRawBytes);
    *((uint64_t*)pdevice->outData) = regVal;
}


static int16_t PAC1xxx_Set_Register(PAC1xxx_DEVICE_CONTEXT_P pdevice, uint8_t *pregisterBytes, uint8_t registerAddr, size_t regSize, PAC1xxx_procState processingState){
    int16_t retcode;
    bool bSuccess;
    
    if (pdevice == NULL) return PAC1xxx_INVALID_PARAMETER;
    /* pregisterBytes is not NULL - ensured by the function callers) */
    
    // check if new device request is allowed
    // lock the processingState mutex
    if( PAC1xxx_MUTEX_Lock(&(pdevice->mutexProcState)) == false )
    {
        return PAC1xxx_BUSY;
    }
    
    if(pdevice->processingState == Idle){
        // start new request 
        pdevice->processingState = processingState;
        pdevice->deviceEventStatus = PAC1xxx_EVENT_NONE;
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));        
    }else
    {
        // reject new request
        PAC1xxx_MUTEX_Unlock(&(pdevice->mutexProcState));        
        return PAC1xxx_BUSY;
    }
    
    pdevice->i2cTxBuffer[0] = registerAddr;
    for(size_t cnt=0; cnt < regSize; cnt++){
        pdevice->i2cTxBuffer[cnt+1] = pregisterBytes[cnt];
    }

    pdevice->i2cCommStatus = PAC1xxx_I2C_TRANSFER_NO_EVENT;
    bSuccess = PAC1xxx_I2C_Write(&(pdevice->i2c_context), (void*) pdevice->i2cTxBuffer, (regSize + 1));
    if(bSuccess == false){
        pdevice->processingState = Idle;
        pdevice->processError = PAC1xxx_I2C_FAIL;
        return PAC1xxx_I2C_FAIL;
    }    
    // wait here for the request processing completion if the library is in SYNC mode
    retcode = RequestReturn(pdevice);
    return retcode;
}


static inline void PAC1xxx_UpdateContext_ScaleValues(PAC1xxx_DEVICE_CONTEXT_P pdevice){
    (pdevice->VbusScaleRange)   = PAC1xxx_VbusScaleRange(pdevice->VbusMAX, pdevice->negPwrFsr_LAT);
    (pdevice->VsenseScaleRange) = PAC1xxx_VsenseScaleRange(pdevice->VsenseMAX, pdevice->negPwrFsr_LAT);
    (pdevice->VPowerScaleRange) = PAC1xxx_VpowerScaleRange(pdevice->VPowerMAX, pdevice->negPwrFsr_LAT);
    
    pdevice->IsSignedPower = PAC1xxx_IsSignedVpower(pdevice->negPwrFsr_LAT);
    pdevice->IsSignedVsense = PAC1xxx_IsSignedVsense(pdevice->negPwrFsr_LAT);
    pdevice->IsSignedVbus = PAC1xxx_IsSignedVbus(pdevice->negPwrFsr_LAT);
}


void PAC1xxx_CtrlBytesToRegfields(uint8_t* pCtrlBytes, PAC1xxx_CONTROL_REGFIELDS_P pCtrlRegfields){

    pCtrlRegfields->SAMPLE_MODE  = (pCtrlBytes[0] >> PAC1xxx_CONTROL_SAMPLE_MODE_BITPOSMSB)  & PAC1xxx_CONTROL_SAMPLE_MODE_BITMASK;
    pCtrlRegfields->GPIO_ALERT1  = (pCtrlBytes[0] >> PAC1xxx_CONTROL_GPIO_ALERT1_BITPOSMSB)  & PAC1xxx_CONTROL_GPIO_ALERT_BITMASK;
    pCtrlRegfields->SLOW_ALERT0  = (pCtrlBytes[0] /* >> PAC1xxx_CONTROL_SLOW_ALERT0_BITPOSMSB */) & PAC1xxx_CONTROL_GPIO_ALERT_BITMASK;
    pCtrlRegfields->AVERAGE      = (pCtrlBytes[1] >> PAC1xxx_CONTROL_AVERAGE_BITPOSLSB)      & PAC1xxx_CONTROL_AVERAGE_BITMASK;
    pCtrlRegfields->AA           = (pCtrlBytes[1] >> PAC1xxx_CONTROL_AA_BITPOSLSB)           & PAC1xxx_CONTROL_AA_BITMASK;
    pCtrlRegfields->ACC_CONFIG   = (pCtrlBytes[1] >> PAC1xxx_CONTROL_ACC_CONFIG_BITPOSLSB)   & PAC1xxx_CONTROL_ACC_CONFIG_BITMASK; 
    pCtrlRegfields->AUTO_REFRESH = (pCtrlBytes[1] /* >> PAC1xxx_CONTROL_AUTO_REFRESH_BITPOSLSB */) & PAC1xxx_CONTROL_AUTO_REFRESH_BITMASK;    
}


void PAC1xxx_CtrlRegfieldsToBytes(PAC1xxx_CONTROL_REGFIELDS CtrlRegfields, uint8_t* pCtrlBytes){
    //MSB
    pCtrlBytes[0] = (uint8_t)(
                    (CtrlRegfields.SAMPLE_MODE << PAC1xxx_CONTROL_SAMPLE_MODE_BITPOSMSB) |
                    (CtrlRegfields.GPIO_ALERT1 << PAC1xxx_CONTROL_GPIO_ALERT1_BITPOSMSB) |
                    (CtrlRegfields.SLOW_ALERT0 /* << PAC1xxx_CONTROL_SLOW_ALERT0_BITPOSMSB */ ));
    //LSB
    pCtrlBytes[1] = (uint8_t)(
                    (CtrlRegfields.AVERAGE << PAC1xxx_CONTROL_AVERAGE_BITPOSLSB)       |
                    (CtrlRegfields.AA << PAC1xxx_CONTROL_AA_BITPOSLSB)                 |
                    (CtrlRegfields.ACC_CONFIG << PAC1xxx_CONTROL_ACC_CONFIG_BITPOSLSB) |
                    (CtrlRegfields.AUTO_REFRESH /* << PAC1xxx_CONTROL_AUTO_REFRESH_BITPOSLSB */ )); 
}


void PAC1xxx_SMBusBytesToRegfields(uint8_t* pSMBusBytes, PAC1xxx_SMBUS_SETTINGS_REGFIELDS_P pSMBusRegfields){
       
    pSMBusRegfields->GPIO_DATA1  = (pSMBusBytes[0] >> PAC1xxx_SMBUS_GPIODATA1_BITPOS) & PAC1xxx_SMBUS_BITMASK;
    pSMBusRegfields->GPIO_DATA0  = (pSMBusBytes[0] >> PAC1xxx_SMBUS_GPIODATA0_BITPOS) & PAC1xxx_SMBUS_BITMASK;
    pSMBusRegfields->ANY_ALERT   = (pSMBusBytes[0] >> PAC1xxx_SMBUS_ANYALERT_BITPOS)  & PAC1xxx_SMBUS_BITMASK;
    pSMBusRegfields->POR         = (pSMBusBytes[0] >> PAC1xxx_SMBUS_POR_BITPOS)       & PAC1xxx_SMBUS_BITMASK;
    pSMBusRegfields->TIMEOUT     = (pSMBusBytes[0] >> PAC1xxx_SMBUS_TIMEOUT_BITPOS)   & PAC1xxx_SMBUS_BITMASK;
    pSMBusRegfields->BYTE_COUNT  = (pSMBusBytes[0] >> PAC1xxx_SMBUS_BYTECOUNT_BITPOS) & PAC1xxx_SMBUS_BITMASK;
    pSMBusRegfields->I2C_HISPEED = (pSMBusBytes[0] /* >> PAC1xxx_SMBUS_I2CSPEED_BITPOS */)  & PAC1xxx_SMBUS_BITMASK;    
}


void PAC1xxx_SMBusRegfieldsToBytes(PAC1xxx_SMBUS_SETTINGS_REGFIELDS SMBusRegfields, uint8_t* pSMBusBytes){
    pSMBusBytes[0] = (uint8_t)(
                     (SMBusRegfields.GPIO_DATA1 << PAC1xxx_SMBUS_GPIODATA1_BITPOS) |
                     (SMBusRegfields.GPIO_DATA0 << PAC1xxx_SMBUS_GPIODATA0_BITPOS) |
                     (SMBusRegfields.ANY_ALERT  << PAC1xxx_SMBUS_ANYALERT_BITPOS)  |
                     (SMBusRegfields.POR        << PAC1xxx_SMBUS_POR_BITPOS)       |
                     (SMBusRegfields.TIMEOUT    << PAC1xxx_SMBUS_TIMEOUT_BITPOS)   |
                     (SMBusRegfields.BYTE_COUNT << PAC1xxx_SMBUS_BYTECOUNT_BITPOS) |
                     (SMBusRegfields.I2C_HISPEED /* << PAC1xxx_SMBUS_I2CSPEED_BITPOS */ ));  
}


void PAC1xxx_NegPwrFsrBytesToRegfields(uint8_t* pNegPwrFsrBytes, PAC1xxx_NEGPWRFSR_REGFIELDS_P pNegPwrFsrRegfields){
    pNegPwrFsrRegfields->CFG_VS = (pNegPwrFsrBytes[0] >> PAC1xxx_NEGPWRFSR_VS1_BITPOS) & PAC1xxx_NEGPWRFSR_BITMASK;   
    pNegPwrFsrRegfields->CFG_VB = (pNegPwrFsrBytes[0] /* >> PAC1xxx_NEGPWRFSR_VB1_BITPOS */) & PAC1xxx_NEGPWRFSR_BITMASK;
}


void PAC1xxx_NegPwrFsrRegfieldsToBytes(PAC1xxx_NEGPWRFSR_REGFIELDS NegPwrFsrRegfields, uint8_t* pNegPwrFsrBytes){
    pNegPwrFsrBytes[0] = (uint8_t)((NegPwrFsrRegfields.CFG_VS << PAC1xxx_NEGPWRFSR_VS1_BITPOS) |
                                   (NegPwrFsrRegfields.CFG_VB /* << PAC1xxx_NEGPWRFSR_VB1_BITPOS */ ));   
}


void PAC1xxx_SlowBytesToRegfields(uint8_t* pSlowBytes, PAC1xxx_SLOW_REGFIELDS_P pSlowRegfields){
    pSlowRegfields->Slow         = (pSlowBytes[0] >> PAC1xxx_SLOW_SLOW_BITPOS)   & PAC1xxx_SLOW_BITMASK;
    pSlowRegfields->SlowLowHigh  = (pSlowBytes[0] >> PAC1xxx_SLOW_LH_BITPOS)     & PAC1xxx_SLOW_BITMASK;
    pSlowRegfields->SlowHighLow  = (pSlowBytes[0] >> PAC1xxx_SLOW_HL_BITPOS)     & PAC1xxx_SLOW_BITMASK;
    pSlowRegfields->RefreshRise  = (pSlowBytes[0] >> PAC1xxx_SLOW_RRISE_BITPOS)  & PAC1xxx_SLOW_BITMASK;
    pSlowRegfields->RefreshVRise = (pSlowBytes[0] >> PAC1xxx_SLOW_RVRISE_BITPOS) & PAC1xxx_SLOW_BITMASK;
    pSlowRegfields->RefreshFall  = (pSlowBytes[0] >> PAC1xxx_SLOW_RFALL_BITPOS)  & PAC1xxx_SLOW_BITMASK;
    pSlowRegfields->RefreshVFall = (pSlowBytes[0] >> PAC1xxx_SLOW_RVFALL_BITPOS) & PAC1xxx_SLOW_BITMASK;
}


void PAC1xxx_SlowRegfieldsToBytes(PAC1xxx_SLOW_REGFIELDS SlowRegfields, uint8_t* pSlowBytes){
    pSlowBytes[0] = (uint8_t)(
                    (SlowRegfields.Slow << PAC1xxx_SLOW_SLOW_BITPOS)           |
                    (SlowRegfields.SlowLowHigh << PAC1xxx_SLOW_LH_BITPOS)      |
                    (SlowRegfields.SlowHighLow << PAC1xxx_SLOW_HL_BITPOS)      |
                    (SlowRegfields.RefreshRise << PAC1xxx_SLOW_RRISE_BITPOS)   |
                    (SlowRegfields.RefreshVRise << PAC1xxx_SLOW_RVRISE_BITPOS) |
                    (SlowRegfields.RefreshFall << PAC1xxx_SLOW_RFALL_BITPOS)   |
                    (SlowRegfields.RefreshVFall << PAC1xxx_SLOW_RVFALL_BITPOS)); 
}


void PAC1xxx_AlertStatusBytesToRegfields(uint8_t* pAlertBytes, PAC1xxx_ALERT_STATUS_REGFIELDS_P pAlertStatusRegfields){
    pAlertStatusRegfields->RV = (pAlertBytes[0] >> PAC1xxx_ALERT_RV_BITPOSMSB) & PAC1xxx_ALERT_BITMASK;
    pAlertStatusRegfields->FV = (pAlertBytes[0] >> PAC1xxx_ALERT_FV_BITPOSMSB) & PAC1xxx_ALERT_BITMASK;
    pAlertStatusRegfields->RC = (pAlertBytes[0] >> PAC1xxx_ALERT_RC_BITPOSMSB) & PAC1xxx_ALERT_BITMASK;
    pAlertStatusRegfields->FC = (pAlertBytes[0] >> PAC1xxx_ALERT_FC_BITPOSMSB) & PAC1xxx_ALERT_BITMASK;
    pAlertStatusRegfields->OC = (pAlertBytes[0] >> PAC1xxx_ALERT_OC_BITPOSMSB) & PAC1xxx_ALERT_BITMASK;
    pAlertStatusRegfields->UC =  pAlertBytes[0] & PAC1xxx_ALERT_BITMASK;
    
    pAlertStatusRegfields->OV        = (pAlertBytes[1] >> PAC1xxx_ALERT_OV_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;
    pAlertStatusRegfields->UV        = (pAlertBytes[1] >> PAC1xxx_ALERT_UV_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;
    pAlertStatusRegfields->OPC       = (pAlertBytes[1] >> PAC1xxx_ALERT_OPC_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;
    pAlertStatusRegfields->OPW       = (pAlertBytes[1] >> PAC1xxx_ALERT_OPW_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;
    pAlertStatusRegfields->ACC_OVF   = (pAlertBytes[1] >> PAC1xxx_ALERT_ACCOVF_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;
    pAlertStatusRegfields->ACC_COUNT = (pAlertBytes[1] >> PAC1xxx_ALERT_ACCCNT_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;

}


void PAC1xxx_AlertEnableBytesToRegfields(uint8_t* pAlertBytes, PAC1xxx_ALERT_ENABLE_REGFIELDS_P pAlertEnableRegfields){
    pAlertEnableRegfields->RV = (pAlertBytes[0] >> PAC1xxx_ALERT_RV_BITPOSMSB) & PAC1xxx_ALERT_BITMASK;
    pAlertEnableRegfields->FV = (pAlertBytes[0] >> PAC1xxx_ALERT_FV_BITPOSMSB) & PAC1xxx_ALERT_BITMASK;
    pAlertEnableRegfields->RC = (pAlertBytes[0] >> PAC1xxx_ALERT_RC_BITPOSMSB) & PAC1xxx_ALERT_BITMASK;
    pAlertEnableRegfields->FC = (pAlertBytes[0] >> PAC1xxx_ALERT_FC_BITPOSMSB) & PAC1xxx_ALERT_BITMASK;
    pAlertEnableRegfields->OC = (pAlertBytes[0] >> PAC1xxx_ALERT_OC_BITPOSMSB) & PAC1xxx_ALERT_BITMASK;
    pAlertEnableRegfields->UC =  pAlertBytes[0] & PAC1xxx_ALERT_BITMASK;
    
    pAlertEnableRegfields->OV        = (pAlertBytes[1] >> PAC1xxx_ALERT_OV_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;
    pAlertEnableRegfields->UV        = (pAlertBytes[1] >> PAC1xxx_ALERT_UV_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;
    pAlertEnableRegfields->OPC       = (pAlertBytes[1] >> PAC1xxx_ALERT_OPC_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;
    pAlertEnableRegfields->OPW       = (pAlertBytes[1] >> PAC1xxx_ALERT_OPW_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;
    pAlertEnableRegfields->ACC_OVF   = (pAlertBytes[1] >> PAC1xxx_ALERT_ACCOVF_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;
    pAlertEnableRegfields->ACC_COUNT = (pAlertBytes[1] >> PAC1xxx_ALERT_ACCCNT_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;
    pAlertEnableRegfields->ALERT_CCx = (pAlertBytes[1] >> PAC1xxx_ALERT_CC_BITPOSLSB) & PAC1xxx_ALERT_BITMASK;    
}


void PAC1xxx_AlertEnableRegfieldsToBytes(PAC1xxx_ALERT_ENABLE_REGFIELDS AlertEnableRegfields, uint8_t* pAlertBytes){
    //MSB
    pAlertBytes[0] = (uint8_t)(
                     (AlertEnableRegfields.RV << PAC1xxx_ALERT_RV_BITPOSMSB) |
                     (AlertEnableRegfields.FV << PAC1xxx_ALERT_FV_BITPOSMSB) |
                     (AlertEnableRegfields.RC << PAC1xxx_ALERT_RC_BITPOSMSB) |
                     (AlertEnableRegfields.FC << PAC1xxx_ALERT_FC_BITPOSMSB) |
                     (AlertEnableRegfields.OC << PAC1xxx_ALERT_OC_BITPOSMSB) |
                     (AlertEnableRegfields.UC /* << PAC1xxx_ALERT_UC_BITPOSMSB */ )); 
    //LSB
    pAlertBytes[1] = (uint8_t)(
                     (AlertEnableRegfields.OV << PAC1xxx_ALERT_OV_BITPOSLSB) |
                     (AlertEnableRegfields.UV << PAC1xxx_ALERT_UV_BITPOSLSB) |
                     (AlertEnableRegfields.OPC << PAC1xxx_ALERT_OPC_BITPOSLSB) |
                     (AlertEnableRegfields.OPW << PAC1xxx_ALERT_OPW_BITPOSLSB) |
                     (AlertEnableRegfields.ACC_OVF << PAC1xxx_ALERT_ACCOVF_BITPOSLSB) |
                     (AlertEnableRegfields.ACC_COUNT << PAC1xxx_ALERT_ACCCNT_BITPOSLSB) |
                     (AlertEnableRegfields.ALERT_CCx << PAC1xxx_ALERT_CC_BITPOSLSB)); 
}


void PAC1xxx_AccFullnessBytesToRegfields(uint8_t* pAccFullnessBytes, PAC1xxx_ACCUM_LIMITS_REGFIELDS_P pAccFullnessRegfields){
    pAccFullnessRegfields->ACC_FULL       = (pAccFullnessBytes[0] >> PAC1xxx_ACC_FULL_BITPOS) & PAC1xxx_ACC_FULL_BITMASK;
    pAccFullnessRegfields->ACC_COUNT_FULL = (pAccFullnessBytes[0] /* >> PAC1xxx_ACC_COUNT_FULL_BITPOS */) & PAC1xxx_ACC_COUNT_FULL_BITMASK;
}


void PAC1xxx_AccFullnessRegfieldsToBytes(PAC1xxx_ACCUM_LIMITS_REGFIELDS AccFullnessRegfields, uint8_t* pAccFullnessBytes){
    pAccFullnessBytes[0] = (uint8_t)((AccFullnessRegfields.ACC_FULL << PAC1xxx_ACC_FULL_BITPOS) |
                                     (AccFullnessRegfields.ACC_COUNT_FULL /* << PAC1xxx_ACC_COUNT_FULL_BITPOS */ ));  
}


void PAC1xxx_StepLimitBytesToRegfields(uint8_t* pStepLimitBytes, PAC1xxx_STEP_LIMIT_REGFIELDS_P pStepLimitRegfields){
    pStepLimitRegfields->STEP_RV = (pStepLimitBytes[0] >> PAC1xxx_STEPLIMIT_RV_BITPOS) & PAC1xxx_STEP_LIMIT_BITMASK;
    pStepLimitRegfields->STEP_FV = (pStepLimitBytes[0] >> PAC1xxx_STEPLIMIT_FV_BITPOS) & PAC1xxx_STEP_LIMIT_BITMASK;
    pStepLimitRegfields->STEP_RC = (pStepLimitBytes[0] >> PAC1xxx_STEPLIMIT_RC_BITPOS) & PAC1xxx_STEP_LIMIT_BITMASK;
    pStepLimitRegfields->STEP_FC = (pStepLimitBytes[0] /* >> PAC1xxx_STEPLIMIT_FC_BITPOS */) & PAC1xxx_STEP_LIMIT_BITMASK;    
}


void PAC1xxx_StepLimitRegfieldsToBytes(PAC1xxx_STEP_LIMIT_REGFIELDS StepLimitRegfields, uint8_t* pStepLimitBytes){
    pStepLimitBytes[0] =  (uint8_t)(
                          (StepLimitRegfields.STEP_RV << PAC1xxx_STEPLIMIT_RV_BITPOS) |
                          (StepLimitRegfields.STEP_FV << PAC1xxx_STEPLIMIT_FV_BITPOS) |
                          (StepLimitRegfields.STEP_RC << PAC1xxx_STEPLIMIT_RC_BITPOS) |
                          (StepLimitRegfields.STEP_FC /* << PAC1xxx_STEPLIMIT_FC_BITPOS */ ));    
}


void PAC1xxx_NsamplesBytesToRegfields(uint8_t* pNsamplesBytes, PAC1xxx_LIMIT_NSAMPLES_REGFIELDS_P pNsamplesRegfields){
    pNsamplesRegfields->Nsamples_OPC = (pNsamplesBytes[0] >> PAC1xxx_LIMITNSAMP_OPC_BITPOSMSB) & PAC1xxx_LIMIT_NSAMPLES_BITMASK;
    pNsamplesRegfields->Nsamples_OPW = (pNsamplesBytes[0] /* >> PAC1xxx_LIMITNSAMP_OPW_BITPOSMSB */) & PAC1xxx_LIMIT_NSAMPLES_BITMASK;
    pNsamplesRegfields->Nsamples_OC = (pNsamplesBytes[1] >> PAC1xxx_LIMITNSAMP_OC_BITPOSLSB) & PAC1xxx_LIMIT_NSAMPLES_BITMASK;
    pNsamplesRegfields->Nsamples_UC = (pNsamplesBytes[1] >> PAC1xxx_LIMITNSAMP_UC_BITPOSLSB) & PAC1xxx_LIMIT_NSAMPLES_BITMASK;
    pNsamplesRegfields->Nsamples_OV = (pNsamplesBytes[1] >> PAC1xxx_LIMITNSAMP_OV_BITPOSLSB) & PAC1xxx_LIMIT_NSAMPLES_BITMASK;
    pNsamplesRegfields->Nsamples_UV = (pNsamplesBytes[1] /* >> PAC1xxx_LIMITNSAMP_UV_BITPOSLSB */) & PAC1xxx_LIMIT_NSAMPLES_BITMASK;    
}


void PAC1xxx_NsamplesRegfieldsToBytes(PAC1xxx_LIMIT_NSAMPLES_REGFIELDS NsamplesRegfields, uint8_t* pNsamplesBytes){

    pNsamplesBytes[0] =   (uint8_t)(
                          (NsamplesRegfields.Nsamples_OPC << PAC1xxx_LIMITNSAMP_OPC_BITPOSMSB) |
                          (NsamplesRegfields.Nsamples_OPW /* << PAC1xxx_LIMITNSAMP_OPW_BITPOSMSB */ ));
    pNsamplesBytes[1] =   (uint8_t)(
                          (NsamplesRegfields.Nsamples_OC << PAC1xxx_LIMITNSAMP_OC_BITPOSLSB) |
                          (NsamplesRegfields.Nsamples_UC << PAC1xxx_LIMITNSAMP_UC_BITPOSLSB) |
                          (NsamplesRegfields.Nsamples_OV << PAC1xxx_LIMITNSAMP_OV_BITPOSLSB) |
                          (NsamplesRegfields.Nsamples_UV /* << PAC1xxx_LIMITNSAMP_UV_BITPOSLSB */ ));    
}


float PAC1xxx_VaccReg64bitToPower_mW(uint64_t VAccReg, bool IsSignedPower, uint16_t VPowerScaleRange, bool is12bitADCres, uint32_t rsense){
    float VpowerAccReal, PowerUnit;
    
    if(rsense == 0) return 0.0;
    
    if(IsSignedPower == true){
        if( (VAccReg & 0x80000000000000) == 0x80000000000000){
            VAccReg = VAccReg | 0xFF80000000000000; //sign extension
        }
        VpowerAccReal = (float)((int64_t)VAccReg);
    }else{
        VpowerAccReal = (float)(VAccReg);
    }

    PowerUnit = (float)VPowerScaleRange / (float)rsense;
    if(is12bitADCres == true){
        PowerUnit = PowerUnit * (1000000.0 / 16777216.0);     //milli-Watts/bit. VACC accumulates 24-bit VPOWER values
    }else{
        PowerUnit = PowerUnit * (1000000.0 / 4294967296.0);   //milli-Watts/bit. VACC accumulates 32-bit VPOWER values
    }
    VpowerAccReal = VpowerAccReal * PowerUnit;
    return VpowerAccReal;
}


float PAC1xxx_VaccReg64bitToVoltage_mV(uint64_t VAccReg, bool IsSignedVoltage, uint16_t VoltageScaleRange, bool is12bitADCres){
    float VAccReal, VoltageLsb;
        
    if(IsSignedVoltage == true){
        if( (VAccReg & 0x80000000000000) == 0x80000000000000){
            VAccReg = VAccReg | 0xFF80000000000000; //sign extension
        }
        VAccReal = (float)((int64_t)VAccReg);
    }else{
        VAccReal = (float)(VAccReg);
    }

    if(is12bitADCres == true){
        VoltageLsb = VoltageScaleRange / 4096.0;    //VACC accumulates 12-bit VBUS/VSENSE values
    }else{
        VoltageLsb = VoltageScaleRange / 65536.0;   //VACC accumulates 16-bit VBUS/VSENSE values
    }
    VAccReal = VAccReal * VoltageLsb; //mV
    return VAccReal;
}


float PAC1xxx_VoltageReg16bitToVoltage_mV(uint16_t VoltageReg, bool IsSignedVoltage, uint16_t VoltageScaleRange){
    float VoltReal;
    if (IsSignedVoltage == true){
        int16_t signedReg;
        int32_t tempProd;
        signedReg = (int16_t)VoltageReg;
        tempProd = (int32_t)signedReg * (int32_t)VoltageScaleRange;
        VoltReal = (float)tempProd;
    }else{
        uint32_t tempProd;
        tempProd = (uint32_t)VoltageReg * (uint32_t)VoltageScaleRange;
        VoltReal = (float)tempProd;
    }
    
    VoltReal = VoltReal / 65536.0;    //mV   
    return VoltReal;
}


float PAC1xxx_VoltageReg16bitToCurrent_mA(uint16_t VsenseReg, bool IsSignedVoltage, uint16_t VsenseScaleRange, uint32_t rsense){
    float VsenseReal, IsenseReal;

    if(rsense == 0) return 0.0;
    
    VsenseReal = PAC1xxx_VoltageReg16bitToVoltage_mV(VsenseReg, IsSignedVoltage, VsenseScaleRange);
    IsenseReal = ( VsenseReal * 1000000.0 ) / (float)(rsense);     //mA. VsenseReal in mV, rsense in micro-Ohm
    return IsenseReal;
}


float PAC1xxx_VpowerReg32bitToPower_mW(uint32_t VpowerReg, bool IsSignedPower, uint16_t VPowerScaleRange, uint32_t rsense){
    float VpowerReal, PowerUnit;
    
    if(rsense == 0) return 0.0;
    
    if(IsSignedPower == true){
        int32_t signedVpowerReg;
        signedVpowerReg = (int32_t)VpowerReg;
        VpowerReal = (float)signedVpowerReg;
    }else{
        VpowerReal = (float)VpowerReg;
    }

    PowerUnit = (float)VPowerScaleRange * (1000000.0 / 4294967296.0) / (float)rsense;
    VpowerReal = VpowerReal * PowerUnit;
    return VpowerReal;
}


void PAC1xxx_Reg16bitToRawBytes(uint16_t regVal, uint8_t* pRawBytes){
    pRawBytes[0] = (regVal >> 8) & 0xff;
    pRawBytes[1] = regVal & 0xff;
}


uint16_t PAC1xxx_RawBytestoReg16bit(uint8_t* pRawBytes){
    uint16_t regVal;
    regVal = (uint16_t)pRawBytes[0];
    regVal = (regVal << 8) | (uint16_t)pRawBytes[1];
    return regVal;
}


uint32_t PAC1xxx_RawBytestoReg32bit(uint8_t* pRawBytes){
    uint32_t regVal;
    regVal = (uint32_t)pRawBytes[0];
    regVal = (regVal << 8) | (uint32_t)pRawBytes[1];
    regVal = (regVal << 8) | (uint32_t)pRawBytes[2];
    regVal = (regVal << 8) | (uint32_t)pRawBytes[3];
    return regVal;
}


uint64_t PAC1xxx_RawBytesToReg64bit(uint8_t* pRawBytes){
    uint64_t regVal;
    regVal = (uint64_t)pRawBytes[0];
    regVal = (regVal << 8) | (uint64_t)pRawBytes[1];
    regVal = (regVal << 8) | (uint64_t)pRawBytes[2];
    regVal = (regVal << 8) | (uint64_t)pRawBytes[3];
    regVal = (regVal << 8) | (uint64_t)pRawBytes[4];
    regVal = (regVal << 8) | (uint64_t)pRawBytes[5];
    regVal = (regVal << 8) | (uint64_t)pRawBytes[6];
    return regVal;
}


bool PAC1xxx_IsSignedVbus(PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr){
    // either Vbus is bipolar or FSR/2
    return ( negpwrfsr.CFG_VB !=0 );
}


bool PAC1xxx_IsSignedVsense(PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr){
    // either Vsense is bipolar or FSR/2
    return ( negpwrfsr.CFG_VS !=0 );
}


bool PAC1xxx_IsSignedVpower(PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr){
    // either Vbus or Vsense are bipolar or FSR/2
    return ( (negpwrfsr.CFG_VB | negpwrfsr.CFG_VS) != 0 );
}


uint16_t PAC1xxx_VbusScaleRange(uint16_t VbusMAX, PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr){
    if (negpwrfsr.CFG_VB == 0x1) {
        VbusMAX *= 2; // LSB value is double for bipolar channels
    }
    return VbusMAX;
}


uint16_t PAC1xxx_VsenseScaleRange(uint16_t VsenseMax, PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr){
    if (negpwrfsr.CFG_VS == 0x1) {
        VsenseMax *= 2; // LSB value is double for bipolar channels
    }
    return VsenseMax;
}


uint16_t PAC1xxx_VpowerScaleRange(uint16_t VPowerMax, PAC1xxx_NEGPWRFSR_REGFIELDS negpwrfsr){
    if ((negpwrfsr.CFG_VB | negpwrfsr.CFG_VS) == 0x1) {
        VPowerMax *= 2; // LSB value is double for "bipolar mode" channels
                        // either Vbus or Vsense are bipolar and no one is FSR/2
    }
    return VPowerMax;
}


float PAC1xxx_VaccPowerToEnergy_mWh(float accumulatedPower_mW, int16_t sampleRate){
    float Energy;
    if( sampleRate > 1 ) {
        // single-shot requires the use of PAC1xxx_VaccPowerTimedToEnergy_mWh()
        Energy = accumulatedPower_mW / (float)sampleRate;
    }else{
        Energy = 0.0;
    } 
    Energy *= ENERGY_UNIT_CONVERSION;
    return Energy;
 }


float PAC1xxx_VaccPowerTimedToEnergy_mWh(float accumulatedPower_mW, uint32_t sampleCount, uint32_t time_ms){
    float Energy;
    if(sampleCount > 0) {
        Energy = accumulatedPower_mW / (float)sampleCount;
    }else{
        Energy = 0.0;
    }

    Energy *= (float)time_ms;
    Energy *= (ENERGY_UNIT_CONVERSION / 1000.0); // milli-Watt*h
    return Energy;
}


float PAC1xxx_VaccVoltageToCoulombCnt(float accumulatedVoltage_mV, int16_t sampleRate, uint32_t rsense){
    float CoulombCnt;
    if(rsense == 0) return 0.0;
    
    if( sampleRate > 1 ) {
        // single-shot requires the use of PAC1xxx_VaccVoltageTimedToCoulombCnt() 
        CoulombCnt = accumulatedVoltage_mV / (float)sampleRate;
    }else{
        CoulombCnt = 0.0;
    } 
    
    CoulombCnt = (1000000.0 * CoulombCnt) / (float)rsense; //milli-Coulomb
    return CoulombCnt;
}


float PAC1xxx_VaccVoltageTimedToCoulombCnt(float accumulatedVoltage_mV, uint32_t sampleCount, uint32_t time_ms, uint32_t rsense){
    float CoulombCnt;
    if(rsense == 0) return 0.0;
    
    if(sampleCount > 0){
        CoulombCnt = accumulatedVoltage_mV / (float)sampleCount; 
    }else{
        CoulombCnt = 0.0;
    }

    CoulombCnt *= (float)time_ms * 1000.0 / (float)rsense;  // milli-Amp*s  
    return CoulombCnt;
}


float PAC1xxx_CLimitRegisterToCurrent_mA(uint8_t limitRegister, uint16_t VsenseMAX, uint32_t rsense){
    int8_t signedReg;
    int32_t tempProd;
    float limitReal;

    if(rsense == 0) return 0.0;
    
    signedReg = (int8_t)limitRegister; // limit register is always signed
    tempProd = (int32_t)signedReg * (int32_t)VsenseMAX;
    limitReal = (float)tempProd * (1000000.0 / 128.0) / (float)rsense;  //mA
    return limitReal;
}


float PAC1xxx_PLimitRegisterToPower_mW(uint16_t limitRegister, uint16_t VPowerMAX, uint32_t rsense){
    int16_t signedReg;
    float PowerUnit;
    float limitReal;

    if(rsense == 0) return 0.0;
   
    signedReg = (int16_t)limitRegister; // limit register is always signed
    PowerUnit = (float)VPowerMAX * (1000000.0 / 32768.0) / (float)rsense;  // milli-Watts/bit
    limitReal = (float)signedReg * PowerUnit;
    return limitReal;
}


float PAC1xxx_VLimitRegisterToVoltage_mV(uint8_t limitRegister, uint16_t VbusMAX){
    int8_t signedReg;
    int32_t tempProd;
    float limitReal;

    signedReg = (int8_t)limitRegister; // limit register is always signed
    tempProd = (int32_t)signedReg * (int32_t)VbusMAX;
    limitReal = (float)tempProd / 128.0;    //mV    
    return limitReal;
}


uint16_t PAC1xxx_Plimit_mWtoRegisterVal(float PowerLimit, uint16_t VPowerMAX, uint32_t rsense){
    uint16_t limitRegister;
    float maxLimit;
    float flimit;
    int16_t ilimit;
    
    if( (rsense == 0) || (VPowerMAX == 0) ) return 0;
    
    //compute the device maximum supported power limit = VPowerFSR * VrailToVbusRatio / Rsense
    maxLimit = (float)VPowerMAX * 1000000;
    maxLimit /= (float)rsense;

    if(PowerLimit >= maxLimit){
        //if the requested limit value is above the device +FSR,
        //set the limit register to the max positive value
        limitRegister = 0x7fff; //32767
    }else if(PowerLimit < (-maxLimit)){
        //if the requested limit value is below the device -FSR,
        //set the limit register to the min negative value        
        limitRegister = 0x8000; //-32768
    }else{
        //compute the limit register = value * 2^15 * Rsense / (VsenseFSR * VrailToVbusRatio)
        flimit = PowerLimit * 32768.0 / maxLimit;
        ilimit = (int16_t)flimit;
        limitRegister = (uint16_t)ilimit;
    }
    return limitRegister;
}


uint8_t PAC1xxx_Climit_mAtoRegisterVal(float CurrentLimit, uint16_t VsenseMAX, uint32_t rsense){
    uint8_t limitRegister;
    float maxLimit;
    float flimit;
    int8_t ilimit;

    if( (rsense == 0) || (VsenseMAX == 0) ) return 0;
    
    //compute the device maximum supported current limit = VsenseFSR / Rsense
    maxLimit = (float)VsenseMAX * 1000000;
    maxLimit /= (float)rsense;

    if(CurrentLimit >= maxLimit){
        //if the requested limit value is above the device +FSR,
        //set the limit register to the max positive value
        limitRegister = 0x7f; //127
    }else if(CurrentLimit < (-maxLimit)){
        //if the requested limit value is below the device -FSR,
        //set the limit register to the min negative value    
        limitRegister = 0x80; //-128
    }else{
        //compute the limit register = value * 2^7 * Rsense / VsenseFSR
        flimit = CurrentLimit * 128.0 / maxLimit;
        ilimit = (int8_t)flimit;
        limitRegister = (uint8_t)ilimit;
    } 
    return limitRegister;
}


uint8_t PAC1xxx_Vlimit_mVtoRegisterVal(float VoltageLimit, uint16_t VbusMAX){
    uint8_t limitRegister;
    float maxLimit;
    float flimit;
    int8_t ilimit;

    if(VbusMAX == 0) return 0;
    
    //compute the maximum supported voltage limit = VbusFSR * VrailToVbusRatio
    maxLimit = (float)VbusMAX;

    if(VoltageLimit >= maxLimit){
        //if the requested limit value is above the device +FSR,
        //set the limit register to the max positive value
        limitRegister = 0x7f; //127
    }else if(VoltageLimit < (-maxLimit)){
        //if the requested limit value is below the device -FSR,
        //set the limit register to the min negative value        
        limitRegister = 0x80; //-128
    }else{
        //compute the limit register = value * 2^7 / (VbusFSR * VrailToVbusRatio)  
        flimit = VoltageLimit * 128.0 / maxLimit;
        ilimit = (int8_t)flimit;
        limitRegister = (uint8_t)ilimit;
    }
    return limitRegister;
}