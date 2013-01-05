all:
	@echo "invoking vidc/firmware-720p make"
	$(MAKE) -f $(SRCDIR)/vidc/firmware-720p/firmware-720p.mk
	#@echo "invoking vidc/enc make"
	#$(MAKE) -f $(SRCDIR)/vidc/venc/venc.mk
	#mv $(SYSROOTLIB_DIR)/mm-venc-omx-test720p $(SYSROOTBIN_DIR)
