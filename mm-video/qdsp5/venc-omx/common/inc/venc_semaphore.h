#ifndef _VENC_SEMAPHORE_H
#define _VENC_SEMAPHORE_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/inc/venc_semaphore.h#1 $

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
int venc_semaphore_create(void** handle, int init_count, int max_count);

/**
 * @brief Destructor
 */
int venc_semaphore_destroy(void* handle);

/**
 * @brief Waits for the semaphore
 */
int venc_semaphore_wait(void* handle, int timeout);

/**
 * @brief Posts the semaphore
 */
int venc_semaphore_post(void* handle);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _VENC_SEMAPHORE_H
