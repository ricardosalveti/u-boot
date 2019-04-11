-include include/config/auto.conf

ifeq ("${CONFIG_IMX8QM}", "y")
	DEPLOY_IMG ?= /build/krm/oe-core_3.0/build/deploy/images/apalis-imx8
	MKIMG = $(DEPLOY_IMG)/imx-boot-tools/mkimage_imx8

	SOC = QM
	BOOTCPU = a53
	SECO ?= mx8qm-ahab-container.img
	SCFW ?= scfw_tcm.bin
	ATF ?= bl31-imx8qm.bin
	U_BOOT_BIN ?= ../u-boot.bin
else ifeq ("${CONFIG_IMX8QXP}", "y")
	DEPLOY_IMG ?= /build/krm/oe-core_3.0/build/deploy/images/colibri-imx8qxp
	MKIMG = $(DEPLOY_IMG)/imx-boot-tools/mkimage_imx8

	SOC = QX
	BOOTCPU = a53
	SECO ?= mx8qx-ahab-container.img
	SCFW ?= scfw_tcm.bin
	ATF ?= bl31-imx8qxp.bin
	U_BOOT_BIN ?= ../u-boot.bin
else
	$(error creating the bootimg file is only supported for apalis/colibri-imx8)
endif

OUTFILE = ../flash.bin

ifneq ("$(wildcard $(MKIMG))","")
	DEFTARGET = flash
else
	DEFTARGET = print-error
endif

DEFAULT: $(DEFTARGET)

print-error:
	@echo "$(MKIMG) not found."; touch $(OUTFILE);

$(SECO):
	cp $(DEPLOY_IMG)/$(SECO) $(SECO)

$(SCFW).$(SOC):
	cp $(DEPLOY_IMG)/imx-boot-tools/$(SCFW) $(SCFW).$(SOC)

$(ATF):
	cp $(DEPLOY_IMG)/imx-boot-tools/$(ATF) $(ATF)

u-boot-atf.bin: $(U_BOOT_BIN) $(ATF) Makefile
	@cp $(ATF) u-boot-atf.bin
	$(MKIMG) -commit > head.hash
	@cat $(U_BOOT_BIN) head.hash > u-boot-hash.bin
	rm head.hash
	@dd if=u-boot-hash.bin of=u-boot-atf.bin bs=1K seek=128

flash: $(MKIMG) $(SECO) $(SCFW).$(SOC) u-boot-atf.bin
	$(MKIMG) -soc $(SOC) -rev B0 -append $(SECO) -c -scfw $(SCFW).$(SOC) -ap u-boot-atf.bin $(BOOTCPU) 0x80000000 -out $(OUTFILE)
	@echo;echo "To update an SD card use e.g.:"
	@echo '    => tftp $$loadaddr flash.bin'
	@echo '    => setexpr blkcnt $${filesize} + 0x1ff && setexpr blkcnt $${blkcnt} / 0x200; mmc dev 0 1'
	@echo '    => mmc write $${loadaddr} 0x40 $${blkcnt}'; echo


.PHONY = print-error
