/*========================================================================

                                   TBD

*//** @file TODO

                  Copyright (c) 2009 QUALCOMM Incorporated.
                           All Rights Reserved.
                     Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                          Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/src/venc_signal.c#3 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2010-05-20   as     Fixed Hang issue for stats thread in pause state.
2010-01-13   as     Fixed Signal miss issue for condition timed out 
2009-09-14   as     Fixed Signal miss issue  
2009-07-14   bc     Initial revision

========================================================================== */

/*========================================================================
Include Files
==========================================================================*/
#include "venc_signal.h"
#include "venc_debug.h"
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include "venc_def.h"



typedef struct venc_signal_type
{
   pthread_cond_t cond;
   pthread_mutex_t mutex;
   boolean m_bSignalSet ;
} venc_signal_type;



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_signal_create(void** handle)
{
   int result = 0;
   
   if (handle)
   {
      venc_signal_type* sig;
      
      *handle = malloc(sizeof(venc_signal_type));
      sig = (venc_signal_type*) *handle;
      sig->m_bSignalSet = FALSE ;
      if (*handle)
      {
         if (pthread_cond_init(&sig->cond,NULL) == 0)
         {
            if (pthread_mutex_init(&sig->mutex, NULL) != 0)
            {
               VENC_MSG_ERROR("failed to create mutex", 0, 0, 0);
               result = 1;
               pthread_cond_destroy(&sig->cond);
               free(*handle);
            }
         }
         else
         {
            VENC_MSG_ERROR("failed to create cond var", 0, 0, 0);
            result = 1;
            free(*handle);
         }
      }
      else
      {
         VENC_MSG_ERROR("failed to alloc handle", 0, 0, 0);
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
int venc_signal_destroy(void* handle)
{
   int result = 0;

   if (handle)
   {
      venc_signal_type* sig = (venc_signal_type*) handle;
	  sig->m_bSignalSet = FALSE ;
      pthread_cond_destroy(&sig->cond);
      pthread_mutex_destroy(&sig->mutex);	  
      free(handle);
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
int venc_signal_set(void* handle)
{
   int result = 0;

   if (handle)
   {
      venc_signal_type* sig = (venc_signal_type*) handle;
      
      if (pthread_mutex_lock(&sig->mutex) == 0)
      {
	   	 sig->m_bSignalSet = TRUE ;
		 if (pthread_cond_signal(&sig->cond) != 0)
         {
            VENC_MSG_ERROR("error setting signal", 0, 0, 0);
            result = 1;
         }
         
         if (pthread_mutex_unlock(&sig->mutex) != 0)
         {
            VENC_MSG_ERROR("error unlocking mutex", 0, 0, 0);
            result = 1;
         }
      }
      else
      {
         VENC_MSG_ERROR("error locking mutex", 0, 0, 0);
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
int venc_signal_wait(void* handle, int timeout)
{
   int result = 0;

   if (handle)
   {
      venc_signal_type* sig = (venc_signal_type*) handle;
      
      if (pthread_mutex_lock(&sig->mutex) == 0)
      {
         
         if (timeout > 0)
         {
            int wait_result;
            struct timespec time;
			if(sig->m_bSignalSet == TRUE)
			{
				VENC_MSG_MEDIUM("signal is set change timeout= %d",timeout, 0, 0);
       		                timeout = 1 ;
                                sig->m_bSignalSet = FALSE ;
			}
            clock_gettime(CLOCK_REALTIME, &time);
            time.tv_sec += timeout / 1000;
            time.tv_nsec += (timeout % 1000) * 1000000;

            wait_result = pthread_cond_timedwait(&sig->cond, &sig->mutex, &time);
            if (wait_result == ETIMEDOUT)
            {
               result = 2;
            }
            else if (wait_result != 0)
            {
               result = 1;
            }
	        if((timeout == 1) || (sig->m_bSignalSet == TRUE))
	      {
               result = 0;
	      }
         }
         else
		 {
			 if(sig->m_bSignalSet == TRUE)
			 {
				 struct timespec time;
				 timeout = 1 ;
				 clock_gettime(CLOCK_REALTIME, &time);
				 time.tv_sec += timeout / 1000;
				 time.tv_nsec += (timeout % 1000) * 1000000;
				 VENC_MSG_MEDIUM("error waiting for signal but its already set", 0, 0, 0);
				 pthread_cond_timedwait(&sig->cond, &sig->mutex, &time) ;
				 sig->m_bSignalSet = FALSE ;
			 }
			 else
			 {
				 if (pthread_cond_wait(&sig->cond, &sig->mutex) != 0)
				 {
					 VENC_MSG_ERROR("error waiting for signal", 0, 0, 0);
					 result = 1;
				 }
				 sig->m_bSignalSet = FALSE ;
			 }		
		 }
         
         if (pthread_mutex_unlock(&sig->mutex) != 0)
         {
            VENC_MSG_ERROR("error unlocking mutex", 0, 0, 0);
            result = 1;
         }

      }
      else
      {
         VENC_MSG_ERROR("error locking mutex", 0, 0, 0);
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
