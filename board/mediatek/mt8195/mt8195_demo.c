// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 BayLibre SAS
 * Author: Fabien Parent <fparent@baylibre.com>
 */

#include <common.h>
#include <dm.h>
#include <efi_loader.h>
#include <net.h>
#include <asm/io.h>
#include <linux/kernel.h>

#if CONFIG_IS_ENABLED(EFI_HAVE_CAPSULE_SUPPORT)
static struct efi_fw_image fw_images[] = {
#if defined(CONFIG_EFI_CAPSULE_FIRMWARE_RAW)
	{
		.image_type_id = MT8195_DEMO_BL2_IMAGE_GUID,
		.fw_name = u"MT8195-DEMO-BL2",
		.image_index = 1,
	},
	{
		.image_type_id = MT8195_DEMO_FIP_IMAGE_GUID,
		.fw_name = u"MT8195-DEMO-FIP",
		.image_index = 2,
	},
#endif
};

struct efi_capsule_update_info update_info = {
	.dfu_string = "mmc 0=bl2.img raw 0x0 0x100 mmcpart 1;"
			"fip.bin part 0 1",
	.images = fw_images,
};

u8 num_image_type_guids = ARRAY_SIZE(fw_images);
#endif

int board_init(void)
{
	struct udevice *dev;
	int ret;

	if (CONFIG_IS_ENABLED(USB_GADGET)) {
		ret = uclass_get_device(UCLASS_USB_GADGET_GENERIC, 0, &dev);
		if (ret) {
			pr_err("%s: Cannot find USB device\n", __func__);
			return ret;
		}
	}

	if (CONFIG_IS_ENABLED(USB_ETHER))
		usb_ether_init();

	printf("Disabling WDT\n");
	writel(0, 0x10007000);

	printf("Enabling SCP SRAM\n");
	for (unsigned int val = 0xFFFFFFFF; val != 0U;) {
		val = val >> 1;
		writel(val, 0x1072102C);
	}

	return 0;
}
