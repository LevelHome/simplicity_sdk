################################################################################
# CS Initiator Client component                                                #
################################################################################

override C_SRC += \
$(SDK_DIR)/app/bluetooth/common/cs_initiator/src/cs_initiator_client.c

override CFLAGS += \
-DSL_CATALOG_CS_INITIATOR_CLIENT_PRESENT

override INCLUDEPATHS += \
$(SDK_DIR)/app/bluetooth/common/cs_initiator/inc \
$(SDK_DIR)/app/bluetooth/common/cs_initiator/config
