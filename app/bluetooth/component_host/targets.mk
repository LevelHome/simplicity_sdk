################################################################################
# Common target rules                                                          #
################################################################################

.SUFFIXES:				# ignore builtin rules
.PHONY: all debug release pre-build clean clean_export export help

# Default directories
OBJ_DIR ?= obj
EXE_DIR ?= exe
EXPORT_DIR ?= export

CFLAGS_DEBUG ?= -O0 -g3

HELP_MESSAGE += \
"Available build targets for $(PROJECTNAME)\n" \
"  debug   - default target, optimized for debugging\n" \
"  release - build with defult compiler optimization\n" \
"  clean   - remove build directories\n" \
"  export  - copy all project resources into '$(EXPORT_DIR)' directory\n" \
"  help    - print this help message\n"

ifeq (, $(filter $(UNAME), darwin linux))
# Enable escape sequence
HELP_FLAG = -e
else
HELP_FLAG =
endif

# GATT database resources
ifneq ($(GATT_XML_FILES),)

override INCLUDEPATHS += \
$(PROJ_DIR)/autogen

override C_SRC += \
$(PROJ_DIR)/autogen/gatt_db.c

PROJ_FILES += $(wildcard $(SDK_DIR)/protocol/bluetooth/bin/gatt/*)
PROJ_FILES += $(GATT_XML_FILES)
HELP_MESSAGE += \
"  gattdb  - generate GATT database\n"

endif

# uniq is a function which removes duplicate elements from a list
uniq = $(strip $(if $1,$(firstword $1) \
       $(call uniq,$(filter-out $(firstword $1),$1))))

INCFLAGS = $(addprefix -I, $(INCLUDEPATHS))

C_FILES = $(notdir $(C_SRC) )
C_PATHS = $(call uniq, $(dir $(C_SRC) ) )

C_OBJS = $(addprefix $(OBJ_DIR)/, $(C_FILES:.c=.o))
C_DEPS = $(addprefix $(OBJ_DIR)/, $(C_FILES:.c=.d))
OBJS = $(C_OBJS)

# Project resources
INC_FILES = $(foreach dir,$(INCLUDEPATHS),$(wildcard $(dir)/*.h))
PROJ_FILES += $(call uniq, $(C_SRC) $(INC_FILES) $(MAKEFILE_LIST))
EXPORT_FILES = $(foreach file,$(PROJ_FILES),$(EXPORT_DIR)$(subst $(SDK_DIR),,$(realpath $(file))))

vpath %.c $(C_PATHS)

# Create directories and do a clean which is compatible with parallell make
$(shell mkdir $(OBJ_DIR)>$(NULLDEVICE) 2>&1)
$(shell mkdir $(EXE_DIR)>$(NULLDEVICE) 2>&1)
ifeq (clean,$(findstring clean, $(MAKECMDGOALS)))
  ifneq ($(filter $(MAKECMDGOALS),all debug release),)
    $(shell rm -rf $(OBJ_DIR)/*.*>$(NULLDEVICE) 2>&1)
    $(shell rm -rf $(EXE_DIR)/*.*>$(NULLDEVICE) 2>&1)
  endif
endif

# Check the availability of the required packages
ifneq ($(REQUIRED_PACKAGES),)
  ifneq ($(PKG_TOOL),)
    MISSING_PACKAGES := $(foreach pkg,$(REQUIRED_PACKAGES),$(shell $(PKG_TOOL) --exists $(pkg) || echo $(pkg)))
    PKGFLAGS := $(if $(filter -static, $(LDFLAGS)), --static)
    override CFLAGS += $(shell ${PKG_TOOL} --cflags $(PKGFLAGS) $(REQUIRED_PACKAGES))
    override LDFLAGS += $(shell ${PKG_TOOL} --libs $(PKGFLAGS) $(REQUIRED_PACKAGES))
  else
    MISSING_TOOL := 1
  endif
endif

# Default build is debug build
all:      debug

debug:    CFLAGS += $(CFLAGS_DEBUG)
debug:    | pre-build $(EXE_DIR)/$(PROJECTNAME)$(EXT)

release:  CFLAGS += $(CFLAGS_RELEASE)
release:  | pre-build $(EXE_DIR)/$(PROJECTNAME)$(EXT)

# Required packages are checked as a pre-build step
pre-build:
	$(if $(MISSING_TOOL),$(error Package dependencies detected but no package config tool found. Please install pkg-config or pkgconf tool using your package manager))
	$(if $(strip $(MISSING_PACKAGES)),$(error Required package(s) not found: $(MISSING_PACKAGES). Please install them using your package manager or build them from source))

# Create objects from C SRC files
$(OBJ_DIR)/%.o: %.c
	@echo "Building file: $(notdir $<)"
	$(CC) $(CFLAGS) $(INCFLAGS) -c -o $@ $<

# Link
$(EXE_DIR)/$(PROJECTNAME)$(EXT): $(OBJS) $(LIBS)
	@echo "Linking target: $@"
	$(CC) $^ $(LDFLAGS) -o $@

clean: clean_export
ifeq ($(filter $(MAKECMDGOALS),all debug release export),)
	rm -rf $(OBJ_DIR) $(EXE_DIR)
endif

clean_export:
	@if [ -d $(EXPORT_DIR) ]; then \
		read -p "Enter 'y' to remove '$(EXPORT_DIR)': " ans && if [ _$$ans = _y ]; then rm -rf $(EXPORT_DIR); fi \
	fi

# Collect project files for exporting
$(EXPORT_FILES) : $(addprefix $(EXPORT_DIR), %) : $(SDK_DIR)%
	@mkdir -p $(dir $@) && cp -pRv $< $@

export: $(EXPORT_FILES)
	@echo "Exporting done."

help:
	@echo $(HELP_FLAG) $(HELP_MESSAGE)

# GATT database targets
ifneq ($(GATT_XML_FILES),)

.PHONY: gattdb clean_gattdb

BGBUILD := $(SDK_DIR)/protocol/bluetooth/bin/gatt/bgbuild.py

# GATT XML directory
GATT_XML_DIR = $(PROJ_DIR)/config/btconf

# Generate a list of GATT XML destination files
GATT_XML_DST_FILES = $(addprefix $(GATT_XML_DIR)/,$(notdir $(GATT_XML_FILES)))

# The bgbuild tool expects the GATT XML files to be in the same folder
$(GATT_XML_DIR)/%.xml: $(filter %.xml,$(GATT_XML_FILES))
	@cp -f $< $@

clean: clean_gattdb

clean_gattdb:
	rm -rf $(GATT_XML_DIR)/*.xml

gattdb: $(GATT_XML_DST_FILES)
	@echo Generate GATT database
	@python3 $(BGBUILD) $(GATT_XML_DIR) -o $(PROJ_DIR)/autogen

$(PROJ_DIR)/autogen/gatt_db.c: $(GATT_XML_DST_FILES)
	@echo
	@echo The GATT database might be outdated. Please run: make gattdb
	@echo
endif

# include auto-generated dependency files (explicit rules)
ifneq (clean,$(findstring clean, $(MAKECMDGOALS)))
-include $(C_DEPS)
endif
