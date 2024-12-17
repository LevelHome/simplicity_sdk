################################################################################
# Mocked Core API for Host applications.                                       #
################################################################################

override INCLUDEPATHS += \
$(SDK_DIR)/platform/common/inc

override C_SRC += \
$(SDK_DIR)/app/bluetooth/common_host/sl_core_mock/sl_core_mock.c
