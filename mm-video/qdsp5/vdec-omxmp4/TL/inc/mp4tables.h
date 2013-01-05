#ifndef _MP4_TABLES_H
#define _MP4_TABLES_H
/*=============================================================================
FILE:       mp4tables.h

SERVICES:   Storage module of all MP4 Tables and global arrays

DESCRIPTION:
  This one module contain most of the global tables and arrays used in MP4
  decoding.
  
  Many of the tables were generated automatically.
  
  
LIMITATIONS:


PUBLIC CLASSES:   Not Applicable


EXTERNALIZED FUNCTIONS



        Copyright © 1999-2003 QUALCOMM Incorporated.
               All Rights Reserved.
            QUALCOMM Proprietary

============================================================================*/


/* <EJECT> */
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/MPEG4Vdec/main/latest/inc/mp4tables.h#1 $
$DateTime: 2008/11/10 16:49:14 $
$Change: 781094 $

========================================================================== */
/* <EJECT> */
/* =======================================================================
**                          Macro Definitions
** ======================================================================= */
/* These macro definitions are for array bounds check in LUT */
#define MAX_INTRA_CBPC_VLD_TAB_SIZE 12
#define MAX_INTER_CBPC_VLD_TAB_SIZE 80
#define MIN_CBPCY_VLD_TAB_VALID_IDX 2
#define MAX_CBPCY_VLD_TAB_SIZE 64
#define MAX_MVD_INFO_VLD_TAB_SIZE 11
#define MAX_AC_TCOEFF_INFO_VLD_TAB_SIZE 9

/* <EJECT> */
/*===========================================================================

                TYPE DECLARATIONS FOR MODULE

===========================================================================*/
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct
{
  char mcbpc;                           /* MCBPC value */
  char  cbpc;                           /*  CBPC value */
} Mp4MCBPCType;

typedef struct
{
  unsigned char shift;              /* Shift bits (cumulative) */
  unsigned char min;                /*  minimum value (inclusive) */
  unsigned char max;                /*  maximum value (inclusive) */
  unsigned char index;
} Mp4VLCDecodeType;

/* Code Information for tables B.16 and B.17 */
typedef struct
{
  unsigned char codeLength;
  signed char last;
  signed char run;
  short level;
} Mp4TCoeffDecodeType;

/* For each leading zero value, store information for calculating LUT index */
typedef struct
{
  unsigned char mask;
  unsigned char shift;
  const Mp4TCoeffDecodeType *vlcLUTptr;
} Mp4TCoeffTableInfo;

/* For smaller VLC tables, just store Code Length and Values */
typedef struct
{
  signed char CodeLength;
  signed char DecodedValue;
} Mp4smallVLCDecode;

/* For CBP element, store Code Length, mbtype, cbpc           */
typedef struct
{
  short CodeLength;                     /* Length of VLC Code */
  char mcbpc;                           /* MCBPC value */
  char cbpc;                            /*  CBPC value */
} Mp4MCBPCDecodeType;

/* For each leading zero value, store information for calculating LUT index */
typedef struct
{
  unsigned char mask;
  unsigned char shift;
  const Mp4smallVLCDecode *vlcLUTptr;
} Mp4MVDTableInfo;

typedef struct
{
  int value;
  int size;
} Mp4TableInfoType;
//#endif
/* <EJECT> */
/*===========================================================================

                VARIABLE DECLARATIONS FOR MODULE

===========================================================================*/

/*-----------------------------------------------------------------------------
** Huffman Tables
**---------------------------------------------------------------------------*/

extern const signed short Mp4DequantizationDelta[4];

/* Used for H.263 Annex T */
extern const signed short Mp4DequantizationDelta_ModQuantLum[2][31];
extern const signed short Mp4DequantizationDelta_ModQuantChr[31];

extern signed char **Mp4LMAX[], **Mp4RMAX[];

/*
** Huffman Table for DC Coefficients.
** Tables B-14. Blocks 0, 1, 2 and 3 for Luminance. Page 344.
*/
/*{codewordLen, min, max, index}*/
extern const Mp4VLCDecodeType Mp4DCDecodeLuminance[10];
extern const unsigned char Mp4DCLuminanceCoefficients[];

/*
** Huffman Table for DC Coefficients.
** Tables B-14. Blocks 0, 1, 2 and 3 for Chrominance. Page 344.
*/
/*{codewordLen, min, max, index}*/
extern const Mp4VLCDecodeType Mp4DCDecodeChrominance[11];
extern const unsigned char Mp4DCChrominanceCoefficients[];

/*{codewordLen, min, max, index}*/
extern const Mp4VLCDecodeType Mp4DecodeMCBPCIntra[4];
extern const Mp4MCBPCType Mp4MCBPCIntra[];

/* {codewordLen, min, max, index} */
extern const Mp4VLCDecodeType Mp4DecodeMCBPCInter[11];
extern const Mp4MCBPCType Mp4MCBPCInter[];

/* {codewordLen, min, max, index} */
extern const Mp4VLCDecodeType Mp4DecodeCBPY[4];
extern const unsigned char Mp4CBPY[16];

extern const Mp4VLCDecodeType Mp4DecodeMotionVector[10];
extern const signed char Mp4MotionVector[];

extern const rvlc_last_run_level_type *Mp4DecodeRVLC[2][13];

extern const rvlc_last_run_level_type *Mp4DecodeRVLC[2][13];

extern const rvlc_last_run_level_type **Mp4VLCCoefficients[];

/* default sizes used in short header */
extern const unsigned short Mp4ShortVideoWidth[8];
extern const unsigned short Mp4ShortVideoHeight[8];

extern const Mp4TCoeffDecodeType Mp4ACInterTCoeffTab[];
extern const Mp4TCoeffDecodeType Mp4ACIntraTCoeffTab[];
extern const Mp4TCoeffTableInfo  *Mp4TCoeffTableInfoPtrs[];
extern const Mp4TCoeffTableInfo  Mp4ACInterTCoeffTabInfo[];
extern const Mp4TCoeffTableInfo  Mp4ACIntraTCoeffTabInfo[];

extern const Mp4smallVLCDecode   Mp4VLCDCDecodeLuminance[];
extern const Mp4MCBPCDecodeType  Mp4VLCIntraCBPC[];
extern const Mp4MCBPCDecodeType  Mp4VLCInterCBPC[];
extern const Mp4smallVLCDecode   Mp4VLCCBPY[];

extern const Mp4smallVLCDecode   Mp4VLCMVD[];
extern const Mp4MVDTableInfo     Mp4MVDTabInfo[];

extern const Mp4TableInfoType MbTypeTableInfo[];
extern const Mp4TableInfoType DbquantTableInfo[];
#ifdef __cplusplus
}
#endif
#endif /* _MP4_TABLES_H */
