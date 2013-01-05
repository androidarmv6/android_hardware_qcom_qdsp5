# --------------------------------------------------------------------------------- 
# Copyright (c) 2007-2009 by QUALCOMM, Incorporated.  All Rights Reserved.
# QUALCOMM Proprietary and Confidential.
# --------------------------------------------------------------------------------- 

ifneq ($(BUILD_TINY_ANDROID),true)

ROOT_DIR := $(call my-dir)
LOCAL_PATH:= $(ROOT_DIR)

include $(LOCAL_PATH)/../vdec-common.cfg

# ---------------------------------------------------------------------------------
# 			Build the test app (mm-vdec-omx-test)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

COMMON_CFLAGS := -D_ANDROID_
COMMON_CFLAGS += $(VDEC_COMMON_DEF)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/mm-core/omxcore

LOCAL_PRELINK_MODULE 	:= false
LOCAL_CFLAGS 		:= $(COMMON_CFLAGS)
LOCAL_SRC_FILES 	:= omx_vdec_test.cpp
LOCAL_SHARED_LIBRARIES 	:= libmm-omxcore libOmxH264Dec libOmxMpeg4Dec libOmxWmvDec
LOCAL_MODULE 		:= mm-vdec-omx-test

include $(BUILD_EXECUTABLE)

endif #BUILD_TINY_ANDROID

# ---------------------------------------------------------------------------------
# 					END
# ---------------------------------------------------------------------------------
