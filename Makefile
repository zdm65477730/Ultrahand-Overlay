##################################################################################
# Makefile for Ultrahand Overlay
# Author: ppkantorski
# Description:
#   This Makefile is used to build the Ultrahand Overlay homebrew application for
#   Nintendo Switch.
#
#   For more details and usage instructions, please refer to the project's
#   documentation and README.md.
#
#   GitHub Repository: https://github.com/ppkantorski/Ultrahand-Overlay
#
# Licensed under GPLv2
# Copyright (c) 2024 ppkantorski
##################################################################################

#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------


ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# ROMFS is the directory containing data to be added to RomFS, relative to the Makefile (Optional)
#
# NO_ICON: if set to anything, do not use icon.
# NO_NACP: if set to anything, no .nacp file is generated.
# APP_TITLE is the name of the app stored in the .nacp file (Optional)
# APP_AUTHOR is the author of the app stored in the .nacp file (Optional)
# APP_VERSION is the version of the app stored in the .nacp file (Optional)
# APP_TITLEID is the titleID of the app stored in the .nacp file (Optional)
# ICON is the filename of the icon (.jpg), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#	 - <Project name>.jpg
#	 - icon.jpg
#	 - <libnx folder>/default_icon.jpg
#
# CONFIG_JSON is the filename of the NPDM config file (.json), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#	 - <Project name>.json
#	 - config.json
#   If a JSON file is provided or autodetected, an ExeFS PFS0 (.nsp) is built instead
#   of a homebrew executable (.nro). This is intended to be used for sysmodules.
#   NACP building is skipped as well. #lib/Atmosphere-libs/libexosphere/source/pmic
#---------------------------------------------------------------------------------
APP_TITLE	:= Ultrahand
APP_AUTHOR	:= ppkantorski
APP_VERSION	:= 1.9.3
TARGET		:= ovlmenu
BUILD		:= build
SOURCES		:= source common lib/libultrahand/libultra/source
INCLUDES	:= source common include lib/libultrahand/libultra/include lib/libultrahand/libtesla/include
NO_ICON		:= 1

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH := -march=armv8-a+simd+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS := -g -Wall -Os -ffunction-sections -fdata-sections -flto -fomit-frame-pointer -finline-small-functions \
			$(ARCH) $(DEFINES)

CFLAGS += $(INCLUDE) -D__SWITCH__ -DAPPTITLE=\"$(APP_TITLE)\" -DAPP_VERSION="\"$(APP_VERSION)\"" -D_FORTIFY_SOURCE=2

# For compiling Ultrahand Overlay only
IS_LAUNCHER_DIRECTIVE := 1
CFLAGS += -DIS_LAUNCHER_DIRECTIVE=$(IS_LAUNCHER_DIRECTIVE)

# Enable Widget
USING_WIDGET_DIRECTIVE := 1  # or true
CFLAGS += -DUSING_WIDGET_DIRECTIVE=$(USING_WIDGET_DIRECTIVE)

# Enable Logging
USING_LOGGING_DIRECTIVE := 1  # or true
CFLAGS += -DUSING_LOGGING_DIRECTIVE=$(USING_LOGGING_DIRECTIVE)

USING_FPS_INDICATOR_DIRECTIVE := 0
CFLAGS += -DUSING_FPS_INDICATOR_DIRECTIVE=$(USING_FPS_INDICATOR_DIRECTIVE)

# Disable fstream
#NO_FSTREAM_DIRECTIVE := 1
#CFLAGS += -DNO_FSTREAM_DIRECTIVE=$(NO_FSTREAM_DIRECTIVE)

CXXFLAGS := $(CFLAGS) -std=c++23 -Wno-dangling-else -ffast-math -fno-unwind-tables -fno-asynchronous-unwind-tables

ASFLAGS := $(ARCH)
LDFLAGS += -specs=$(DEVKITPRO)/libnx/switch.specs $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS := -lcurl -lz -lzzip -lmbedtls -lmbedx509 -lmbedcrypto -ljansson -lnx

CXXFLAGS += -fno-exceptions -ffunction-sections -fdata-sections -fno-rtti
LDFLAGS += -Wl,--as-needed -Wl,--gc-sections

# For Ensuring Parallel LTRANS Jobs w/ GCC, make -j6
CXXFLAGS += -flto -fuse-linker-plugin -flto=6
LDFLAGS += -flto=6


# Add -z notext to LDFLAGS to allow dynamic relocations in read-only segments
#LDFLAGS += -z notext

# For Ensuring Parallel LTRANS Jobs w/ Clang, make -j6
#CXXFLAGS += -flto -flto-jobs=6
#LDFLAGS += -flto-jobs=6

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS := $(PORTLIBS) $(LIBNX)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD := $(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD := $(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN := $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(CONFIG_JSON)),)
	jsons := $(wildcard *.json)
	ifneq (,$(findstring $(TARGET).json,$(jsons)))
		export APP_JSON := $(TOPDIR)/$(TARGET).json
	else
		ifneq (,$(findstring config.json,$(jsons)))
			export APP_JSON := $(TOPDIR)/config.json
		endif
	endif
else
	export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
endif

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)


$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

	@rm -rf $(CURDIR)/SdOut
	@mkdir -p $(CURDIR)/SdOut/switch/.overlays
	@mkdir -p $(CURDIR)/SdOut/config/$(APP_TITLE)/lang
	@mkdir -p $(CURDIR)/SdOut/config/$(APP_TITLE)/themes
	@cp -r ovlmenu.ovl $(CURDIR)/SdOut/switch/.overlays/
	@cp -r $(CURDIR)/lang/* $(CURDIR)/SdOut/config/$(APP_TITLE)/lang/
	@cp -r $(CURDIR)/themes/* $(CURDIR)/SdOut/config/$(APP_TITLE)/themes/
	@cd $(CURDIR)/SdOut; zip -r -q -9 $(APP_TITLE).zip switch config; cd $(CURDIR)

#---------------------------------------------------------------------------------
clean:
	@rm -rf $(BUILD) $(CURDIR)/SdOut $(TARGET).ovl $(TARGET).nro $(TARGET).nacp $(TARGET).elf

#---------------------------------------------------------------------------------
dist: all
	@echo making dist ...

	@rm -f $(CURDIR)/SdOut/$(APP_TITLE).zip
	@cd $(CURDIR)/SdOut; zip -r -q -9 $(APP_TITLE).zip switch config; cd $(CURDIR)

#---------------------------------------------------------------------------------
else
.PHONY: all

DEPENDS := $(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all : $(OUTPUT).ovl

$(OUTPUT).ovl: $(OUTPUT).elf $(OUTPUT).nacp 
	@elf2nro $< $@ $(NROFLAGS)
	@echo "built ... $(notdir $(OUTPUT).ovl)"



$(OUTPUT).elf: $(OFILES)

$(OFILES_SRC): $(HFILES_BIN)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o %_bin.h : %.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
