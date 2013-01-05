/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/src/venc_queue.c#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-07-14   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venc_debug.h"
#include "venc_queue.h"
#include <stdlib.h>

typedef struct venc_queue_type
{
   int head;
   int size;
   unsigned char* data;
   int max_queue_size;
   int max_data_size;
} venc_queue_type;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_queue_create(void** handle, int max_queue_size, int max_data_size)
{
   int result = 0;
   
   if (handle)
   {
      venc_queue_type* queue = (venc_queue_type*) malloc(sizeof(venc_queue_type));
      *handle = (void*) queue;
      
      queue->max_queue_size = max_queue_size;
      queue->max_data_size = max_data_size;
      queue->data = NULL;
      queue->head = 0;
      queue->size = 0;

      if (queue->max_queue_size > 0)
      {
         if (queue->max_data_size > 0)
         {
            queue->data = malloc(queue->max_queue_size * queue->max_data_size);
            if (!queue->data)
            {
               VENC_MSG_ERROR("error allocating data array", 0, 0, 0);
               free((void*) queue);
               result = 1;
            }
         }
      }
   }
   
   return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_queue_destroy(void* handle)
{
   int result = 0;
   
   if (handle)
   {
      free(handle);
   }
   else
   {
      VENC_MSG_ERROR("invalid handle", 0, 0, 0);
      result = 1;
   }
   
   return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_queue_pop(void* handle, void* data, int data_size)
{
   int result = 0;
   
   if (handle)
   {
      venc_queue_type* queue = (venc_queue_type*) handle;
      result = venc_queue_peek(handle, data, data_size);

      if (result == 0)
      {
         --queue->size;
         queue->head = (queue->head + 1) % queue->max_queue_size;
      }
   }
   else
   {
      VENC_MSG_ERROR("invalid handle", 0, 0, 0);
      result = 1;
   }
   
   return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_queue_push(void* handle, void* data, int data_size)
{
   int result = 0;

   if (handle)
   {
      venc_queue_type* queue = (venc_queue_type*) handle;

      // see if queue is full
      if (queue->size < queue->max_queue_size)
      {
         // see if data size is okay
         if (data_size >= 0 && data_size <= queue->max_data_size)
         {
            // make sure we have no null data
            if (data != NULL)
            {
               int index = (queue->head + queue->size) % queue->max_queue_size;
               int byte_offset = index * queue->max_data_size;
               memcpy(&queue->data[byte_offset], data, data_size);
               ++queue->size;
            }
            else
            {
               VENC_MSG_ERROR("Data is null", 0, 0, 0);
               result = 1;
            }
         }
         else
         {
            VENC_MSG_ERROR("Data size is wrong", 0, 0, 0);
            result = 1;
         }
      }
      else
      {
         VENC_MSG_ERROR("Q is full", 0, 0, 0);
         result = 1;
      }
   }
   else
   {
      VENC_MSG_ERROR("invalid handle", 0, 0, 0);
      result = 1;
   }

   return result;
}

int venc_queue_peek(void* handle, void* data, int data_size)
{
   int result = 0;

   if (handle)
   {
      venc_queue_type* queue = (venc_queue_type*) handle;
      // see if data size is okay
      if (data_size >= 0 && data_size <= queue->max_data_size)
      {
         // make sure we have no null data
         if (data != NULL)
         {
            // make sure we have something on the queue
            if (venc_queue_size(handle) > 0)
            {
               int byte_offset = queue->head * queue->max_data_size;
               memcpy(data, &queue->data[byte_offset], data_size);
            }
            else
            {
               VENC_MSG_ERROR("queue is empty", 0, 0, 0);
               result = 1;
            }
         }
         else
         {
            VENC_MSG_ERROR("Data is null", 0, 0, 0);
            result = 1;
         }
      }
      else
      {
         VENC_MSG_ERROR("Data size is wrong", 0, 0, 0);
         result = 1;
      }
   }
   else
   {
      VENC_MSG_ERROR("invalid handle", 0, 0, 0);
      result = 1;
   }

   return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_queue_size(void* handle)
{
   int size = 0;
   
   if (handle)
   {
      venc_queue_type* queue = (venc_queue_type*) handle;
      size = queue->size;
   }
   else
   {
      VENC_MSG_ERROR("invalid handle", 0, 0, 0);
   }
   
   return size;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_queue_full(void* handle)
{
   venc_queue_type* queue = (venc_queue_type*) handle;
   return (queue && queue->size == queue->max_queue_size) ? 1 : 0;
}
