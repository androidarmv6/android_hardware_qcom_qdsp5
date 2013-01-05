/* QDSP VDEC Constants and Structures (VIDEOTASK)
**
** Copyright (c) 2007 by QUALCOMM, Incorporated.  All Rights Reserved.
*/

#ifndef _QDSP_VDEC_H_
#define _QDSP_VDEC_H_

#define QDSP_lpmCommandQueue              0
#define QDSP_mpuAfeQueue                  1
#define QDSP_mpuGraphicsCmdQueue          2
#define QDSP_mpuModmathCmdQueue           3
#define QDSP_mpuVDecCmdQueue              4
#define QDSP_mpuVDecPktQueue              5
#define QDSP_mpuVEncCmdQueue              6
#define QDSP_rxMpuDecCmdQueue             7
#define QDSP_rxMpuDecPktQueue             8
#define QDSP_txMpuEncQueue                9
#define QDSP_uPAudPPCmd1Queue             10
#define QDSP_uPAudPPCmd2Queue             11
#define QDSP_uPAudPPCmd3Queue             12
#define QDSP_uPAudPlay0BitStreamCtrlQueue 13
#define QDSP_uPAudPlay1BitStreamCtrlQueue 14
#define QDSP_uPAudPlay2BitStreamCtrlQueue 15
#define QDSP_uPAudPlay3BitStreamCtrlQueue 16
#define QDSP_uPAudPlay4BitStreamCtrlQueue 17
#define QDSP_uPAudPreProcCmdQueue         18
#define QDSP_uPAudRecBitStreamQueue       19
#define QDSP_uPAudRecCmdQueue             20
#define QDSP_uPJpegActionCmdQueue         21
#define QDSP_uPJpegCfgCmdQueue            22
#define QDSP_uPVocProcQueue               23
#define QDSP_vfeCommandQueue              24
#define QDSP_vfeCommandScaleQueue         25
#define QDSP_vfeCommandTableQueue         26

/* from CVdecDriver.cpp */
typedef struct
{
    unsigned short Cmd;
    unsigned short Reserved;
#ifndef T_WINNT
} __attribute__((packed)) VdecDriverSleepCmdType;
#else
} VdecDriverSleepCmdType;
#endif //T_WINNT

typedef struct
{
    unsigned short Cmd;
    unsigned short Reserved;
#ifndef T_WINNT
} __attribute__((packed)) VdecDriverActiveCmdType;
#else
} VdecDriverActiveCmdType;
#endif //T_WINNT

/* from h264_pal_vld.h */
typedef struct
{
    unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
    unsigned short Slice_Info_0;
    unsigned short Slice_Info_1;
    unsigned short Slice_Info_2;
    unsigned short NumBytesInRBSPHigh;
    unsigned short NumBytesInRBSPLow;
    unsigned short NumBitsInRBSPConsumed;
    unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
#ifndef T_WINNT
} __attribute__((packed)) H264PL_VLD_I_SliceHeaderPacketType;
#else
} H264PL_VLD_I_SliceHeaderPacketType;
#endif //T_WINNT

typedef struct
{
    unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
    unsigned short Slice_Info_0;
    unsigned short Slice_Info_1;
    unsigned short Slice_Info_2;
    unsigned short Slice_Info_3;
    unsigned short RefIdxMapInfo_0;
    unsigned short RefIdxMapInfo_1;
    unsigned short RefIdxMapInfo_2;
    unsigned short RefIdxMapInfo_3;
    unsigned short NumBytesInRBSPHigh;
    unsigned short NumBytesInRBSPLow;
    unsigned short NumBitsInRBSPConsumed;
    unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
#ifndef T_WINNT
} __attribute__((packed)) H264PL_VLD_P_SliceHeaderPacketType;
#else
} H264PL_VLD_P_SliceHeaderPacketType;
#endif //T_WINNT

typedef struct
{
    unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
    unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
} H264PL_VLD_EOS_FrameHeaderPacketType;

typedef struct
{
    unsigned short PacketId;            /* Should be MP4_FRAME_HEADER_PACKET_TYPE */
    unsigned short X_Dimension;         /* Width of frame in pixels               */
    unsigned short Y_Dimension;         /* Height of frame in pixels              */
    unsigned short LineWidth;           /* Line width for MSM7500                  */
    unsigned short FrameInfo_0;         /* FRAME_INFO_0 bits                       */
    unsigned short FrameBuf0High;       /* High 16 bits of VOP buffer 0, byte-addr*/
    unsigned short FrameBuf0Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf1High;       /* High 16 bits of VOP buffer 1, byte-addr*/
    unsigned short FrameBuf1Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf2High;       /* High 16 bits of VOP buffer 2, byte-addr*/
    unsigned short FrameBuf2Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf3High;       /* High 16 bits of VOP buffer 3, byte-addr*/
    unsigned short FrameBuf3Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf4High;       /* High 16 bits of VOP buffer 4, byte-addr*/
    unsigned short FrameBuf4Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf5High;       /* High 16 bits of VOP buffer 5, byte-addr*/
    unsigned short FrameBuf5Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf6High;       /* High 16 bits of VOP buffer 6, byte-addr*/
    unsigned short FrameBuf6Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf7High;       /* High 16 bits of VOP buffer 7, byte-addr*/
    unsigned short FrameBuf7Low;        /* Low 16 bits...                         */
    unsigned short FrameBuf8High;       /* High 16 bits of VOP buffer 8, byte-addr */
    unsigned short FrameBuf8Low;        /* Low 16 bits...                          */
    unsigned short FrameBuf9High;       /* High 16 bits of VOP buffer 9, byte-addr */
    unsigned short FrameBuf9Low;        /* Low 16 bits...                          */
    unsigned short FrameBuf10High;      /* High 16 bits of VOP buffer 10, byte-addr*/
    unsigned short FrameBuf10Low;       /* Low 16 bits...                          */
    unsigned short FrameBuf11High;      /* High 16 bits of VOP buffer 11, byte-addr*/
    unsigned short FrameBuf11Low;       /* Low 16 bits...                          */
    unsigned short FrameBuf12High;      /* High 16 bits of VOP buffer 12, byte-addr*/
    unsigned short FrameBuf12Low;       /* Low 16 bits...                          */
    unsigned short FrameBuf13High;      /* High 16 bits of VOP buffer 13, byte-addr*/
    unsigned short FrameBuf13Low;       /* Low 16 bits...                          */
    unsigned short FrameBuf14High;      /* High 16 bits of VOP buffer 14, byte-addr*/
    unsigned short FrameBuf14Low;       /* Low 16 bits...                          */
    unsigned short FrameBuf15High;      /* High 16 bits of VOP buffer 15, byte-addr*/
    unsigned short FrameBuf15Low;       /* Low 16 bits...                          */
    unsigned short OutputFrameBufHigh;  /* High 16 bits of Output frame,  byte-addr*/
    unsigned short OutputFrameBufLow;   /* Low 16 bits...                          */
    unsigned short EndMarker;           /* Should be MP4_MB_END_MARKER            */
#ifndef T_WINNT
} __attribute__((packed)) H264PL_VLD_FrameHeaderPacketType;
#else
} H264PL_VLD_FrameHeaderPacketType;
#endif //T_WINNT

/* from VDL_RTOS.h */
typedef struct
{
    unsigned short cmd;
    unsigned short PacketSeqNum;   /* Seq Num given to the Subframe */
    unsigned short StreamId;
    unsigned short PktSizeHi;      /* High 16 bits of Subframe Packet Size */
    unsigned short PktSizeLo;      /* Low 16 bits of Subframe Packet Size */
    unsigned short PktAddrHi;      /* High 16 bits of Packet Address */
    unsigned short PktAddrLo;      /* Low 16 bits of Packet Address  */
    unsigned short Partition;
    unsigned short Stats_PktSizeHi;      /* High 16 bits of statistics Packet Size */
    unsigned short Stats_PktSizeLo;      /* Low 16 bits of statistics Packet Size */
    unsigned short Stats_PktAddrHi;      /* High 16 bits of statistics Packet Address */
    unsigned short Stats_PktAddrLo;      /* Low 16 bits of statistics Packet Address  */
    unsigned short Stats_Partition;
    unsigned short SubframeInfo_1;   /* DM/DME exchange flag info for stats & data */ 
    unsigned short SubframeInfo_0;   /* Whether subframe is first or last, continuous etc */  
    unsigned short CodecType;
    unsigned short NumMBs;         /* Number of MBs in Subframe */
#ifndef T_WINNT
} __attribute__((packed)) VDL_QDSP_Video_Subframe_Packet_Cmd_Type;
#else
} VDL_QDSP_Video_Subframe_Packet_Cmd_Type;
#endif //T_WINNT

#endif
