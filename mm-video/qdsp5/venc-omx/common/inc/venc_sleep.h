#ifndef _VENC_SLEEPER_H
#define _VENC_SLEEPER_H
/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/inc/venc_sleep.h#1 $

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
 * @brief Sleep for the specified time
 *
 * @param time_millis The time to sleep in milliseconds
 */
int venc_sleep(int time_millis);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _VENC_SLEEPER_H
