#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of object file images to be generated ()
#   GEN_BINS - list of binaries to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#
TARGET = eagle
#FLAVOR = release
FLAVOR = debug

#EXTRA_CCFLAGS += -u

ifndef PDIR # {
GEN_IMAGES= eagle.app.v6.out
GEN_BINS= eagle.app.v6.bin
SPECIAL_MKTARGETS=$(APP_MKTARGETS)
SUBDIRS=    	\
	flashapi 	\
	encrypt 	\
	driver		\
	misc 		\
	smartlink 	\
	upgrade 	\
	softpwm 	\
	fsm			\
	task		\
	fcmd	\
	onenet	\
	main 

ifdef BOOT_OPEN_SOURCE
SUBDIRS+=    	\
	boot
endif   	

endif # } PDIR

APPDIR = .
LDDIR = ../ld

CCFLAGS += -Os

TARGET_LDFLAGS = 			\
	-nostdlib 				\
	-Wl,-EL 				\
	--longcalls 			\
	--text-section-literals

ifeq ($(FLAVOR),debug)
    TARGET_LDFLAGS += -g -O2
endif

ifeq ($(FLAVOR),release)
    TARGET_LDFLAGS += -g -O0
endif

COMPONENTS_eagle.app.v6 = 		\
	flashapi/libflashapi.a 		\
	encrypt/libencrypt.a 		\
	driver/libdriver.a 			\
	misc/libmisc.a 				\
	smartlink/libsmartlink.a 	\
	upgrade/libupgrade.a 		\
	softpwm/libsoftpwm.a 		\
	fsm/libfsm.a				\
	task/libtask.a				\
	fcmd/libfcmd.a				\
	onenet/libonenet.a			\
	main/libmain.a 					

ifdef BOOT_OPEN_SOURCE
COMPONENTS_eagle.app.v6+=    	\
	boot/libmain.a
endif  

LINKFLAGS_eagle.app.v6 = 		\
	-L../lib        			\
	-nostdlib					\
    -T$(LD_FILE)   				\
	-Wl,--no-check-sections		\
    -u call_user_start			\
	-Wl,-static					\
	-Wl,--start-group			\
	-lc							\
	-lgcc						\
	-lhal						\
	-lphy						\
	-lpp						\
	-lnet80211					\
	-llwip						\
	-lwpa						\
	-lmain						\
	-lcrypto					\
	-lairkiss					\
	-ljson						\
	-lupgrade 					\
	-lsmartconfig 				\
	$(DEP_LIBS_eagle.app.v6)					

LINKFLAGS_eagle.app.v6 += 						\
	-Wl,--end-group
DEPENDS_eagle.app.v6 = 							\
                $(LD_FILE) 						\
                $(LDDIR)/eagle.rom.addr.v6.ld

#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#

#UNIVERSAL_TARGET_DEFINES =		\

# Other potential configuration flags include:
#	-DTXRX_TXBUF_DEBUG
#	-DTXRX_RXBUF_DEBUG
#	-DWLAN_CONFIG_CCX
CONFIGURATION_DEFINES =	-DICACHE_FLASH

DEFINES += 						\
	$(UNIVERSAL_TARGET_DEFINES) \
	$(CONFIGURATION_DEFINES)

DDEFINES +=	 					\
	$(UNIVERSAL_TARGET_DEFINES) \
	$(CONFIGURATION_DEFINES)


#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

INCLUDES := $(INCLUDES) -I $(PDIR)include
INCLUDES += -I $(PDIR)flashapi
INCLUDES += -I $(PDIR)encrypt
INCLUDES += -I $(PDIR)driver
INCLUDES += -I $(PDIR)misc
INCLUDES += -I $(PDIR)smartlink
INCLUDES += -I $(PDIR)upgrade
INCLUDES += -I $(PDIR)fsm
INCLUDES += -I $(PDIR)task
INCLUDES += -I $(PDIR)fcmd
INCLUDES += -I $(PDIR)softpwm
INCLUDES += -I $(PDIR)onenet
INCLUDES += -I $(PDIR)main

PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile

.PHONY: FORCE
FORCE:

