################################################################################
# General project settings                                                     #
################################################################################

PROJECTNAME = bt_host_cpc_hci_bridge
PROJ_DIR := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
SDK_DIR := $(realpath $(PROJ_DIR)/../../../..)


################################################################################
# This example app requires CPC library to be installed in your environment.   #
# Follow the instructions at https://github.com/SiliconLabs/cpc-daemon.        #
################################################################################
REQUIRED_PACKAGES = libcpc


################################################################################
# Components                                                                   #
#                                                                              #
# After setting up the toolchain, components can contribute to the project by  #
# appending items to the project variables like INCLUDEPATHS or C_SRC.         #
################################################################################

include $(SDK_DIR)/app/bluetooth/component_host/toolchain.mk
include $(SDK_DIR)/app/bluetooth/component_host/app_log.mk


################################################################################
# Include paths                                                                #
################################################################################

override INCLUDEPATHS += $(PROJ_DIR) \
$(SDK_DIR)/platform/common/inc \
$(SDK_DIR)/protocol/bluetooth/bgstack/ll/utils/hci_packet/inc


################################################################################
# Input files                                                                  #
################################################################################

override C_SRC += \
$(PROJ_DIR)/main.c

# Some linux distros require libutil for PTY
override LDFLAGS += -lutil -lpthread

################################################################################
# Target rules                                                                 #
################################################################################

include $(SDK_DIR)/app/bluetooth/component_host/targets.mk
