/* SIE CONFIDENTIAL
 * HeatWave 1.44
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All Rights Reserved.
 */

/**
* @file heatwave_api_errors.h
*
* @brief Error codes used by HeatWave API functions.
*
* @history
*   1.1 Add SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT
*       Explicitly set error codes as int32_t values
*/
#ifndef _SCE_HEATWAVE_API_ERRORS_H_
#define _SCE_HEATWAVE_API_ERRORS_H_

// ******************************************************************************************
/**
 * @name Error Codes
 *
 * @{
 */

#define SCE_HEATWAVE_API_OK                         0           ///< The operation was successful.
#define SCE_HEATWAVE_API_ERROR                      -2147483647 ///< (0x80000001) The operation was not successful.
#define SCE_HEATWAVE_API_ERROR_OUT_OF_ASSETS        -2147483646 ///< (0x80000002) The operation failed because there was insufficient memory available.
#define SCE_HEATWAVE_API_ERROR_PARAM                -2147483645 ///< (0x80000003) The operation failed because a parameter was invalid or missing.
#define SCE_HEATWAVE_API_ERROR_HANDLE               -2147483644 ///< (0x80000004) The operation failed because the supplied handle was not recognized as a HeatWave handle.
#define SCE_HEATWAVE_API_ERROR_FILE                 -2147483643 ///< (0x80000005) The operation failed because a file was missing.
#define SCE_HEATWAVE_API_ERROR_NO_HARDWARE_CONTEXT  -2147483642 ///< (0x80000006) The operation failed because there is no hardware context.
#define SCE_HEATWAVE_API_ERROR_INVALID_OPERATION    -2147483641 ///< (0x80000007) The operation failed because it is not supported.
#define SCE_HEATWAVE_API_ERROR_NO_OUTPUT_DEVICE     -2147483640 ///< (0x80000008) The operation failed because there is no audio output device.
#define SCE_HEATWAVE_API_ERROR_UNSUPPORTED_FORMAT   -2147483639 ///< (0x80000009) The operation failed because a file had an invalid format.

/** @} */ /* Error Codes */

// ******************************************************************************************
/**
 * @name Macros
 *
 * @{
 */

/**
* @brief    Checks if an operation was successful.
*
* @param    res     The return code to check, which should be an <c>SceHwError</c>.
*
* @return   A value of true if <c>res</c> is <c>SCE_HEATWAVE_API_OK</c>; otherwise false is returned.
*/
#define SCE_HEATWAVE_API_SUCCEEDED(res)     (res == SCE_HEATWAVE_API_OK)

/**
* @brief    Checks if an operation was <b>not</b> successful.
*
* @param    res     The return code to check, which should be an <c>SceHwError</c>.
*
* @return   A value of true if <c>res</c> is an error code; otherwise false is returned.
*/
#define SCE_HEATWAVE_API_FAILED(res)        (res != SCE_HEATWAVE_API_OK)

/** @} */ /* Macros */

#endif /* _SCE_HEATWAVE_API_ERRORS_H_ */
