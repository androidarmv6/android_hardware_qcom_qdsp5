/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/src/venc_semaphore.c#2 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-07-14   as     Initial the sem value to 1 before destroy.
2009-07-14   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venc_semaphore.h"
#include "venc_debug.h"
#include <semaphore.h>
#include <time.h>
#include <errno.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_semaphore_create(void** handle, int init_count, int max_count)
{
   int result = 0;

   (void) max_count;

   if (handle)
   {
      sem_t* sem = (sem_t*) malloc(sizeof(sem_t));
      
      if (sem)
      {
         if (sem_init(sem, 0, 0) == 0)
         {
            *handle = (void*) sem;
         }
         else
         {
            VENC_MSG_ERROR("failed to create semaphore", 0, 0, 0);
            free((void*) sem);
            result = 1;
         }
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
int venc_semaphore_destroy(void* handle)
{
   int result = 0;
   sem_init((sem_t*) handle, 0, 1) ;
   if (handle)
   {
      if (sem_destroy((sem_t*) handle) == 0)
      {
         free(handle);
      }
      else
      {
         VENC_MSG_ERROR("failed to destroy sem %s", strerror(errno), 0, 0);
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
int venc_semaphore_wait(void* handle, int timeout)
{
   int result = 0;

   if (handle)
   {
      if (timeout > 0)
      {
         struct timespec ts;
         if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
         {
            int ret;
            // get the number of seconds in timeout (from millis)
            ts.tv_sec = ts.tv_sec + (time_t) (timeout / 1000);

            // get the number of nanoseconds left over
            ts.tv_nsec = ts.tv_nsec + (long) ((timeout % 1000) * 1000);

            if (sem_timedwait((sem_t*) handle, &ts) != 0)
            {
               if (errno == ETIMEDOUT)
               {
                  result = 2;
               }
               else
               {
                  VENC_MSG_ERROR("error waiting for sem", 0, 0, 0);
                  result = 1;
               }
            }
         }

      }
      else // no timeout
      {
         if (sem_wait((sem_t*) handle) != 0)
         {
            VENC_MSG_ERROR("error waiting for sem", 0, 0, 0);
            result = 1;
         }
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
int venc_semaphore_post(void* handle)
{
   int result = 0;

   if (handle)
   {
      if (sem_post((sem_t*) handle) != 0)
      {
         VENC_MSG_ERROR("Failed to post semaphore", 0, 0, 0);
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
