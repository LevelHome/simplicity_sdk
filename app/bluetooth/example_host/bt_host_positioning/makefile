################################################################################
# General project settings                                                     #
################################################################################

PROJECTNAME = bt_host_positioning
PROJ_DIR := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
SDK_DIR := $(realpath $(PROJ_DIR)/../../../..)

################################################################################
# Components                                                                   #
#                                                                              #
# After setting up the toolchain, components can contribute to the project by  #
# appending items to the project variables like INCLUDEPATHS or C_SRC.         #
################################################################################

include $(SDK_DIR)/app/bluetooth/component_host/toolchain.mk
include $(SDK_DIR)/app/bluetooth/component_host/app_log.mk
include $(SDK_DIR)/app/bluetooth/component_host/app_assert.mk
include $(SDK_DIR)/app/bluetooth/component_host/app_signal.mk
include $(SDK_DIR)/app/bluetooth/component_host/cjson.mk
include $(SDK_DIR)/app/bluetooth/component_host/mqtt.mk
include $(SDK_DIR)/app/bluetooth/component_host/rtl_lib.mk
include $(SDK_DIR)/app/bluetooth/component_host/aoa_angle.mk
include $(SDK_DIR)/app/bluetooth/component_host/antenna_array.mk

################################################################################
# Include paths                                                                #
################################################################################

override INCLUDEPATHS += $(PROJ_DIR) \
$(SDK_DIR)/app/bluetooth/common_host/aoa_parse \
$(SDK_DIR)/app/bluetooth/common_host/aoa_util \
$(SDK_DIR)/app/bluetooth/common_host/angle_queue \
$(SDK_DIR)/app/bluetooth/common_host/aoa_loc \
$(SDK_DIR)/platform/common/inc


################################################################################
# Input files                                                                  #
################################################################################

override C_SRC += \
$(SDK_DIR)/app/bluetooth/common_host/aoa_parse/aoa_parse.c \
$(SDK_DIR)/app/bluetooth/common_host/aoa_parse/aoa_parse_enum.c \
$(SDK_DIR)/app/bluetooth/common_host/aoa_util/aoa_serdes.c \
$(SDK_DIR)/app/bluetooth/common_host/aoa_util/aoa_util.c \
$(SDK_DIR)/app/bluetooth/common_host/angle_queue/angle_queue.c \
$(SDK_DIR)/app/bluetooth/common_host/aoa_loc/aoa_loc.c \
$(PROJ_DIR)/app.c \
$(PROJ_DIR)/main.c

PROJ_FILES += $(wildcard $(PROJ_DIR)/config/*)


################################################################################
# Target rules                                                                 #
################################################################################

include $(SDK_DIR)/app/bluetooth/component_host/targets.mk
