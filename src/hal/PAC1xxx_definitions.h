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

#ifndef PAC1xxx_TYPES_H
#define PAC1xxx_TYPES_H

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

/*
    Section: Included Files
*/
#include <stdint.h>
   
/*
 * PAC1xxx device IDs
 */
#define PAC1xxx_MANUFACTURER_ID   0x54

#define PAC1711PDN_PRODUCT_ID          0x88
#define PAC1711_PRODUCT_ID             0x80
#define PAC1721PDN_PRODUCT_ID          0x89
#define PAC1721_PRODUCT_ID             0x81
#define PAC1761_PRODUCT_ID             0x82
#define PAC1811PDN_PRODUCT_ID          0x8C
#define PAC1811_PRODUCT_ID             0x84
#define PAC1821PDN_PRODUCT_ID          0x8D
#define PAC1821_PRODUCT_ID             0x85
#define PAC1861_PRODUCT_ID             0x86
#define PAC1711TEST_PRODUCT_ID         0x83

/*
 * PAC1xxx device attributes
 */
/* ADC resolution */
#define PAC17xx_ADC_RESOLUTION_12BIT      12
#define PAC18xx_ADC_RESOLUTION_16BIT      16

#define PAC1xxx_CH_COUNT      1
    
/* VSENSE full-scale MAX value - milliVolts */
#define PAC1xxx_VSENSE_MAX_mV     100
    
/* VBUS full-scale MAX value - milliVolts */
#define PAC1711_VBUS_MAX_mV     42000
#define PAC1811_VBUS_MAX_mV     42000
#define PAC1721_VBUS_MAX_mV      9000
#define PAC1821_VBUS_MAX_mV      9000
#define PAC1761_VBUS_MAX_mV     65000
#define PAC1861_VBUS_MAX_mV     65000

/* VPOWER full-scale power product MAX value - milli-Volt^2 */
#define PAC1711_VPOWER_MAX_mV2      4200
#define PAC1811_VPOWER_MAX_mV2      4200
#define PAC1721_VPOWER_MAX_mV2       900
#define PAC1821_VPOWER_MAX_mV2       900
#define PAC1761_VPOWER_MAX_mV2      6500
#define PAC1861_VPOWER_MAX_mV2      6500

/* Limit registers border values (no Alert is generated) */
#define PAC1xxx_OVER_CURRENT_BORDER     0x7F
#define PAC1xxx_UNDER_CURRENT_BORDER    0x80
#define PAC1xxx_OVER_VOLTAGE_BORDER     0x7F
#define PAC1xxx_UNDER_VOLTAGE_BORDER    0x80
#define PAC1xxx_OVER_POWER_BORDER     0x7FFF
    
/*
 * PAC1xxx register address
 */
#define PAC1xxx_REFRESH_CMD_ADDR            0x00
#define PAC1xxx_CONTROL_ADDR                0x01
#define PAC1xxx_ACC_COUNT_ADDR              0x02
#define PAC1xxx_VACC_ADDR                   0x03
#define PAC1xxx_VBUS_ADDR                   0x04
#define PAC1xxx_VSENSE_ADDR                 0x05
#define PAC1xxx_VBUS_AVG_ADDR               0x06
#define PAC1xxx_VSENSE_AVG_ADDR             0x07
#define PAC1xxx_VPOWER_ADDR                 0x08
#define PAC1xxx_VBUS_MIN_ADDR               0x09
#define PAC1xxx_VBUS_MAX_ADDR               0x0A
#define PAC1xxx_VSENSE_MIN_ADDR             0x0B
#define PAC1xxx_VSENSE_MAX_ADDR             0x0C
#define PAC1xxx_VPOWER_MIN_ADDR             0x0D
#define PAC1xxx_VPOWER_MAX_ADDR             0x0E
#define PAC1xxx_CONTROL_LAT_ADDR            0x0F    
#define PAC1xxx_NEG_PWR_FSR_LAT_ADDR        0x10
#define PAC1xxx_ALERT_STATUS_ADDR           0x11    
#define PAC1xxx_SMBUS_SETTINGS_ADDR         0x12
#define PAC1xxx_NEG_PWR_FSR_ADDR            0x13
#define PAC1xxx_REFRESH_G_CMD_ADDR          0x14
#define PAC1xxx_REFRESH_V_CMD_ADDR          0x15
#define PAC1xxx_SLOW_ADDR                   0x16
#define PAC1xxx_CONTROL_ACT_ADDR            0x17
#define PAC1xxx_NEG_PWR_FSR_ACT_ADDR        0x18
#define PAC1xxx_SLOW_ALERT0_ADDR            0x19
#define PAC1xxx_GPIO_ALERT1_ADDR            0x1A
#define PAC1xxx_ACC_FULLNESS_LIMITS_ADDR    0x1B    
#define PAC1xxx_OC_LIMIT_ADDR               0x1C
#define PAC1xxx_UC_LIMIT_ADDR               0x1D
#define PAC1xxx_OPW_LIMIT_ADDR              0x1E
#define PAC1xxx_OPC_LIMIT_ADDR              0x1F
#define PAC1xxx_OV_LIMIT_ADDR               0x20
#define PAC1xxx_UV_LIMIT_ADDR               0x21
#define PAC1xxx_STEP_LIMIT_ADDR             0x22
#define PAC1xxx_LIMIT_NSAMPLES_ADDR         0x23
#define PAC1xxx_ALERT_ENABLE_ADDR           0x24
#define PAC1xxx_ACC_COUNT_PRESET_ADDR       0x25
#define PAC1xxx_ACC_PRESET_ADDR             0x26

#define PAC1xxx_PRODUCT_ID_ADDR             0xFD
#define PAC1xxx_MANUFACTURER_ID_ADDR        0xFE
#define PAC1xxx_REVISION_ID_ADDR            0xFF
    
/*
 * DEVICE REGISTER SIZES (bytes)
 */
#define PAC1xxx_ACC_COUNT_SZ       4
#define PAC1xxx_VACC_SZ            7
#define PAC1xxx_VPOWER_SZ          4
#define PAC1xxx_LIMIT_SZ           1
#define PAC1xxx_OTHERLIMIT_SZ      2
#define PAC1xxx_IDREG_SZ           1
#define PAC1xxx_CONTROL_SZ         2
#define PAC1xxx_VBUS_VSENSE_SZ     2
#define PAC1xxx_SMBUS_SZ           1
#define PAC1xxx_NEGPWRFSR_SZ       1
#define PAC1xxx_SLOW_SZ            1
#define PAC1xxx_ACC_PRESET_SZ      2
#define PAC1xxx_ALERT_SZ           2
#define PAC1xxx_PRODUCT_ID_SZ      1
#define PAC1xxx_MANUFACTURER_ID_SZ 1
#define PAC1xxx_REVISION_ID_SZ     1
#define PAC1xxx_ID_REGS_SZ         (PAC1xxx_PRODUCT_ID_SZ + PAC1xxx_MANUFACTURER_ID_SZ + PAC1xxx_REVISION_ID_SZ)

/** The size increase to be included for the response to the SMBus Block Read protocol. */
#define SMBUS_BYTECNT_SZ   1

/*
 * DEVICE REGISTER WIDTH (bits)
 */
#define PAC1xxx_VPOWER_WIDTH       30
#define PAC1xxx_VBUS_WIDTH         12
#define PAC1xxx_VSENSE_WIDTH       12
#define PAC1xxx_VLIMIT_WIDTH       16
#define PAC1xxx_PLIMIT_WIDTH       24

/* 
 * PAC1xxx data structures
 */

/** @struct _PAC1xxx_deviceID
 * @brief PAC1xxx_deviceID struct type holds the device ID register values. */
typedef struct _PAC1xxx_deviceID {
    uint8_t product;        /**< PRODUCT_ID register value */
    uint8_t manufacturer;   /**< MANUFACTURER_ID register value */
    uint8_t revision;       /**< REVISION_ID register value */
 } PAC1xxx_deviceID, *PAC1xxx_deviceID_P;

 
/** @struct _PAC1xxx_CONTROL_REGFIELDS
 * @brief PAC1xxx_CONTROL_REGFIELDS struct type holds the bit-field values from 
 * CONTROL, CONTROL_ACT or CONTROL_LAT registers. */
typedef struct _PAC1xxx_CONTROL_REGFIELDS{  
    /* register MSB */
    uint8_t SAMPLE_MODE  : 4;
    uint8_t GPIO_ALERT1  : 2;
    uint8_t SLOW_ALERT0  : 2;
    /* register LSB */
    uint8_t AVERAGE      : 3;
    uint8_t AA           : 1;
    uint8_t ACC_CONFIG   : 2;
    uint8_t AUTO_REFRESH : 2;
} PAC1xxx_CONTROL_REGFIELDS, *PAC1xxx_CONTROL_REGFIELDS_P;
/* CONTROL register bit-field positions */
#define PAC1xxx_CONTROL_SAMPLE_MODE_BITPOSMSB    4    /**< SAMPLE_MODE field position in the CONTROL register MSB */
#define PAC1xxx_CONTROL_GPIO_ALERT1_BITPOSMSB    2    /**< GPIO_ALERT1 field position in the CONTROL register MSB */
#define PAC1xxx_CONTROL_SLOW_ALERT0_BITPOSMSB    0    /**< SLOW_ALERT0 field position in the CONTROL register MSB */
#define PAC1xxx_CONTROL_AVERAGE_BITPOSLSB        5    /**< AVERAGE field position in the CONTROL register LSB */
#define PAC1xxx_CONTROL_AA_BITPOSLSB             4    /**< AA field position in the CONTROL register LSB */
#define PAC1xxx_CONTROL_ACC_CONFIG_BITPOSLSB     2    /**< ACC_CONFIG field position in the CONTROL register LSB */
#define PAC1xxx_CONTROL_AUTO_REFRESH_BITPOSLSB   0    /**< AUTO_REFRESH field position in the CONTROL register LSB */
/* CONTROL register bit-field masks */
#define PAC1xxx_CONTROL_SAMPLE_MODE_BITMASK   0x0F
#define PAC1xxx_CONTROL_GPIO_ALERT_BITMASK    0x03
#define PAC1xxx_CONTROL_AVERAGE_BITMASK       0x07
#define PAC1xxx_CONTROL_AA_BITMASK            0x01
#define PAC1xxx_CONTROL_ACC_CONFIG_BITMASK    0x03
#define PAC1xxx_CONTROL_AUTO_REFRESH_BITMASK  0x03
/* CONTROL ACC_CONFIG bit-field values */
#define PAC1xxx_CONTROL_ACC_CONFIG_VPOWER       0b00   /**< CONTROL ACC_CONFIG register field value for VPOWER accumulation */
#define PAC1xxx_CONTROL_ACC_CONFIG_VSENSE       0b01   /**< CONTROL ACC_CONFIG register field value for VSENSE accumulation */
#define PAC1xxx_CONTROL_ACC_CONFIG_VBUS         0b10   /**< CONTROL ACC_CONFIG register field value for VBUS accumulation */
#define PAC1xxx_CONTROL_ACC_CONFIG_RESERVED     0b11   /**< CONTROL ACC_CONFIG register field RESERVED value */


/** @struct _PAC1xxx_SMBUS_SETTINGS_REGFIELDS
 * @brief PAC1xxx_SMBUS_SETTINGS_REGFIELDS struct type holds register bit-field values from 
 * the SMBUS SETTINGS register. */
typedef struct _PAC1xxx_SMBUS_SETTINGS_REGFIELDS {
    uint8_t GPIO_DATA1  : 1;
    uint8_t GPIO_DATA0  : 1;
    uint8_t ANY_ALERT   : 1;
    uint8_t POR         : 1;
    uint8_t TIMEOUT     : 1;
    uint8_t BYTE_COUNT  : 1;
    uint8_t             : 1;                        /**< Reserved bit-field */
    uint8_t I2C_HISPEED : 1;
} PAC1xxx_SMBUS_SETTINGS_REGFIELDS, *PAC1xxx_SMBUS_SETTINGS_REGFIELDS_P;
/* SMBUS SETTINGS register bit-field positions */
#define PAC1xxx_SMBUS_GPIODATA1_BITPOS    7
#define PAC1xxx_SMBUS_GPIODATA0_BITPOS    6
#define PAC1xxx_SMBUS_ANYALERT_BITPOS     5
#define PAC1xxx_SMBUS_POR_BITPOS          4
#define PAC1xxx_SMBUS_TIMEOUT_BITPOS      3
#define PAC1xxx_SMBUS_BYTECOUNT_BITPOS    2
#define PAC1xxx_SMBUS_I2CSPEED_BITPOS     0
/** SMBUS SETTINGS register bit-field mask. 
 * All bit-fields have identical width. */
#define PAC1xxx_SMBUS_BITMASK             0x01


/** @struct _PAC1xxx_NEGPWRFSR_REGFIELDS
 * @brief _PAC1xxx_NEGPWRFSR_REGFIELDS struct type holds register bit-field values from 
 * the NEG_PWR_FSR, NEG_PWR_FSR_ACT or NEG_PWR_FSR_LAT registers. */
typedef struct _PAC1xxx_NEGPWRFSR_REGFIELDS {
    uint8_t CFG_VS : 2;
    uint8_t CFG_VB : 2;  
} PAC1xxx_NEGPWRFSR_REGFIELDS, *PAC1xxx_NEGPWRFSR_REGFIELDS_P;
/* NEG_PWR_FSR register bit-field positions */
#define PAC1xxx_NEGPWRFSR_VS1_BITPOS    2      
#define PAC1xxx_NEGPWRFSR_VB1_BITPOS    0
/** NEG_PWR_FSR register bit-field mask.
 * All bit-fields have identical width. */
#define PAC1xxx_NEGPWRFSR_BITMASK       0x03
/* NEG_PWR_FSR bit-field values */
#define PAC1xxx_NEGPWRFSR_MODE_UNIPOLAR 0b00        /**< NEG_PWR_FSR register field value for unipolar configuration */
#define PAC1xxx_NEGPWRFSR_MODE_BIPOLAR  0b01        /**< NEG_PWR_FSR register field value for bipolar configuration */
#define PAC1xxx_NEGPWRFSR_MODE_HALFFSR  0b10        /**< NEG_PWR_FSR register field value for bipolar half range configuration */
#define PAC1xxx_NEGPWRFSR_MODE_RESERVED 0b11        /**< NEG_PWR_FSR register field RESERVED value */


/** @struct _PAC1xxx_SLOW_REGFIELDS
 * @brief PAC1xxx_SLOW_REGFIELDS struct type holds register bit-field values from 
 * the SLOW register. */
typedef struct _PAC1xxx_SLOW_REGFIELDS {
    uint8_t Slow          : 1;
    uint8_t SlowLowHigh   : 1;
    uint8_t SlowHighLow   : 1;
    uint8_t RefreshRise   : 1;
    uint8_t RefreshVRise  : 1;
    uint8_t RefreshFall   : 1;
    uint8_t RefreshVFall  : 1;
    uint8_t               : 1;                      /**< Reserved bit-field */
} PAC1xxx_SLOW_REGFIELDS, *PAC1xxx_SLOW_REGFIELDS_P;
/* SLOW register bit-field positions */
#define PAC1xxx_SLOW_SLOW_BITPOS      7
#define PAC1xxx_SLOW_LH_BITPOS        6
#define PAC1xxx_SLOW_HL_BITPOS        5
#define PAC1xxx_SLOW_RRISE_BITPOS     4
#define PAC1xxx_SLOW_RVRISE_BITPOS    3
#define PAC1xxx_SLOW_RFALL_BITPOS     2
#define PAC1xxx_SLOW_RVFALL_BITPOS    1
/** SLOW register bit-field mask.
 * All bit-fields have identical width. */
#define PAC1xxx_SLOW_BITMASK          0x01


/** @struct _PAC1xxx_ALERT_STATUS_REGFIELDS
 * @brief PAC1xxx_ALERT_STATUS_REGFIELDS struct type holds register bit-field values from 
 * the ALERT_STATUS register. */
typedef struct _PAC1xxx_ALERT_STATUS_REGFIELDS {
    /* register MSB */
    uint8_t           : 2;                      /**< Reserved bitfield */
    uint8_t RV        : 1;
    uint8_t FV        : 1;
    uint8_t RC        : 1;
    uint8_t FC        : 1;
    uint8_t OC        : 1;
    uint8_t UC        : 1;
    /* register LSB */
    uint8_t OV        : 1;
    uint8_t UV        : 1;
    uint8_t OPC       : 1;
    uint8_t OPW       : 1;
    uint8_t ACC_OVF   : 1;
    uint8_t ACC_COUNT : 1;
    uint8_t           : 1;                      /**< Reserved bitfield */
    uint8_t           : 1;                      /**< Reserved bitfield */
} PAC1xxx_ALERT_STATUS_REGFIELDS, *PAC1xxx_ALERT_STATUS_REGFIELDS_P;


/** @struct _PAC1xxx_ALERT_ENABLE_REGFIELDS
 * @brief PAC1xxx_ALERT_ENABLE_REGFIELDS struct type holds register bit-field values from 
 * the ALERT_ENABLE, SLOW_ALERT0, or GPIO_ALERT1 registers. */
typedef struct _PAC1xxx_ALERT_ENABLE_REGFIELDS {
    /* register MSB */
    uint8_t           : 2;                      /** Reserved bit-field */
    uint8_t RV        : 1;
    uint8_t FV        : 1;
    uint8_t RC        : 1;
    uint8_t FC        : 1;
    uint8_t OC        : 1;
    uint8_t UC        : 1;
    /* register LSB */
    uint8_t OV        : 1;
    uint8_t UV        : 1;
    uint8_t OPC       : 1;
    uint8_t OPW       : 1;
    uint8_t ACC_OVF   : 1;
    uint8_t ACC_COUNT : 1;    
    uint8_t ALERT_CCx : 1;
    uint8_t           : 1;                      /**< Reserved bitfield */
} PAC1xxx_ALERT_ENABLE_REGFIELDS, *PAC1xxx_ALERT_ENABLE_REGFIELDS_P;


/* Bit-field positions for the ALERT_STATUS, ALERT_ENABLE, 
 * SLOW_ALERT0, or GPIO_ALERT1 registers. */
/* Bit-field position in the register MSB */
#define PAC1xxx_ALERT_RV_BITPOSMSB      5
#define PAC1xxx_ALERT_FV_BITPOSMSB      4
#define PAC1xxx_ALERT_RC_BITPOSMSB      3
#define PAC1xxx_ALERT_FC_BITPOSMSB      2
#define PAC1xxx_ALERT_OC_BITPOSMSB      1
#define PAC1xxx_ALERT_UC_BITPOSMSB      0
/* Bit-field position in the register LSB */
#define PAC1xxx_ALERT_OV_BITPOSLSB      7
#define PAC1xxx_ALERT_UV_BITPOSLSB      6
#define PAC1xxx_ALERT_OPC_BITPOSLSB     5
#define PAC1xxx_ALERT_OPW_BITPOSLSB     4
#define PAC1xxx_ALERT_ACCOVF_BITPOSLSB  3
#define PAC1xxx_ALERT_ACCCNT_BITPOSLSB  2
#define PAC1xxx_ALERT_CC_BITPOSLSB      1
/** Bit-field mask for the ALERT_STATUS, ALERT_ENABLE, 
 * SLOW_ALERT0, or GPIO_ALERT1 registers.
 * All bit-fields have identical width. */
#define PAC1xxx_ALERT_BITMASK        0x01


/** @struct _PAC1xxx_ACCUM_LIMITS_REGFIELDS
 * @brief PAC1xxx_ACCUM_LIMITS_REGFIELDS struct type holds register bit-field values from 
 * the ACC_Fullness_limits register. */
typedef struct _PAC1xxx_ACCUM_LIMITS_REGFIELDS {
    uint8_t ACC_FULL       : 6;
    uint8_t ACC_COUNT_FULL : 2;
} PAC1xxx_ACCUM_LIMITS_REGFIELDS, *PAC1xxx_ACCUM_LIMITS_REGFIELDS_P;
/* ACC_Fullness_limits register bit-field positions */
#define PAC1xxx_ACC_FULL_BITPOS             2 
#define PAC1xxx_ACC_COUNT_FULL_BITPOS       0 
/* ACC_Fullness_limits register bit-field masks */
#define PAC1xxx_ACC_FULL_BITMASK            0x3F
#define PAC1xxx_ACC_COUNT_FULL_BITMASK      0x03
/* ACC_Fullness_limits bit-field values */
#define PAC1xxx_ACCLIMITS_COUNT_FULL    0b00    /**< ACC_COUNT fulness limit set to "Full" */
#define PAC1xxx_ACCLIMITS_COUNT_15BY16  0b01    /**< ACC_COUNT fulness limit set to "15/16 Full" */
#define PAC1xxx_ACCLIMITS_COUNT_7BY8    0b10    /**< ACC_COUNT fulness limit set to "7/8 Full" */
#define PAC1xxx_ACCLIMITS_COUNT_3BY4    0b11    /**< ACC_COUNT fulness limit set to "3/4 Full" */


/** @struct _PAC1xxx_STEP_LIMIT_REGFIELDS
 * @brief PAC1xxx_STEP_LIMIT_REGFIELDS struct type holds register bit-field values from 
 * the STEP_limit register. */
typedef struct _PAC1xxx_STEP_LIMIT_REGFIELDS {
    uint8_t STEP_RV  : 2;
    uint8_t STEP_FV  : 2;    
    uint8_t STEP_RC  : 2;
    uint8_t STEP_FC  : 2;
} PAC1xxx_STEP_LIMIT_REGFIELDS, *PAC1xxx_STEP_LIMIT_REGFIELDS_P;
/* STEP_limit register bit-field positions */
#define PAC1xxx_STEPLIMIT_RV_BITPOS     6   /**< RV field position in the STEP_limit register */
#define PAC1xxx_STEPLIMIT_FV_BITPOS     4   /**< FV field position in the STEP_limit register */
#define PAC1xxx_STEPLIMIT_RC_BITPOS     2   /**< RC field position in the STEP_limit register */
#define PAC1xxx_STEPLIMIT_FC_BITPOS     0   /**< FC field position in the STEP_limit register */
/** STEP_limit register bit-fields mask.
 * All bit-fields have identical width. */
#define PAC1xxx_STEP_LIMIT_BITMASK       0x03
/* STEP_limit bit-field values */
#define PAC1xxx_STEP_LIMIT_25    0b00       /**< STEP limit value set to 25% sample value change */
#define PAC1xxx_STEP_LIMIT_50    0b01       /**< STEP limit value set to 50% sample value change */
#define PAC1xxx_STEP_LIMIT_75    0b10       /**< STEP limit value set to 75% sample value change */
#define PAC1xxx_STEP_LIMIT_100   0b11       /**< STEP limit value set to 100% sample value change */


/** @struct _PAC1xxx_LIMIT_NSAMPLES_REGFIELDS
 * @brief PAC1xxx_LIMIT_NSAMPLES_REGFIELDS struct type holds register bit-field values from 
 * the Limit_Nsamples register. */
typedef struct _PAC1xxx_LIMIT_NSAMPLES_REGFIELDS {
    /* register MSB */
    uint8_t              : 2;                   /**< Reserved bitfield */
    uint8_t Nsamples_OPC : 2;
    uint8_t Nsamples_OPW : 2;
    /* register LSB */
    uint8_t Nsamples_OC  : 2;
    uint8_t Nsamples_UC  : 2;    
    uint8_t Nsamples_OV  : 2;
    uint8_t Nsamples_UV  : 2;
} PAC1xxx_LIMIT_NSAMPLES_REGFIELDS, *PAC1xxx_LIMIT_NSAMPLES_REGFIELDS_P;
/* Limit_Nsamples register bit-field positions in the register MSB */
#define PAC1xxx_LIMITNSAMP_OPC_BITPOSMSB    2   /**< Nsamples_OPC field position in the Limit_Nsamples register MSB */
#define PAC1xxx_LIMITNSAMP_OPW_BITPOSMSB    0   /**< Nsamples_OPW field position in the Limit_Nsamples register MSB */
/* Limit_Nsamples register bit-field positions in the register LSB */
#define PAC1xxx_LIMITNSAMP_OC_BITPOSLSB     6   /**< Nsamples_OC field position in the Limit_Nsamples register MSB */
#define PAC1xxx_LIMITNSAMP_UC_BITPOSLSB     4   /**< Nsamples_UC field position in the Limit_Nsamples register MSB */
#define PAC1xxx_LIMITNSAMP_OV_BITPOSLSB     2   /**< Nsamples_OV field position in the Limit_Nsamples register MSB */
#define PAC1xxx_LIMITNSAMP_UV_BITPOSLSB     0   /**< Nsamples_UV field position in the Limit_Nsamples register MSB */
/** Limit_Nsamples register bit-fields mask.
 * All bit-fields have identical width. */
#define PAC1xxx_LIMIT_NSAMPLES_BITMASK       0x03
/* Limit_Nsamples bit-field values */
#define PAC1xxx_LIMIT_NSAMPLES_01    0b00       /**< Nsamples limit value set to 1 sample */
#define PAC1xxx_LIMIT_NSAMPLES_04    0b01       /**< Nsamples limit value set to 4 samples */
#define PAC1xxx_LIMIT_NSAMPLES_08    0b10       /**< Nsamples limit value set to 8 samples */
#define PAC1xxx_LIMIT_NSAMPLES_16    0b11       /**< Nsamples limit value set to 16 samples */

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	// _PAC1xxx_TYPES_H

/**
  End of File
*/
