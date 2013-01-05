# ---------------------------------------------------------------------------------
# Copyright (c) 2007-2009 by QUALCOMM, Incorporated.  All Rights Reserved.
# QUALCOMM Proprietary and Confidential.
# ---------------------------------------------------------------------------------

ifneq ($(BUILD_TINY_ANDROID),true)

ROOT_DIR := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)

include $(LOCAL_PATH)/../vdec-common/vdec-common.cfg

# ---------------------------------------------------------------------------------
# 				Common definitons
# ---------------------------------------------------------------------------------

libmm-vdec-wmv-def := -D__align=__alignx
libmm-vdec-wmv-def += -D__alignx\(x\)=__attribute__\(\(__aligned__\(x\)\)\)
libmm-vdec-wmv-def += -g -O3
libmm-vdec-wmv-def += -DIMAGE_APPS_PROC
libmm-vdec-wmv-def += -Dinline=__inline
libmm-vdec-wmv-def += -D_ANDROID_
libmm-vdec-wmv-def += -DCDECL
libmm-vdec-wmv-def += -DNO_ARM_CLZ

# ---------------------------------------------------------------------------------
# 			Deploy the WMV static library
# ---------------------------------------------------------------------------------

LOCAL_PATH:= $(ROOT_DIR)/TL/lib
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS:= libwmvdecoder.a
include $(BUILD_MULTI_PREBUILT)

# ---------------------------------------------------------------------------------
# 			Make the Shared library (libOmxWmvDec)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)

libmm-vdec-wmv-inc	:= $(LOCAL_PATH)/TL/inc
libmm-vdec-wmv-inc	+= $(LOCAL_PATH)/PL/src
libmm-vdec-wmv-inc	+= $(LOCAL_PATH)/../vdec-common/inc
libmm-vdec-wmv-inc	+= $(TARGET_OUT_HEADERS)/mm-core/omxcore
libmm-vdec-wmv-inc	+= $(TARGET_OUT_HEADERS)/mm-core/adspsvc

LOCAL_MODULE		:= libOmxWmvDec
LOCAL_CFLAGS		:= $(libmm-vdec-wmv-def)
LOCAL_CFLAGS		+= $(VDEC_COMMON_DEF)
LOCAL_C_INCLUDES	:= $(libmm-vdec-wmv-inc)
LOCAL_PRELINK_MODULE	:= false
LOCAL_SHARED_LIBRARIES	:= liblog libutils libmm-adspsvc libbinder
LOCAL_STATIC_LIBRARIES	:= libwmvdecoder

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
LOCAL_SRC_FILES += PL/src/wmvqdsp_drv.c

include $(BUILD_SHARED_LIBRARY)

endif #BUILD_TINY_ANDROID

# ---------------------------------------------------------------------------------
# 					END
# ---------------------------------------------------------------------------------
