// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2017-2019 Toradex
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <fsl_ifc.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <environment.h>
#include <fsl_esdhc.h>
#include <i2c.h>

#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/sci/sci.h>
#include <asm/arch/imx8-pins.h>
#include <dm.h>
#include <imx8_hsio.h>
#include <usb.h>
#include <asm/arch/iomux.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/video.h>
#include <asm/arch/video_common.h>
#include <power-domain.h>
#include <asm/arch/lpcg.h>

#include "../common/tdx-cfg-block.h"

DECLARE_GLOBAL_DATA_PTR;

#define ESDHC_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define ESDHC_CLK_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define GPIO_PAD_CTRL	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define UART_PAD_CTRL	((SC_PAD_CONFIG_OUT_IN << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) \
						| (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define PCB_VERS_DETECT		((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) | \
				 (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT) | (SC_PAD_28FDSOI_PS_PU << PADRING_PULL_SHIFT))

#define PCB_VERS_DEFAULT	((SC_PAD_CONFIG_NORMAL << PADRING_CONFIG_SHIFT) | (SC_PAD_ISO_OFF << PADRING_LPCONFIG_SHIFT) | \
				 (SC_PAD_28FDSOI_PS_PD << PADRING_PULL_SHIFT) | (SC_PAD_28FDSOI_DSE_DV_HIGH << PADRING_DSE_SHIFT))

typedef enum {
	PCB_VERSION_1_0,
	PCB_VERSION_1_1
} pcb_rev_t;

static iomux_cfg_t pcb_vers_detect[] = {
	SC_P_MIPI_DSI0_GPIO0_00 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(PCB_VERS_DETECT),
	SC_P_MIPI_DSI0_GPIO0_01 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(PCB_VERS_DETECT),
};

static iomux_cfg_t pcb_vers_default[] = {
	SC_P_MIPI_DSI0_GPIO0_00 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(PCB_VERS_DEFAULT),
	SC_P_MIPI_DSI0_GPIO0_01 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(PCB_VERS_DEFAULT),
};

static iomux_cfg_t uart1_pads[] = {
	SC_P_UART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	SC_P_UART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx8_iomux_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

int board_early_init_f(void)
{
	sc_ipc_t ipcHndl = 0;
	sc_err_t sciErr = 0;

	ipcHndl = gd->arch.ipc_channel_handle;

	/* Power up UART1, this is very early while power domain is not working */
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_UART_1, SC_PM_PW_MODE_ON);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Set UART1 clock root to 80 MHz */
	sc_pm_clock_rate_t rate = 80000000;
	sciErr = sc_pm_set_clock_rate(ipcHndl, SC_R_UART_1, 2, &rate);
	if (sciErr != SC_ERR_NONE)
		return 0;

	/* Enable UART1 clock root */
	sciErr = sc_pm_clock_enable(ipcHndl, SC_R_UART_1, 2, true, false);
	if (sciErr != SC_ERR_NONE)
		return 0;

	LPCG_AllClockOn(LPUART_1_LPCG);

	setup_iomux_uart();

	return 0;
}

#ifdef CONFIG_MXC_GPIO

#define BKL1_GPIO   IMX_GPIO_NR(1, 10)

static iomux_cfg_t board_gpios[] = {
	SC_P_LVDS1_GPIO00 | MUX_MODE_ALT(3) | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

static void board_gpio_init(void)
{
	imx8_iomux_setup_multiple_pads(board_gpios, ARRAY_SIZE(board_gpios));

	gpio_request(BKL1_GPIO, "BKL1_GPIO");
	gpio_direction_output(BKL1_GPIO, 1);
}
#endif

void board_mem_get_layout(uint64_t *phys_sdram_1_start,
			  uint64_t *phys_sdram_1_size,
			  uint64_t *phys_sdram_2_start,
			  uint64_t *phys_sdram_2_size)
{
	sc_ipc_t ipc = gd->arch.ipc_channel_handle;
	uint32_t is_quadplus = 0, val = 0;
	sc_err_t sciErr = sc_misc_otp_fuse_read(ipc, 6, &val);

	if (sciErr == SC_ERR_NONE) {
		/* QP has one A72 core disabled */
		is_quadplus = ((val >> 4) & 0x3) != 0x0;
	}

	*phys_sdram_1_start = PHYS_SDRAM_1;
	*phys_sdram_1_size = PHYS_SDRAM_1_SIZE;
	*phys_sdram_2_start = PHYS_SDRAM_2;
	if (is_quadplus)
		/* Our QP based SKUs only have 2 GB RAM (PHYS_SDRAM_1_SIZE) */
		*phys_sdram_2_size = 0x0UL;
	else
		*phys_sdram_2_size = PHYS_SDRAM_2_SIZE;
}

int checkboard(void)
{
	puts("Board: Apalis iMX8\n");

	print_bootinfo();

	/* Note:  After reloc, ipcHndl will no longer be valid.  If handle
	 *        returned by sc_ipc_open matches SC_IPC_CH, use this
	 *        macro (valid after reloc) for subsequent SCI calls.
	 */
	if (gd->arch.ipc_channel_handle != SC_IPC_CH)
		printf("\nSCI error! Invalid handle\n");

#ifdef SCI_FORCE_ABORT
	sc_rpc_msg_t abort_msg;

	puts("Send abort request\n");
	RPC_SIZE(&abort_msg) = 1;
	RPC_SVC(&abort_msg) = SC_RPC_SVC_ABORT;
	sc_ipc_write(SC_IPC_CH, &abort_msg);

	/* Close IPC channel */
	sc_ipc_close(SC_IPC_CH);
#endif /* SCI_FORCE_ABORT */

	return 0;
}

#ifdef CONFIG_FSL_HSIO

#define PCIE_PAD_CTRL	((SC_PAD_CONFIG_OD_IN << PADRING_CONFIG_SHIFT))
static iomux_cfg_t board_pcie_pins[] = {
	SC_P_PCIE_CTRL0_CLKREQ_B | MUX_MODE_ALT(0) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
	SC_P_PCIE_CTRL0_WAKE_B | MUX_MODE_ALT(0) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
	SC_P_PCIE_CTRL0_PERST_B | MUX_MODE_ALT(0) | MUX_PAD_CTRL(PCIE_PAD_CTRL),
};

static void imx8qm_hsio_initialize(void)
{
	struct power_domain pd;
	int ret;

	if (!power_domain_lookup_name("hsio_sata0", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("hsio_sata0 Power up failed! (error = %d)\n", ret);
	}

	if (!power_domain_lookup_name("hsio_pcie0", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("hsio_pcie0 Power up failed! (error = %d)\n", ret);
	}

	if (!power_domain_lookup_name("hsio_pcie1", &pd)) {
		ret = power_domain_on(&pd);
		if (ret)
			printf("hsio_pcie1 Power up failed! (error = %d)\n", ret);
	}

	imx8_iomux_setup_multiple_pads(board_pcie_pins, ARRAY_SIZE(board_pcie_pins));
}

void pci_init_board(void)
{
	/* test the 1 lane mode of the PCIe A controller */
	mx8qm_pcie_init();
}
#endif

int board_init(void)
{
#ifdef CONFIG_MXC_GPIO
	board_gpio_init();
#endif

#ifdef CONFIG_FSL_HSIO
	imx8qm_hsio_initialize();
#ifdef CONFIG_SCSI_AHCI_PLAT
	sata_init();
#endif
#endif
	return 0;
}

void board_quiesce_devices(void)
{
	const char *power_on_devices[] = {
		"dma_lpuart1",
	};

	power_off_pd_devices(power_on_devices, ARRAY_SIZE(power_on_devices));
}

void detail_board_ddr_info(void)
{
	puts("\nDDR    ");
}

/*
 * Board specific reset that is system reset.
 */
void reset_cpu(ulong addr)
{
	puts("SCI reboot request");
	sc_pm_reboot(SC_IPC_CH, SC_PM_RESET_TYPE_COLD);
	while (1)
		putc('.');
}

#ifdef CONFIG_OF_BOARD_SETUP
int ft_board_setup(void *blob, bd_t *bd)
{
	return ft_common_board_setup(blob, bd);
}
#endif

int mmc_map_to_kernel_blk(int dev_no)
{
	return dev_no;
}

static int check_mmc_autodetect(void)
{
	char *autodetect_str = env_get("mmcautodetect");

	if ((autodetect_str != NULL) &&
		(strcmp(autodetect_str, "yes") == 0)) {
		return 1;
	}

	return 0;
}

void board_late_mmc_env_init(void)
{
	char cmd[32];
	char mmcblk[32];
	u32 dev_no = mmc_get_env_dev();

	if (!check_mmc_autodetect())
		return;

	env_set_ulong("mmcdev", dev_no);

	/* Set mmcblk env */
	sprintf(mmcblk, "/dev/mmcblk%dp2 rootwait rw",
		mmc_map_to_kernel_blk(dev_no));
	env_set("mmcroot", mmcblk);

	sprintf(cmd, "mmc dev %d", dev_no);
	run_command(cmd, 0);
}

static pcb_rev_t get_pcb_revision(void)
{
	unsigned int pcb_vers = 0;

	imx8_iomux_setup_multiple_pads(pcb_vers_detect,
				       ARRAY_SIZE(pcb_vers_detect));

	gpio_request(IMX_GPIO_NR(1, 18), \
		     "PCB version detection on PAD SC_P_MIPI_DSI0_GPIO0_00");
	gpio_request(IMX_GPIO_NR(1, 19), \
		     "PCB version detection on PAD SC_P_MIPI_DSI0_GPIO0_01");
	gpio_direction_input(IMX_GPIO_NR(1, 18));
	gpio_direction_input(IMX_GPIO_NR(1, 19));

	udelay(1000);

	pcb_vers = gpio_get_value(IMX_GPIO_NR(1, 18));
	pcb_vers |= gpio_get_value(IMX_GPIO_NR(1, 19)) << 1;

	/* Set muxing back to default values for saving energy */
	imx8_iomux_setup_multiple_pads(pcb_vers_default,
				       ARRAY_SIZE(pcb_vers_default));

	switch(pcb_vers) {
		case 0b11:
			return PCB_VERSION_1_0;
			break;
		case 0b10:
			return PCB_VERSION_1_1;
			break;
		default:
			return -ENODEV;
			break;
	}
}

static int select_dt_from_module_version(void)
{
	char *fdt_env = env_get("fdtfile");

	switch(get_pcb_revision()) {
		case PCB_VERSION_1_0:
			if (strcmp(FDT_FILE_V1_0, fdt_env)) {
				env_set("fdtfile", FDT_FILE_V1_0);
				printf("Detected a V1.0 module, setting " \
					"correct devicetree\n");
#ifndef CONFIG_ENV_IS_NOWHERE
				env_save();
#endif
			}
			break;
		default:
			break;
	}
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
/* TODO move to common */
	env_set("board_name", "Apalis iMX8QM");
	env_set("board_rev", "v1.0");
#endif

#ifdef CONFIG_AHAB_BOOT
	env_set("sec_boot", "yes");
#else
	env_set("sec_boot", "no");
#endif

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

#ifdef IMX_LOAD_HDMI_FIMRWARE
	char *end_of_uboot;
	char command[256];
	end_of_uboot = (char *)(ulong)(CONFIG_SYS_TEXT_BASE + _end_ofs + fdt_totalsize(gd->fdt_blob));
	end_of_uboot += 9;

	/* load hdmitxfw.bin and hdmirxfw.bin*/
	memcpy(IMX_HDMI_FIRMWARE_LOAD_ADDR, end_of_uboot,
			IMX_HDMITX_FIRMWARE_SIZE + IMX_HDMIRX_FIRMWARE_SIZE);

	sprintf(command, "hdp load 0x%x", IMX_HDMI_FIRMWARE_LOAD_ADDR);
	run_command(command, 0);

	sprintf(command, "hdprx load 0x%x",
			IMX_HDMI_FIRMWARE_LOAD_ADDR + IMX_HDMITX_FIRMWARE_SIZE);
	run_command(command, 0);
#endif

	select_dt_from_module_version();

	return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
	return 0; /*TODO*/
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/

#if defined(CONFIG_VIDEO_IMXDPUV1)
static void enable_lvds(struct display_info_t const *dev)
{
	display_controller_setup((PS2KHZ(dev->mode.pixclock) * 1000));
	lvds_soc_setup(dev->bus, (PS2KHZ(dev->mode.pixclock) * 1000));
	lvds_configure(dev->bus);
	lvds2hdmi_setup(6);
}

struct display_info_t const displays[] = {{
	.bus	= 0, /* LVDS0 */
	.addr	= 0, /* Unused */
	.pixfmt	= IMXDPUV1_PIX_FMT_BGRA32,
	.detect	= NULL,
	.enable	= enable_lvds,
	.mode	= {
		.name           = "IT6263", /* 720P60 */
		.refresh        = 60,
		.xres           = 1280,
		.yres           = 720,
		.pixclock       = 13468, /* 74250000 */
		.left_margin    = 110,
		.right_margin   = 220,
		.upper_margin   = 5,
		.lower_margin   = 20,
		.hsync_len      = 40,
		.vsync_len      = 5,
		.sync           = FB_SYNC_EXT,
		.vmode          = FB_VMODE_NONINTERLACED
} } };
size_t display_count = ARRAY_SIZE(displays);

#endif /* CONFIG_VIDEO_IMXDPUV1 */
