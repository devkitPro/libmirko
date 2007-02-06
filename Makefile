#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

include $(DEVKITARM)/gp32_rules

BUILD		:=	build
SOURCES		:=	source/lowlevel/chatboard \
			source/lowlevel/common \
			source/lowlevel/font \
			source/lowlevel/grafik \
			source/lowlevel/smfs \
			source/lowlevel/sound \
			source/addon/asmclip \
			source/addon/sbox \
			source/addon/zlib

INCLUDES	:=	include \
			source/addon/zlib/include

VERSION		:=	0.9.7

ARCH	:=	-marm

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
CFLAGS	:=	-g -O3 -Wall -mcpu=arm9tdmi -mtune=arm9tdmi $(ARCH) $(INCLUDE)
ASFLAGS	:=	-g -Wa,--warn $(ARCH)

#---------------------------------------------------------------------------------
# path to tools - this can be deleted if you set the path in windows
#---------------------------------------------------------------------------------
export PATH	:=	$(DEVKITARM)/bin:$(PATH)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export MIRKO	:=	$(CURDIR)/lib/libmirkoSDK.a
export ADDON	:=	$(CURDIR)/lib/libaddon.a

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/build

export	LIBMIRKOFILES	:=	gp_chatboard.o \
				gp_arm.o gp_buttons.o gp_cpuspeed.o gp_grafik.o gp_irq.o gp_memory.o gp_timer.o \
				gp_8x8font.o gp_setfont.o \
				gp_line.o gp_sprite.o gp_tiled.o \
				mixermod.o modplayer.o \
				gp_filesys.o \
				smf_buf.o smf_conf.o smf_fat.o smf_io.o \
				smf_lpt.o smf_uni.o

export LIBADDONFILES	:=	gp_asmlib.o gp_clipped.o \
				gp_sbox.o \
				gp_zdaindex.o adler32.o crc32.o inffast.o inftrees.o uncompr.o \
				deflate.o gzio.o inflate.o zutil.o


.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d lib ] || mkdir -p lib
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile


clean:
	@echo clean ...
	@rm -fr $(BUILD) *.tar.bz2

dist: $(BUILD)
	@tar --exclude=*CVS* --exclude=*build* --exclude=*.bz2 -cvjf libmirko-src-$(VERSION).tar.bz2 include source Makefile license.txt
	@tar --exclude=*CVS* -cvjf libmirko-$(VERSION).tar.bz2 include lib license.txt

install: dist
	mkdir -p $(DEVKITPRO)/libmirko
	bzip2 -cd libmirko-$(VERSION).tar.bz2 | tar -xv -C $(DEVKITPRO)/libmirko


#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(LIBMIRKOFILES:.o=.d)

#---------------------------------------------------------------------------------
all:	$(MIRKO) $(ADDON)

#---------------------------------------------------------------------------------
$(MIRKO): $(LIBMIRKOFILES)
$(ADDON): $(LIBADDONFILES)

-include $(DEPENDS)

endif
#---------------------------------------------------------------------------------
