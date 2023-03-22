/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration for MT8195 based boards
 *
 * Copyright (C) 2021 BayLibre, SAS
 * Author: Fabien Parent <fparent@baylibre.com
 */

#ifndef __MT8195_H
#define __MT8195_H

#include <linux/sizes.h>

#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	-4
#define CONFIG_SYS_NS16550_MEM32
#define CONFIG_SYS_NS16550_COM1		0x11002000
#define CONFIG_SYS_NS16550_CLK		26000000

#define MT8195_DEMO_BL2_IMAGE_GUID \
	EFI_GUID(0x0538fd5d, 0x00dc, 0x47c5, 0x8d, 0x26, \
		 0x43, 0x48, 0x03, 0xb1, 0xe3, 0x27)

#define MT8195_DEMO_FIP_IMAGE_GUID \
	EFI_GUID(0xf65ec70e, 0x45ab, 0x40fb, 0x99, 0x52, \
		 0xe5, 0xbc, 0x30, 0x70, 0xda, 0x5c)

/* Environment settings */
#include <config_distro_bootcmd.h>

#ifdef CONFIG_CMD_MMC
#define BOOT_TARGET_MMC(func) func(MMC, mmc, 0)
#else
#define BOOT_TARGET_MMC(func)
#endif

#ifdef CONFIG_CMD_USB
#define BOOT_TARGET_USB(func) func(USB, usb, 0)
#else
#define BOOT_TARGET_USB(func)
#endif

#ifdef CONFIG_CMD_SCSI
#define BOOT_TARGET_SCSI(func) func(SCSI, scsi, 2)
#else
#define BOOT_TARGET_SCSI(func)
#endif

#define BOOT_TARGET_DEVICES(func) \
	BOOT_TARGET_MMC(func) \
	BOOT_TARGET_USB(func) \
	BOOT_TARGET_SCSI(func)

#if !defined(CONFIG_EXTRA_ENV_SETTINGS)
#define CONFIG_EXTRA_ENV_SETTINGS \
	"scriptaddr=0x40000000\0" \
	"fdt_addr_r=0x44000000\0" \
	"fdtoverlay_addr_r=0x44c00000\0" \
	"kernel_addr_r=0x45000000\0" \
	"ramdisk_addr_r=0x46000000\0" \
	"fdtfile=" CONFIG_DEFAULT_DEVICE_TREE ".dtb\0" \
	BOOTENV
#endif

#endif
