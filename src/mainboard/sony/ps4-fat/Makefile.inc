## Makefile for Sony PS4 Fat mainboard

ifeq ($(CONFIG_BOARD_SONY_PS4_FAT),y)

bootblock-y += bootblock.c
bootblock-y += ../aeolia/aeolia.c

romstage-y += romstage.c
romstage-y += ../aeolia/aeolia.c
romstage-y += ../../../southbridge/sony/aeolia/romstage.c

ramstage-y += ramstage.c
ramstage-y += ../aeolia/aeolia.c
ramstage-y += ../../../southbridge/sony/aeolia/ramstage.c
ramstage-y += ../../../northbridge/amd/ps4_belize/northbridge.c

smm-$(CONFIG_HAVE_SMI_HANDLER) += smihandler.c

CPPFLAGS_common += -I$(src)/mainboard/sony/ps4-fat
CPPFLAGS_common += -I$(src)/southbridge/sony/aeolia
CPPFLAGS_common += -I$(src)/northbridge/amd/ps4_belize

endif