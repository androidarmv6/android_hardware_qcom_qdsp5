VIDEO_DIR := $(call my-dir)
include $(CLEAR_VARS)

ifeq "$(findstring msm7627,$(TARGET_PRODUCT))" "msm7627"
    include $(VIDEO_DIR)/qdsp5/vdec-omxh264/Android.mk
    MP4_VLD_DSP := true
    include $(VIDEO_DIR)/qdsp5/vdec-omxmp4/Android.mk
    include $(VIDEO_DIR)/qdsp5/vdec-omxwmv/Android.mk
    include $(VIDEO_DIR)/qdsp5/vdec-common/test/Android.mk
    include $(VIDEO_DIR)/qdsp5/venc-omx/Android.mk
endif

ifeq "$(findstring msm7625,$(TARGET_PRODUCT))" "msm7625"
    include $(VIDEO_DIR)/qdsp5/vdec-omxh264/Android.mk
    MP4_VLD_DSP := true
    include $(VIDEO_DIR)/qdsp5/vdec-omxmp4/Android.mk
    include $(VIDEO_DIR)/qdsp5/vdec-omxwmv/Android.mk
    include $(VIDEO_DIR)/qdsp5/vdec-common/test/Android.mk
    include $(VIDEO_DIR)/qdsp5/venc-omx/Android.mk
endif

ifeq "$(findstring msm7630,$(TARGET_PRODUCT))" "msm7630"
    include $(VIDEO_DIR)/vidc/firmware-720p/Android.mk
    include $(VIDEO_DIR)/vidc/venc-omx720p/Android.mk
endif

ifeq "$(findstring msm8660,$(TARGET_PRODUCT))" "msm8660"
    include $(VIDEO_DIR)/vidc/firmware-1080p/Android.mk
endif
