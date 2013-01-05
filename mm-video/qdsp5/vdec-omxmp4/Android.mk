# ---------------------------------------------------------------------------------
# Copyright (c) 2007-2009 by QUALCOMM, Incorporated.  All Rights Reserved.
# QUALCOMM Proprietary and Confidential.
# ---------------------------------------------------------------------------------

ifneq ($(BUILD_TINY_ANDROID),true)

ROOT_DIR := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)

COMPILE_MP4_TL_SRC := false
COMPILE_MP4_TL_SRC := $(shell if [ -d $(ROOT_DIR)/TL/src ] ; then echo true; fi)

include $(LOCAL_PATH)/../vdec-common/vdec-common.cfg

# ---------------------------------------------------------------------------------
# 				Common definitons
# ---------------------------------------------------------------------------------

libmm-vdec-mp4-def := -D__align=__alignx
libmm-vdec-mp4-def += -D__alignx\(x\)=__attribute__\(\(__aligned__\(x\)\)\)
libmm-vdec-mp4-def += -g -O3
libmm-vdec-mp4-def += -DIMAGE_APPS_PROC
libmm-vdec-mp4-def += -Dinline=__inline
libmm-vdec-mp4-def += -D_ANDROID_
libmm-vdec-mp4-def += -DCDECL
libmm-vdec-mp4-def += -DNO_ARM_CLZ

# ---------------------------------------------------------------------------------
# 			Compile the MP4 static library
# ---------------------------------------------------------------------------------

ifeq ($(strip $(COMPILE_MP4_TL_SRC)),true)

LOCAL_CFLAGS := $(libmm-vdec-mp4-def)
LOCAL_CFLAGS += $(VDEC_COMMON_DEF)
LOCAL_CFLAGS += -DMV_OPT

LOCAL_SRC_FILES := TL/src/mp4ErrorConceal.c
LOCAL_SRC_FILES += TL/src/mp4tables.c
LOCAL_SRC_FILES += TL/src/mp4huffman.c
LOCAL_SRC_FILES += TL/src/mp4bitstream.c
LOCAL_SRC_FILES += TL/src/MP4_TL.cpp
LOCAL_SRC_FILES += TL/src/vdecoder_mp4_i.cpp
LOCAL_SRC_FILES += TL/src/partner.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/TL/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/TL/src
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../vdec-common/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/PL/inc

ifeq ($(strip $(MP4_VLD_DSP)),true)
LOCAL_MODULE := libmp4decodervlddsp
else
LOCAL_MODULE := libmp4decoder
endif #MP4_VLD_DSP

include $(BUILD_STATIC_LIBRARY)

#Uncomment the copy command, whenever any change is being made to QCELP files
#to copy the stripped version of library back to lib for customer releases
#$(shell cp -f $(TARGET_OUT_INTERMEDIATE_LIBRARIES)/$(LOCAL_MODULE) $(ROOT_DIR)/TL/lib )

# ---------------------------------------------------------------------------------
# 			Deploy the MP4 static library
# ---------------------------------------------------------------------------------

else

LOCAL_PATH:= $(ROOT_DIR)/TL/lib
include $(CLEAR_VARS)

ifeq ($(strip $(MP4_VLD_DSP)),true)
LOCAL_PREBUILT_LIBS:= libmp4decodervlddsp.a
else
LOCAL_PREBUILT_LIBS:= libmp4decoder.a
endif #MP4_VLD_DSP

include $(BUILD_MULTI_PREBUILT)

endif

# ---------------------------------------------------------------------------------
# 			Make the Shared library (libOmxMP4Dec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)

libmm-vdec-mp4-inc	:= $(LOCAL_PATH)/TL/inc
libmm-vdec-mp4-inc	+= $(LOCAL_PATH)/PL/inc
libmm-vdec-mp4-inc	+= $(LOCAL_PATH)/../vdec-common/inc
libmm-vdec-mp4-inc	+= $(TARGET_OUT_HEADERS)/mm-core/omxcore
libmm-vdec-mp4-inc	+= $(TARGET_OUT_HEADERS)/mm-core/adspsvc

LOCAL_MODULE		:= libOmxMpeg4Dec
LOCAL_CFLAGS		:= $(libmm-vdec-mp4-def)
LOCAL_CFLAGS		+= $(VDEC_COMMON_DEF)
LOCAL_C_INCLUDES	:= $(libmm-vdec-mp4-inc)
LOCAL_PRELINK_MODULE	:= false
LOCAL_SHARED_LIBRARIES	:= liblog libutils libmm-adspsvc libbinder 

ifeq ($(strip $(MP4_VLD_DSP)),true)
LOCAL_STATIC_LIBRARIES	:= libmp4decodervlddsp
else
LOCAL_STATIC_LIBRARIES	:= libmp4decoder
endif #MP4_VLD_DSP

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
LOCAL_SRC_FILES += PL/src/MP4_PAL.cpp

ifeq ($(strip $(MP4_VLD_DSP)),true)
LOCAL_SRC_FILES += PL/src/mp4_pal_vld_dsp.cpp
endif #MP4_VLD_DSP

include $(BUILD_SHARED_LIBRARY)

endif #BUILD_TINY_ANDROID

# ---------------------------------------------------------------------------------
# 					END
# ---------------------------------------------------------------------------------
