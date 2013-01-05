#ifndef _VENC_SIGNAL_H
#define _VENC_SIGNAL_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/inc/venc_signal.h#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-07-14   bc     Initial revision

 ========================================================================== */
#ifdef __cplusplus
extern "C" {
#endif
/*========================================================================

                     INCLUDE FILES FOR MODULE

==========================================================================*/

/**
* @brief Constructor
*/
int venc_signal_create(void** handle);

/**
* @brief Destructor
*/
int venc_signal_destroy(void* handle);

/**
* @brief Set a signal
*/
int venc_signal_set(void* handle);

/**
* @brief Wait for signal to be set
*
* @param timeout Milliseconds before timeout. Specify 0 for infinite.
*
* @return 0 on success, 1 on error, 2 on timeout
*/
int venc_signal_wait(void* handle, int timeout);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _VENC_SIGNAL_H
