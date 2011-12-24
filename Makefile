#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITXENON)),)
$(error "Please set DEVKITXENON in your environment. export DEVKITXENON=<path to>devkitPPC")
endif

include $(DEVKITXENON)/rules
MACHDEP =  -DXENON -m32 -mno-altivec -fno-pic  -fno-pic -mpowerpc64 -mhard-float -L$(DEVKITXENON)/xenon/lib/32 -u read -u _start -u exc_base 
#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
GUI_SRC         := gui/source gui/source/libwiigui gui/source/images gui/source/fonts gui/source/sounds \
                    gui/source/lang gui/source/vec gui/source/genesisplus

GUI_INC         := gui/source gui/source/genesisplus

TARGET		:=  $(notdir $(CURDIR))
BUILD		:=  build
DATA		:=  
SOURCES		:=	$(GUI_SRC) source source/m68k source/z80 source/sound source/ntsc source/input_hw source/cart_hw source/cart_hw/svp source/xenon source/xenon/filter
INCLUDES	:=	$(GUI_INC) source source/m68k source/z80 source/sound source/ntsc source/input_hw  source/cart_hw source/cart_hw/svp source/xenon

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ASFLAGS	= -Wa,$(INCLUDE) -Wa,-a32
CFLAGS	= -ffunction-sections -fdata-sections -g -Ofast -fno-tree-vectorize -fno-tree-slp-vectorize -ftree-vectorizer-verbose=1 -Wall $(MACHDEP) $(INCLUDE) -DNO_SOUND -DLOG_STDOUT -DLIBXENON -DUSE_32BPP_RENDERING
CXXFLAGS	=	$(CFLAGS)

LDFLAGS	=	-g $(MACHDEP) -Wl,--gc-sections -Wl,-Map,$(notdir $@).map

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=	-lpng -lbz2  -lxenon -lm -lz -lfreetype

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= libs 

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
PSUFILES        :=      $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.psu)))
VSUFILES        :=      $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.vsu)))
BINFILES        :=      $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.bin)))
PNGFILES        :=      $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.png)))
ABCFILES        :=      $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.abc)))
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
TTFFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.ttf)))
LANGFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.lang)))
PNGFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.png)))
OGGFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.ogg)))
PCMFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.pcm)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(CPPFILES:.cpp=.cpp.o) $(CFILES:.c=.c.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o) \
					$(TTFFILES:.ttf=.ttf.o) $(LANGFILES:.lang=.lang.o) \
					$(PNGFILES:.png=.png.o) \
					$(OGGFILES:.ogg=.ogg.o) $(PCMFILES:.pcm=.pcm.o)
					

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                        $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                        -I$(CURDIR)/$(BUILD) \
			-I$(LIBXENON_INC) -I$(LIBXENON_INC)/freetype2

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					-L$(LIBXENON_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).elf32

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).elf32: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)
%.vsu.o : %.vsu
	@echo $(notdir $<)
	@$(bin2o)
%.psu.o : %.psu
	@echo $(notdir $<)
	@$(bin2o)
%.bin.o : %.bin
	@echo $(notdir $<)
	@$(bin2o)
%.abc.o : %.abc
	@echo $(notdir $<)
	@$(bin2o)
%.ttf.o : %.ttf
	@echo $(notdir $<)
	$(bin2o)

%.lang.o : %.lang
	@echo $(notdir $<)
	$(bin2o)

%.png.o : %.png
	@echo $(notdir $<)
	$(bin2o)

%.ogg.o : %.ogg
	@echo $(notdir $<)
	$(bin2o)

%.pcm.o : %.pcm
	@echo $(notdir $<)
	$(bin2o)

%.cpp.o: %.cpp
	@echo [$(notdir $<)]
	@$(CXX) -MMD -MP -MF $(DEPSDIR)/$*.d $(CXXFLAGS) -c $< -o $@

%.c.o: %.c
	@echo [$(notdir $<)]
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) -c $< -o $@
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------


run: $(BUILD) $(OUTPUT).elf32
	cp $(OUTPUT).elf32 /var/lib/tftpboot/tftpboot/xenon
	$(PREFIX)strip /var/lib/tftpboot/tftpboot/xenon
