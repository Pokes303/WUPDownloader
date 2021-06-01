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

SOURCES		:=	zlib/contrib/minizip \
				src/cJSON \
				src/menu \
				src

DATA		:=	
INCLUDES	:=	include \
				payload \
				src/cJSON \
				zlib/contrib/minizip

ifeq ($(strip $(HBL)), 1)
ROMFS		:=	data
else
ROMFS		:=	
endif


#-------------------------------------------------------------------------------
# options for code generation
#-------------------------------------------------------------------------------
include $(PORTLIBS_PATH)/wiiu/share/romfs-wiiu.mk

CFLAGS		:=	$(MACHDEP) -Ofast -flto=auto -fno-fat-lto-objects \
				-fuse-linker-plugin -pipe -D__WIIU__ -D__WUT__ \
				-DNUSSPLI_VERSION=\"$(NUSSPLI_VERSION)\" \
				-DIOAPI_NO_64 $(ROMFS_CFLAGS)
ifeq ($(strip $(HBL)), 1)
CFLAGS		+=	-DNUSSPLI_HBL
endif

CXXFLAGS	:=	$(CFLAGS)
ASFLAGS		:=	-g $(ARCH)
LDFLAGS		:=	-g $(ARCH) $(RPXSPECS) $(CFLAGS) -Wl,-Map,$(notdir $*.map)

LIBS		:=	-lgui -lwut -lfreetype -lgd -lpng -ljpeg -lz -lmad \
				-lvorbisidec -logg -lbz2 -liosuhax $(ROMFS_LIBS)

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

real: $(CURDIR)/payload/arm_kernel_bin.h
	@git submodule deinit --force libgui
	@git submodule update --init --recursive
	@rm -f data/titleDB.json
	@wget -U "NUSspli builder" -O data/titleDB.json http://napi.nbg01.v10lator.de/v2/t
	@rm -f zlib/contrib/minizip/iowin* zlib/contrib/minizip/mini* zlib/contrib/minizip/zip.? zlib/contrib/minizip/mztools.? zlib/contrib/minizip/configure.ac zlib/contrib/minizip/Makefile zlib/contrib/minizip/Makefile.am zlib/contrib/minizip/*.com zlib/contrib/minizip/*.txt
	@cd zlib && git apply ../minizip.patch || true
	@mv $(CURDIR)/src/cJSON/test.c $(CURDIR)/src/cJSON/test.c.old 2>/dev/null || true
	@sed -i 's/-save-temps/-pipe/g' libgui/Makefile
	@sed -i '/			-ffunction-sections -fdata-sections \\/d' libgui/Makefile
	@sed -i 's/$$(ARCH) -/$$(ARCH) $$(CFLAGS) -/g' libgui/Makefile
	@sed -i 's/-DNDEBUG=1 -O2 -s/$(LIBGUIFLAGS)/g' libgui/Makefile
	@cd libgui && for patch in $(TOPDIR)/libgui-patches/*.patch; do echo Applying $$patch && git apply $$patch; done && $(MAKE)
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)
	@$(MAKE) -C $(BUILD) -f $(CURDIR)/Makefile BUILD=$(BUILD) NUSSPLI_VERSION=$(NUSSPLI_VERSION) $(MAKE_CMD)

#-------------------------------------------------------------------------------
$(CURDIR)/payload/arm_kernel_bin.h:  $(CURDIR)/payload/arm_user_bin.h
	@$(MAKE) -C $(CURDIR)/arm_kernel -f  $(CURDIR)/arm_kernel/Makefile
	@-mkdir -p $(CURDIR)/payload
	@cp -p $(CURDIR)/arm_kernel/arm_kernel_bin.h $@

#-------------------------------------------------------------------------------
$(CURDIR)/payload/arm_user_bin.h:
	@$(MAKE) -C $(CURDIR)/arm_user -f  $(CURDIR)/arm_user/Makefile
	@-mkdir -p $(CURDIR)/payload
	@cp -p $(CURDIR)/arm_user/arm_user_bin.h $@

#-------------------------------------------------------------------------------
clean:
	@git submodule deinit --force --all
	@$(MAKE) -C $(CURDIR)/arm_user -f  $(CURDIR)/arm_user/Makefile clean
	@$(MAKE) -C $(CURDIR)/arm_kernel -f  $(CURDIR)/arm_kernel/Makefile clean
	@rm -fr debug release payload $(TARGET).rpx $(TARGET).elf

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
OFILES 		:=	$(OFILES_BIN) $(OFILES_SRC) $(ROMFS_TARGET)
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
