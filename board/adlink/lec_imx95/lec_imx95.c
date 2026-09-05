// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 NXP
 */

#include <env.h>
#include <init.h>
#include <asm/gpio.h>
#include <asm/arch/clock.h>
#include <linux/bitops.h>
#include <scmi_agent.h>
#include "../dts/upstream/src/arm64/freescale/imx95-power.h"
#include <asm/arch/sys_proto.h>
#include <dm/uclass.h>
#include <i2c.h>
#include <dm/uclass-internal.h>


#define SKU_CFG_DDR_2G 0
#define SKU_CFG_DDR_4G 1
#define SKU_CFG_DDR_8G 2
#define SKU_CFG_DDR_16G 3
#define PDIR 0x50


struct lec_imx95_ddr_size {
	phys_size_t bank0;
	phys_size_t bank1;
};

static const struct lec_imx95_ddr_size ddr_cfg[] = {
	[SKU_CFG_DDR_2G] = {
		.bank0 = 0x70000000ULL,
		.bank1 = 0x00000000ULL,
	},
	[SKU_CFG_DDR_4G] = {
		.bank0 = 0x70000000ULL,
		.bank1 = 0x80000000ULL,
	},
	[SKU_CFG_DDR_8G] = {
		.bank0 = 0x70000000ULL,
		.bank1 = 0x180000000ULL,
	},
	[SKU_CFG_DDR_16G] = {
		.bank0 = 0x70000000ULL,
		.bank1 = 0x380000000ULL,
	},
};

/*
static int imx9_scmi_power_domain_enable(u32 domain, bool enable)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_name(UCLASS_CLK, "protocol@14", &dev);
	if (ret)
		return ret;

	return scmi_pwd_state_set(dev, 0, domain, enable ? 0 : BIT(30));
}
*/

int board_init(void)
{
	/*int ret;
	ret = imx9_scmi_power_domain_enable(IMX95_PD_HSIO_TOP, true);
	if (ret) {
		printf("SCMI_POWWER_STATE_SET Failed for USB\n");
		return ret;
	}*/
	struct udevice *i2c_dev = NULL;
	struct udevice *bus;
	uint8_t i2c_data[64];
	int ret;

	ret = uclass_get_device_by_seq(UCLASS_I2C, 0x5, &bus);
	if (ret) {
		printf("%s: Can't find bus\n", __func__);
		return -EINVAL;
	}

	ret = dm_i2c_probe(bus, 0x70, 0, &i2c_dev);//GPIO expander address
	if (ret) {
		printf("%s: Can't find device id 0x70\n", __func__);
		return -ENODEV;
	}

	dm_i2c_reg_write(i2c_dev, 0x0, 0);
	dm_i2c_reg_write(i2c_dev, 0xe, 0);dm_i2c_reg_write(i2c_dev, 0xf, 0xff);

	ret = dm_i2c_read(i2c_dev, 0x0, i2c_data, 0x2A);
	if (ret) {
		printf("%s dm_i2c_read failed, err %d\n", __func__, ret);
		return -EIO;
	}
	return 0;
}

int board_late_init(void)
{
	if (IS_ENABLED(CONFIG_ENV_IS_IN_MMC))
		board_late_mmc_env_init();

	return 0;
}

static int lec_imx95_get_sku(void)
{
	u32 skucfg;

	u32 gpio;
	gpio = ((readl(GPIO5_BASE_ADDR + PDIR) & 0x70) >> 4);
	switch (gpio) {
		case 0:
		case 2:
		case 4:
		case 6:
			skucfg = gpio >> 1;
			break;
		default:
			return SKU_CFG_DDR_2G;
	}

	return skucfg;
}

int board_phys_sdram_size(phys_size_t *size)
{
	int sku;

	if (!size)
		return -EINVAL;

	sku = lec_imx95_get_sku();

	if (sku < 0 || sku >= ARRAY_SIZE(ddr_cfg))
		sku = SKU_CFG_DDR_2G;

	*size = ddr_cfg[sku].bank0 + ddr_cfg[sku].bank1;

	return 0;
}

#if IS_ENABLED(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, struct bd_info *bd)
{
    return 0;
}
#endif

#if IS_ENABLED(CONFIG_OF_BOARD_FIXUP)
int board_fix_fdt(void *rw_fdt_blob)
{
    return 0;
}
#endif
