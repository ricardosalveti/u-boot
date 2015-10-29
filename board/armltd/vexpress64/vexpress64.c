/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 * Sharma Bhupesh <bhupesh.sharma@freescale.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <netdev.h>
#include <asm/io.h>
#include <linux/compiler.h>
#include <dm/platdata.h>
#include <dm/platform_data/serial_pl01x.h>
#include "pcie.h"
#include <asm/armv8/mmu.h>

DECLARE_GLOBAL_DATA_PTR;

static const struct pl01x_serial_platdata serial_platdata = {
	.base = V2M_UART0,
	.type = TYPE_PL011,
	.clock = CONFIG_PL011_CLOCK,
};

U_BOOT_DEVICE(vexpress_serials) = {
	.name = "serial_pl01x",
	.platdata = &serial_platdata,
};

static struct mm_region vexpress64_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0x80000000UL,
		.phys = 0x80000000UL,
		.size = 0xff80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = vexpress64_mem_map;

/* This function gets replaced by platforms supporting PCIe.
 * The replacement function, eg. on Juno, initialises the PCIe bus.
 */
__weak void vexpress64_pcie_init(void)
{
}

int board_init(void)
{
	vexpress64_pcie_init();
	return 0;
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE;
	return 0;
}

void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
#ifdef PHYS_SDRAM_2
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
#endif
}

#define SYS_CFG_START		(1 << 31)
#define SYS_CFG_WRITE		(1 << 30)
#define SYS_CFG_REBOOT		(9 << 20)
#define SYS_CFG_SITE_MB		(0 << 16)

#define SYS_CFG_ERR		(1 << 1)
#define SYS_CFG_COMPLETE	(1 << 0)

int v2m_cfg_write(u32 devfn, u32 data)
{
	/* Configuration interface broken? */
	u32 val;

	devfn |= SYS_CFG_START | SYS_CFG_WRITE;

	val = readl(V2M_SYS_CFGSTAT);
	writel(val & ~SYS_CFG_COMPLETE, V2M_SYS_CFGSTAT);

	writel(data, V2M_SYS_CFGDATA);
	writel(devfn, V2M_SYS_CFGCTRL);

	do {
		val = readl(V2M_SYS_CFGSTAT);
	} while (val == 0);

	return !!(val & SYS_CFG_ERR);
}
/*
 * Board specific reset that is system reset.
 */
void reset_cpu(ulong addr)
{
	if (v2m_cfg_write(SYS_CFG_REBOOT | SYS_CFG_SITE_MB, 0))
		printf("Unable to reboot\n");
	else
		/* The reset doesn't happen immediately */
		/* Delay while the motherboard gets around to it */
		/* rather than while(1) to hang forever, in case something went wrong */
		mdelay(1000);
}

/*
 * Board specific ethernet initialization routine.
 */
int board_eth_init(bd_t *bis)
{
	int rc = 0;
#ifdef CONFIG_TARGET_VEXPRESS64_JUNO
	char *bootargs_sky2 = getenv("bootargs_sky2");

	if (!bootargs_sky2){
		char macstr[255];
		u8 mac[6];
		u32 macl, mach;

		macl = readl(V2M_SYS_CFGMACL);
		mach = readl(V2M_SYS_CFGMACH);

		if (mach && macl){
			mac[0] = (mach >> 8) & 0xff;
			mac[1] = mach & 0xff;
			mac[2] = (macl >> 24) & 0xff;
			mac[3] = (macl >> 16) & 0xff;
			mac[4] = (macl >> 8) & 0xff;
			mac[5] = macl & 0xff;

			sprintf(macstr, "sky2.mac_address=0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x,0x%2.2x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			setenv("bootargs_sky2", macstr);
		}
	}
#endif
#ifdef CONFIG_SMC91111
	rc = smc91111_initialize(0, CONFIG_SMC91111_BASE);
#endif
#ifdef CONFIG_SMC911X
	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);
#endif
	return rc;
}
