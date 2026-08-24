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
    - Symbol prefix renamed PAC1711_ -> PAC1xxx_
    - Removed the dead PIC32/Harmony handle, open and transfer-setup machinery
    - Added a caller-supplied `void *i2cContext` that is threaded through the
      transfer function pointers, so a transport back end can resolve which
      bus/instance a call belongs to. This is what allows several PAC1xxx
      devices to coexist on different TwoWire buses in one sketch.
    - Transfers are treated as synchronous and blocking (the Arduino Wire API
      is), so the transfer-status query always reports COMPLETE and the event
      callback registration always declines, which makes the HAL fall back to
      polling.
  ---------------------------------------------------------------------------
*/

#ifndef PAC1xxx_I2C_H
#define PAC1xxx_I2C_H

/** @file PAC1xxx_i2c.h
 * Transport adaptation layer for the PAC1xxx library.
 *
 * The library core never talks to a peripheral directly. It calls through the
 * two function pointers held in @ref PAC1xxx_I2C_CONTEXT, which the Arduino
 * wrapper (PAC1xxx.cpp) fills in with static trampolines that forward to a
 * TwoWire instance. Replace those two functions to port the library to another
 * transport with no changes to the core.
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/** I2C transfer event identifiers reported by the transport layer. */
typedef enum {
    /** Transfer request is pending. */
    PAC1xxx_I2C_TRANSFER_EVENT_PENDING,
    /** All data from or to the buffer was transferred successfully. */
    PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE,
    /** The transfer handle expired: the transfer completed, but whether it
     *  completed with an error is not known. */
    PAC1xxx_I2C_TRANSFER_EVENT_HANDLE_EXPIRED,
    /** An error occurred while processing the buffer transfer request. */
    PAC1xxx_I2C_TRANSFER_EVENT_ERROR,
    PAC1xxx_I2C_TRANSFER_EVENT_HANDLE_INVALID,
    /* additional event codes */
    PAC1xxx_I2C_TRANSFER_NO_EVENT = 100
} PAC1xxx_I2C_TRANSFER_EVENT, *PAC1xxx_I2C_TRANSFER_EVENT_P;

/**
 * @brief I2C event callback function type.
 *
 * Implemented by the PAC1xxx library core and registered with the transport
 * layer by PAC1xxx_I2C_TransferEventHandlerSet(). On Arduino the Wire API is
 * blocking, so no callback is ever registered and the core polls instead.
 * @param event   [in] - transfer event
 * @param context [in] - opaque pointer supplied at registration time
 */
typedef void (*PAC1xxx_I2C_TRANSFER_EVENT_HANDLER_P)(PAC1xxx_I2C_TRANSFER_EVENT event,
                                                     uintptr_t context);

/**
 * @brief Pointer type to the transport "write then read" function.
 *
 * Performs an I2C write (typically the register address) followed by a
 * repeated-start read.
 * @param ctx        [in]  - opaque back-end context (PAC1xxx_I2C_CONTEXT::i2cContext)
 * @param address    [in]  - 7-bit device address
 * @param writeBuf   [in]  - bytes to write
 * @param writeSize  [in]  - number of bytes to write
 * @param readBuf    [out] - buffer receiving the read bytes
 * @param readSize   [in]  - number of bytes to read
 * @return true if the transfer completed successfully.
 */
typedef bool (*PAC1xxx_I2C_WriteRead_P)(void *ctx,
                                        uint8_t address,
                                        uint8_t *writeBuf,
                                        size_t writeSize,
                                        uint8_t *readBuf,
                                        size_t readSize);

/**
 * @brief Pointer type to the transport "write" function.
 *
 * @param ctx       [in] - opaque back-end context (PAC1xxx_I2C_CONTEXT::i2cContext)
 * @param address   [in] - 7-bit device address
 * @param writeBuf  [in] - bytes to write
 * @param writeSize [in] - number of bytes to write
 * @return true if the transfer completed successfully.
 */
typedef bool (*PAC1xxx_I2C_Write_P)(void *ctx,
                                    uint8_t address,
                                    uint8_t *writeBuf,
                                    size_t writeSize);

/** @struct _PAC1xxx_I2C_INIT
 * @brief Initialization parameters for PAC1xxx_I2C_Initialize().
 */
typedef struct _PAC1xxx_I2C_INIT {
    uint16_t                 i2cAddress;   /**< 7-bit device address. */
    void                    *i2cContext;   /**< Opaque back-end context handed back to
                                                the transfer functions. May be NULL. */
    PAC1xxx_I2C_WriteRead_P  i2cWriteRead; /**< Write-then-read function. Required. */
    PAC1xxx_I2C_Write_P      i2cWrite;     /**< Write function. Required. */
} PAC1xxx_I2C_INIT, *PAC1xxx_I2C_INIT_P;

/** @struct _PAC1xxx_I2C_CONTEXT
 * @brief Per-device transport state, held inside PAC1xxx_DEVICE_CONTEXT.
 */
typedef struct _PAC1xxx_I2C_CONTEXT {
    uint16_t                             i2cAddress;                 /**< 7-bit device address. */
    void                                *i2cContext;                 /**< Opaque back-end context. */
    PAC1xxx_I2C_TRANSFER_EVENT_HANDLER_P pi2cEventHandler;           /**< Library event callback. */
    uintptr_t                            pi2cEventHandlerContext;    /**< Context returned to the callback. */
    bool                                 i2cEventCallbackRegistered; /**< True if a callback is in use. */
    PAC1xxx_I2C_WriteRead_P              i2cWriteRead;               /**< Write-then-read function. */
    PAC1xxx_I2C_Write_P                  i2cWrite;                   /**< Write function. */
} PAC1xxx_I2C_CONTEXT, *PAC1xxx_I2C_CONTEXT_P;

/**
 * @brief Set up the transport for one PAC1xxx device.
 *
 * Called by PAC1xxx_Device_Initialize().
 * @param pi2c_context [out] - transport context to populate
 * @param i2c_init     [in]  - initialization values
 * @return false if pi2c_context is NULL or either transfer function is NULL.
 */
static inline bool PAC1xxx_I2C_Initialize(PAC1xxx_I2C_CONTEXT_P pi2c_context,
                                          PAC1xxx_I2C_INIT i2c_init)
{
    if ((pi2c_context == NULL) ||
        (i2c_init.i2cWriteRead == NULL) ||
        (i2c_init.i2cWrite == NULL)) {
        return false;
    }

    pi2c_context->i2cAddress                 = i2c_init.i2cAddress;
    pi2c_context->i2cContext                 = i2c_init.i2cContext;
    pi2c_context->i2cWriteRead               = i2c_init.i2cWriteRead;
    pi2c_context->i2cWrite                   = i2c_init.i2cWrite;
    pi2c_context->pi2cEventHandler           = NULL;
    pi2c_context->pi2cEventHandlerContext    = 0;
    pi2c_context->i2cEventCallbackRegistered = false;

    return true;
}

/**
 * @brief Report the status of the last transfer.
 *
 * Transfers on Arduino are blocking and their result is returned directly by
 * PAC1xxx_I2C_WriteRead() / PAC1xxx_I2C_Write(), so by the time the core asks,
 * the transfer is always finished.
 * @param i2c_context [in] - transport context
 * @return always PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE.
 */
static inline PAC1xxx_I2C_TRANSFER_EVENT
PAC1xxx_I2C_TransferStatusGet(PAC1xxx_I2C_CONTEXT i2c_context)
{
    (void)i2c_context;
    return PAC1xxx_I2C_TRANSFER_EVENT_COMPLETE;
}

/**
 * @brief Write then read, with a repeated start in between.
 *
 * @param pi2c_context [in,out] - transport context
 * @param writeBuffer  [in]  - bytes to write (typically the register address)
 * @param writeSize    [in]  - number of bytes to write
 * @param readBuffer   [out] - buffer receiving the read bytes
 * @param readSize     [in]  - number of bytes to read
 * @return true if the transfer succeeded.
 */
static inline bool PAC1xxx_I2C_WriteRead(PAC1xxx_I2C_CONTEXT_P pi2c_context,
                                         void *const writeBuffer,
                                         const size_t writeSize,
                                         void *const readBuffer,
                                         const size_t readSize)
{
    if ((pi2c_context == NULL) || (pi2c_context->i2cWriteRead == NULL)) {
        return false;
    }

    return pi2c_context->i2cWriteRead(pi2c_context->i2cContext,
                                      (uint8_t)pi2c_context->i2cAddress,
                                      (uint8_t *)writeBuffer, writeSize,
                                      (uint8_t *)readBuffer, readSize);
}

/**
 * @brief Write bytes to the device.
 *
 * @param pi2c_context [in,out] - transport context
 * @param writeBuffer  [in] - bytes to write
 * @param writeSize    [in] - number of bytes to write
 * @return true if the transfer succeeded.
 */
static inline bool PAC1xxx_I2C_Write(PAC1xxx_I2C_CONTEXT_P pi2c_context,
                                     void *const writeBuffer,
                                     const size_t writeSize)
{
    if ((pi2c_context == NULL) || (pi2c_context->i2cWrite == NULL)) {
        return false;
    }

    return pi2c_context->i2cWrite(pi2c_context->i2cContext,
                                  (uint8_t)pi2c_context->i2cAddress,
                                  (uint8_t *)writeBuffer, writeSize);
}

/**
 * @brief Register the library's transfer event callback.
 *
 * Always declines on Arduino: Wire is blocking, so there is nothing to call
 * back from. Returning false makes the library core poll the transfer status
 * instead, which is the correct behaviour here.
 * @return always false.
 */
static inline bool PAC1xxx_I2C_TransferEventHandlerSet(
    const PAC1xxx_I2C_CONTEXT_P pi2c_context,
    const PAC1xxx_I2C_TRANSFER_EVENT_HANDLER_P eventHandler,
    const uintptr_t context)
{
    (void)pi2c_context;
    (void)eventHandler;
    (void)context;
    return false;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PAC1xxx_I2C_H */

/**
  End of File
*/
