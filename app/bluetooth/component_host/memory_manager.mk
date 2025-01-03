################################################################################
# Memory manager Wrapper                                                       #
################################################################################

override INCLUDEPATHS += \
$(SDK_DIR)/platform/service/memory_manager/inc \
$(SDK_DIR)/platform/service/memory_manager/config \
$(SDK_DIR)/platform/service/memory_manager/src

override C_SRC += \
$(SDK_DIR)/platform/service/memory_manager/src/sl_memory_manager.c \
$(SDK_DIR)/platform/service/memory_manager/src/sl_memory_manager_dynamic_reservation.c \
$(SDK_DIR)/platform/service/memory_manager/src/sli_memory_manager_common.c \
$(SDK_DIR)/platform/service/memory_manager/src/sl_memory_manager_region_host.c \
$(SDK_DIR)/platform/service/memory_manager/src/sl_memory_manager_pool.c

override CFLAGS += -DSL_CATALOG_MEMORY_MANAGER_PRESENT