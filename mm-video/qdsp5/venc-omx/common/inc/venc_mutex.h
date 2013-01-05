#ifndef _VENC_MUTEX_H
#define _VENC_MUTEX_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/inc/venc_mutex.h#1 $

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
int venc_mutex_create(void** handle);

/**
 * @brief Destructor
 */
int venc_mutex_destroy(void* handle);

/**
 * @brief Locks the mutex
 */
int venc_mutex_lock(void* handle);

/**
 * @brief Unlocks the mutex
 */
int venc_mutex_unlock(void* handle);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _VENC_MUTEX_H
