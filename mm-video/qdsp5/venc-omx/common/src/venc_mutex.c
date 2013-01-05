/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/src/venc_mutex.c#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-07-14   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venc_mutex.h"
#include "venc_debug.h"
#include "venc_debug.h"
#include <pthread.h>
#include <stdlib.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_mutex_create(void** handle)
{
   int result = 0;
   pthread_mutex_t* mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));

   if (handle)
   {
      if (pthread_mutex_init(mutex, NULL) == 0)
      {
         *handle = mutex;
      }
      else
      {
         VENC_MSG_ERROR("init mutex failed", 0, 0, 0);
         free(mutex);
         result = 1;
      }
   }
   else
   {
      VENC_MSG_ERROR("handle is null", 0, 0, 0);
      result = 1;
   }

   return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_mutex_destroy(void* handle)
{
   int result = 0;
   pthread_mutex_t* mutex = (pthread_mutex_t*) handle;

   if (mutex)
   {
      if (pthread_mutex_destroy(mutex) == 0)
      {
         free(handle);
      }
      else
      {
         VENC_MSG_ERROR("destroy mutex failed", 0, 0, 0);
         result = 1;
      }
   }
   else
   {
      VENC_MSG_ERROR("handle is null", 0, 0, 0);
      result = 1;
   }

   return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_mutex_lock(void* handle)
{
   int result = 0;
   pthread_mutex_t* mutex = (pthread_mutex_t*) handle;

   if (mutex)
   {
      if (pthread_mutex_lock(mutex) != 0)
      {
         VENC_MSG_ERROR("lock mutex failed", 0, 0, 0);
      }
   }
   else
   {
      VENC_MSG_ERROR("handle is null", 0, 0, 0);
      result = 1;
   }

   return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_mutex_unlock(void* handle)
{
   int result = 0;
   pthread_mutex_t* mutex = (pthread_mutex_t*) handle;

   if (mutex)
   {
      if (pthread_mutex_unlock(mutex) != 0)
      {
         VENC_MSG_ERROR("lock mutex failed", 0, 0, 0);
      }
   }
   else
   {
      VENC_MSG_ERROR("handle is null", 0, 0, 0);
      result = 1;
   }

   return result;
}
