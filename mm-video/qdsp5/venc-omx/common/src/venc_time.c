/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/src/venc_time.c#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-07-14   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venc_time.h"
#include <sys/time.h>
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
long long venc_time_microsec()
{
   struct timeval time;
   long long time_microsec = 0;
   
   if (gettimeofday(&time, NULL) == 0)
   {
      time_microsec = ((long long) time.tv_sec * 1000 * 1000) + ((long long) time.tv_usec) ;
   }
   
   return time_microsec;
}
