#ifndef _FRAME_BUFFER_POOL_H_
#define _FRAME_BUFFER_POOL_H_
/* =======================================================================

                          frame_buffer_pool.h

DESCRIPTION
  Definition of the FrameBuffer class, used to store frame buffers
  for vdec_ objects.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/VdecCommon/main/latest/src/frame_buffer_pool.h#2 $
$DateTime: 2008/08/13 14:12:09 $
$Change: 722620 $

========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include <stdlib.h>

#include "mp4buf.h"
#include "vdecoder_types.h"

/* ==========================================================================

                        DATA DECLARATIONS

========================================================================== */
/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Type Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global Constant Data Declarations
** ----------------------------------------------------------------------- */

/* -----------------------------------------------------------------------
** Global Data Declarations
** ----------------------------------------------------------------------- */

/* =======================================================================
**                        Macro Definitions
** ======================================================================= */

/* =======================================================================
**                        Class Declarations
** ======================================================================= */
class FrameBufferPool
{
public:
  FrameBufferPool
  (
    const VDEC_STREAM_ID   stream,
    VDEC_MALLOC_FN         pfnMalloc,
    VDEC_FREE_FN           pfnFree,
    void *                 pCbData,
    const VDEC_DIMENSIONS &frameSize,
    const unsigned         numReferenceYUV_buffers,
    const unsigned         numOutputYUV_buffers = 2,
    const unsigned         numOutputRGB_buffers = 2
  );

  FrameBufferPool
  (
    const VDEC_STREAM_ID   stream,
    VDEC_MALLOC_FN         pfnMalloc,
    VDEC_FREE_FN           pfnFree,
    void *                 pCbData,
    const VDEC_DIMENSIONS &referenceFrameSize,
    const unsigned         numReferenceYUV_buffers,
    const VDEC_DIMENSIONS &outputFrameSize,
    const unsigned         numOutputYUV_buffers,
    const VDEC_DIMENSIONS &rgbFrameSize,
    const unsigned         numOutputRGB_buffers
  );
  ~FrameBufferPool( void );

  // This operator allows implicit conversion from this class to the
  // MP4BufType used at the mp4codec level.
  //
  operator MP4BufType*( void );
  const MP4BufType* operator->() { return &m_buf; }

private:
  // Externally supplied malloc and free functions give the application
  // control over the management of frame buffers.
  //
  VDEC_MALLOC_FN m_pfnMalloc;
  VDEC_FREE_FN   m_pfnFree;
  const VDEC_STREAM_ID   m_streamID;
  void *         m_pCbData;
  // The buffers are stored in an MP4BufType, for now.  It'd be nice
  // to define FrameBuffers independently of the lower type in the future.
  //
  MP4BufType m_buf;

  bool AllocateAllBuffers
  (
    const VDEC_DIMENSIONS &frameSize,
    const unsigned         numReferenceYUV_buffers,
    const unsigned         numOutputYUV_buffers,
    const unsigned         numOutputRGB_buffers
  );
 
  bool AllocateAllBuffers
  (
    const VDEC_DIMENSIONS &referenceFrameSize,
    const unsigned         numReferenceYUV_buffers,
    const VDEC_DIMENSIONS &outputFrameSize,
    const unsigned         numOutputYUV_buffers,
    const VDEC_DIMENSIONS &rgbFrameSize,
    const unsigned         numOutputRGB_buffers
  ); 

  bool  AllocateBufferArray( void **        &array_out,
                             const size_t    nBytesPerBuffer,
                             const unsigned  numBuffers,
                             unsigned short         &numBuffers_out );
  void* AllocateOneBuffer( const size_t nBytes );
  void  FreeAllBuffers( void );
  void  FreeBufferArray( void **        &array,
                         const unsigned numBuffers );
};

/* =======================================================================
**                        Function Declarations
** ======================================================================= */

#endif /* _FRAME_BUFFER_POOL_H_ */
