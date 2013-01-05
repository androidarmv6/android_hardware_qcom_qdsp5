#ifndef _OMX_UTILS_H_
#define _OMX_UTILS_H_

/*========================================================================

                                 O p e n M M
         U t i l i t i e s   a n d   H e l p e r   R o u t i n e s

*//** @file OmmUtils.h
This module contains utilities and helper routines.

Copyright (c) 2006-2008 QUALCOMM Incorporated.
All Rights Reserved. Qualcomm Proprietary and Confidential.
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/OmxVdec/rel/1.2/OmxUtils.h#2 $

when       who     what, where, why
--------   ---     -------------------------------------------------------

========================================================================== */

/* =======================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include<stdio.h>
#include "Map.h"
#include "OMX_Core.h"
#include "OMX_QCOMExtns.h"
#include "omx_vdec.h"
#include "qtv_msg.h"

/* =======================================================================

                        DATA DECLARATIONS

========================================================================== */

/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */
/* =======================================================================
**                          Function Declarations
** ======================================================================= */

/* -----------------------------------------------------------------------
** Type Declarations
** ---------------------------------------------------------------------- */
#define STD_MIN(x,y) (((x) < (y)) ? (x) : (y))
// units that need to go in the meta data.
struct H264ParamNalu {
    unsigned long int picSetID;
    unsigned long int seqSetID;
    unsigned long int picOrderCntType;
    bool frameMbsOnlyFlag;
    bool picOrderPresentFlag;
    unsigned long int picWidthInMbsMinus1;
    unsigned long int picHeightInMapUnitsMinus1;
    unsigned long int log2MaxFrameNumMinus4;
    unsigned long int log2MaxPicOrderCntLsbMinus4;
    bool deltaPicOrderAlwaysZeroFlag;
    unsigned long int crop_left;
    unsigned long int crop_right;
    unsigned long int crop_top;
    unsigned long int crop_bot;
    //std::vector<unsigned char> nalu;
    unsigned long int nalu;
};
typedef Map<unsigned long int, H264ParamNalu *> H264ParamNaluSet;

/******************************************************************************
 ** This class is used to convert an H.264 NALU (network abstraction layer
 ** unit) into RBSP (raw byte sequence payload) and extract bits from it.
 *****************************************************************************/
class RbspParser
{
public:
    RbspParser (const unsigned char *begin, const unsigned char *end);

    virtual ~RbspParser ();

    unsigned long int next ();
    void advance ();
    unsigned long int u (unsigned long int n);
    unsigned long int ue ();
    signed long int se ();

private:
    const unsigned char *begin, *end;
    signed long int pos;
    unsigned long int bit;
    unsigned long int cursor;
    bool advanceNeeded;

};

class omx_vdec;

class OmxUtils
{
public:
    OmxUtils(){}

    virtual ~OmxUtils ();

    virtual OMX_ERRORTYPE parseInputBitStream(OMX_BUFFERHEADERTYPE* bufferHdr);
    virtual bool validateMetaData(OMX_U8 *encodedBytes, 
                                  OMX_U32 totalBytes,
                                  OMX_U32 nal_len,
                                  unsigned &height,
                                  unsigned &width,
                                  unsigned &cropx,
                                  unsigned &cropy,
                                  unsigned &cropdx, 
                                  unsigned &cropdy,
				  unsigned &numOutFrames);
    virtual bool validate_profile_and_level(unsigned long int profile, unsigned long int level);
    //typedef Map<OMX_BUFFERHEADERTYPE*, bool> ebd_pending_list;

    //ebd_pending_list m_ebd_pending;
protected:

private:
};

class H264Utils: public OmxUtils
{
public:
    H264Utils(omx_vdec* omx_vdec):
                       m_omx_vdec(omx_vdec),
                       m_size(0),
                       m_partial_nal_size(0),
                       m_partial_buf_add(NULL),
                       m_buffer(0),
                       m_height(0),
                       m_width(0),
                       m_cpy_flag(false)
    {}
    ~H264Utils();
    OMX_ERRORTYPE parseInputBitStream(OMX_BUFFERHEADERTYPE* bufferHdr);
    bool validateMetaData(OMX_U8 *encodedBytes, 
                          OMX_U32 totalBytes,
                          OMX_U32 nal_len,
                          unsigned &height,
                          unsigned &width,
                          unsigned &cropx,
                          unsigned &cropy,
                          unsigned &cropdx, 
                          unsigned &cropdy,
			  unsigned &numOutFrames);

private:
    unsigned char            *m_buffer;//current NAL starting address
    signed long int            m_size;// size of NAL
    omx_vdec         *m_omx_vdec;
    unsigned char            m_partial_nal_size;// size of NAL
    OMX_U8           *m_partial_buf_add;
    unsigned         m_height;
    unsigned         m_width;
    bool             m_cpy_flag;
    OMX_U8           m_cpy_buf[128*1024*2];
    H264ParamNaluSet pic;
    H264ParamNaluSet seq;
    bool validate_profile_and_level(unsigned long int profile, unsigned long int level);
};

class Mpeg4Utils: public OmxUtils
{
public:
    Mpeg4Utils(){}
    ~Mpeg4Utils();
//    bool parseInputBitStream(OMX_BUFFERHEADERTYPE* bufferHdr);
private:
};

#endif /* _OMX_UTILS_H_ */
