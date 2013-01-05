#ifndef MP4_TYPES_H
#define MP4_TYPES_H

/* =======================================================================
                               MP4_Types.h
DESCRIPTION
  This file defines all the types used by the MPEG-4 decoder.

Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved. 
QUALCOMM Proprietary and Confidential.
========================================================================== */

/* =======================================================================
**               Includes and Public Data Declarations
** ======================================================================= */

/* ==========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */

#include "mp4buf.h"
#include "TL_types.h"
/* ==========================================================================

                        DATA DECLARATIONS

========================================================================== */
/* -----------------------------------------------------------------------
** Constant / Define Declarations
** ----------------------------------------------------------------------- */

#define FEATURE_RTP_ERROR_CONCEAL_PATTERN

//Absolute Value
#ifndef ABS
#define ABS(x) (((x)<0) ? -(x) : (x))
#endif

#define XSCALAR_CMD_LENGTH 19
#define LCDB_CMD_LENGTH 7

// VOL setup return codes
#define MP4_SUCCESS             (0x0000)  //cmd successful
#define MP4_INVALID_VOL_PARAM   (0x0001)  // unsupported VOL parameter
#define MP4_INVALID_VOL_STATE   (0x0001)  // unsupported video version num
#define MP4_UNSUPPORTED_MODE    (0x0002)  // unsupported video mode
#define MP4_SHORT_HEADER_MODE   (0x0003) // short (H.263) header
#define MP4_FAILURE             (0xFFFF)  // cmd unsuccessful 

// Frame types
#define UNKNOWN_FRAME_TYPE   0
#define I_FRAME_TYPE         1
#define P_FRAME_TYPE         2
#define B_FRAME_TYPE         3
#define QFRE_FRAME_TYPE      4

#if defined(FEATURE_MP4_TEMPORAL_SCALABILITY)
/*  Must be >= 1. If it equals 1, then no enhancement layer may be
**  present. */
# define MAX_MP4_LAYERS 4
#else
/*  No temporal scalability, so no enhancement layers */
# define MAX_MP4_LAYERS 1
#endif /* FEATURE_MP4_TEMPORAL_SCALABILITY */

#define NBLOCKS  6     /* total nr. of blocks per macroblock */

#define NYBLOCKS 4     /* total nr. of Y blocks per macroblock */

#ifdef T_MSM7500
  //This value should always be >= DEFAULT_MAX_STATS_QUEUE_LENGTH
  #define MP4_NUM_YUV_BUFFERS 5
#else
  #define MP4_NUM_YUV_BUFFERS 8
#endif

#ifdef FEATURE_MP4_TEMPORAL_SCALABILITY
/*
** Table 7-13 (p. 244) used to decode ref_select_code
** for temporal scalability.
*/
#define REF_ENHANCEMENT_LAYER      0
#define REF_REFERENCE_LAYER        1
#define REF_NEXT_REF_LAYER         2
#define REF_SAME_TIME_REF_LAYER    3
#endif /* FEATURE_MP4_TEMPORAL_SCALABILITY */

#ifdef FEATURE_MP4_H263_PROFILE3
/* define constants for H.263 MB_INFO */
  #define MP4_DISABLE_DB_FILTER   (0x0)
  #define MP4_POST_LOOP_DB_FILTER (0x1)
  #ifdef FEATURE_MP4_H263_ANNEX_J
    #define MP4_IN_LOOP_DB_FILTER   (0x2)
  #endif /* FEATURE_MP4_H263_ANNEX_J */
#endif /* FEATURE_MP4_H263_PROFILE3 */
#define MP4_TIMESTAMP_INVALID   0xFFFFFFFF

typedef signed long int MP4_ERROR_TYPE;

/*---------------------------------------------------------------------------
      Type Declarations
----------------------------------------------------------------------------*/

typedef struct {
  unsigned char  pending_release;          /* TRUE if this YUV is with the client and pending release. */
  unsigned long long   pts;                      /* Presentation timestamp for this frame */
  unsigned char  used_as_reference;            /* For each frame which references this YUV we increment the count. */
  VDL_Frame_Stats_Pkt_Type *pFrameStatistics; /*Used in case of VLD in DSP */
} mp4_dec_yuv_buf_info_type;

typedef struct {
  /* YUV buffer info array. There is a 1:1 mapping between this info
     and the actual buf pointer stored inside MP4CodecInfo.OutputBufInfo */
  mp4_dec_yuv_buf_info_type  bufs[MP4_NUM_YUV_BUFFERS];
  /* Critical section for thread safety */
  /* Timestamp from last released YUV buffer */
  unsigned long long                     prev_pts_released;

} mp4_dec_yuv_info_type;

/*
** Structure for returning video decoder status on request
*/
typedef struct
{
  unsigned long int   NumFrames;              /* # frames decoded in this clip */
} MP4CodecClipStatsType;

/*
** Define a structure for handling a Slice.
** This structure does book keeping of raw video bitstream
*/
typedef struct
{
  unsigned char *Mp4bits;         /* Pointer to the raw video bitstream. One frame */
  unsigned char  fIsFirstSlice;   /* TRUE if this is fist slice of the frame */
  unsigned long int startCode;       /* StartCode of the current buffer */
  unsigned long int numBytes;        /* Number of bytes from above. Don't change */

  unsigned long int FirstMB;         /* MBnum of the first MB in the slice. */

  unsigned long int NextMBnum;       /* MBnum in next Slice. */
  unsigned long int NextSlicePos;    /* Start of next slice without resync marker */

  //  unsigned short uiRvlcBegin; /* Do we need these two? Ask Bill. Hari. */
  //  unsigned short uiRvlcEnd;
  unsigned long int Fpos;            /* forward bit position */
  unsigned long int FBits;           /* no. of forward bits left in the buffer */
  unsigned long int bitBuf;          /* Internal bit buffer for forward reading */
  unsigned long int Rpos;            /* Reverse bit position */
  unsigned long int RBits;           /* no. of bits in bitBufRev already used */
  unsigned long int bitBufRev;       /* Internal bit buffer for REVERSE reading */
  unsigned long int MarkerEndPos;    /* last bit of DC/Motion marker PLUS 1. */

  unsigned char  fResyncMarkerFound;/* we found a resync marker in this slice */
  unsigned char  fDC_MotionMarkerFound;  /* Found a DC/Motion marker in this slice */
#ifdef FEATURE_RTP_ERROR_CONCEAL_PATTERN
  unsigned long int Spos;            /* To track the dropped packets from RTP */
  unsigned char  fGotDroppedPacket; /* To track the dropped packets from RTP */
#endif
      //short block[64];
  unsigned long int MarkerStartPos;
} mp4_slice_type;
/*
** Define the motion vector array
*/
typedef struct MotionVectorType_t{
  signed short x;                /* x motion vectors 0-3 */
  signed short y;                /* y motion vectors 0-3 */
}
#ifndef T_WINNT
MotionVectorType[4];
//__attribute__((packed)) MotionVectorType[4];
#else
MotionVectorType[4];
#endif //T_WINNT

typedef struct MotionVector
{
    signed short x, y;
} MotionVector;
#ifdef MV_OPT
typedef struct {
  signed short x;                /* x motion vector      */
  signed short y;                /* y motion vector      */
} MVType;


typedef struct {
  MVType top;
  MVType topright;
} MVLineHistType;
	
typedef struct {
  MVType topleft;
  MVType bottomleft;
} MVColumnHistType;

typedef struct {
  MVLineHistType   mvlineHistBuf[MP4_MAX_DECODE_WIDTH / 16];
  MVColumnHistType mvcolumnHistBuf;
} MVHistType;

#endif

/*
** Define a structure for a single Macro Block.
** All the information needed (except for YUV[t-1]) for reconstructing
** a single macroblock
*/
typedef struct
{
  unsigned char fReverse;  /* denotes that RVLCs are in reverse */
  unsigned char fACPred;   /* 0 or 1 */
  unsigned char    cMBType;  /* Macroblocktype. table B-1 p. 338 */
  unsigned char fNotCoded; /* Page 131. Indicates that MB is not coded.*/
                     /* all MV and DCT are 0. Type is Inter, so that */
                     /* previous MB is copied. Also used in Error Resilience*/
  unsigned char fStartSlice;    /* indicates first MB in a slice */

  MotionVectorType MV;    /* x and y motion vectors */
#ifdef FEATURE_MPEG4_B_FRAMES
  MotionVectorType BMV;   // try to union BMX with ordinary MV
  MotionVectorType FMV;   // so that they use the same address
#endif

  //unsigned short uiDCScalerChr;   /* chrominance part of Table 7-1, p. 174 */
  //unsigned short uiDCScalerLum;   /* luminance part of Table 7-1, p. 174 */

  char fSwitched;         /* VLC DC-table switching is implemented according */
                          /*    to Tab 6-21, p. 122 */
  unsigned short uiVOPQuant;      /* VOP quantization parameter */
  unsigned short uiVOPQuantChr;   /* VOP quantization paremeter for Chrominance
                           * special for modified quantization for H.263 Annex T
               */
#ifdef FEATURE_MP4_H263_ANNEX_I
  unsigned short intraMode;       /* Indicates whether MB is of type INTER (none), */
                          /* INTRA, or INTRA (w/ Advanced INTRA Coding) */
#endif /* FEATURE_MP4_H263_ANNEX_I */
} MB_type ;
/* Decoded VLC values in MPEG4 consistsof last, run and level
** The decoder routines use this since this is how the AC components
** are coded.  However, these values are painfully transfered to
** rvlc_run_level_type before passed to QDSP
**
*/
typedef struct
{
  signed char  last;   /* RVLC entry "last" flag */
  signed char  run;    /* RVLC entry "run" length */
  short        level;  /* RVLC entry "level" value */
} rvlc_last_run_level_type;

#if defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET)
typedef struct
{
  unsigned char first_run; /* Do not use this field to store size, it is used for the first run in the block */
  unsigned char count;
} rvlc_count_type;

typedef struct {
  signed short x;                /* x motion vectors 0-3 */
  signed short y;                /* y motion vectors 0-3 */
} OneMotionVectorType[1];

/* 
 * WARNING - Do NOT indiscriminately change the order of words in these structures,
 * it will affect packing of DSP slice buffer.
 */
typedef struct
{
  unsigned short   NumWords;         /* number of words for this macro block */
                              /* includes the ending MB marker */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
  unsigned short   PacketId;         /* Type of packet, = MPEG4 MB packet */
#endif
  unsigned short   MBInfo;           /* macro block information */
  unsigned short   VOPQuantScale;    /* quantizer scale */
} MP4QDSPIntraMBHeaderType;

typedef struct
{
  unsigned short   NumWords;         /* number of words for this macro block */
                              /* includes the ending MB marker */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
  unsigned short   PacketId;         /* Type of packet, = MPEG4 MB packet */
#endif
  unsigned short   MBInfo;           /* macro block information */
  OneMotionVectorType MV;
  unsigned short   VOPQuantScale;    /* quantizer scale */
} MP4QDSPInter1MVMBHeaderType;

typedef struct
{
  unsigned short   NumWords;         /* number of words for this macro block */
                              /* includes the ending MB marker */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
  unsigned short   PacketId;         /* Type of packet, = MPEG4 MB packet */
#endif
  unsigned short   MBInfo;           /* macro block information */
  MotionVectorType MV;
  unsigned short   VOPQuantScale;    /* quantizer scale */
} MP4QDSPInter4MVMBHeaderType;
#endif /* defined (FEATURE_QDSP_RTOS) && defined (FEATURE_MPEG4_COMPACT_PACKET) */

#ifdef FEATURE_MPEG4_B_FRAMES

typedef struct {
  signed short Fx;                /* x motion vectors 0-3 */
  signed short Fy;                /* y motion vectors 0-3 */
  signed short Bx;                /* x motion vectors 0-3 */
  signed short By;                /* y motion vectors 0-3 */
} BMotionVectorType;

typedef struct
{
  unsigned short   NumWords;         /* number of words for this macro block */
                              /* includes the ending MB marker */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
  unsigned short   PacketId;         /* Type of packet, = MPEG4 MB packet */
#endif
  unsigned short   MBInfo;           /* macro block information */
  BMotionVectorType MV;
} MP4QDSPB1MVMBHeaderType;

typedef struct
{
  unsigned short   NumWords;         /* number of words for this macro block */
                              /* includes the ending MB marker */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
  unsigned short   PacketId;         /* Type of packet, = MPEG4 MB packet */
#endif
  unsigned short   MBInfo;           /* macro block information */
  BMotionVectorType MV;
  unsigned short   VOPQuantScale;    /* quantizer scale */
  unsigned short   DCScalerLum;      /* dc scale over 4 luminance block */
  unsigned short   DCScalerChr;      /* dc scale over 2 chromance blocks */
} MP4QDSPB1MVQuantMBHeaderType;

typedef struct
{
  unsigned short   NumWords;         /* number of words for this macro block */
                              /* includes the ending MB marker */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
  unsigned short   PacketId;         /* Type of packet, = MPEG4 MB packet */
#endif
  unsigned short   MBInfo;           /* macro block information */
  BMotionVectorType MV[4];
} MP4QDSPB4MVMBHeaderType;

typedef struct
{
  unsigned short   NumWords;         /* number of words for this macro block */
                              /* includes the ending MB marker */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
  unsigned short   PacketId;         /* Type of packet, = MPEG4 MB packet */
#endif
  unsigned short   MBInfo;           /* macro block information */
  BMotionVectorType MV[4];
  unsigned short   VOPQuantScale;    /* quantizer scale */
  unsigned short   DCScalerLum;      /* dc scale over 4 luminance block */
  unsigned short   DCScalerChr;      /* dc scale over 2 chromance blocks */
} MP4QDSPB4MVQuantMBHeaderType;


#endif /* FEATURE_MPEG4_B_FRAMES */


typedef struct
{
  unsigned short   NumWords;         /* number of words for this macro block */
                              /* includes the ending MB marker */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
  unsigned short   PacketId;         /* Type of packet, = MPEG4 MB packet */
#endif
  unsigned short   MBInfo;           /* macro block information */
  MotionVectorType MV;
  unsigned short   VOPQuantScale;    /* quantizer scale */
  unsigned short   DCScalerLum;      /* dc scale over 4 luminance block */
  unsigned short   DCScalerChr;      /* dc scale over 2 chromance blocks */
} MP4QDSPMBHeaderType;

/* the structure passed to QDSP doesn't have last */
typedef struct
{
  unsigned short run;          /* RVLC entry "run" length */
  unsigned short level;        /* RVLC entry "level" value */
} rvlc_run_level_type;
/*
** Define a structure for a decoded video packet. This contains
** decodec info, that can be shipped to the QDSP. It may contain
** 1 to 99 MBs, depending on content, encoder options, delivery method, etc.
*/
typedef struct
{
  unsigned long int            FirstMB;        /* start MBnum of current slice */
  unsigned long int            MBnum;          /* first MBnum of sMB */
  unsigned long int            MBlast;         /* last MB of packet */

  // HEC information ... etc... etc...
  MB_type          MB[MAX_MB];     /* macroblock data */
  VDL_Slice_Pkt_Type *pSlice;         /* input data form PV layer */

} dsp_packet_type;
/*
** Define a codec initialization return type enumeration
*/
typedef enum
{
  MP4_INIT_SUCCESS = 0,     /* command sucessful */
//  MP4_INIT_CONFIG_SUCCESS,  /* video codec configuration complete */
#ifdef FEATURE_MP4_FRAME_TRANSFORMATIONS
  MP4_INIT_ROTATION_DONE,   /* rotation command sucessful */
  MP4_INIT_SCALING_DONE,    /* scaling command sucessful */
#endif /* FEATURE_MP4_FRAME_TRANSFORMATIONS */
  MP4_INIT_IF_VERSION,      /* unsupported interface version */
  MP4_INIT_INVALID_CB,      /* init cmd contains a NULL cb pointer */
  MP4_INIT_INVALID_BUF,     /* init cmd specs NULL YUV buf pointer */
  MP4_INIT_INVALID_STATE,   /* init cmd issued during invalid state */
  MP4_INIT_INVALID_FORMAT,  /* unsupported output format */
  MP4_INIT_OUT_OF_MEMORY,
  MP4_INIT_MAX
} MP4InitReturnType;

/*
** Define a decode frame return type enumeration
*/
typedef enum
{
  MP4_DECODE_NO_ERROR  = 0,   /* no frame error, ClipStats returned */
  MP4_DECODE_FRAME_ERROR,     /* rbg buffer contents bad, pFrameBuf == NULL */
                              /* drop output frame and continue */
  MP4_DECODE_FATAL_ERROR,     /* fatal error, restart decoder */
  MP4_DECODE_FRAME_NOT_CODED, /* frame not coded, no frame data */
  MP4_MAX_DECODE_ERRORS       /* Maximum frame decoder errors */
#ifdef FEATURE_FRE
  ,MP4_DECODE_FRAME_FRE
#endif
} MP4DecodeReturnType;

/*
** Define the Visual Object Layer structure type
*/
typedef struct
{
  unsigned char       VOLID;                    /* MP4VOLType identification */
  unsigned char     fIsShortHeader;           /* TRUE if short header frame */
  unsigned char     fRandomAccess;            /* TRUE if random access allowed */
  unsigned char       ObjectType;               /* 1=simple, 2=scalable. Else invalid */
  unsigned char     fIsObjectLayerIdentifier; /* read VerID and priority, if TRUE */
  unsigned char       VerID;
  unsigned char       Priority;
  unsigned char       AspectRatioInfo;          /* Aspect ratio == 1 */
  unsigned char       ChromaFormat;             /* only type 1 allowed. (= 4:2:0) */
  unsigned char       fLowDelay;                /* 1 = no B-VOPs allowed */
  unsigned long int      VBVPeakBitRate;           /* bit rate in units of 400 bits/sec */
  unsigned long int      VBVBufferSize;            /* in units of 16384 bits */
  unsigned long int      VBVOccupancy;             /* in units of 64 bits (ignored) */
  unsigned char       VOLShape;                 /*0=rect. Other shapes not supported*/
  unsigned short      TimeIncrementResolution;  /* Fps. Max. fps is 15 for Simple L1 */
  unsigned char       NBitsTime;                /* # bits to read time information */
                                        /* from uiTimeIncrementResolution */
  unsigned char     fFixedVOPRate;            /* if TRUE, read from stream */
  unsigned short      FixedTimeIncrement;       /* 1 or fFixedVOPRate */
  unsigned short      Height;                   /* 13 bits for height. Should be 176 */
  unsigned short      Width;                    /* 13 bits for width. Should be 144 */
  unsigned char     fInterlaced;              /* should be FALSE */
  unsigned char     fOBMCDisabled;            /* should be TRUE */
  unsigned char     fSpriteEnabled;           /* should be FALSE */
  unsigned char     fSADCTDisable;            /* should be TRUE */
  unsigned char       QuantPrecision;           /* between 3-9 (p 115).  default=5 */
  unsigned char       BitsPerPixel;             /* shouldn't be > 8. default=8 */
  unsigned short      MaxQuant;                 /* page 132 for values */
  unsigned char     fQuantType;               /* TRUE=>MPEG4, False=>H263 quant */
  unsigned char       *pIntraQMat;              /* user defined Intra quant matrix*/
  unsigned char       *pInterQMat;              /* user defined Inter quant matrix*/
  unsigned char     fQuarterSample;           /* should be FALSE */
  unsigned char     fComplexityEstimation;    /* should be TRUE */
  unsigned char     fResyncMarker;            /* TRUE if ResyncMarker is disabled. */
  unsigned char     fDataPartition;           /* TRUE if data partitioning enabled */
  unsigned char     fRVLC;                    /* TRUE if RVLC is enabled */
  unsigned char     fACDCPred;                /* TRUE if short hdr. FALSE I/P-VOP*/
  unsigned char     fNewpredEnabled;          /* NEWPRED mode for error resilience*/
  unsigned char     fReducedResolutionVOP;    /* FALSE. reduced resolution vop */
  unsigned char     fScalability;             /* TRUE for scalable streams. */
/* Scalability related definitions in MP4VOLType. For future enhancements */
  unsigned char       ScalHierachyType;         /* 0 = spacial, 1=temporal */
  unsigned char       ScalRefLayerID;
  unsigned char     fScalRefLayerSamplingDir; /* 1 = high res ref.layer */
  unsigned char       ScalHorSamplingFactorNum;
  unsigned char       ScalHorSamplingFactorDenom;
  unsigned char       ScalVertSamplingFactorNum;
  unsigned char       ScalVertSamplingFactorDenom;
  unsigned char     fScalEnhancementType;     /* 1=enhancement layer */
  unsigned char     fScalUseRefShape;
  unsigned char     fScalUseRefTexture;
  unsigned char       ScalShapeHorSamplingFacNum;
  unsigned char       ScalShapeHorSamplingFacDenom;
  unsigned char       ScalShapeVertSamplingFacNum;
  unsigned char       ScalShapeVertSamplingFacDenom;
  unsigned long int      TimeIncrementResDivisorQ16; /* 1000/FixedTimeIncrement Q16 */
#ifdef FEATURE_MP4_TEMPORAL_SCALABILITY

  unsigned char     hierarchy_type;            /* 0=spatial, 1=temporal */
                                         /* cf. p. 119 of standard */
  unsigned char       ref_layer_id;
  unsigned char     ref_layer_sampling_direc;  /* 1 = high res ref.layer */
  unsigned char       hor_sampling_factor_n;
  unsigned char       hor_sampling_factor_m;
  unsigned char       vert_sampling_factor_n;
  unsigned char       vert_sampling_factor_m;

#endif /* FEATURE_MP4_TEMPORAL_SCALABILITY */

#ifdef FEATURE_MP4_H263_ANNEX_J
  unsigned char     fDeblockingFilter;        /* TRUE if Annex J is enabled */
#endif /* FEATURE_MP4_H263_ANNEX_J */

  unsigned char     fCustomSourceFormat;
} MP4VOLType;

/*
** Define a Group of VOP structure.
** Note, this optional header immediately preceeds a coded I-VOP indictating
** the modulo time base for the next VOP.  The TimeCode field is set to 0
** when clip decoding begins or is assumed if no GOV is present.  TimeCode
** shall be set to 0 after every call to mp4_frame_reset().
*/
typedef struct
{
  unsigned long int     TimeCode;        /* starting time in seconds */
  unsigned char   fClosedGOV;       /* Not applicable to simple profile, level 0 */
  unsigned char   fBrokenLink;      /* Not applicable to simple profile, level 0 */
} MP4GOVType;



/*
** Define a structure to contain border information of a macro block
** This Look-up Table (LUT) is populated in mp4_vol_setup
** We use this information in motion vector prediction
*/

/* Define a structure of codec information
**
** Basically almost all of the decoder global variables
** are stored here
*/

/*
** Define a structure for handling error resilience
** This has some redundant fields, due to the new mp4codec.h API
** To be cleaned up.
*/
typedef struct
{
  signed long int iErrCode1;        /* first partition error */
  signed long int iErrCode2;        /* Second partition Error */
  signed long int iRVLCForward;     /* Error in forward direction */
  signed long int iRVLCReverse;     /* Error in reverse direction */
  signed long int iErrCodeH;        /* Error in the Slice header */
  signed long int iErrCodeFrame;        /* Error in the Frame header */

  signed long int iMBnum1;         /* MB with error in 1st partition */
  signed long int iMBnum2;         /* MB with error in 2nd partition, PartA */
  signed long int iMBnum2Forward;  /* MB with error in 2nd partition in forward */
  signed long int iMBnum2Reverse;  /* MB with error in 2nd partition in forward */
  unsigned char  fNoMarker;       /* Indicates if Marker is missing */

  signed long int iConcealed;      /* number of macro blocks concealed in this frame */

  signed long int L, L1, L2, N1, N2, N; /* For E.1.4.4.2.1; page 423. Concealment */
} error_status_type;
typedef struct {
    int last;
    int run;
    int level;
} event_t;

struct item
{
    event_t value;
    char length;
};

typedef struct item item_t;

struct ShortVector
{
    short x;
    short y;
};

struct mv_item
{
    struct ShortVector value;
    short length;
};
typedef struct mv_item mv_item_t;
/*
** Define a structure to contain all the VOP header information
*/
typedef struct
{
  unsigned char   cPredType;            /* 0 = IVOP, 1 = PVOP  */
  unsigned char   QP;
  /* for DivX 3.11 */
  unsigned char   vol_mode;
  unsigned char	  has_skips;
  int switch_rounding;
  short (*dc_chrom_table) (mp4_slice_type*);
  short (*dc_lum_table) (mp4_slice_type*);
  int mb_xpos;
  int mb_ypos;
  short *cbp_store;
  int cbp_store_stride;
  event_t (*ac_inter_table) (mp4_slice_type*);
  event_t (*ac_intra_chrom_table) (mp4_slice_type*);
  event_t (*ac_intra_lum_table) (mp4_slice_type*);
  void (*mv_table) (mp4_slice_type*, int*, int*);
  short (*get_cbp) (mp4_slice_type*);
/* end DivX 3.11 variables */
  unsigned long int  ModTimeBase;          /* time base */
  unsigned long int  prevMTB;    /* previous modulo time base */
  unsigned long int  VOPTimeIncrement;
  unsigned long int  prevVOPTimeIncrement; /* previous time Incriment */
  unsigned char fVOPCoded;            /* coded flag */

#ifdef FEATURE_MPEG4_B_FRAMES
  unsigned long int  TimeBase;
  unsigned long int  PreviousTimeBase;
  unsigned long int  previousDisplayTime;
  unsigned long int  nextDisplayTime;
#endif

  /* 1 indicates rounding_control=1 used for pixel  */
  char fRoundingType;
  /* interpolation in motion compensation  */
  char cIntraDCVLCThr;
  unsigned short uiQuant;       /* starting quantization param */
  char cFcodeForward;           /* for decoding motion vectors */
#ifdef FEATURE_MPEG4_B_FRAMES
  char cFcodeBackward;
#endif
  /* this is computed at the end of VOP decoding  */
  char cResyncMarkerLength;

  /* = IntraDCVLCThr*2+11 per Tab 6-21 p.122  */
  signed long int lIntraDCSwitchThreshold;

  unsigned short TotalMB;               /* tot nr. of MBs (= 99 for QCIF) */

  /* nr. of bits for macroblock_number in */
  unsigned char ucMBLength;

  char fUseIntraDcVLC;          /* flag, computed from IntraDCThresh */

#ifdef FEATURE_MP4_TEMPORAL_SCALABILITY
  unsigned char ref_select_code;  /* p124 */
#endif /* FEATURE_MP4_TEMPORAL_SCALABILITY */

  /* The following features are signalled in the PLUSPTYPE part of the H.263
  ** picture header.
  */
#ifdef FEATURE_MP4_H263_ANNEX_K
  unsigned char fSliceStructuredMode;      /* TRUE if Annex K is enabled */
#endif /* FEATURE_MP4_H263_ANNEX_K */
#ifdef FEATURE_MP4_H263_ANNEX_I
  unsigned char fAdvancedIntraCodingMode;  /* TRUE if Annex I is enabled */
#endif
#ifdef FEATURE_MP4_H263_ANNEX_J
  unsigned char fDeblockingFilter;         /* TRUE if Annex J is enabled */
#endif

  unsigned char fModifiedQuantizationMode; /* TRUE if Annex T Modified Quant mode is enabled */

  /* Used by Ericsson H.263 Annex K bitstreams.  This is not an Annex K specific
  ** parameter, thus it should not be featurized under FEATURE_MP4_H263_ANNEX_K
  */
  unsigned char fCustomPCF;

  unsigned char fCustomSourceFormat;       /* TRUE if picture uses custom source format */
} VOP_type;

#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
/*
** Define a structure for the Frame Header packet that goes to the DSP
*/
typedef struct
{
  unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
  unsigned short Width;               /* Width of frame in pixels               */
  unsigned short Height;              /* Height of frame in pixels              */
  unsigned short LineWidth;           /* Used for interlaced support            */
  unsigned short FrameInfo;           /* FRAME_INFO bits                        */
  unsigned short VopBuf0Hi;           /* High 16 bits of VOP buffer 0, byte-addr*/
  unsigned short VopBuf0Lo;           /* Low 16 bits...                         */
  unsigned short VopBuf1Hi;           /* etc..                                  */
  unsigned short VopBuf1Lo;
  unsigned short VopBuf2Hi;
  unsigned short VopBuf2Lo;
  unsigned short OutBufHi;            /* High 16 bits of output buffer          */
  unsigned short OutBufLo;            /* Low 16 bits of output buffer           */
#ifdef FEATURE_QTV_DECODER_LCDB_ENABLE
  unsigned short lcdbwords[LCDB_CMD_LENGTH];
#endif
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A_XSCALE
  unsigned short xScalarBuffer[XSCALAR_CMD_LENGTH];   /* In band xScaling*/
#endif
#ifdef PLATFORM_LTK
  unsigned short OutRGBBufHi;            /* High 16 bits of output buffer          */
  unsigned short OutRGBBufLo;            /* Low 16 bits of output buffer           */
#endif /* PLATFORM_LTK */
  unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
} MP4QDSPFrameHeaderType;
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */

#ifdef FEATURE_MP4_H263_PROFILE3
/* 
** Define the marcoblock structure for H.263 Profile 3
*/
typedef struct
{
   unsigned short   NumWords;            /* number of words for this macro block */
                                 /* includes the ending MB marker */
#ifdef FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A
   unsigned short   PacketId;            /* Should be H263P3_FRAME_HEADER_PACKET_TYPE */
#endif /* FEATURE_VIDEO_PLAYER_INTERFACE_REV_2A */
   unsigned short   MBInfo;              /* macro block information */
   MotionVectorType MV;
   unsigned short   VOPQuantScaleLuma;   /* the QUANT or modified QUANT for luma blocks */
   unsigned short   VOPQuantScaleChroma; /* the QUANT or QUANT_C for chroma blocks */
   unsigned short   AdvancedIntraPredictionMode; /* the Advanced Intra prediction mode */
} MP4QDSPH263Profile3MBHeaderType;
#endif /* FEATURE_MP4_H263_PROFILE3 */

/*
**  Define the MPEG4 audio constants
*/
typedef enum
{
  QTVQDSP_AUDIO_NONE,
  QTVQDSP_AUDIO_EVRC8K,
  QTVQDSP_AUDIO_QCELP13K,
  QTVQDSP_AUDIO_AMR,
  QTVQDSP_AUDIO_AAC,
  QTVQDSP_AUDIO_AAC_PLUS,
  QTVQDSP_AUDIO_MP3,
  QTVQDSP_AUDIO_WMA,
  QTVQDSP_AUDIO_CONC,
  QTVQDSP_AUDIO_REAL,
  QTVQDSP_AUDIO_AMR_WB,
  QTVQDSP_AUDIO_AMR_WB_PLUS,
  QTVQDSP_VIDEO_MPEG4_ENCODER,
  QTVQDSP_VIDEO_H264_ENCODER,
  QTVQDSP_AUD_OVER_ENC_QCELP13K,
  QTVQDSP_AUD_OVER_ENC_AMR,
  QTVQDSP_AUD_OVER_ENC_EVRC,
  QTVQDSP_QVP_OVER_ENC_EVRC,
  QTVQDSP_QVP_OVER_ENC_EVRC1P,
  QTVQDSP_QVP_OVER_ENC_AMR,
  QTVQDSP_QVP_OVER_ENC_AMR1P,
  QTVQDSP_QVP_OVER_ENC_4GV,
  QTVQDSP_QVP_OVER_ENC_4GV1P,
  QTVQDSP_QVP_OVER_ENC_4GVWB,
  QTVQDSP_QVP_OVER_ENC_4GVWB1P,
  QTVQDSP_AUDIO_MAX
} QtvQDSPAudioType;

typedef enum
{
  QTVQDSP_VIDEO_NONE,
  QTVQDSP_VIDEO_MPEG4,
  QTVQDSP_VIDEO_H263,
  QTVQDSP_VIDEO_H264,
  QTVQDSP_VIDEO_WMV,
  QTVQDSP_VIDEO_OSCAR,
  QTVQDSP_VIDEO_REAL,
  QTVQDSP_VIDEO_MAX
} QtvQDSPVideoType;

/* Multi-media decode command constants */
/* Same value that DL_QDSP_Video_Subframe_Packet_Cmd_Type expects.  DL
 * should eventually expose an enum here */
#define QTVQDSP_CODEC_MPEG4      (0x0000)
#define QTVQDSP_CODEC_H263       (0x0001)
#define QTVQDSP_CODEC_H264       (0x0002)
#define QTVQDSP_CODEC_H263P3     (0x0003)
#define QTVQDSP_CODEC_FRUC       (0x0007)
#define QTVQDSP_CODEC_DIVX_311   (0x0009)
//#define QTVQDSP_CODEC_WMV        (0x0003)
//#define QTVQDSP_CODEC_REAL       (0x0004)

#ifdef FEATURE_MP4_TEMPORAL_SCALABILITY
  #define MAX_LAYERS 4
#else
  #define MAX_LAYERS 1
#endif

#define START_CODE_PREFIX       0x00000100
#define START_CODE_MASK         0xFFFFFF00
#define SHORT_HEADER_MASK       0xFFFFFC00
#define LAST_BYTE_MASK          0x000000FF
#define GOV_START_CODE          0x000001B3
#define VOP_START_CODE          0x000001B6
#define SHORT_HEADER_START_CODE 0x00008000

#define MOTION_MARKER_COMB        126977
#define MOTION_MARKER_COMB_LENGTH 17

#define DC_MARKER        438273
#define DC_MARKER_LENGTH 19
#define GOB_RESYNC_UNUSED_INDICATOR 0x8000

#if defined (FEATURE_QTV_VGA_ENABLE) || defined (FEATURE_QTV_WVGA_ENABLE)
#define MAX_UCMBLENGTH     11
#else
#define MAX_UCMBLENGTH     9
#endif /* (FEATURE_QTV_VGA_ENABLE) || (FEATURE_QTV_WVGA_ENABLE) */

#define MAX_QUANTPRECISION 9
#define MIN_QUANTPRECISION 3
#define MAX_NBITSTIME      16
#define MPEG4_RESYNC_MARKER_BASE_LEN 16

#define GOB_RESYNC_MARKER_LENGTH 17
#define GOB_RESYNC_MARKER        0x1
#define GOB_FRAME_ID_LENGTH 2

#ifdef FEATURE_MP4_H263_ANNEX_K
#define SLICE_RESYNC_MARKER_LENGTH 17
#define SLICE_RESYNC_MARKER        0x1
#define SLICE_EMULATION_PREVENTION_BIT_LENGTH 1
#define SLICE_EMULATION_PREVENTION_BIT        0x1
#define SLICE_QUANT_LENGTH 5
#endif /* FEATURE_MP4_H263_ANNEX_K */


// valid decoding types (Table 6-20, p121)
#define MPEG4_IVOP 0
#define MPEG4_PVOP 1
#define MPEG4_BVOP 2
#define MPEG4_SVOP 3

// macroblock types
#define MODE_INTER   0
#define MODE_INTERQ  1
#define MODE_INTER4V 2
#define MODE_INTRA   3
#define MODE_INTRAQ  4
#define MODE_INTER4VQ 5  

/* MBCPC bit stuffing code length */
#define MCBPC_BIT_STUFFING_LENGTH 9

/* INTRA mode to determine VLC decoding method */
#define INTRA_MODE_NONE  0x00
#define INTRA_MODE_MP4   0x01
#ifdef FEATURE_MP4_H263_ANNEX_I
#define INTRA_MODE_H263I 0x02
#endif /* FEATURE_MP4_H263_ANNEX_I */

#ifdef FEATURE_MP4_H263_ANNEX_I
/* VLCs that are read from the bitstream */
#define PREDICTION_MODE_VLC_DC_ONLY    0x0000
#define PREDICTION_MODE_VLC_VERTICAL   0x0002
#define PREDICTION_MODE_VLC_HORIZONTAL 0x0003
/* values that are sent to the DSP */
#define PREDICTION_MODE_DC_ONLY        0x0000
#define PREDICTION_MODE_VERTICAL       0x0001
#ifdef FEATURE_QDSP_RTOS
  #define PREDICTION_MODE_HORIZONTAL     0x0002
#else
#define PREDICTION_MODE_HORIZONTAL     0xFFFF
#endif  /* FEATURE_QDSP_RTOS */
#endif /* FEATURE_MP4_H263_ANNEX_I */

/* Define a macro block end marker constant */
#define MP4_MB_END_MARKER       (0x7FFF)
#define QTVQDSP_PKT_END_MARKER_HI (0x7FFF)
#define QTVQDSP_PKT_END_MARKER_LO (0xFFFF)

/* modb types */
#define MODB_1                 0
#define MODB_00                1
#define MODB_01                2

// later create an enum with mb type for P & I frames - kveera
#define MB_TYPE_1              10 // direct mode
#define MB_TYPE_01             11 // bi-directional mode
#define MB_TYPE_001            12 // backward mode
#define MB_TYPE_0001           13 // forward mode

#define DIRECT                 MB_TYPE_1
#define BIDIRECTIONAL          MB_TYPE_01
#define BACKWARD               MB_TYPE_001
#define FORWARD                MB_TYPE_0001
#define DIRECT4MV              14 // use this to signal 4mv case
                                  // of direct mode else
                                  // use DIRECT which signals 1MV
                                  // case

#define INVALID_REFERENCE_INDEX -1

#endif /* MP4_TYPES_H */
