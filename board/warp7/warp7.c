/*
 * Copyright (C) 2016 NXP Semiconductors
 * Author: Fabio Estevam <fabio.estevam@nxp.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/mx7-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/mach-imx/hab.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/io.h>
#include <common.h>
#include <fsl_esdhc.h>
#include <i2c.h>
#include <mmc.h>
#include <asm/arch/crm_regs.h>
#include <usb.h>
#include <netdev.h>
#include <power/pmic.h>
#include <power/pfuze3000_pmic.h>
#include "../freescale/common/pfuze.h"

#ifndef CONFIG_SPL_BUILD
#include <console.h>
#include <fuse.h>
#endif /* CONFIG_SPL_BUILD */

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL  (PAD_CTL_DSE_3P3V_49OHM | PAD_CTL_PUS_PU100KOHM | \
			PAD_CTL_HYS)
#define USDHC_PAD_CTRL (PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW |	\
			PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU47KOHM)

#define I2C_PAD_CTRL	(PAD_CTL_DSE_3P3V_32OHM | PAD_CTL_SRE_SLOW | \
	PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PUS_PU100KOHM)

#ifdef CONFIG_SYS_I2C_MXC
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
/* I2C1 for PMIC */
static struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = MX7D_PAD_I2C1_SCL__I2C1_SCL | PC,
		.gpio_mode = MX7D_PAD_I2C1_SCL__GPIO4_IO8 | PC,
		.gp = IMX_GPIO_NR(4, 8),
	},
	.sda = {
		.i2c_mode = MX7D_PAD_I2C1_SDA__I2C1_SDA | PC,
		.gpio_mode = MX7D_PAD_I2C1_SDA__GPIO4_IO9 | PC,
		.gp = IMX_GPIO_NR(4, 9),
	},
};
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_SIZE;

	/* Subtract the defined OPTEE runtime firmware length */
#ifdef CONFIG_OPTEE_TZDRAM_SIZE
		gd->ram_size -= CONFIG_OPTEE_TZDRAM_SIZE;
#endif

	return 0;
}

static iomux_v3_cfg_t const wdog_pads[] = {
	MX7D_PAD_GPIO1_IO00__WDOG1_WDOG_B | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static iomux_v3_cfg_t const uart1_pads[] = {
	MX7D_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX7D_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc3_pads[] = {
	MX7D_PAD_SD3_CLK__SD3_CLK     | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_CMD__SD3_CMD     | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA0__SD3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA1__SD3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA2__SD3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA3__SD3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA4__SD3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA5__SD3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA6__SD3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_DATA7__SD3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX7D_PAD_SD3_RESET_B__SD3_RESET_B | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
};

static struct fsl_esdhc_cfg usdhc_cfg[1] = {
	{USDHC3_BASE_ADDR},
};

/*
 * OCOTP_TESTER
 * i.MX 7Solo Applications Processor Reference Manual, Rev. 0.1, 08/2016
 * OCOTP_TESTER describes a unique ID based on silicon wafer
 * and die X/Y position
 *
 * OCOTOP_TESTER offset 0x410
 * 31:0 fuse 0
 * FSL-wide unique, encoded LOT ID STD II/SJC CHALLENGE/ Unique ID
 *
 * OCOTP_TESTER1 offset 0x420
 * 31:24 fuse 1
 * The X-coordinate of the die location on the wafer/SJC CHALLENGE/ Unique ID
 * 23:16 fuse 1
 * The Y-coordinate of the die location on the wafer/SJC CHALLENGE/ Unique ID
 * 15:11 fuse 1
 * The wafer number of the wafer on which the device was fabricated/SJC
 * CHALLENGE/ Unique ID
 * 10:0 fuse 1
 * FSL-wide unique, encoded LOT ID STD II/SJC CHALLENGE/ Unique ID
 */
#define WARP7_USB_SERIALID_BANK 0
#define WARP7_USB_SERIALID_MSWORD 1
#define WARP7_USB_SERIALID_LSWORD 2

static int warp7_get_serialid(u64 *id)
{
	u32 val;
	int ret;

	if (!id)
		return -EINVAL;

	/* Read first word */
	ret = fuse_read(WARP7_USB_SERIALID_BANK, WARP7_USB_SERIALID_MSWORD, &val);
	if (ret)
		goto done;

	*id = val;
	*id <<= 32;

	/* Read second word */
	ret = fuse_read(WARP7_USB_SERIALID_BANK, WARP7_USB_SERIALID_LSWORD, &val);
	if (ret)
		goto done;

	*id |= val;

done:
	return ret;
}

int board_mmc_getcd(struct mmc *mmc)
{
		/* Assume uSDHC3 emmc is always present */
		return 1;
}

int board_mmc_init(bd_t *bis)
{
	imx_iomux_v3_setup_multiple_pads(usdhc3_pads, ARRAY_SIZE(usdhc3_pads));
	usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);

	return fsl_esdhc_initialize(bis, &usdhc_cfg[0]);
}

int board_early_init_f(void)
{
	setup_iomux_uart();

	return 0;
}

#ifdef CONFIG_POWER
#define I2C_PMIC       0
static struct pmic *pfuze;
int power_init_board(void)
{
	int ret;
	unsigned int reg, rev_id;

	ret = power_pfuze3000_init(I2C_PMIC);
	if (ret)
		return ret;

	pfuze = pmic_get("PFUZE3000");
	ret = pmic_probe(pfuze);
	if (ret)
		return ret;

	pmic_reg_read(pfuze, PFUZE3000_DEVICEID, &reg);
	pmic_reg_read(pfuze, PFUZE3000_REVID, &rev_id);
	printf("PMIC: PFUZE3000 DEV_ID=0x%x REV_ID=0x%x\n", reg, rev_id);

	/* disable Low Power Mode during standby mode */
	pmic_reg_write(pfuze, PFUZE3000_LDOGCTL, 0x1);

	return 0;
}
#endif

int board_eth_init(bd_t *bis)
{
	int ret = 0;

#ifdef CONFIG_USB_ETHER
	ret = usb_eth_initialize(bis);
	if (ret < 0)
		printf("Error %d registering USB ether.\n", ret);
#endif

	return ret;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	#ifdef CONFIG_SYS_I2C_MXC
		setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);
	#endif

	return 0;
}

int checkboard(void)
{
	char *mode;

	if (IS_ENABLED(CONFIG_ARMV7_BOOT_SEC_DEFAULT))
		mode = "secure";
	else
		mode = "non-secure";

#ifdef CONFIG_OPTEE_TZDRAM_SIZE
	unsigned long optee_start, optee_end;

	optee_end = PHYS_SDRAM + PHYS_SDRAM_SIZE;
	optee_start = optee_end - CONFIG_OPTEE_TZDRAM_SIZE;

	printf("Board: WARP7 in %s mode OPTEE DRAM 0x%08lx-0x%08lx\n",
	       mode, optee_start, optee_end);
#else
	printf("Board: WARP7 in %s mode\n", mode);
#endif

	return 0;
}

int board_usb_phy_mode(int port)
{
	return USB_INIT_DEVICE;
}

int board_late_init(void)
{
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;
	u64 serial_id = 0;
	char serial_string[0x20];
	int ret;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	/*
	 * Do not assert internal WDOG_RESET_B_DEB(controlled by bit 4),
	 * since we use PMIC_PWRON to reset the board.
	 */
	clrsetbits_le16(&wdog->wcr, 0, 0x10);

	/* Determine HAB state */
	setenv_ulong(HAB_ENABLED_ENVNAME, imx_hab_is_enabled());

	/* Set serial# standard environment variable based on OTP settings */
	ret = warp7_get_serialid(&serial_id);
	if (ret)
		printf("error %d reading from serial# OTP fuse\n", ret);

	snprintf(serial_string, sizeof(serial_string), "WaRP7-0x%016llx", serial_id);
	setenv("serial#", serial_string);

	return 0;
}

#ifndef CONFIG_SPL_BUILD
/*
 * This function will calculate an IVT address and set env->hab_load_address to
 * that value based on subtracting the size of the IVT space from the passed
 * address.
 *
 * Using u-boot to produce an IVT prefixed image
 *
 * mkimage -n board/warp7/imximage.cfg.cfgtmp -T imximage -e 0x87800000 -d u-boot.bin u-boot.imx
 *
 * and following NXP's guide to signing an IVT prefixed image will result in a
 * CSF footer being added i.e.
 *
 * ./cst --i u-boot_sign.csf --o u-boot_sign.csf-csf-header
 *
 * cat u-boot.imx u-boot_sign.csf-csf-header > u-boot.imx-signed
 *
 * Results in an image that looks like
 * IVT | BINARY | CSF
 *
 * This routine takes the address of the binary and sets the required IVT
 * address for a binary at that address to env->hab_load_address
 *
 * Since IVT, Binary and CSF can come in any order there is no requirement for
 * a particular image layout and therefore it is up to the BSP designer to
 * arrange the IVT, Binary and CSF in a way that works best for his or her
 * project.
 *
 * The image format chosen for WaRP7 is IVT | BINARY | CSF since that is the
 * least path of resistence given how the tools operate.
 */
#define IVT_PAD_SIZE 0xC00
static int set_ivt_load_address_env(ulong addr)
{
	setenv_hex("hab_load_address", addr - IVT_PAD_SIZE);

	return CMD_RET_SUCCESS;
}

static int do_warp7_calculate_ivt_address(cmd_tbl_t *cmdtp, int flag, int argc,
					  char * const argv[])
{
	ulong	addr;
	int	rcode = 0;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!imx_hab_is_enabled())
		return CMD_RET_FAILURE;

	addr = simple_strtoul(argv[1], NULL, 16);
	rcode = set_ivt_load_address_env(addr);

	return rcode;
}

U_BOOT_CMD(
		warp7_get_ivt_addr, CONFIG_SYS_MAXARGS, 2,
		do_warp7_calculate_ivt_address,
		"calculate load address of IVT binary store in $warp7_hab_load_address",
		"addr - non-IVT binary load address"
	  );

#endif /* ifndef CONFIG_SPL_BUILD */
