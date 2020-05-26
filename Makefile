#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)

include $(DEVKITPRO)/wut/share/wut_rules

#-------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#-------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		?=	debug

SOURCES		:=	src/cJSON \
				src/menu \
				src

DATA		:=	data
INCLUDES	:=	include \
				src/cJSON

#-------------------------------------------------------------------------------
# options for code generation
#-------------------------------------------------------------------------------
CFLAGS		:=	$(MACHDEP) -Ofast -flto=auto -fno-fat-lto-objects \
				-fuse-linker-plugin -pipe -D__WIIU__ -D__WUT__ \
				-DNUSSPLI_VERSION=\"$(NUSSPLI_VERSION)\"

CXXFLAGS	:=	$(CFLAGS)
ASFLAGS		:=	-g $(ARCH)
LDFLAGS		:=	-g $(ARCH) $(RPXSPECS) $(CFLAGS) -Wl,-Map,$(notdir $*.map)

LIBS		:=	-lgui -lwut -lfreetype -lgd -lpng -ljpeg -lz -lmad \
				-lvorbisidec -logg -lbz2 -liosuhax

#-------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level
# containing include and lib
#-------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(WUT_ROOT) $(TOPDIR)/libgui


#-------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#-------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#-------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
NUSSPLI_VERSION	:=	$(shell grep \<version\> meta/hbl/meta.xml | sed 's/.*<version>//' | sed 's/<\/.*//')

.PHONY: debug release real clean all

#-------------------------------------------------------------------------------
all: debug

real: 
	@git submodule deinit --force libgui
	@git submodule update --init --recursive
	@mv $(CURDIR)/src/cJSON/test.c $(CURDIR)/src/cJSON/test.c.old 2>/dev/null || true
	@sed -i 's/-save-temps/-pipe/g' libgui/Makefile
	@sed -i '/			-ffunction-sections -fdata-sections \\/d' libgui/Makefile
	@sed -i 's/$$(ARCH) -/$$(ARCH) $$(CFLAGS) -/g' libgui/Makefile
	@sed -i 's/-DNDEBUG=1 -O2 -s/$(LIBGUIFLAGS)/g' libgui/Makefile
	@cd libgui && for patch in $(TOPDIR)/libgui-patches/*.patch; do echo Applying $$patch && git apply $$patch; done && $(MAKE)
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)
	@$(MAKE) -C $(BUILD) -f $(CURDIR)/Makefile BUILD=$(BUILD) NUSSPLI_VERSION=$(NUSSPLI_VERSION) $(MAKE_CMD)

#-------------------------------------------------------------------------------
clean:
	@git submodule deinit --force --all
	@rm -fr debug release $(TARGET).rpx $(TARGET).elf

#-------------------------------------------------------------------------------
debug:		MAKE_CMD	:=	debug
debug:		LIBGUIFLAGS	:=	-O0 -g
debug:		export V	:=	1
debug:		real

#-------------------------------------------------------------------------------
release:	MAKE_CMD	:=	all
release:	LIBGUIFLAGS	:=	-Ofast -flto=auto -fno-fat-lto-objects \
							-fuse-linker-plugin -s -DNDEBUG=1
release:	BUILD		:=	release
release:	real

#-------------------------------------------------------------------------------
else
.PHONY:	debug all


CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(TOPDIR)/$(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(TOPDIR)/$(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(TOPDIR)/$(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(TOPDIR)/$(dir)/*.*)))

#-------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#-------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#-------------------------------------------------------------------------------
	export LD	:=	$(CC)
#-------------------------------------------------------------------------------
else
#-------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------

OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
OFILES 		:=	$(OFILES_BIN) $(OFILES_SRC)
HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(TOPDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(TOPDIR)/$(BUILD) -I$(PORTLIBS_PATH)/ppc/include/freetype2

LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
VPATH	:=	$(foreach dir,$(SOURCES),$(TOPDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(TOPDIR)/$(dir))

DEPSDIR	:=	$(TOPDIR)/$(BUILD)
DEPENDS	:=	$(OFILES:.o=.d)

CFLAGS += $(INCLUDE)
CXXFLAGS += $(INCLUDE)
LDFLAGS += $(INCLUDE)

#-------------------------------------------------------------------------------
# main targets
#-------------------------------------------------------------------------------
all	:	$(OUTPUT).rpx

$(OUTPUT).rpx	:	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#-------------------------------------------------------------------------------
debug: CFLAGS	+=	-g -Wall -O0 -fno-lto \
					-fno-use-linker-plugin -DNUSSPLI_DEBUG
debug: CXXFLAGS	+=	-g -Wall -O0 -fno-lto \
					-fno-use-linker-plugin -DNUSSPLI_DEBUG
debug: LDFLAGS	+=	-g -Wall -O0 -fno-lto -fno-use-linker-plugin -DNUSSPLI_DEBUG
debug: all

#-------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#-------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#-------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

# https://github.com/devkitPro/devkitppc-rules/pull/2
define bin2o
	bin2s -a 32 $< | $(AS) -o $(@)
	echo "#pragma once" > `(echo $(<F) | tr . _)`.h
	echo "#include <stddef.h>" >> `(echo $(<F) | tr . _)`.h
	echo "#include <stdint.h>" >> `(echo $(<F) | tr . _)`.h
	echo "extern const uint8_t" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' -e 's/[^A-Za-z0-9_]/_/g')`"_end[];" >> `(echo $(<F) | tr . _)`.h
	echo "extern const uint8_t" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' -e 's/[^A-Za-z0-9_]/_/g')`"[];" >> `(echo $(<F) | tr . _)`.h
	echo "extern const size_t" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' -e 's/[^A-Za-z0-9_]/_/g')`_size";" >> `(echo $(<F) | tr . _)`.h
endef

%.mp3.o %_mp3.h	:	%.mp3
	@echo $(notdir $<)
	@$(bin2o)

exports.def.o :	exports.def
	$(SILENTMSG) $(notdir $<)
	$(SILENTCMD)rplexportgen $< $*.s
	$(SILENTCMD)$(CC) -x assembler-with-cpp $(ASFLAGS) -c $*.s -o $@ $(ERROR_FILTER)

exports_def.h :	exports.def
	@echo "" > /dev/null

-include $(DEPENDS)

#-------------------------------------------------------------------------------
# We need to overwrite some WUT rules here
#-------------------------------------------------------------------------------
%.o: %.c
	$(SILENTMSG) $(notdir $<)
	$(SILENTCMD)$(CC) -MMD -MP -MF $*.d $(CFLAGS) $(if $(filter memdebug.o,$@),-fno-sanitize=all )-c $< -o $@ $(ERROR_FILTER)

#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------
