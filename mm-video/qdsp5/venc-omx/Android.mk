# Copyright (c) 2007-2009 by QUALCOMM, Incorporated.  All Rights Reserved.
# QUALCOMM Proprietary and Confidential.

ROOT_DIR := $(call my-dir)

# Create the shared library
include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)
include $(LOCAL_PATH)/venc-omx.cfg
# ---------------------------------------------------------------------------------
# 				Common definitons
# ---------------------------------------------------------------------------------

COMMON_CFLAGS := \
		-g -O3 \
		-D_POSIX_SOURCE -DPOSIX_C_SOURCE=199506L \
		-D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENDED=1 \
		-D_BSD_SOURCE=1 -D_SVID_SOURCE=1 \
		-D_GNU_SOURCE \
		-DT_ARM \
		-D_ANDROID_ \
                -include stdio.h \
		$(KERNEL_HEADERS:%=-I%) \

# ---------------------------------------------------------------------------------
# 			Make the Static library if src files are present/
#                       Deploy the venc static library
# ---------------------------------------------------------------------------------
#Define BUILD_VENC_STATIC if anyone wants to generate new static Library.
#Till then the static library would be picked up from venc-omx\lib folder.
#BUILD_VENC_STATIC:=1
ifdef BUILD_VENC_STATIC

include $(CLEAR_VARS)

libmm-venc-static-inc		:= $(LOCAL_PATH)/device/inc
libmm-venc-static-inc		+= $(LOCAL_PATH)/../../../common/inc
libmm-venc-static-inc		+= $(LOCAL_PATH)/driver/inc
libmm-venc-static-inc		+= $(LOCAL_PATH)/common/inc

LOCAL_MODULE			:= venc-device-android
LOCAL_CFLAGS	  		:= $(COMMON_CFLAGS)
LOCAL_CFLAGS	  		+= $(VENC_FEATURE_DEF)
LOCAL_C_INCLUDES  		:= $(libmm-venc-static-inc)
LOCAL_PRELINK_MODULE		:= false
LOCAL_SHARED_LIBRARIES		:= liblog libutils libbinder

LOCAL_SRC_FILES		 	:= \
				device/src/qvp_bitstream.c \
				device/src/qvp_encbp.c \
				device/src/qvp_encfrm.c \
				device/src/qvp_enchdr.c \
				device/src/qvp_encir.c \
				device/src/qvp_encmb.c \
				device/src/qvp_encmv.c \
				device/src/qvp_encpred.c \
				device/src/qvp_encrc.c \
				device/src/qvp_encrc_rho.c \
				device/src/qvp_enctb.c \
				device/src/qvp_encvlc.c \
				device/src/venc_dev_api.c \
				device/src/video_enc_rc.c \
				device/src/video_enc_util.c \

include $(BUILD_STATIC_LIBRARY)

else

# Deploy the Venc static library
LOCAL_PATH:= $(ROOT_DIR)/lib
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS:= venc-device-android.a
include $(BUILD_MULTI_PREBUILT)

endif

# ---------------------------------------------------------------------------------
# 			Make the Shared library (libOmxVidEnc)
# ---------------------------------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_PATH:= $(ROOT_DIR)

libmm-venc-inc			:= $(LOCAL_PATH)/device/inc
libmm-venc-inc			+= $(LOCAL_PATH)/omx/inc
libmm-venc-inc			+= $(LOCAL_PATH)/driver/inc
libmm-venc-inc			+= $(LOCAL_PATH)/common/inc
libmm-venc-inc			+= $(TARGET_OUT_HEADERS)/mm-core/omxcore
libmm-venc-inc			+= $(LOCAL_PATH)/../../../common/inc
libmm-venc-inc			+= $(LOCAL_PATH)/../../../mm-core/adspsvc/inc

LOCAL_MODULE			:= libOmxVidEnc
LOCAL_CFLAGS	  		:= $(COMMON_CFLAGS)
LOCAL_CFLAGS	  		+= $(VENC_FEATURE_DEF)
LOCAL_C_INCLUDES  		:= $(libmm-venc-inc)
LOCAL_PRELINK_MODULE		:= false
LOCAL_SHARED_LIBRARIES		:= liblog libutils libmm-adspsvc libbinder
LOCAL_STATIC_LIBRARIES 		:= venc-device-android

LOCAL_SRC_FILES		 	:= \
				driver/src/venc_drv.c \
				omx/src/OMX_Venc.cpp \
				omx/src/OMX_VencBufferManager.cpp \
				common/src/venc_queue.c \
				common/src/venc_semaphore.c \
				common/src/venc_mutex.c \
				common/src/venc_time.c \
				common/src/venc_sleep.c \
				common/src/venc_signal.c \
				common/src/venc_file.c \
				common/src/venc_thread.c \

include $(BUILD_SHARED_LIBRARY)
# ---------------------------------------------------------------------------------
# 			Make the apps-test (mm-venc-omx-test)
# ---------------------------------------------------------------------------------
include $(CLEAR_VARS)

mm-venc-test-inc		:= $(TARGET_OUT_HEADERS)/mm-core/omxcore
mm-venc-test-inc		+= $(LOCAL_PATH)/driver/inc
mm-venc-test-inc		+= $(LOCAL_PATH)/device/inc
mm-venc-test-inc		+= $(LOCAL_PATH)/test/common/inc
mm-venc-test-inc		+= $(LOCAL_PATH)/common/inc
mm-venc-test-inc		+= $(LOCAL_PATH)/omx/inc
mm-venc-test-inc		+= $(LOCAL_PATH)/../../../common/inc

LOCAL_MODULE			:= mm-venc-omx-test
LOCAL_CFLAGS	  		:= $(COMMON_CFLAGS)
LOCAL_CFLAGS	  		+= $(VENC_FEATURE_DEF)
LOCAL_C_INCLUDES  		:= $(mm-venc-test-inc)
LOCAL_PRELINK_MODULE		:= false
LOCAL_SHARED_LIBRARIES		:= libutils libOmxCore libOmxVidEnc libmm-adspsvc libbinder 

LOCAL_SRC_FILES			:= \
				test/common/src/venctest_Config.cpp \
				test/common/src/venctest_Encoder.cpp \
				test/common/src/venctest_File.cpp \
				test/common/src/venctest_FileSink.cpp \
				test/common/src/venctest_FileSource.cpp \
				test/common/src/venctest_ITestCase.cpp \
				test/common/src/venctest_Mutex.cpp \
				test/common/src/venctest_Parser.cpp \
				test/common/src/venctest_Queue.cpp \
				test/common/src/venctest_Script.cpp \
				test/common/src/venctest_Signal.cpp \
				test/common/src/venctest_SignalQueue.cpp \
				test/common/src/venctest_Sleeper.cpp \
				test/common/src/venctest_TestCaseFactory.cpp \
				test/common/src/venctest_TestChangeQuality.cpp \
				test/common/src/venctest_TestFlush.cpp \
				test/common/src/venctest_TestGetSyntaxHdr.cpp \
				test/common/src/venctest_TestEncode.cpp \
				test/common/src/venctest_TestStateExecutingToIdle.cpp \
				test/common/src/venctest_TestStatePause.cpp \
				test/common/src/venctest_Thread.cpp \
				test/common/src/venctest_Time.cpp \
				test/app/src/venctest_App.cpp 

include $(BUILD_EXECUTABLE)


