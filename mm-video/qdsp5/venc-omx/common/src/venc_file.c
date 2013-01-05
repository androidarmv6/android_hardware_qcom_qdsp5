/*========================================================================

                                      TBD

*//** @file TODO

                     Copyright (c) 2009 QUALCOMM Incorporated.
                              All Rights Reserved.
                        Qualcomm Confidential and Proprietary
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Enc/VencOmx/rel/1.3/common/src/venc_file.c#1 $

when         who    what, where, why
----------   ---    ------------------------------------------------------
2009-07-14   bc     Initial revision

========================================================================== */

/*========================================================================
  Include Files
 ==========================================================================*/
#include "venc_file.h"
#include "venc_debug.h"
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_file_open(void** handle, 
                   char* filename, 
                   int readonly)
{
   int result = 0;
   FILE* f = NULL;

   if (handle != NULL && filename != NULL && (readonly == 0 || readonly == 1))
   {
      if (readonly == 1)
      {
         f = fopen(filename, "rb");
      }
      else
      {
         f = fopen(filename, "wb");
      }

      if (f == NULL)
      {
         VENC_MSG_ERROR("Unable to open file", 0, 0, 0);
         result = 1;
      }
      
      *handle = f;
   }
   else
   {
      VENC_MSG_ERROR("bad param", 0, 0, 0);
      result = 1;
   }
   return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_file_close(void* handle)
{
   int result = 0;

   if (handle != NULL)
   {
      fclose((FILE*) handle);
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
int venc_file_read(void* handle, 
                   void* buffer, 
                   int bytes)
{
   int result = 0;
   
   if (buffer != NULL)
   {
      if (bytes > 0)
      {
         result = (int) fread(buffer, 1, bytes, (FILE*) handle);
      }
      else
      {
         VENC_MSG_ERROR("Bytes must be > 0", 0, 0, 0);
         result = -1;
      }
   }
   else
   {
      VENC_MSG_ERROR("Null param", 0, 0, 0);
      result = -1;
   }
   
   return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_file_write(void* handle, 
                    void* buffer,
                    int bytes)
{
   int result = 0;
   
   if (buffer != NULL)
   {
      if (bytes > 0)
      {
         result = (int) fwrite(buffer, 1, bytes, (FILE*) handle);
	      }
      else
      {
         VENC_MSG_ERROR("Bytes must be > 0", 0, 0, 0);
         result = -1;
      }
   }
   else
   {
      VENC_MSG_ERROR("Null param", 0, 0, 0);
      result = -1;
   }
   
   return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int venc_file_seek_start(void* handle, 
                         int bytes)
{
   int result = 0;

   if (bytes >= 0)
   {
      if (fseek((FILE*) handle, bytes, SEEK_SET) != 0)
      {
         VENC_MSG_ERROR("failed to seek", 0, 0, 0);
         result = 1;
      }
   }

   return result;
}
