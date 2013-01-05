/*========================================================================

         V i d e o   U t i l i t i e s

*//** @file OmxUtils.cpp
  This module contains utilities and helper routines.

@par EXTERNALIZED FUNCTIONS

@par INITIALIZATION AND SEQUENCING REQUIREMENTS
  (none)

Copyright (c) 2006-2008 QUALCOMM Incorporated.
All Rights Reserved. Qualcomm Proprietary and Confidential.
*//*====================================================================== */

/*========================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/OmxVdec/rel/1.2/OmxUtils.cpp#3 $

when       who     what, where, why
--------   ---     -------------------------------------------------------

========================================================================== */

/* =======================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include <string.h>
#include "OmxUtils.h"

/* =======================================================================
                DEFINITIONS AND DECLARATIONS FOR MODULE
This section contains definitions for constants, macros, types, variables
and other items needed by this module.

========================================================================= */

#define BASELINE_PROFILE  66
#define START_CODE 0x00000001

RbspParser::RbspParser (const unsigned char *_begin, const unsigned char *_end)
: begin (_begin), end(_end), pos (- 1), bit (0),
cursor (0xFFFFFF), advanceNeeded (true)
{
}

// Destructor
/*lint -e{1540}  Pointer member neither freed nor zeroed by destructor
 * No problem
 */
RbspParser::~RbspParser () {}

// Return next RBSP byte as a word
unsigned long int RbspParser::next ()
{
    if (advanceNeeded) advance ();
    //return static_cast<unsigned long int> (*pos);
    return static_cast<unsigned long int> (begin[pos]);
}

// Advance RBSP decoder to next byte
void RbspParser::advance ()
{
    ++pos;
    //if (pos >= stop)
    if (begin + pos == end)
    {
        /*lint -e{730}  Boolean argument to function
         * I don't see a problem here
         */
        //throw false;
        DEBUG_PRINT("H264Parser-->NEED TO THROW THE EXCEPTION...\n");
    }
    cursor <<= 8;
    //cursor |= static_cast<unsigned long int> (*pos);
    cursor |= static_cast<unsigned long int> (begin[pos]);
    if ((cursor & 0xFFFFFF) == 0x000003)
    {
        advance ();
    }
    advanceNeeded = false;
}

// Decode unsigned integer
unsigned long int RbspParser::u (unsigned long int n)
{
    unsigned long int i, s, x = 0;
    for (i = 0; i < n; i += s)
    {
        s = static_cast<unsigned long int>STD_MIN(static_cast<int>(8 - bit),
            static_cast<int>(n - i));
        x <<= s;

        x |= ((next () >> ((8 - static_cast<unsigned long int>(bit)) - s)) &
            ((1 << s) - 1));

        bit = (bit + s) % 8;
        if (!bit)
        {
            advanceNeeded = true;
        }
    }
    return x;
}


// Decode unsigned integer Exp-Golomb-coded syntax element
unsigned long int RbspParser::ue ()
{
    int leadingZeroBits = -1;
    for (unsigned long int b = 0; !b; ++leadingZeroBits)
    {
        b = u (1);
    }
    return ((1 << leadingZeroBits) - 1) +
        u (static_cast<unsigned long int>(leadingZeroBits));
}

// Decode signed integer Exp-Golomb-coded syntax element
signed long int RbspParser::se ()
{
    const unsigned long int x = ue ();
    if (!x) return 0;
    else if (x & 1) return static_cast<signed long int> ((x >> 1) + 1);
    else return - static_cast<signed long int> (x >> 1);
}



OmxUtils::~OmxUtils()
{

}

bool OmxUtils::validateMetaData(OMX_U8  *encodedBytes,
                                 OMX_U32 totalBytes,
                                 OMX_U32 sizeOfNALLengthField,
                                 unsigned &height,
                                 unsigned &width,
                                 unsigned &cropx,
                                  unsigned &cropy,
                                  unsigned &cropdx, 
                                  unsigned &cropdy,
				  unsigned &numOutFrames)
{

    DEBUG_PRINT("parseInputForHxW::No definition in base class...\n");
    return 0;
}

/*===========================================================================
FUNCTION:
  validate_profile_and_level

DESCRIPTION:
  This function validate the profile and level that is supported.

INPUT/OUTPUT PARAMETERS:
  unsigned long int profile
  unsigned long int level

RETURN VALUE:
  false it it's not supported
  true otherwise

SIDE EFFECTS:
  None.
===========================================================================*/
bool OmxUtils::validate_profile_and_level(unsigned long int profile, unsigned long int level)
{
    DEBUG_PRINT_ERROR("Base class: No implementation...\n");
    return false;
}

OMX_ERRORTYPE OmxUtils::parseInputBitStream(OMX_BUFFERHEADERTYPE* bufferHdr)
{
    DEBUG_PRINT("Base class implementation...\n");
    return OMX_ErrorNone;
}

H264Utils::~H264Utils()
{
    m_omx_vdec         = NULL;
    m_size             = 0;
    m_partial_nal_size = 0;
    m_partial_buf_add  = NULL;
}

/* ======================================================================
FUNCTION
  H264Utils::validateMetaData

DESCRIPTION
  Validate the metadata (SPS), get the width and height
  return true if level and profile are supported

PARAMETERS
  IN OMX_U8 *encodedBytes
      the starting point of metadata
 
  IN OMX_U32 totalBytes
      the number of bytes of the metadata
 
  IN OMX_U32 &sizeOfNALLengthField
      the size of the NAL lenght
 
  OUT unsigned &height
      the height of the clip

  OUT unsigned &width
      the width of the clip

RETURN VALUE
  true if level and profile are supported
  false otherwise
========================================================================== */
bool H264Utils::validateMetaData(OMX_U8  *encodedBytes,
                                 OMX_U32 totalBytes,
                                 OMX_U32 sizeOfNALLengthField,
                                 unsigned &height,
                                 unsigned &width,
                                 unsigned &cropx,
                                 unsigned &cropy,
                                 unsigned &cropdx, 
                                 unsigned &cropdy,
				 unsigned &numOutFrames)
{
    bool keyFrame = 0;
    bool stripSeiAud = 0;
    bool nalSize = 0;
    unsigned long long bytesConsumed = 0;
    unsigned char frame[64];
    struct H264ParamNalu temp = {0};

    // Scan NALUs until a frame boundary is detected.  If this is the first
    // frame, scan a second time to find the end of the frame.  Otherwise, the
    // first boundary found is the end of the current frame.  While scanning,
    // collect any sequence/parameter set NALUs for use in constructing the
    // stream header.
    bool inFrame = true;
    bool inNalu = false;
    bool vclNaluFound = false;
    unsigned char naluType = 0;
    unsigned long int naluStart = 0, naluSize = 0;
    unsigned long int prevVclFrameNum = 0, vclFrameNum = 0;
    bool prevVclFieldPicFlag = false, vclFieldPicFlag = false;
    bool prevVclBottomFieldFlag = false, vclBottomFieldFlag = false;
    unsigned char prevVclNalRefIdc = 0, vclNalRefIdc = 0;
    unsigned long int prevVclPicOrderCntLsb = 0, vclPicOrderCntLsb = 0;
    signed long int prevVclDeltaPicOrderCntBottom = 0, vclDeltaPicOrderCntBottom = 0;
    signed long int prevVclDeltaPicOrderCnt0 = 0, vclDeltaPicOrderCnt0 = 0;
    signed long int prevVclDeltaPicOrderCnt1 = 0, vclDeltaPicOrderCnt1 = 0;
    unsigned char vclNaluType = 0;
    unsigned long int vclPicOrderCntType = 0;
    unsigned long long pos;
    unsigned long long posNalDetected = 0xFFFFFFFF;
    unsigned long int cursor = 0xFFFFFFFF;
    unsigned long int profile = 0;
    unsigned long int level = 0;

    H264ParamNalu *seq=NULL, *pic=NULL;
    // used to  determin possible infinite loop condition
    int loopCnt = 0;
    for (pos = 0; ; ++pos)
    {
        // return early, found possible infinite loop
        if (loopCnt > 100000)
            return 0;
        // Scan ahead next byte.
        cursor <<= 8;
        cursor |= static_cast<unsigned long int> (encodedBytes [pos]);

        if (sizeOfNALLengthField != 0)
        {
          inNalu = true;
          naluStart = sizeOfNALLengthField;
        }

        // If in NALU, scan forward until an end of NALU condition is
        // detected.
        if (inNalu)
        {
            if (sizeOfNALLengthField == 0)
            {
             // Detect end of NALU condition.
              if (((cursor & 0xFFFFFF) == 0x000000)
                  || ((cursor & 0xFFFFFF) == 0x000001)
                  || (pos >= totalBytes))
              {
                inNalu = false;
                if (pos < totalBytes)
                {
                    pos -= 3;
                }
                naluSize = static_cast<unsigned long int>((static_cast<unsigned long int>(pos) -
                                                naluStart) + 1);
                DEBUG_PRINT("H264Parser-->1.nalusize=%x pos=%x naluStart=%x\n",\
                                            naluSize,pos,naluStart);
              }
              else
              {
                ++loopCnt;
                continue;
              }
            }
            // Determine NALU type.
            naluType = (encodedBytes [naluStart] & 0x1F);
            DEBUG_PRINT("H264Parser-->2.naluType=%x....\n",naluType);
            if (naluType == 5)
            keyFrame = true;

            // For NALUs in the frame having a slice header, parse additional
            // fields.
            bool isVclNalu = false;
            if ((naluType == 1) || (naluType == 2) || (naluType == 5))
            {
                DEBUG_PRINT("H264Parser-->3.naluType=%x....\n",naluType);
                // Parse additional fields.
                RbspParser rbsp (&encodedBytes [naluStart + 1],
                                 &encodedBytes [naluStart + naluSize]);
                vclNaluType = naluType;
                vclNalRefIdc = ((encodedBytes [naluStart] >> 5) & 0x03);
                (void) rbsp.ue ();
                (void) rbsp.ue ();
                const unsigned long int picSetID = rbsp.ue ();
                pic = this->pic.find(picSetID);
                DEBUG_PRINT("H264Parser-->4.sizeof %x %x\n", this->pic.size(),\
                                                this->seq.size());
                if (!pic)
                {
                    if (this->pic.empty ())
                    {
                        // Found VCL NALU before needed picture parameter set
                        // -- assume that we started parsing mid-frame, and
                        // discard the rest of the frame we're in.
                        inFrame = false;
                        //frame.clear ();
                        DEBUG_PRINT("H264Parser-->5.pic empty........\n");
                    }
                    else
                    {
                        DEBUG_PRINT("H264Parser-->6.FAILURE to parse..break frm here");
                        break;
                    }
                }
                if (pic)
                {
                    DEBUG_PRINT("H264Parser-->7.sizeof %x %x\n",this->pic.size(),this->seq.size());
                    seq = this->seq.find (pic->seqSetID);
                    if (!seq)
                    {
                        if (this->seq.empty ())
                        {
                            DEBUG_PRINT("H264Parser-->8.seq empty........\n");
                            // Found VCL NALU before needed sequence parameter
                            // set -- assume that we started parsing
                            // mid-frame, and discard the rest of the frame
                            // we're in.
                            inFrame = false;
                            //frame.clear ();
                        }
                        else
                        {
                            DEBUG_PRINT("H264Parser-->9.FAILURE to parse...break");
                            break;
                        }
                    }
                }
                DEBUG_PRINT("H264Parser-->10.sizeof %x %x\n",this->pic.size(),this->seq.size());
                if (pic && seq )
                {
                    DEBUG_PRINT("H264Parser-->11.pic and seq[%x][%x]........\n",pic,seq);
                    isVclNalu = true;
                    vclFrameNum = rbsp.u(seq->log2MaxFrameNumMinus4 + 4);
                    if (!seq->frameMbsOnlyFlag)
                    {
                        vclFieldPicFlag = (rbsp.u (1) == 1);
                        if (vclFieldPicFlag)
                        {
                            vclBottomFieldFlag = (rbsp.u (1) == 1);
                        }
                    }
                    else
                    {
                        vclFieldPicFlag = false;
                        vclBottomFieldFlag = false;
                    }
                    if (vclNaluType == 5)
                    {
                        (void) rbsp.ue ();
                    }
                    vclPicOrderCntType = seq->picOrderCntType;
                                       if (seq->picOrderCntType == 0)
                    {
                        vclPicOrderCntLsb = rbsp.u
                            (seq->log2MaxPicOrderCntLsbMinus4 + 4);
                        if (pic->picOrderPresentFlag
                            && !vclFieldPicFlag)
                        {
                            vclDeltaPicOrderCntBottom = rbsp.se ();
                        }
                        else
                        {
                            vclDeltaPicOrderCntBottom = 0;
                        }
                    }
                    else
                    {
                        vclPicOrderCntLsb = 0;
                        vclDeltaPicOrderCntBottom = 0;
                    }
                    if ((seq->picOrderCntType == 1)
                        && !seq->deltaPicOrderAlwaysZeroFlag)
                    {
                        vclDeltaPicOrderCnt0 = rbsp.se ();
                        if (pic->picOrderPresentFlag
                            && !vclFieldPicFlag)
                       {
                            vclDeltaPicOrderCnt1 = rbsp.se ();
                        }
                        else
                        {
                            vclDeltaPicOrderCnt1 = 0;
                        }
                    }
                    else
                    {
                        vclDeltaPicOrderCnt0 = 0;
                        vclDeltaPicOrderCnt1 = 0;
                    }
                }
            }

            //////////////////////////////////////////////////////////////////
            // Perform frame boundary detection.
            //////////////////////////////////////////////////////////////////

            // The end of the bitstream is always a boundary.
            bool boundary = (pos >= totalBytes);

            // The first of these NALU types always mark a boundary, but skip
           // any that occur before the first VCL NALU in a new frame.
            DEBUG_PRINT("H264Parser-->12.naluType[%x].....\n",naluType);
            if ((((naluType >= 6) && (naluType <= 9))
                || ((naluType >= 13) && (naluType <= 18)))
                && (vclNaluFound || !inFrame))
            {
                boundary = true;
            }

            // If a VCL NALU is found, compare with the last VCL NALU to
            // determine if they belong to different frames.
            else if (vclNaluFound && isVclNalu)
            {
                // Clause 7.4.1.2.4 -- detect first VCL NALU through
                // parsing of portions of the NALU header and slice
                // header.
                /*lint -e{731} Boolean argument to equal/not equal
                 * It's ok
                 */
                if ((prevVclFrameNum != vclFrameNum)
                    || (prevVclFieldPicFlag != vclFieldPicFlag)
                    || (prevVclBottomFieldFlag != vclBottomFieldFlag)
                    || ((prevVclNalRefIdc != vclNalRefIdc)
                        && ((prevVclNalRefIdc == 0)
                            || (vclNalRefIdc == 0)))
                    || ((vclPicOrderCntType == 0)
                        && ((prevVclPicOrderCntLsb != vclPicOrderCntLsb)
                            || (prevVclDeltaPicOrderCntBottom
                                != vclDeltaPicOrderCntBottom)))
                    || ((vclPicOrderCntType == 1)
                        && ((prevVclDeltaPicOrderCnt0
                             != vclDeltaPicOrderCnt0)
                            || (prevVclDeltaPicOrderCnt1
                                != vclDeltaPicOrderCnt1))))
                {
                    boundary = true;
                }
            }

            // If a frame boundary is reached and we were in the frame in
            // which at least one VCL NALU was found, we are done processing
            // this frame.  Remember to back up to NALU start code to make
            // sure it is available for when the next frame is parsed.
            if (boundary && inFrame && vclNaluFound)
            {
                pos = static_cast<unsigned long long>(naluStart - 3);
                DEBUG_PRINT("H264Parser-->13.Break  \n");
                break;
            }
            inFrame = (inFrame || boundary);

            // Process sequence and parameter set NALUs specially.
            if ((naluType == 7) || (naluType == 8))
            {
                unsigned tmp;
                DEBUG_PRINT("H264Parser-->14.naluType[%x].....\n",naluType);
                DEBUG_PRINT("H264Parser-->15.sizeof %x %x\n",this->pic.size(),this->seq.size());
                H264ParamNaluSet &naluSet
                    = ((naluType == 7) ? this->seq : this->pic);

                // Parse parameter set ID and other stream information.
                H264ParamNalu newParam;
                RbspParser rbsp (&encodedBytes [naluStart + 1],
                                 &encodedBytes [naluStart + naluSize]);
                unsigned long int id;
                if (naluType == 7)
                {
                    DEBUG_PRINT("H264Parser-->16.naluType[%x].....\n",naluType);             
                    profile = rbsp.u(8);
                    (void) rbsp.u(8);
                    level = rbsp.u(8);
                    id = newParam.seqSetID = rbsp.ue ();
                    newParam.log2MaxFrameNumMinus4 = rbsp.ue ();
                    newParam.picOrderCntType = rbsp.ue ();
                    if (newParam.picOrderCntType == 0)
                    {
                        newParam.log2MaxPicOrderCntLsbMinus4 = rbsp.ue ();
                    }
                    else if (newParam.picOrderCntType == 1)
                    {
                        newParam.deltaPicOrderAlwaysZeroFlag
                            = (rbsp.u (1) == 1);
                        (void) rbsp.se ();
                        (void) rbsp.se ();
                        const unsigned long int numRefFramesInPicOrderCntCycle =
                            rbsp.ue ();
                        for (unsigned long int i = 0; i < numRefFramesInPicOrderCntCycle;
                            ++i)
                        {
                            (void) rbsp.se ();
                        }
                    }
                    numOutFrames = (unsigned) rbsp.ue ();
		    if(numOutFrames <= OMX_VIDEO_DEC_NUM_OUTPUT_BUFFERS) {
			    numOutFrames = OMX_VIDEO_DEC_NUM_OUTPUT_BUFFERS + 2;
		    } else {
			    numOutFrames = MAX_OUT_BUFFERS;
		    }
                    (void) rbsp.u (1);
                    newParam.picWidthInMbsMinus1 = rbsp.ue ();
                    newParam.picHeightInMapUnitsMinus1 = rbsp.ue ();
                    newParam.frameMbsOnlyFlag = (rbsp.u (1) == 1);
                    if(!newParam.frameMbsOnlyFlag)
                        (void)rbsp.u (1);
                    (void)rbsp.u (1);
                    tmp = rbsp.u (1);
                    newParam.crop_left = 0;
                    newParam.crop_right = 0;
                    newParam.crop_top = 0;
                    newParam.crop_bot = 0;

                    if(tmp)
                    {
                        newParam.crop_left = rbsp.ue () ;
                        newParam.crop_right = rbsp.ue () ;
                        newParam.crop_top  = rbsp.ue () ;
                        newParam.crop_bot = rbsp.ue () ;
                    }
                   DEBUG_PRINT("H264Parser--->34 crop left %d, right %d, top %d, bot %d\n",\
                                 newParam.crop_left,newParam.crop_right,newParam.crop_top,\  
                                 newParam.crop_bot);

                }
                else
                {
                    DEBUG_PRINT("H264Parser-->17.naluType[%x].....\n",naluType);
                    id = newParam.picSetID = rbsp.ue ();
                    newParam.seqSetID = rbsp.ue ();
                    (void) rbsp.u (1);
                    newParam.picOrderPresentFlag = (rbsp.u (1) == 1);
                }

                // We currently don't support updating existing parameter
                // sets.
                //const H264ParamNaluSet::const_iterator it = naluSet.find (id);
                H264ParamNalu *it = naluSet.find (id);
                if (it)
                {
                    const unsigned long int tempSize =
                        static_cast<unsigned long int>(it->nalu);// ???
                    if ((naluSize != tempSize)
                        || (0 != memcmp (&encodedBytes[naluStart],
                                             &it->nalu,
                                             static_cast<int>(naluSize))))
                    {
                        DEBUG_PRINT("H264Parser-->18.H264 stream contains two or \
                                     more parameter set NALUs having the \
                                     same ID -- this requires either a \
                                     separate parameter set ES or \
                                     multiple sample description atoms, \
                                     neither of which is currently \
                                     supported!");
                        break;
                    }
                }

                // Otherwise, add NALU to appropriate NALU set.
                else
                {
                  H264ParamNalu *newParamInSet = naluSet.find(id);
                    DEBUG_PRINT("H264Parser-->19.newParamInset[%x]\n",newParamInSet);
                    if(!newParamInSet)
                    {
                        DEBUG_PRINT("H264Parser-->20.newParamInset[%x]\n",newParamInSet);
                        newParamInSet = &temp;
                        memcpy(newParamInSet, &newParam, sizeof(struct H264ParamNalu));
                    }
                    DEBUG_PRINT("H264Parser-->21.encodebytes=%x naluStart=%x\n",\
                                     encodedBytes,naluStart,naluSize,newParamInSet);
                    DEBUG_PRINT("H264Parser-->22.naluSize=%x newparaminset=%p\n",\
                                                            naluSize,newParamInSet);
                    DEBUG_PRINT("H264Parser-->23.-->0x%x 0x%x 0x%x 0x%x\n",\
                           (encodedBytes + naluStart),(encodedBytes + naluStart+1),
                           (encodedBytes + naluStart+2),(encodedBytes + naluStart+3));

                    memcpy(&newParamInSet->nalu,
                          (encodedBytes + naluStart),
                        sizeof(newParamInSet->nalu));
                    DEBUG_PRINT("H264Parser-->24.nalu=0x%x \n", newParamInSet->nalu);
                    naluSet.insert(id,newParamInSet);
                }
            }

            // Otherwise, if we are inside the frame, convert the NALU
            // and append it to the frame output, if its type is acceptable.
            else if (inFrame && (naluType != 0) && (naluType < 12)
                && (!stripSeiAud || (naluType != 9) && (naluType != 6)))
            {
                unsigned char sizeBuffer [4];
                sizeBuffer [0] = static_cast<unsigned char> (naluSize >> 24);
                sizeBuffer [1] = static_cast<unsigned char> ((naluSize >> 16) & 0xFF);
                sizeBuffer [2] = static_cast<unsigned char> ((naluSize >> 8) & 0xFF);
                sizeBuffer [3] = static_cast<unsigned char> (naluSize & 0xFF);
                /*lint -e{1025, 1703, 119, 64, 534}
                 * These are known lint issues
                 */
               //frame.insert (frame.end (), sizeBuffer,
                //              sizeBuffer + sizeof (sizeBuffer));
                /*lint -e{1025, 1703, 119, 64, 534, 632}
                 * These are known lint issues
                 */
                //frame.insert (frame.end (), encodedBytes + naluStart,
                //              encodedBytes + naluStart + naluSize);
            }

            // If NALU was a VCL, save VCL NALU parameters
            // for use in frame boundary detection.
            if (isVclNalu)
            {
                DEBUG_PRINT("H264Parser-->25.isvclnalu check passed\n");
                vclNaluFound = true;
                prevVclFrameNum = vclFrameNum;
                prevVclFieldPicFlag = vclFieldPicFlag;
                prevVclNalRefIdc = vclNalRefIdc;
                prevVclBottomFieldFlag = vclBottomFieldFlag;
                prevVclPicOrderCntLsb = vclPicOrderCntLsb;
                prevVclDeltaPicOrderCntBottom = vclDeltaPicOrderCntBottom;
                prevVclDeltaPicOrderCnt0 = vclDeltaPicOrderCnt0;
                prevVclDeltaPicOrderCnt1 = vclDeltaPicOrderCnt1;
            }
        }
       // If not currently in a NALU, detect next NALU start code.
        if ((cursor & 0xFFFFFF) == 0x000001)
        {
            DEBUG_PRINT("H264Parser-->26..here\n");
            inNalu = true;
            naluStart = static_cast<unsigned long int>(pos + 1);
            if ( 0xFFFFFFFF == posNalDetected )
                posNalDetected = pos - 2;
        }
        else if (pos >= totalBytes)
        {
            DEBUG_PRINT("H264Parser-->27.pos[%x] totalBytes[%x]\n",pos,totalBytes);
            break;
        }
    }

    unsigned long long tmpPos = 0;
    // find the first non-zero byte
   if (pos > 0)
    {
        DEBUG_PRINT("H264Parser-->28.last loop[%x]\n",pos);
        tmpPos = pos - 1;
        while (tmpPos != 0 && encodedBytes [tmpPos] == 0)
            --tmpPos;
        // add 1 to get the beginning of the start code
        ++tmpPos;
    }
    DEBUG_PRINT("H264Parser-->29.tmppos=%ld bytesConsumed=%x %x\n",\
                             tmpPos,bytesConsumed,posNalDetected);
    bytesConsumed = tmpPos;
    nalSize = static_cast<unsigned long int>(bytesConsumed - posNalDetected);

    // Fill in the height and width
    DEBUG_PRINT("H264Parser-->30.seq[%x] pic[%x]\n",this->seq.size(), this->pic.size());
    if(this->seq.size())
    {
        m_height =  (unsigned)(16 * (this->seq.begin()->picHeightInMapUnitsMinus1 + 1));
        m_width  =  (unsigned)(16 * (this->seq.begin()->picWidthInMbsMinus1 + 1));
        if((m_height % 16) != 0)
        {
            DEBUG_PRINT("\n Height %d is not a multiple of 16", m_height);
            m_height = (m_height / 16 + 1) *  16;
            DEBUG_PRINT("\n Height adjusted to %d \n", m_height);
        }
        if ((m_width % 16) != 0)
        {
            DEBUG_PRINT("\n Width %d is not a multiple of 16", m_width);
            m_width = (m_width / 16 + 1) *  16;
            DEBUG_PRINT("\n Width adjusted to %d \n", m_width);
        }
        height = m_height;
        width  = m_width;
        cropx = this->seq.begin()->crop_left << 1;
        cropy = this->seq.begin()->crop_top  << 1;
        cropdx = width - ((this->seq.begin()->crop_left + this->seq.begin()->crop_right) << 1);
	cropdy = height - ((this->seq.begin()->crop_top + this->seq.begin()->crop_bot) << 1);

        DEBUG_PRINT("H264Parser-->31.Height [%x] Width[%x]\n",\
                                                             height,width);
        DEBUG_PRINT("H264Parser-->31.cropdy [%x] cropdx[%x]\n",\
                                                             cropdy,cropdx);
    }
    this->seq.eraseall();
    this->pic.eraseall(); 

    return validate_profile_and_level(profile, level);
}


OMX_ERRORTYPE H264Utils::parseInputBitStream(OMX_BUFFERHEADERTYPE* pBufHdr)
{
    unsigned int code = 0;
    signed long int    readSize = 0;
    unsigned len=0;
    unsigned in_len = pBufHdr->nFilledLen;
    OMX_U8* inputBitStream = pBufHdr->pBuffer;
    if(m_cpy_flag)
    {
        m_buffer = inputBitStream;
    }    
    if(pBufHdr->nFlags == OMX_BUFFERFLAG_EOS)
    {
        // No parsing required; Just return the size and ptr
        // of the previously parsed buffer
        pBufHdr->nFlags = 0;
        m_omx_vdec->send_nal(pBufHdr,m_cpy_buf,m_size);

        // Now send the EOS info
        pBufHdr->nFlags = OMX_BUFFERFLAG_EOS;
        m_omx_vdec->send_nal(pBufHdr,m_cpy_buf,0);

        m_omx_vdec->omx_vdec_inform_ebd(pBufHdr);
        return OMX_ErrorNone;
    }
    readSize = m_size ;
    while(len < in_len)
    {
        code <<= 8;
        code |= (0x000000FF & *inputBitStream);
        if( code == START_CODE )
        {
            if(m_buffer && (readSize > 3))
            {
                readSize = readSize -3;
                if(m_cpy_flag)
                {
                    //***************************************************
                    DEBUG_PRINT("Sending out of sync bufs %p [%d]\n",\
                                                     m_cpy_buf,readSize);
                    //***************************************************
                    memcpy(&m_cpy_buf[m_size],m_buffer,readSize);  
                    m_omx_vdec->send_nal(pBufHdr,m_cpy_buf,readSize);
                }
                else
                { 
                    m_omx_vdec->send_nal(pBufHdr,m_buffer, readSize);
                }
                m_cpy_flag = false;
                {
                    // store the starting add of the next NAL
                    m_buffer = inputBitStream - 3;
                    m_size = readSize = 3;
                }
            }
            else if(!m_buffer)
            {
                // for the first NAL
                m_buffer = inputBitStream - 3;
            }
            else{}
            code = 0;
        }
        inputBitStream++;
        len++;
        readSize++;
    }

    // no start codes found in this buffer
    if(m_cpy_flag == true)
    {
        memcpy(&m_cpy_buf[m_size],m_buffer,readSize);
    }
    else
    {
        m_cpy_flag = true;
        memcpy(m_cpy_buf,m_buffer,readSize);
    }
    m_size = readSize ; 

    m_omx_vdec->omx_vdec_inform_ebd(pBufHdr);
    
    return OMX_ErrorNone; 
}

/*===========================================================================
FUNCTION:
  validate_profile_and_level

DESCRIPTION:
  This function validate the profile and level that is supported.

INPUT/OUTPUT PARAMETERS:
  unsigned long int profile
  unsigned long int level

RETURN VALUE:
  false it it's not supported
  true otherwise

SIDE EFFECTS:
  None.
===========================================================================*/
bool H264Utils::validate_profile_and_level(unsigned long int profile, unsigned long int level)
{
  DEBUG_PRINT("H264 profile %d, level %d\n", profile, level);
  (void) level;
  /* 7k will support baseline profile only */
  if (profile != BASELINE_PROFILE) 
  {
    return false;
  }

  return true;
}
