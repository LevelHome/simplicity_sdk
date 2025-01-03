################################################################################
# CS ACP                                                                       #
################################################################################

override CFLAGS += \
-DSL_CATALOG_CS_ACP_PRESENT

override INCLUDEPATHS += \
$(SDK_DIR)/app/bluetooth/common/cs_acp
