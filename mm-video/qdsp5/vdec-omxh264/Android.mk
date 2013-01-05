# ---------------------------------------------------------------------------------
# Copyright (c) 2007-2009 by QUALCOMM, Incorporated.  All Rights Reserved.
# QUALCOMM Proprietary and Confidential.
# ---------------------------------------------------------------------------------

ifneq ($(BUILD_TINY_ANDROID),true)

ROOT_DIR := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)

COMPILE_H264_TL_SRC := false
COMPILE_H264_TL_SRC := $(shell if [ -d $(ROOT_DIR)/TL/src ] ; then echo true; fi)

include $(LOCAL_PATH)/../vdec-common/vdec-common.cfg

# ---------------------------------------------------------------------------------
# 				Common definitons
# ---------------------------------------------------------------------------------

libmm-vdec-h264-def := -D__align=__alignx
libmm-vdec-h264-def += -D__alignx\(x\)=__attribute__\(\(__aligned__\(x\)\)\)
libmm-vdec-h264-def += -g -O3
libmm-vdec-h264-def += -DIMAGE_APPS_PROC
libmm-vdec-h264-def += -Dinline=__inline
libmm-vdec-h264-def += -D_ANDROID_
libmm-vdec-h264-def += -DCDECL
libmm-vdec-h264-def += -DNO_ARM_CLZ

# ---------------------------------------------------------------------------------
# 			Compile the H264 static library
# ---------------------------------------------------------------------------------\

ifeq ($(strip $(COMPILE_H264_TL_SRC)),true)

LOCAL_MODULE := libh264decoder
LOCAL_CFLAGS := $(libmm-vdec-h264-def)
LOCAL_CFLAGS += $(VDEC_COMMON_DEF)

LOCAL_SRC_FILES  := TL/src/h264_dec_bits.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_deblock.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_init.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_mv.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_ref.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_block.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_expgolomb.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_intrapred.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_nal.cpp
LOCAL_SRC_FILES  += TL/src/h264_tl.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_cavlc.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_fmo.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_mb.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_paraset.cpp
LOCAL_SRC_FILES  += TL/src/vdecoder_h264_i.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_fruc.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_mc.cpp
LOCAL_SRC_FILES  += TL/src/h264_dec_parser.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/TL/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/TL/src
LOCAL_C_INCLUDES += $(LOCAL_PATH)/PL/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../vdec-common/inc

include $(BUILD_STATIC_LIBRARY)

#Uncomment the copy command, whenever any change is being made to QCELP files
#to copy the stripped version of library back to lib for customer releases
#$(shell cp -f $(TARGET_OUT_INTERMEDIATE_LIBRARIES)/$(LOCAL_MODULE) $(ROOT_DIR)/TL/lib )

# ---------------------------------------------------------------------------------
# 			Deploy the H264 static library
# ---------------------------------------------------------------------------------

else

LOCAL_PATH:= $(ROOT_DIR)/TL/lib
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS:= libh264decoder.a
include $(BUILD_MULTI_PREBUILT)

endif

# ---------------------------------------------------------------------------------
# 			Make the Shared library (libOmxH264Dec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)

libmm-vdec-h264-inc	:= $(LOCAL_PATH)/TL/inc
libmm-vdec-h264-inc	+= $(LOCAL_PATH)/PL/inc
libmm-vdec-h264-inc	+= $(LOCAL_PATH)/../vdec-common/inc
libmm-vdec-h264-inc	+= $(TARGET_OUT_HEADERS)/mm-core/omxcore
libmm-vdec-h264-inc	+= $(TARGET_OUT_HEADERS)/mm-core/adspsvc

LOCAL_MODULE		:= libOmxH264Dec
LOCAL_CFLAGS		:= $(libmm-vdec-h264-def)
LOCAL_CFLAGS		+= $(VDEC_COMMON_DEF)
LOCAL_C_INCLUDES	:= $(libmm-vdec-h264-inc)
LOCAL_PRELINK_MODULE	:= false
LOCAL_SHARED_LIBRARIES	:= liblog libutils libmm-adspsvc libbinder
LOCAL_STATIC_LIBRARIES	:= libh264decoder

LOCAL_SRC_FILES := ../vdec-common/src/queue.c
LOCAL_SRC_FILES += ../vdec-common/src/pmem.c
LOCAL_SRC_FILES += ../vdec-common/src/qutility.c
LOCAL_SRC_FILES += ../vdec-common/src/qtvsystem.cpp
LOCAL_SRC_FILES += ../vdec-common/src/vdecoder_i.cpp
LOCAL_SRC_FILES += ../vdec-common/src/VDL_RTOS.cpp
LOCAL_SRC_FILES += ../vdec-common/src/VDL_API.cpp
LOCAL_SRC_FILES += ../vdec-common/src/vdl.cpp
LOCAL_SRC_FILES += ../vdec-common/src/vdec.cpp
LOCAL_SRC_FILES += ../vdec-common/src/vdecoder_utils.cpp
LOCAL_SRC_FILES += ../vdec-common/src/frame_buffer_pool.cpp
LOCAL_SRC_FILES += ../vdec-common/src/omx_vdec.cpp
LOCAL_SRC_FILES += ../vdec-common/src/OmxUtils.cpp
LOCAL_SRC_FILES += ../vdec-common/src/MP4_Utils.cpp
LOCAL_SRC_FILES += PL/src/h264_pal.cpp
LOCAL_SRC_FILES += PL/src/h264_pal_vld.cpp

include $(BUILD_SHARED_LIBRARY)

endif #BUILD_TINY_ANDROID

# ---------------------------------------------------------------------------------
# 					END
# ---------------------------------------------------------------------------------
