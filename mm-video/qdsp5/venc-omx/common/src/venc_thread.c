/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/src/venc_thread.c#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-07-14   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include <pthread.h>
#include "venc_debug.h"
#include "venc_thread.h"
#include <stdlib.h>

typedef struct venc_thread_type
{
   thread_fn_type pfn;
   int priority;
   pthread_t thread;
   void* thread_data;
} venc_thread_type;


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static void* venc_thread_entry(void* thread_data)
{
   venc_thread_type* thread = (venc_thread_type*) thread_data;
   return (void*) thread->pfn(thread->thread_data);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_thread_create(void** handle,
                       thread_fn_type pfn,
                       void* thread_data,
                       int priority)
{
   int result = 0;
   venc_thread_type* thread = (venc_thread_type*) malloc(sizeof(venc_thread_type));
   *handle = thread;
   
   if (thread)
   {
      int create_result;
      thread->pfn = pfn;
      thread->thread_data = thread_data;
      thread->priority = priority;
      create_result = pthread_create(&thread->thread,
                                     NULL,
                                     venc_thread_entry,
                                     (void*) thread);
      if (create_result != 0)
      {
         VENC_MSG_ERROR("failed to create thread", 0, 0, 0);
         result = 1;
      }
   }
   else
   {
      VENC_MSG_ERROR("failed to allocate thread", 0, 0, 0);
      result = 1;
   }
   return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_thread_destroy(void* handle, int* thread_result)
{
   int result = 0;
   venc_thread_type* thread = (venc_thread_type*) handle;
   
   if (thread)
   {
      if (pthread_join(thread->thread, (void**) thread_result) != 0)
      {
         VENC_MSG_ERROR("failed to join thread", 0, 0, 0);
         result = 1;
        }     
      free(thread);
   }
   else
   {
      VENC_MSG_ERROR("handle is null", 0, 0, 0);
      result = 1;
   }
   return result;
}
