################################################################################
# Real-Time Locationing library component                                      #
################################################################################

ifeq (, $(filter $(MAKECMDGOALS), clean export help))
  ifeq (, $(filter $(UNAME), darwin linux))
    ifneq ($(CC), x86_64-w64-mingw32-gcc)
      $(error Toolchain not supported by RTL lib.)
    endif
  endif
endif

RTL_DIR = $(SDK_DIR)/util/silicon_labs/rtl
LIBNAME := rtl_static

override INCLUDEPATHS += $(RTL_DIR)/inc

override CFLAGS += -DRTL_LIB

ifeq ($(UNAME_M), x86_64)
DEVICE_ARCH = x86-64
else
DEVICE_ARCH = $(UNAME_M)
endif

ifeq ($(OS),posix)
override LDFLAGS += \
-L$(RTL_DIR)/lib/gcc/$(UNAME)/$(DEVICE_ARCH)/release \
-l$(LIBNAME) \
-lm \
-lstdc++ \
-lpthread
else
override LDFLAGS += \
"$(RTL_DIR)/lib/gcc/windows/$(DEVICE_ARCH)/release/lib$(LIBNAME).a" \
-lstdc++ \
-lpthread
endif

PROJ_FILES += $(wildcard $(RTL_DIR)/lib/gcc/*/*/release/lib$(LIBNAME).a)
