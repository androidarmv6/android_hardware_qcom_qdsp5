/*=============================================================================
FILE:       MP4Errors.h

SERVICES:   List of all possible error codes returned by the MP4 decoder
   routines.

DESCRIPTION:


LIMITATIONS:


ABSTRACT:



PUBLIC CLASSES:   Not Applicable

EXTERNALIZED FUNCTIONS


INITIALIZATION AND SEQUENCING REQUIREMENTS:


        Copyright © 1999-2003 QUALCOMM Incorporated.
               All Rights Reserved.
            QUALCOMM Proprietary

============================================================================*/
/* =======================================================================
                             Edit History

$Header: //source/qcom/qct/multimedia2/Video/Dec/MPEG4Vdec/main/latest/inc/MP4Errors.h#1 $
$DateTime: 2008/11/10 16:49:14 $
$Change: 781094 $

========================================================================== */

/* <EJECT> */
/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header:
  4-29-03  wyh    Code Review based on original code by Bill H. and Hari G.

===========================================================================*/

#ifndef _MP4ERRORS_H
#define _MP4ERRORS_H


#define MP4ERROR_SUCCESS     0          /* Normal. Yippeee. */

#define MP4ERROR_FAIL       -1
/*
** New Error conditions for concealement
*/
#define MP4_ERROR_I_SLICE_1ST_PARTITION -10
#define MP4_ERROR_I_SLICE_2ND_PARTITION -11
#define MP4_ERROR_P_SLICE_1ST_PARTITION -20
#define MP4_ERROR_P_SLICE_2ND_PARTITION -21
#define MP4_ERROR_TEXTURE_DATA          -30
/*
 * Type 1 Errors. These occur before DCMarker in I-VOP
 */
/* Error while decoding MCBPC for an IVOP*/
#define MP4ERROR_ILL_MCBPC_I                 -100
/* Illegal stuffing bits in MCBPC decoding*/
#define MP4ERROR_ILL_MCBPC_STUFFING_I        -101
/* Num. of macro blocks read does not match nMB in slice*/
#define MP4ERROR_BAD_NUM_MB_I                -102
/* Illegal Huffman code for DCCoeff*/
#define MP4ERROR_ILL_DCCOEFF_I               -103
/* Mismatch in header and HEC*/
#define MP4ERROR_BAD_HEC_I                   -104
/* nMB is not 0 to 99*/
#define MP4ERROR_ILL_MBNUM_I                 -105
/* Used in I and P VOP*/
#define MP4ERROR_MISSING_MARKERBIT_HEADER_I  -106
/* When DC Coeff length>8 bits*/
#define MP4ERROR_MISSING_MARKERBIT_DCSIZE    -107
/* Used in header extension*/
#define MP4ERROR_MISSING_MARKERBIT_HEC_I     -108
/* nr. of MB's decoded != nr. of RVLC MB's decoded*/
#define MP4ERROR_BAD_NMBs_I                  -109
/* IVOP with MBMode != INTRA or INTRAQ*/
#define MP4ERROR_ILL_IVOP_MBMODE             -110

/* Used in I and P VOP*/
#define MP4ERROR_MISSING_MARKERBIT_HEADER    -120
/* Mismatch in header and HEC*/
#define MP4ERROR_BAD_HEC                     -121
/* Used in header extension*/
#define MP4ERROR_MISSING_MARKERBIT_HEC       -122
/* Used in header extension*/
#define MP4ERROR_BAD_NEXT_MB_NUM             -123
/* Illegal Huffman code for DCCoeff*/
#define MP4ERROR_ILL_DCCOEFF                 -124
/* non-zero stuffing bits in the resync marker of short header*/
#define MP4ERROR_INVALID_SH_ZERO_STUFFING    -125
/* short head resync marker error*/
#define MP4ERROR_SH_RESYNC_MARKER_ERROR      -126
/* Error while decoding MVD*/
#define MP4ERROR_ILL_MVD                     -127

/*
 * Type 2 Errors. These occur in the 2nd partition in an I-VOP
 */
/* Failed to detect DC marker in IVOP*/
#define MP4ERROR_MISSING_DC_MARKER           -200
/* Error while decoding CBPY in an I-VOP.*/
#define MP4ERROR_ILL_CBPY_I                  -201
/* Illegal RVLC code for DCT coefficients*/
#define MP4ERROR_ILL_RVLC_I                  -202
/* Num.of macro blocks in 2nd partition does not match nMB in slice*/
#define MP4ERROR_BAD_NUM_CPBY_I              -203
/* Num.of macro blocks in 2nd partition does not match nMB in slice*/
#define MP4ERROR_BAD_NUM_ACPRED              -204
/* Number of DCT coeffs read is not 63*/
#define MP4ERROR_BAD_NUM_DCT_I               -205
/* RVLC ESCAPE Coding*/
#define MP4ERROR_MISSING_MARKERBIT_RVLC_I    -206
/* error reading the VOP start code*/
#define MP4ERROR_MISSING_RESYNCH_MARKER_I    -207
/* error reading resynch marker*/
#define MP4ERROR_MISSING_VOP_START_CODE_I    -208

/*
 * Type 3 Errors. These occur before Motion Marker in P-VOP.
 */
/* Error while decoding MCBPC for an PVOP*/
#define MP4ERROR_ILL_MCBPC_P                 -300
/* Illegal stuffing bits in MCBPC decoding*/
#define MP4ERROR_ILL_MCBPC_STUFFING_P        -301
/* nr. of MB's decoded != nr. of RVLC MB's decoded*/
#define MP4ERROR_BAD_NMBs_P                  -308
/* Num. of macro blocks read does not match nMB in slice*/
#define MP4ERROR_BAD_NUM_MB_P                -302
/* Illegal Huffman code for Motion Vector*/
#define MP4ERROR_ILL_MOTIONVECTOR            -303
/* Mismatch in header and HEC*/
#define MP4ERROR_BAD_HEC_P                   -304
/* nMB is greater than the maximum supported, defined by MAX_MB*/
#define MP4ERROR_ILL_MBNUM_P                 -305
/* Used in I and P VOP*/
#define MP4ERROR_MISSING_MARKERBIT_HEADER_P  -306
/* Used in header extension*/
#define MP4ERROR_MISSING_MARKERBIT_HEC_P     -307

/*
 * Type 4 Erors. These occur in the 2nd partition in a P-VOP.
 */
/* Failed to detect Motion marker in PVOP*/
#define MP4ERROR_MISSING_MOTION_MARKER       -400
/* Error while decoding CBPY in a PVOP.*/
#define MP4ERROR_ILL_CBPY_P                  -401
/* Illegal RVLC code for DCT coefficients*/
#define MP4ERROR_ILL_RVLC_P                  -402
/* Num.of macro blocks in 2nd partition does not match nMB in slice*/
#define MP4ERROR_BAD_NUM_CPBY_P              -403
/* Number of DCT coeffs read is not 64*/
#define MP4ERROR_BAD_NUM_DCT_P               -404
/* Illegal DCCoeff in PVOP decoding*/
#define MP4ERROR_ILL_DCCOEFF_P               -408
/* RVLC ESCAPE Coding*/
#define MP4ERROR_MISSING_MARKERBIT_RVLC_P    -405
/* When DC Coeff length>8 bits*/
#define MP4ERROR_MISSING_MARKERBIT_DCSIZE_P  -409
/* error reading the VOP start code*/
#define MP4ERROR_MISSING_RESYNCH_MARKER_P    -406
/* error reading resynch marker*/
#define MP4ERROR_MISSING_VOP_START_CODE_P    -407

/*
 * Type 5 errors. These occur in the headers at higher layers.
 */
/* Used in all VLC and FLC*/
#define MP4ERROR_MISSING_MARKER_TIMEBASE     -501
/* Error decoding INTRADC pred. size*/
#define MP4ERROR_INTRADC_PRED_SIZE           -502
/* group start code missing from the stream*/
#define MP4ERROR_MISSING_GROUP_START         -503
/* error condition in GOV reading*/
#define MP4ERROR_OPEN_GOV_BROKEN_LINK        -504
/* GOV asked to read user data but the filename is empty*/
#define MP4ERROR_NO_USER_DATA_FILENAME       -505
/* GOV's user data file open failed*/
#define MP4ERROR_GOV_USERFILE_OPEN           -506
/* decoding the time_code in GOV (p120) can't find marker*/
#define MP4ERROR_GOV_MISSING_MARKER          -507
/* Illegal ACTerm when using short header (H.263)*/
#define MP4ERROR_ILL_ACTERM_SHORT_HEADER    -508

/*
 * The following errors may require special handling. To be verified.
 */
/* VOP that is neither an I nor P type*/
#define MP4ERROR_NON_I_P_B_VOP               -1
#define MP4ERROR_NON_I_P_VOP                 -1
/* VOP-header is uncoded ... skip this VOP*/
#define MP4ERROR_UNCODED_VOP                 -2
/* Error in decoding time_code in GOV*/
#define MP4ERROR_TIME_CODE_ERROR             -3
#define MP4ERROR_INCORRECT_STUFFING          -4

/* The video packet that contains VOP header is corrupted */
#define MP4ERROR_BAD_VOP_HEADER              -5

/* Error code grouping is not done for the following.*/
/* These are the errors when we bail out. i.e. we do not attempt any error concealement.*/
/* not implemented*/
#define MP4ERROR_VOP_COMPLEXITY_ESTIMATE     -1001
/* arbitrary shape in VOP header decoding*/
#define MP4ERROR_VOP_ARBITRARY_SHAPE         -1002
#define MP4ERROR_MB_OBMC                     -1003
/* H263 dequantization*/
#define MP4ERROR_H263BLOCK_DEQUANT           -1004
/* bitstream read has reached the end of bitstream sequence*/
#define MP4ERROR_BITSTREAM_EOF               -1005
/* The PredictionType is Spirt, which is not handled yet*/
#define MP4ERROR_SPRITE_VOP_NG               -1006
/**/
#define MP4ERROR_VOP_INTERLACED              -1007
#define MP4ERROR_VOP_GREY_SCALE              -1008
/* fatal ... VOL header is not implemented:*/
#define MP4ERROR_INVALID_VOLTYPE             -1009
/* fatal ... aspect ratio requested*/
#define MP4ERROR_INVALID_ASPECT_RATIO        -1010
/* fatal*/
#define MP4ERROR_CHROMA_FORMAT_NOT_4_2_0     -1011
/* fatal. cannot handle B-VOP's*/
#define MP4ERROR_CANT_HANDLE_BVOP            -1012
#define MP4ERROR_MAX_VBVBUFFERSIZE_EXCEEDED  -1013
/* fatal. Non-rectangular shape*/
#define MP4ERROR_NON_RECTANGULAR_SHAPE       -1014
/* frame rate greater than 15*/
#define MP4ERROR_FRAME_RATE_TOO_HIGH         -1015
/* VOL header requested quarter sample*/
#define MP4ERROR_QUART_SAMP_NOT_IMPLEMENTED  -1017
/* VOL header requested interlaced*/
#define MP4ERROR_INTERLACED_NOT_IMPLEMENTED  -1018
/* VOL header called for OBMC*/
#define MP4ERROR_OBMC_NOT_IMPLEMENTED        -1019
/* VOL header called for Sprite Usage*/
#define MP4ERROR_SPRINT_NOT_IMPLEMENTED      -1020
#define MP4ERROR_NEWPRED_NOT_IMPLEMENTED     -1021
#define MP4ERROR_SCALABILITY_NOT_IMPLEMENTED -1022
#define MP4ERROR_SHORT_HEADER_NOT_IMPLEMENTED -1023
/* while decoding VOL header, invalid bits_per_pixel encountered*/
#define MP4ERROR_INVALID_BITS_PER_PIXEL      -1024
/* administrative ... bitstream filename is probably bad*/
#define MP4ERROR_CANT_OPEN_BITSTREAM_FILE    -1025
/* administrative:  trace file writing error;*/
#define MP4ERROR_TRACE_FILE_ERROR            -1026
/* while decoding VOL header, invalid quant precision encountered*/
#define MP4ERROR_INVALID_QUANT_PRECISION     -1027

#define MP4ERROR_MISSING_MARKERBIT_VOLHEADER -1028
/* We can only handle a max of SIMPLE_L1_MAX_BITRATE*/
#define MP4ERROR_MAX_BITRATE_EXCEEDED            -1029
/**/
#define MP4ERROR_INVALID_VBV_OCCUPANCY           -1030
/* memory for max nr. of RVLC's in ARM exceeded*/
#define MP4ERROR_MAX_NRVLCS_EXCEEDED             -1040
/* fatal*/
#define MP4ERROR_CLOSED_GOV                      -1041
/* max MB = 99 for QCIF (defined by MAX_MB #define'd in qcmpeg4.h) */
#define MP4ERROR_MAX_MB_EXCEEDED                 -1042
/* MAX_PIXELS = 176*144 defined in qcmpeg4.h */
#define MP4ERROR_MAX_PIXELS_EXCEEDED             -1043
/* Quant is moved beyond the limits*/
#define MP4ERROR_ILL_VOPQUANT                    -1044
/* set by sVOL.uiQuantPrecision*/
/* (cf p. 132 of Standard)*/
/* DQUANT not in [0,3] (Tab 6-27, p.132)*/
#define MP4ERROR_ILL_DQUANT                      -1045
/* Current QCOM implementation requires
 * the following: Data Part., RVLC, ErrorResilience
 * MAX_MBROW #defined in qcmpeg4.h exceeded
 */
#define MP4ERROR_VOL_ERRPARTMODE                    -1046
#define MP4ERROR_MAX_MBROW_EXCEEDED                 -1047
#define MP4ERROR_MISSING_MARKERBIT_SHORTVIDEOHEADER -1048
#define MP4ERROR_MISSING_ZEROBIT_SHORTVIDEOHEADER   -1049
#define MP4ERROR_ILL_PLUSPTYPE_SHORTVIDEOHEADER	    -1050
#define MP4ERROR_BAD_ANNEXK_SHORTVIDEOHEADER        -1051
#define MP4ERROR_BAD_SHORTVIDEOHEADER               -1052
#define MP4ERROR_RESERVED_SHORTVIDEO_SOURCE_FORMAT  -1059
#define MP4ERROR_END_OF_BITSTREAM                   -1060
#define MP4ERROR_BITSTREAM_READ_ERROR               -1061
/* fpos exceed rpos or vise versa */
#define MP4ERROR_SLICE_BUFFER_OVERRUN               -1062
#define MP4ERROR_BITSTREAM_OPEN_FATAL               -1063
#define MP4ERROR_START_CODE_NOT_FOUND               -1064
#define MP4ERROR_UNKNOWN_MARKER_FOUND               -1065
#define MP4ERROR_BAD_TIME_BASE                      -1066
/* iGetFrame() generated
 * frame too large for sMp4Slice->ucMp4bits
 */
#define MP4ERROR_FRAME_TOO_LARGE                    -1066

#define MP4ERROR_RVLC_SPACE_GONE                    -1067  /* memory for RVLC's exhaused */

/* New error codes for Extended PTYPE (PLUSPTYPE) options
*/
#define MP4ERROR_UNSUPPORTED_UFEP                   -1068
#define MP4ERROR_UNSUPPORTED_SOURCE_FORMAT          -1069
#ifdef FEATURE_MP4_H263_ANNEX_K
#define MP4ERROR_BAD_MBA                            -1070
#define MP4ERROR_BAD_SEPB1                          -1071
#define MP4ERROR_BAD_SEPB2                          -1072
#define MP4ERROR_RS_UNSUPPORTED                     -1073
#define MP4ERROR_ASO_UNSUPPORTED                    -1074
#endif /* FEATURE_MP4_H263_ANNEX_K */
#ifdef FEATURE_MP4_H263_ANNEX_I
#define MP4ERROR_ILL_INTRA_MODE                     -1080
#endif /* FEATURE_MP4_H263_ANNEX_I */

/* New error codes to support recovery from fatal errors in the decoder */
#define MP4ERROR_ILL_TIME_INCREMENT_RESOLUTION      -1080
#define MP4ERROR_BAD_GOB_RESYNC_MARKER              -1081
#define MP4ERROR_UNSUPPORTED_PROFILE                -1082
#define MP4ERROR_BITSTREAM_FLUSH_ERROR              -1083
#define MP4ERROR_OUT_OF_MEMORY                      -1084
#define MP4ERROR_INVALID_INPUT                      -1085
#define MP4ERROR_INIT_FAILED                        -1086
#define MP4ERROR_OUT_OF_BUFFERS                     -1087

#define MP4ERROR_INVALID_PICTURE_START_CODE         -1999
#define MP4ERROR_CORRUPT_DATA                       -2001
#define MP4ERROR_FIRST_FRAME_NOT_I                  -2002
#define MP4ERROR_QDSP_NOT_IN_DECODE_STATE           -2003

/* Error codes assoiated with B frame decoding */
#define MP4ERROR_ILL_MB_TYPE                        -2010

#endif /* _MP4ERRORS_H */
