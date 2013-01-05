#ifndef _VENC_THREAD_H
#define _VENC_THREAD_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/inc/venc_thread.h#1 $

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
 * @brief Thread start function pointer
 */
typedef int (*thread_fn_type) (void* thread_data);

/**
 * @brief Starts a thread at the specified function
 *
 * @param handle Handle to the thread
 * @param pfn Pointer to the thread start function
 * @param thread_data Arguments passed in to the thread (null is valid)
 * @param priority Priority of the thread, unique to each platform.
 */
int venc_thread_create(void** handle,
                       thread_fn_type pfn,
                       void* thread_data,
                       int priority);

/**
 * @brief Destroys a thread. Blocks until thread exits
 *
 * @return The thread exit code
 */
int venc_thread_destroy(void* handle, int* thread_result);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _VENC_THREAD_H
