/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <asm/mach-types.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <linux/ion.h>
#include <mach/ion.h>

#include "devices.h"
#include "board-8960.h"

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define FB_BUF_NUM  3
#else
#define FB_BUF_NUM  2
#endif

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#if defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ)/* jgshin for wvga 24bpp */
#define MSM_FB_PRIM_BUF_SIZE (800 * 480 * 4 * FB_BUF_NUM) /* 4 bpp x 3 pages */
#elif defined(CONFIG_MACH_MSM8960_OSCAR)
#define MSM_FB_PRIM_BUF_SIZE (960 * roundup(540,32) * 4 * FB_BUF_NUM) /* 4 bpp x 3 pages */
#elif defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S) || defined (CONFIG_MACH_MSM8960_EF46L)||\
	  defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE) || defined(CONFIG_MACH_MSM8960_MAGNUS)
#define  MSM_FB_PRIM_BUF_SIZE (1280 * roundup(720,32) * 4 * FB_BUF_NUM)
#elif defined (CONFIG_MACH_MSM8960_EF44S)
#if(BOARD_VER <WS10)
#define  MSM_FB_PRIM_BUF_SIZE (1280 * 800 * 4 * FB_BUF_NUM)
#else
#define  MSM_FB_PRIM_BUF_SIZE (1280 * roundup(720,32) * 4 * FB_BUF_NUM)
#endif
#else
#define MSM_FB_PRIM_BUF_SIZE (1280 * roundup(720,32) * 4 * FB_BUF_NUM) /* 4 bpp x 3 pages */
#endif
#else
#if defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ)/* jgshin for wvga 24bpp */
#define MSM_FB_PRIM_BUF_SIZE (800 * 480 * 4 * FB_BUF_NUM) /* 4 bpp x 2 pages */
#elif defined(CONFIG_MACH_MSM8960_OSCAR)
#define MSM_FB_PRIM_BUF_SIZE (960 * 540 * 4 * FB_BUF_NUM) /* 4 bpp x 3 pages */
#elif defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S) || defined (CONFIG_MACH_MSM8960_EF46L)||\
	  defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE) || defined(CONFIG_MACH_MSM8960_MAGNUS)
#define  MSM_FB_PRIM_BUF_SIZE (1280 * roundup(720,32) * 4 * FB_BUF_NUM)
#elif defined (CONFIG_MACH_MSM8960_EF44S)
#if(BOARD_VER < WS10)
#define  MSM_FB_PRIM_BUF_SIZE (1280 * 800 * 4 * FB_BUF_NUM)
#else
#define  MSM_FB_PRIM_BUF_SIZE (1280 * roundup(720,32) * 4 * FB_BUF_NUM)
#endif
#else
#define MSM_FB_PRIM_BUF_SIZE (1280 * 720 * 4 * FB_BUF_NUM)
#endif
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#define MSM_FB_EXT_BUF_SIZE \
		(roundup((1920 * 1088 * 2), 4096) * 1) /* 2 bpp x 1 page */
#elif defined(CONFIG_FB_MSM_TVOUT)
#define MSM_FB_EXT_BUF_SIZE \
		(roundup((720 * 576 * 2), 4096) * 2) /* 2 bpp x 2 pages */
#else
#define MSM_FB_EXT_BUF_SIZE	0
#endif

#if defined(CONFIG_MACH_MSM8960_EF44S)
#define MSM_FB_EXT_WFD_BUF_SIZE \
		(roundup((1920 * 1088 * 2), 4096) * 1) /* 2 bpp x 1 page */
#else
#define MSM_FB_EXT_WFD_BUF_SIZE	0
#endif

/* Note: must be multiple of 4096 */
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE + MSM_FB_EXT_BUF_SIZE+MSM_FB_EXT_WFD_BUF_SIZE, 4096)

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
//#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1920 * 1200 * 3 * 2), 4096)
#if defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ) /* jgshin for wvga 24bpp */
#define  MSM_FB_OVERLAY0_WRITEBACK_SIZE (800 * 480 * 3 * 2)
#elif defined(CONFIG_MACH_MSM8960_OSCAR)
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (960 * roundup(540,32) * 3 * 2) /* 4 bpp x 3 pages */
#elif defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S) || defined (CONFIG_MACH_MSM8960_EF46L)||\
	  defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE) || defined(CONFIG_MACH_MSM8960_MAGNUS)
#define  MSM_FB_OVERLAY0_WRITEBACK_SIZE (1280 * roundup(720,32) * 3 * 2)
#elif defined (CONFIG_MACH_MSM8960_EF44S)
#if(BOARD_VER < WS10)
#define  MSM_FB_OVERLAY0_WRITEBACK_SIZE (1280 * 800 * 3 * 2)
#else
#define  MSM_FB_OVERLAY0_WRITEBACK_SIZE (1280 * roundup(720,32) * 3 * 2)
#endif /*f(BOARD_VER < WS10) */

#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (1376 * 768 * 3 * 2)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */


#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY1_WRITEBACK */

#define MDP_VSYNC_GPIO 0

#define MIPI_CMD_NOVATEK_QHD_PANEL_NAME	"mipi_cmd_novatek_qhd"
#define MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME	"mipi_video_novatek_qhd"
#define MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME	"mipi_video_toshiba_wsvga"
#define MIPI_VIDEO_TOSHIBA_WUXGA_PANEL_NAME	"mipi_video_toshiba_wuxga"
#define MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME	"mipi_video_chimei_wxga"
#define MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME	"mipi_video_simulator_vga"
#define MIPI_CMD_RENESAS_FWVGA_PANEL_NAME	"mipi_cmd_renesas_fwvga"
#define HDMI_PANEL_NAME	"hdmi_msm"
#define TVOUT_PANEL_NAME	"tvout_msm"

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
unsigned char hdmi_is_primary = 1;
#else
unsigned char hdmi_is_primary;
#endif

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};

#ifndef CONFIG_FB_MSM_MIPI_PANEL_DETECT
static void set_mdp_clocks_for_wuxga(void);
#endif

static int msm_fb_detect_panel(const char *name)
{
	if (machine_is_msm8960_liquid()) {
		if (!strncmp(name, MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_CHIMEI_WXGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;
	} else {
		if (!strncmp(name, MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_TOSHIBA_WSVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

#ifndef CONFIG_FB_MSM_MIPI_PANEL_DETECT
		if (!strncmp(name, MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME,
				strnlen(MIPI_VIDEO_NOVATEK_QHD_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_CMD_NOVATEK_QHD_PANEL_NAME,
				strnlen(MIPI_CMD_NOVATEK_QHD_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_SIMULATOR_VGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_CMD_RENESAS_FWVGA_PANEL_NAME,
				strnlen(MIPI_CMD_RENESAS_FWVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
			return 0;

		if (!strncmp(name, MIPI_VIDEO_TOSHIBA_WUXGA_PANEL_NAME,
				strnlen(MIPI_VIDEO_TOSHIBA_WUXGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN))) {
			set_mdp_clocks_for_wuxga();
			return 0;
		}
#endif
	}

	if (!strncmp(name, HDMI_PANEL_NAME,
			strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
		return 0;

	if (!strncmp(name, TVOUT_PANEL_NAME,
			strnlen(TVOUT_PANEL_NAME,
				PANEL_NAME_MAX_LEN)))
		return 0;

	pr_warning("%s: not supported '%s'", __func__, name);
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

static void mipi_dsi_panel_pwm_cfg(void)
{
	int rc;
	static int mipi_dsi_panel_gpio_configured;
	static struct pm_gpio pwm_enable = {
		.direction        = PM_GPIO_DIR_OUT,
		.output_buffer    = PM_GPIO_OUT_BUF_CMOS,
		.output_value     = 1,
		.pull             = PM_GPIO_PULL_NO,
		.vin_sel          = PM_GPIO_VIN_VPH,
		.out_strength     = PM_GPIO_STRENGTH_HIGH,
		.function         = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol      = 0,
		.disable_pin      = 0,
	};
	static struct pm_gpio pwm_mode = {
		.direction        = PM_GPIO_DIR_OUT,
		.output_buffer    = PM_GPIO_OUT_BUF_CMOS,
		.output_value     = 0,
		.pull             = PM_GPIO_PULL_NO,
		.vin_sel          = PM_GPIO_VIN_S4,
		.out_strength     = PM_GPIO_STRENGTH_HIGH,
		.function         = PM_GPIO_FUNC_2,
		.inv_int_pol      = 0,
		.disable_pin      = 0,
	};

	if (mipi_dsi_panel_gpio_configured == 0) {
		/* pm8xxx: gpio-21, Backlight Enable */
		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(21),
					&pwm_enable);
		if (rc != 0)
			pr_err("%s: pwm_enabled failed\n", __func__);

		/* pm8xxx: gpio-24, Bl: Off, PWM mode */
		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(24),
					&pwm_mode);
		if (rc != 0)
			pr_err("%s: pwm_mode failed\n", __func__);

		mipi_dsi_panel_gpio_configured++;
	}
}

#if defined(CONFIG_MACH_MSM8960_VEGAPVW)
static struct gpiomux_setting gpio_generals[1] = {
    {
        .func = GPIOMUX_FUNC_GPIO, /* oled panel detect */
        .drv = GPIOMUX_DRV_2MA,
        .pull = GPIOMUX_PULL_NONE,
    },
};
static struct msm_gpiomux_config msm8960_vegapvw_oled_det_configs[] = {
    {
        .gpio = 64,
        .settings = {
            [GPIOMUX_ACTIVE]    = &gpio_generals[0],
            [GPIOMUX_SUSPENDED] = &gpio_generals[0],
        },
    },
};
#endif


#ifdef CONFIG_MACH_MSM8960_MAGNUS
static struct gpiomux_setting gpio_generals[1] = {
    {
        .func = GPIOMUX_FUNC_GPIO, /*lcd vci*/
        .drv = GPIOMUX_DRV_2MA,
//        .pull = GPIOMUX_PULL_NONE,
        .pull = GPIOMUX_PULL_DOWN,
    },
};
static struct msm_gpiomux_config msm8960_magnus_ldo_configs[] = {
    {
        .gpio = 81,
        .settings = {
            [GPIOMUX_ACTIVE]    = &gpio_generals[0],
            [GPIOMUX_SUSPENDED] = &gpio_generals[0],
        },
    },
};
static struct msm_gpiomux_config msm8960_magnus_lcd_en_configs[] = {
    {
        .gpio = 10,
        .settings = {
            [GPIOMUX_ACTIVE]    = &gpio_generals[0],
            [GPIOMUX_SUSPENDED] = &gpio_generals[0],
        },
    },
};
#endif //ifdef CONFIG_MACH_MSM8960_MAGNUS

//PANTECH
//static bool dsi_power_on;
static bool dsi_power_on = false;

int gpio43; /* 43:LCD Reset */
#if defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S)|| \
	defined (CONFIG_MACH_MSM8960_EF46L) || defined (CONFIG_MACH_MSM8960_RACERJ)|| \
	defined (CONFIG_MACH_MSM8960_MAGNUS) || defined (CONFIG_MACH_MSM8960_EF44S)
int gpio16, gpio24; /* 16 : LCD backlight */
#elif defined(CONFIG_MACH_MSM8960_SIRIUSLTE)
int gpio15, gpio24;
#endif //elif defined(CONFIG_MACH_MSM8960_MAGNUS)
#if defined(CONFIG_MACH_MSM8960_MAGNUS)
#define MIPI_VCI		81 /* 81 : MAGNUS LCD VCI */
#define MIPI_LCD_EN		10 /* 81 : MAGNUS LCD VCI */
#endif
#if defined(CONFIG_MACH_MSM8960_VEGAPVW)
#define MIPI_OLED_DET		64 
#endif
/**
 * LiQUID panel on/off
 *
 * @param on
 *
 * @return int
 */
static int mipi_dsi_liquid_panel_power(int on)
{
	static struct regulator *reg_l2, *reg_ext_3p3v;
	static int gpio21, gpio24, gpio43;
	int rc;

	mipi_dsi_panel_pwm_cfg();
	pr_debug("%s: on=%d\n", __func__, on);

	gpio21 = PM8921_GPIO_PM_TO_SYS(21); /* disp power enable_n */
	gpio43 = PM8921_GPIO_PM_TO_SYS(43); /* Displays Enable (rst_n)*/
	gpio24 = PM8921_GPIO_PM_TO_SYS(24); /* Backlight PWM */

	if (!dsi_power_on) {

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		reg_ext_3p3v = regulator_get(&msm_mipi_dsi1_device.dev,
			"vdd_lvds_3p3v");
		if (IS_ERR(reg_ext_3p3v)) {
			pr_err("could not get reg_ext_3p3v, rc = %ld\n",
			       PTR_ERR(reg_ext_3p3v));
		    return -ENODEV;
		}

		rc = gpio_request(gpio21, "disp_pwr_en_n");
		if (rc) {
			pr_err("request gpio 21 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = gpio_request(gpio43, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 43 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = gpio_request(gpio24, "disp_backlight_pwm");
		if (rc) {
			pr_err("request gpio 24 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		dsi_power_on = true;
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_ext_3p3v);
		if (rc) {
			pr_err("enable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}

		/* set reset pin before power enable */
		gpio_set_value_cansleep(gpio43, 0); /* disp disable (resx=0) */

		gpio_set_value_cansleep(gpio21, 0); /* disp power enable_n */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 1); /* disp enable */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 0); /* disp enable */
		msleep(20);
		gpio_set_value_cansleep(gpio43, 1); /* disp enable */
		msleep(20);
	} else {
		gpio_set_value_cansleep(gpio43, 0);
		gpio_set_value_cansleep(gpio21, 1);

		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_ext_3p3v);
		if (rc) {
			pr_err("disable reg_ext_3p3v failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	return 0;
}
//#if !defined(CONFIG_MACH_MSM8960_VEGAPVW)
#if 0
static int mipi_dsi_cdp_panel_power(int on)
{
#if defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ) || \
    defined(CONFIG_MACH_MSM8960_OSCAR)
	static struct regulator *reg_l8, *reg_l29, *reg_l2;
#elif defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE) 
	static struct regulator *reg_l8, *reg_l17, *reg_l2;
#elif defined (CONFIG_MACH_MSM8960_EF44S) || defined(CONFIG_MACH_MSM8960_MAGNUS)
	static struct regulator *reg_l23, *reg_l2;
	static int gpio15 /*, gpio24, gpio43*/;  
#else
	static struct regulator *reg_l8, *reg_l23, *reg_l2;
	static int gpio15 /*, gpio24, gpio43*/;  
#endif
	int rc;

	struct pm_gpio gpio43_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_PAIRED,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
#if defined (CONFIG_MACH_MSM8960_SVLTE) || defined (CONFIG_MACH_MSM8960_CSFB) || \
	defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S) || \
	defined (CONFIG_MACH_MSM8960_EF46L) || defined (CONFIG_MACH_MSM8960_RACERJ) || defined (CONFIG_MACH_MSM8960_EF44S) || \
	defined(CONFIG_MACH_MSM8960_SIRIUSLTE) || defined(CONFIG_MACH_MSM8960_MAGNUS)
	struct pm_gpio gpio15_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 1,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
#endif
#if defined(CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S)|| \
	defined (CONFIG_MACH_MSM8960_EF46L) || defined (CONFIG_MACH_MSM8960_MAGNUS) || defined (CONFIG_MACH_MSM8960_EF44S) 
	struct pm_gpio gpio16_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
#endif
#if defined(CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S)|| \
		defined (CONFIG_MACH_MSM8960_EF46L) || defined (CONFIG_MACH_MSM8960_MAGNUS) || defined (CONFIG_MACH_MSM8960_EF44S)|| \
		defined(CONFIG_MACH_MSM8960_SIRIUSLTE) 
	struct pm_gpio gpio24_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
#endif
	

	pr_debug("%s: state : %d\n", __func__, on);

	if (!dsi_power_on) {

#if !defined (CONFIG_MACH_MSM8960_EF44S) && !defined (CONFIG_MACH_MSM8960_MAGNUS)
		reg_l8 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdc");
		if (IS_ERR(reg_l8)) {
			pr_err("could not get 8921_l8, rc = %ld\n",
				PTR_ERR(reg_l8));
			return -ENODEV;
		}
#endif

#if defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ) || \
    defined(CONFIG_MACH_MSM8960_OSCAR)
		reg_l29 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l29)) {
			pr_err("could not get 8921_l29, rc = %ld\n",
			PTR_ERR(reg_l29));

			return -ENODEV;
		}
#elif defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE) 
		reg_l17 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l17)) {
			pr_err("could not get 8921_l17, rc = %ld\n",
			PTR_ERR(reg_l17));

			return -ENODEV;
		}
#else
		reg_l23 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l23)) {
			pr_err("could not get 8921_l23, rc = %ld\n",
				PTR_ERR(reg_l23));
			return -ENODEV;
		}
#endif

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}


#if defined(CONFIG_MACH_MSM8960_VEGAPVW) 
		rc = regulator_set_voltage(reg_l8, 3100000, 3100000);
#elif defined(CONFIG_MACH_MSM8960_SIRIUSLTE)
#if (BOARD_VER == PT11) || (BOARD_VER == PT10)
		rc = regulator_set_voltage(reg_l8, 3100000, 3100000);
#else
		rc = regulator_set_voltage(reg_l8, 2800000, 2800000);
#endif
#elif defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_OSCAR)
		rc = regulator_set_voltage(reg_l8, 3000000, 3000000);
#elif defined(CONFIG_MACH_MSM8960_STARQ)   
		rc = regulator_set_voltage(reg_l8, 2800000, 2800000); /* VCI */
#elif defined(CONFIG_MACH_MSM8960_EF46L)   
		rc = regulator_set_voltage(reg_l8, 2900000, 2900000); 

#elif defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S)
		rc = regulator_set_voltage(reg_l8, 2850000, 2850000);
#elif defined (CONFIG_MACH_MSM8960_EF44S) || defined (CONFIG_MACH_MSM8960_MAGNUS) 
		rc = 0;
#else   //ev board
		rc = regulator_set_voltage(reg_l8, 2800000, 3000000);
#endif		
		if (rc) {
			pr_err("set_voltage l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
	
#if defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ) || \
    defined(CONFIG_MACH_MSM8960_OSCAR)
		rc = regulator_set_voltage(reg_l29, 1800000, 1800000);
#elif defined(CONFIG_MACH_MSM8960_VEGAPVW) 
		rc = regulator_set_voltage(reg_l17, 2200000, 2200000);
#elif defined(CONFIG_MACH_MSM8960_SIRIUSLTE)
#if (BOARD_VER == PT11) || (BOARD_VER == PT10)
		rc = regulator_set_voltage(reg_l17, 2200000, 2200000);
#else
		rc = regulator_set_voltage(reg_l17, 1800000, 1800000);
#endif
#else /* MAGNUS */
		rc = regulator_set_voltage(reg_l23, 1800000, 1800000);
#endif
		if (rc) {
			pr_err("set_voltage l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		gpio43 = PM8921_GPIO_PM_TO_SYS(43);
		rc = gpio_request(gpio43, "disp_rst_n");
		if (rc) {
			pr_err("request gpio 43 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#if defined (CONFIG_MACH_MSM8960_SVLTE) || defined (CONFIG_MACH_MSM8960_CSFB) || \
	defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S)|| \
	defined (CONFIG_MACH_MSM8960_EF46L) || defined (CONFIG_MACH_MSM8960_MAGNUS) || defined (CONFIG_MACH_MSM8960_EF44S)
		gpio15 = PM8921_GPIO_PM_TO_SYS(15);
		rc = gpio_request(gpio15, "lcd_vci");
		if (rc) {
			pr_err("request gpio 15 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif
#if defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S)|| \
	defined (CONFIG_MACH_MSM8960_EF46L)|| defined (CONFIG_MACH_MSM8960_MAGNUS) || defined (CONFIG_MACH_MSM8960_EF44S) 
		gpio16 = PM8921_GPIO_PM_TO_SYS(16);
		rc = gpio_request(gpio16, "lcd_bl_n");
		if (rc) {
			pr_err("request gpio 16 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		gpio24 = PM8921_GPIO_PM_TO_SYS(24);
		rc = gpio_request(gpio24, "disp_backlight_pwm");
		if (rc) {
			pr_err("request gpio 24 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif
#if defined (CONFIG_MACH_MSM8960_SIRIUSLTE)
				gpio15 = PM8921_GPIO_PM_TO_SYS(15);
				rc = gpio_request(gpio15, "lcd_bl_n");
				if (rc) {
					pr_err("request gpio 15 failed, rc=%d\n", rc);
					return -EINVAL;
				}
				gpio24 = PM8921_GPIO_PM_TO_SYS(24);
				rc = gpio_request(gpio24, "disp_backlight_pwm");
				if (rc) {
					pr_err("request gpio 24 failed, rc=%d\n", rc);
					return -EINVAL;
				}
#endif

		dsi_power_on = true;
	}
	if (on) {
#if !defined (CONFIG_MACH_MSM8960_EF44S) && !defined(CONFIG_MACH_MSM8960_MAGNUS)		
		rc = regulator_set_optimum_mode(reg_l8, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif		
#if defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ) || \
    defined(CONFIG_MACH_MSM8960_OSCAR)
		rc = regulator_set_optimum_mode(reg_l29, 100000);
#elif defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE) 
		rc = regulator_set_optimum_mode(reg_l17, 100000);
#else
		rc = regulator_set_optimum_mode(reg_l23, 100000);
#endif
		if (rc < 0) {
#if defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ) 
			pr_err("set_optimum_mode l29 failed, rc=%d\n", rc);
#elif defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE) 
			pr_err("set_optimum_mode l17 failed, rc=%d\n", rc);
#else
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
#endif
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#if !defined (CONFIG_MACH_MSM8960_EF44S) && !defined(CONFIG_MACH_MSM8960_MAGNUS)		
		rc = regulator_enable(reg_l8);
		if (rc) {
			pr_err("enable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#endif		
#if defined(CONFIG_MACH_MSM8960_MAGNUS)
		rc = gpio_request(MIPI_VCI, "mipi_vci_ldo");
		if (rc) {
			pr_err("mipi vci_en gpio_request failed: %d\n", rc);
//			return rc;
		}
		printk("[MAGNUS] mipi vci_en gpio_request sucess..: %d\n", rc);
		gpio_direction_output(MIPI_VCI, 1);
#endif
#if defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ) || \
    defined(CONFIG_MACH_MSM8960_OSCAR)
		rc = regulator_enable(reg_l29);
#elif defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE) 
		rc = regulator_enable(reg_l17);
#else
		rc = regulator_enable(reg_l23);
#endif
		if (rc) {
#if defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ) 
			pr_err("enable l29 failed, rc=%d\n", rc);
#elif defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE) 
			pr_err("enable l17 failed, rc=%d\n", rc);
#else
			pr_err("enable l23 failed, rc=%d\n", rc);
#endif
			return -ENODEV;
		}
		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		gpio43_param.pull = PM_GPIO_PULL_NO;
		rc = pm8xxx_gpio_config(gpio43, &gpio43_param);
		if (rc) {
			pr_err("gpio_config 43 failed (1), rc=%d\n", rc);
			return -EINVAL;
		}
#if defined(CONFIG_MACH_MSM8960_EF46L) && (BOARD_VER > PT10)
		gpio_set_value_cansleep(gpio43, 1);
		usleep(5);
		gpio_set_value_cansleep(gpio43, 0);
		usleep(10);
		gpio_set_value_cansleep(gpio43, 1);
#endif

#if defined (CONFIG_MACH_MSM8960_SVLTE) || defined (CONFIG_MACH_MSM8960_CSFB) || \
	defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S)|| \
	defined (CONFIG_MACH_MSM8960_EF46L) || defined (CONFIG_MACH_MSM8960_MAGNUS) || defined (CONFIG_MACH_MSM8960_EF44S)
		rc = pm8xxx_gpio_config(gpio15, &gpio15_param);
		if (rc) {
			pr_err("gpio_config 15 failed, rc=%d\n", rc);
			return -EINVAL;
		}
        gpio_set_value_cansleep(gpio15, 1);
#endif        
#if defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S)|| \
	defined (CONFIG_MACH_MSM8960_EF46L) || defined (CONFIG_MACH_MSM8960_MAGNUS) || defined (CONFIG_MACH_MSM8960_EF44S) 
		rc = pm8xxx_gpio_config(gpio16, &gpio16_param);
		if (rc) {
			pr_err("gpio_config 16 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = pm8xxx_gpio_config(gpio24, &gpio24_param);
		if (rc) {
			pr_err("gpio_config 24 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif
#if defined (CONFIG_MACH_MSM8960_SIRIUSLTE)
			rc = pm8xxx_gpio_config(gpio15, &gpio15_param);
			if (rc) {
				pr_err("gpio_config 15 failed, rc=%d\n", rc);
				return -EINVAL;
			}
			rc = pm8xxx_gpio_config(gpio24, &gpio24_param);
			if (rc) {
				pr_err("gpio_config 24 failed, rc=%d\n", rc);
				return -EINVAL;
			}
#endif

	} else {  /* off */
///*
		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#if defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE)
		rc = regulator_disable(reg_l8);
		if (rc) {
			pr_err("disable reg_l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}
	    rc = regulator_disable(reg_l17);
		if (rc) {
			pr_err("disable reg_l17 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#elif defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ) 
	    	rc = regulator_disable(reg_l29);
		if (rc) {
			pr_err("disable reg_l29 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#else /* EF44S LDO23 */
#if defined(CONFIG_MACH_MSM8960_MAGNUS)
		gpio_direction_output(MIPI_VCI, 0);
		gpio_free(MIPI_VCI);
#endif
		rc = regulator_disable(reg_l23);
		if (rc) {
			pr_err("disable reg_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
#endif
//*/
#if !defined (CONFIG_MACH_MSM8960_EF44S) && !defined (CONFIG_MACH_MSM8960_MAGNUS)
		rc = regulator_set_optimum_mode(reg_l8, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif		
#if defined(CONFIG_MACH_MSM8960_CHEETAH) || defined(CONFIG_MACH_MSM8960_STARQ) || \
    defined(CONFIG_MACH_MSM8960_OSCAR)
		rc = regulator_set_optimum_mode(reg_l29, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l29 failed, rc=%d\n", rc);
	
			return -EINVAL;
		}
#elif defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE) 
		rc = regulator_set_optimum_mode(reg_l17, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l17 failed, rc=%d\n", rc);
	
			return -EINVAL;
		}
#else
		rc = regulator_set_optimum_mode(reg_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif
		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		gpio_set_value_cansleep(gpio43, 0);
#if defined(CONFIG_MACH_MSM8960_EF46L) && (BOARD_VER > PT10)		
		gpio_set_value_cansleep(gpio16, 0);
#endif
#if defined (CONFIG_MACH_MSM8960_SVLTE) || defined (CONFIG_MACH_MSM8960_CSFB) || \
	defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S)|| \
	defined (CONFIG_MACH_MSM8960_EF46L) || defined (CONFIG_MACH_MSM8960_MAGNUS) || defined (CONFIG_MACH_MSM8960_EF44S)|| \
	defined (CONFIG_MACH_MSM8960_SIRIUSLTE)
		gpio_set_value_cansleep(gpio15, 0);
		gpio_set_value_cansleep(gpio24, 0);
#endif
	}
	return 0;
}

#elif defined (CONFIG_MACH_MSM8960_MAGNUS)
static int mipi_dsi_magnus_panel_power(int on)
{
	static struct regulator *reg_l23, *reg_l2;
	static int gpio15 /*, gpio24, gpio43*/;  
	static int mipi_dsi_cdp_gpio_configured;
	int rc;

	struct pm_gpio gpio43_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
//		.pull = PM_GPIO_PULL_NO,
		.pull = PM_GPIO_PULL_DN,
		.vin_sel = PM_GPIO_VIN_S4,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_PAIRED,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
	
	struct pm_gpio gpio15_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
//		.pull = PM_GPIO_PULL_NO,
		.pull = PM_GPIO_PULL_DN,
		.vin_sel = PM_GPIO_VIN_S4,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
	
	struct pm_gpio gpio16_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 1,
//		.pull = PM_GPIO_PULL_NO,
		.pull = PM_GPIO_PULL_DN,
		.vin_sel = PM_GPIO_VIN_S4,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};	

	struct pm_gpio gpio24_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
//		.pull = PM_GPIO_PULL_NO,
		.pull = PM_GPIO_PULL_DN,
		.vin_sel = PM_GPIO_VIN_S4,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};	
	
	if (mipi_dsi_cdp_gpio_configured == 0) {
       		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(43),
        		         &gpio43_param);
        	if (rc != 0)
        		pr_err("%s: gpio43 failed\n", __func__);

		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(15), &gpio15_param);
        	if (rc != 0)
        		pr_err("%s: gpio15 failed\n", __func__);

		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(16), &gpio16_param);
        	if (rc != 0)
        		pr_err("%s: gpio16 failed\n", __func__);
		
		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(24), &gpio24_param);
        	if (rc != 0)
        		pr_err("%s: gpio24 failed\n", __func__);

        	mipi_dsi_cdp_gpio_configured++;
	}
	
	if (!dsi_power_on) {
		reg_l23 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l23)) {
			pr_err("could not get 8921_l23, rc = %ld\n",
				PTR_ERR(reg_l23));
			return -ENODEV;
		}	
		
		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		gpio43 = PM8921_GPIO_PM_TO_SYS(43);
		gpio15 = PM8921_GPIO_PM_TO_SYS(15);
		gpio16 = PM8921_GPIO_PM_TO_SYS(16);
		gpio24 = PM8921_GPIO_PM_TO_SYS(24);

	        rc = gpio_request(gpio43, "disp_rst_n");
                if (rc) {
                      pr_err("request gpio 43 failed, rc=%d\n", rc);
                      return -ENODEV;
                }

		rc = gpio_request(gpio15, "lcd_vci");
		if (rc) {
			pr_err("request gpio 15 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = gpio_request(gpio16, "lcd_bl");
		if (rc) {
			pr_err("request gpio 16 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = gpio_request(gpio24, "lcd_blx");
		if (rc) {
			pr_err("request gpio 24 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		dsi_power_on = true;
	}

	if (on)
	{
#if defined(CONFIG_MACH_MSM8960_MAGNUS)
		rc = gpio_request(MIPI_VCI, "mipi_vci_ldo");
		if (rc) {
			pr_err("mipi vci_en gpio_request failed: %d\n", rc);
//			return rc;
		}
		printk("[MAGNUS] mipi vci_en gpio_request sucess..: %d\n", rc);
		gpio_direction_output(MIPI_VCI, 1);
		rc = gpio_request(MIPI_LCD_EN, "mipi_lcd_en");
		if (rc) {
			pr_err("mipi lcd_en gpio_request failed: %d\n", rc);
//			return rc;
		}
		printk("[MAGNUS] mipi lcd_en gpio_request sucess..: %d\n", rc);
#endif
		rc = regulator_set_optimum_mode(reg_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}		

		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}	
#if defined(CONFIG_MACH_MSM8960_MAGNUS)
//		rc = gpio_request(MIPI_VCI, "mipi_vci_ldo");
//		if (rc) {
//			pr_err("mipi vci_en gpio_request failed: %d\n", rc);
//			return rc;
//		}
//		printk("[MAGNUS] mipi vci_en gpio_request sucess..: %d\n", rc);
//		gpio_direction_output(MIPI_VCI, 1);
#endif
		rc = regulator_enable(reg_l23);
		if (rc) {
			pr_err("enable l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}	


	    gpio_set_value_cansleep(gpio43, 0);
	    msleep(10);
        gpio_set_value_cansleep(gpio43, 1);
        msleep(10);

#if defined(CONFIG_MACH_MSM8960_MAGNUS)
	    gpio_direction_output(MIPI_LCD_EN, 1);
#endif
        gpio_set_value_cansleep(gpio15, 1);
//		gpio_set_value_cansleep(gpio16, 1);
#if defined(CONFIG_MACH_MSM8960_EF44S)
		gpio_set_value_cansleep(gpio24, 1);
#endif 
			
	}else 
	{
		gpio_set_value_cansleep(gpio15, 0);
		msleep(1);
		gpio_set_value_cansleep(gpio43, 0);
		msleep(1);
		gpio_set_value_cansleep(gpio16, 0);
#if defined(CONFIG_MACH_MSM8960_EF44S)
		gpio_set_value_cansleep(gpio24, 0);
#endif 
#if defined(CONFIG_MACH_MSM8960_MAGNUS)
		gpio_direction_output(MIPI_VCI, 0);
		gpio_free(MIPI_VCI);
		gpio_direction_output(MIPI_LCD_EN, 0);
		gpio_free(MIPI_LCD_EN);
#endif
		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
//#if defined(CONFIG_MACH_MSM8960_MAGNUS)
//		gpio_direction_output(MIPI_VCI, 0);
//		gpio_free(MIPI_VCI);
//#endif
		rc = regulator_disable(reg_l23);
		if (rc) {
			pr_err("disable reg_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		
		rc = regulator_set_optimum_mode(reg_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	return 0;
}

#elif defined(CONFIG_MACH_MSM8960_EF44S)
static int mipi_dsi_ef44_panel_power(int on)
{
	static struct regulator *reg_l23, *reg_l2;
	static int gpio15 /*, gpio24, gpio43*/;  
	static int mipi_dsi_cdp_gpio_configured;
	int rc;

	struct pm_gpio gpio43_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
//		.pull = PM_GPIO_PULL_NO,
		.pull = PM_GPIO_PULL_DN,
		.vin_sel = PM_GPIO_VIN_S4,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_PAIRED,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
	
	struct pm_gpio gpio15_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
//		.pull = PM_GPIO_PULL_NO,
		.pull = PM_GPIO_PULL_DN,
		.vin_sel = PM_GPIO_VIN_S4,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
	
	struct pm_gpio gpio16_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
//		.pull = PM_GPIO_PULL_NO,
		.pull = PM_GPIO_PULL_DN,
		.vin_sel = PM_GPIO_VIN_S4,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};	

	struct pm_gpio gpio24_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
//		.pull = PM_GPIO_PULL_NO,
		.pull = PM_GPIO_PULL_DN,
		.vin_sel = PM_GPIO_VIN_S4,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};	
	
	if (mipi_dsi_cdp_gpio_configured == 0) {
       		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(43),
        		         &gpio43_param);
        	if (rc != 0)
        		pr_err("%s: gpio43 failed\n", __func__);

		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(15), &gpio15_param);
        	if (rc != 0)
        		pr_err("%s: gpio15 failed\n", __func__);

		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(16), &gpio16_param);
        	if (rc != 0)
        		pr_err("%s: gpio16 failed\n", __func__);
		
		rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(24), &gpio24_param);
        	if (rc != 0)
        		pr_err("%s: gpio24 failed\n", __func__);

        	mipi_dsi_cdp_gpio_configured++;
	}
	
	if (!dsi_power_on) {
		reg_l23 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l23)) {
			pr_err("could not get 8921_l23, rc = %ld\n",
				PTR_ERR(reg_l23));
			return -ENODEV;
		}	
		
		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(reg_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		gpio43 = PM8921_GPIO_PM_TO_SYS(43);
		gpio15 = PM8921_GPIO_PM_TO_SYS(15);
		gpio16 = PM8921_GPIO_PM_TO_SYS(16);
		gpio24 = PM8921_GPIO_PM_TO_SYS(24);

	        rc = gpio_request(gpio43, "disp_rst_n");
                if (rc) {
                      pr_err("request gpio 43 failed, rc=%d\n", rc);
                      return -ENODEV;
                }

		rc = gpio_request(gpio15, "lcd_vci");
		if (rc) {
			pr_err("request gpio 15 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = gpio_request(gpio16, "lcd_bl");
		if (rc) {
			pr_err("request gpio 16 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = gpio_request(gpio24, "lcd_blx");
		if (rc) {
			pr_err("request gpio 24 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		dsi_power_on = true;
	}

	if (on)
	{
		rc = regulator_set_optimum_mode(reg_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}		

		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}	
		rc = regulator_enable(reg_l23);
		if (rc) {
			pr_err("enable l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}	
		gpio_set_value_cansleep(gpio15, 1);
		usleep(10);

		gpio_set_value_cansleep(gpio43, 0);
		msleep(1);
		gpio_set_value_cansleep(gpio43, 1);
              msleep(1);
     		   
     	       //gpio_set_value_cansleep(gpio24, 1);
              //usleep(10);
		//gpio_set_value_cansleep(gpio16, 1);
	}else 
	{
		gpio_set_value_cansleep(gpio43, 0);
		msleep(1);
		gpio_set_value_cansleep(gpio16, 0);
		gpio_set_value_cansleep(gpio24, 0);
		gpio_set_value_cansleep(gpio15, 0);
		
		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_l23);
		if (rc) {
			pr_err("disable reg_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		
		rc = regulator_set_optimum_mode(reg_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	return 0;
}

#elif defined (CONFIG_MACH_MSM8960_VEGAPVW)
static int mipi_dsi_premia_panel_power(int on)
{
	static struct regulator *reg_l8, *reg_l17, *reg_l2;
	static int mipi_dsi_cdp_gpio_configured;
	int rc;

	struct pm_gpio gpio43_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
		.pull = PM_GPIO_PULL_DN,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_PAIRED,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
	

        if (mipi_dsi_cdp_gpio_configured == 0) {
        	rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(43),
        		         &gpio43_param);
        	if (rc != 0)
        		pr_err("%s: gpio43 failed\n", __func__);
        	mipi_dsi_cdp_gpio_configured++;
	}


	if (!dsi_power_on) {
		reg_l17 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l17)) {
			pr_err("could not get 8921_l17, rc = %ld\n",
			PTR_ERR(reg_l17));

			return -ENODEV;
		}

		reg_l8 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdc");
		if (IS_ERR(reg_l8)) {
			pr_err("could not get 8921_l8, rc = %ld\n",
				PTR_ERR(reg_l8));
			return -ENODEV;
		}

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}


		rc = regulator_set_voltage(reg_l8, 3100000, 3100000);
	       	if (rc) {
                        pr_err("set_voltage l8 failed, rc=%d\n", rc);
                        return -EINVAL;
                }
		rc = regulator_set_voltage(reg_l17, 2200000, 2200000);
	       	if (rc) {
                        pr_err("set_voltage l17 failed, rc=%d\n", rc);
                        return -EINVAL;
                }
 		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
	       	if (rc) {
                        pr_err("set_voltage l2 failed, rc=%d\n", rc);
                        return -EINVAL;
                }
		
		gpio43 = PM8921_GPIO_PM_TO_SYS(43);
                rc = gpio_request(gpio43, "disp_rst_n");
                if (rc) {
                        pr_err("request gpio 43 failed, rc=%d\n", rc);
                        return -ENODEV;
                }

		dsi_power_on = true;
	}

	if (on)
	{
                rc = regulator_set_optimum_mode(reg_l17, 100000);
 		if (rc < 0) {
  			pr_err("set_optimum_mode l17 failed, rc=%d\n", rc);
  			return -EINVAL;
   		}	

		rc = regulator_set_optimum_mode(reg_l8, 100000);
                if (rc < 0) {
                        pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
                        return -EINVAL;
                }
	
		rc = regulator_set_optimum_mode(reg_l2, 100000);
        if (rc < 0) {
               pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
               return -EINVAL;
        }
        rc = regulator_enable(reg_l2);
                if (rc) {
                        pr_err("enable l2 failed, rc=%d\n", rc);
                        return -ENODEV;
                }
				msleep(1);
                rc = regulator_enable(reg_l8);
                if (rc) {
                        pr_err("enable l8 failed, rc=%d\n", rc);
                        return -ENODEV;
                }
				msleep(1);

				rc = regulator_enable(reg_l17);
				if (rc) {
                        pr_err("enable l17 failed, rc=%d\n", rc);
                        return -ENODEV;
                }
		rc = gpio_request(MIPI_OLED_DET, "mipi_oled_det");
		if (rc) {
			pr_err("mipi oled_det gpio_request failed: %d\n", rc);
//			return rc;
		}
		msleep(1);

	     	    gpio_set_value_cansleep(gpio43, 0);
	       		msleep(10);
                gpio_set_value_cansleep(gpio43, 1);
                msleep(20);
               // msleep(30);

	}else
	{
		gpio_free(MIPI_OLED_DET);

		rc = regulator_disable(reg_l2);
		if (rc) {
                        pr_err("enable l2 failed, rc=%d\n", rc);
                        return -ENODEV;
                }

				msleep(1);
                rc = regulator_disable(reg_l8);
                if (rc) {
                        pr_err("enable l8 failed, rc=%d\n", rc);
                        return -ENODEV;
                }
				msleep(1);

                rc = regulator_disable(reg_l17);
                if (rc) {
                        pr_err("enable l17 failed, rc=%d\n", rc);
                        return -ENODEV;
                }
				msleep(1);
                rc = regulator_set_optimum_mode(reg_l2, 100);
                if (rc < 0) {
                        pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
                        return -EINVAL;
                }

                rc = regulator_set_optimum_mode(reg_l8, 100);
                if (rc < 0) {
                        pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
                        return -EINVAL;
                }
                rc = regulator_set_optimum_mode(reg_l17, 100);
                if (rc < 0) {
                        pr_err("set_optimum_mode l17 failed, rc=%d\n", rc);
                        return -EINVAL;
                }
				msleep(10);
				gpio_set_value_cansleep(gpio43, 0);

		
	}
	return 0;
	
}
#elif defined (CONFIG_MACH_MSM8960_SIRIUSLTE)
static int mipi_dsi_maruko_panel_power(int on)
{
	static struct regulator *reg_l8, *reg_l17, *reg_l2;
	static int mipi_dsi_cdp_gpio_configured;
	int rc;

	struct pm_gpio gpio43_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_PAIRED,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
	
 	struct pm_gpio gpio15_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 1,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};

#if (BOARD_VER < WS10)
	struct pm_gpio gpio24_param = {
		.direction = PM_GPIO_DIR_OUT,
		.output_buffer = PM_GPIO_OUT_BUF_CMOS,
		.output_value = 0,
		.pull = PM_GPIO_PULL_NO,
		.vin_sel = 2,
		.out_strength = PM_GPIO_STRENGTH_HIGH,
		.function = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol = 0,
		.disable_pin = 0,
	};
#endif
	
        if (mipi_dsi_cdp_gpio_configured == 0) {
        	rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(43),
        		         &gpio43_param);
        	if (rc != 0)
        		pr_err("%s: gpio43 failed\n", __func__);

#if 0
			gpio43_param.pull = PM_GPIO_PULL_NO;
			rc = pm8xxx_gpio_config(gpio43, &gpio43_param);
			if (rc) {
				pr_err("gpio_config 43 failed (1), rc=%d\n", rc);
				return -EINVAL;
			}
#endif
			rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(15), &gpio15_param);
        	if (rc != 0)
        		pr_err("%s: gpio15 failed\n", __func__);

#if (BOARD_VER < WS10)
			rc = pm8xxx_gpio_config(PM8921_GPIO_PM_TO_SYS(24), &gpio24_param);
        	if (rc != 0)
        		pr_err("%s: gpio24 failed\n", __func__);
#endif
		
        	mipi_dsi_cdp_gpio_configured++;
	}

	if (!dsi_power_on) {
		reg_l8 = regulator_get(&msm_mipi_dsi1_device.dev,
							"dsi_vdc");
		if (IS_ERR(reg_l8)) {
			pr_err("could not get 8921_l8, rc = %ld\n",
				PTR_ERR(reg_l8));
			return -ENODEV;
		}

		reg_l17 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l17)) {
			pr_err("could not get 8921_l17, rc = %ld\n",
			PTR_ERR(reg_l17));

			return -ENODEV;
		}

		reg_l2 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2));
			return -ENODEV;
		}

#if (BOARD_VER == PT11) || (BOARD_VER == PT10)
		rc = regulator_set_voltage(reg_l8, 3100000, 3100000);
		if (rc) {
			pr_err("set_voltage l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_voltage(reg_l17, 2200000, 2200000);
		if (rc) {
			pr_err("set_voltage l17 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#else
		rc = regulator_set_voltage(reg_l8, 2800000, 2800000);
		if (rc) {
			pr_err("set_voltage l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_voltage(reg_l17, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage l17 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif
		rc = regulator_set_voltage(reg_l2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		
		gpio43 = PM8921_GPIO_PM_TO_SYS(43);
        rc = gpio_request(gpio43, "disp_rst_n");
        if (rc) {
             pr_err("request gpio 43 failed, rc=%d\n", rc);
             return -ENODEV;
        }

		gpio15 = PM8921_GPIO_PM_TO_SYS(15);
		rc = gpio_request(gpio15, "lcd_bl_n");
		if (rc) {
			pr_err("request gpio 15 failed, rc=%d\n", rc);
			return -EINVAL;
		}

#if (BOARD_VER < WS10)		
		gpio24 = PM8921_GPIO_PM_TO_SYS(24);
		rc = gpio_request(gpio24, "disp_backlight_pwm");
		if (rc) {
			pr_err("request gpio 24 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif
		dsi_power_on = true;
	}

	if (on )
	{
		rc = regulator_set_optimum_mode(reg_l8, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		
		rc = regulator_set_optimum_mode(reg_l17, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l17 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		
		rc = regulator_set_optimum_mode(reg_l2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		
		rc = regulator_enable(reg_l8);
		if (rc) {
			pr_err("enable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_l17);
		if (rc) {
			pr_err("enable l17 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_enable(reg_l2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		gpio_set_value_cansleep(gpio15, 1);  // lcd_en 

	}else
	{
		rc = regulator_disable(reg_l2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		
		rc = regulator_disable(reg_l8);
		if (rc) {
			pr_err("disable reg_l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}

	       rc = regulator_disable(reg_l17);
		if (rc) {
			pr_err("disable reg_l17 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_set_optimum_mode(reg_l8, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		
		rc = regulator_set_optimum_mode(reg_l17, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l17 failed, rc=%d\n", rc);
	
			return -EINVAL;
		}
		
		rc = regulator_set_optimum_mode(reg_l2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		gpio_set_value_cansleep(gpio43, 0);
		gpio_set_value_cansleep(gpio15, 0);
#if (BOARD_VER < WS10)		
		gpio_set_value_cansleep(gpio24, 0);   
#endif
	}

	return 0;
}
#endif

static int mipi_dsi_panel_power(int on)
{
	int ret;

	pr_debug("%s: on=%d\n", __func__, on);

	if (machine_is_msm8960_liquid())
		ret = mipi_dsi_liquid_panel_power(on);
	else
#if defined(CONFIG_MACH_MSM8960_VEGAPVW)
		ret = mipi_dsi_premia_panel_power(on);
#elif defined(CONFIG_MACH_MSM8960_MAGNUS)
	    ret = mipi_dsi_magnus_panel_power(on);
#elif defined(CONFIG_MACH_MSM8960_EF44S) 
	    ret = mipi_dsi_ef44_panel_power(on);
#elif defined(CONFIG_MACH_MSM8960_SIRIUSLTE)
	    ret = mipi_dsi_maruko_panel_power(on);
#else
	//	ret = mipi_dsi_cdp_panel_power(on);
#endif

	return ret;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
#ifdef CONFIG_MACH_MSM8960_OSCAR
	.vsync_gpio = -1,
#else
	.vsync_gpio = MDP_VSYNC_GPIO,
#endif
	.dsi_power_save = mipi_dsi_panel_power,
};

#ifdef CONFIG_MSM_BUS_SCALING

static struct msm_bus_vectors rotator_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors rotator_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1024 * 600 * 4 * 2 * 60),
		.ib  = (1024 * 600 * 4 * 2 * 60 * 1.5),
	},
};

static struct msm_bus_vectors rotator_vga_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (640 * 480 * 2 * 2 * 30),
		.ib  = (640 * 480 * 2 * 2 * 30 * 1.5),
	},
};
static struct msm_bus_vectors rotator_720p_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1280 * 736 * 2 * 2 * 30),
		.ib  = (1280 * 736 * 2 * 2 * 30 * 1.5),
	},
};

static struct msm_bus_vectors rotator_1080p_vectors[] = {
	{
		.src = MSM_BUS_MASTER_ROTATOR,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = (1920 * 1088 * 2 * 2 * 30),
		.ib  = (1920 * 1088 * 2 * 2 * 30 * 1.5),
	},
};

static struct msm_bus_paths rotator_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(rotator_init_vectors),
		rotator_init_vectors,
	},
	{
		ARRAY_SIZE(rotator_ui_vectors),
		rotator_ui_vectors,
	},
	{
		ARRAY_SIZE(rotator_vga_vectors),
		rotator_vga_vectors,
	},
	{
		ARRAY_SIZE(rotator_720p_vectors),
		rotator_720p_vectors,
	},
	{
		ARRAY_SIZE(rotator_1080p_vectors),
		rotator_1080p_vectors,
	},
};

struct msm_bus_scale_pdata rotator_bus_scale_pdata = {
	rotator_bus_scale_usecases,
	ARRAY_SIZE(rotator_bus_scale_usecases),
	.name = "rotator",
};

static struct msm_bus_vectors mdp_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
static struct msm_bus_vectors hdmi_as_primary_vectors[] = {
	/* If HDMI is used as primary */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 2000000000,
		.ib = 2000000000,
	},
};
static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
	{
		ARRAY_SIZE(hdmi_as_primary_vectors),
		hdmi_as_primary_vectors,
	},
};
#else
static struct msm_bus_vectors mdp_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 230400000 * 2,
		.ib = 288000000 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
#if defined(CONFIG_MACH_MSM8960_VEGAPVW) /* p13447 shinjg for mdp rotate underrun issue */
		.ab = 334080000 * 3,
		.ib = 417600000 * 3,
#else
		.ab = 334080000 * 2,
		.ib = 417600000 * 2,
#endif
	},
};

static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};
#endif

static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};

#endif

static int mdp_core_clk_rate_table[] = {
	85330000,
	128000000,
	160000000,
	200000000,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = MDP_VSYNC_GPIO,
	.mdp_core_clk_rate = 85330000,
	.mdp_core_clk_table = mdp_core_clk_rate_table,
	.num_mdp_clk = ARRAY_SIZE(mdp_core_clk_rate_table),
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_42,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_MM_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	.cont_splash_enabled = 0x01,
};
#if defined(CONFIG_MACH_MSM8960_CHEETAH)
static struct platform_device mipi_dsi_samsung_oled_panel_device = {
	.name = "mipi_samsung_oled",
	.id = 0,
};
#elif defined(CONFIG_MACH_MSM8960_STARQ)
static struct platform_device mipi_dsi_lgd_panel_device = {
	.name = "mipi_lgd",
	.id = 0,
};
#elif defined(CONFIG_MACH_MSM8960_VEGAPVW) 
static struct platform_device mipi_dsi_samsung_oled_hd_panel_device = {
	.name = "mipi_samsung_oled_hd",
	.id = 0,
};
#elif defined(CONFIG_MACH_MSM8960_SIRIUSLTE)
#if (BOARD_VER == PT11) || (BOARD_VER == PT10)
static struct platform_device mipi_dsi_samsung_oled_hd_panel_device = {
	.name = "mipi_samsung_oled_hd",
	.id = 0,
};
#else	
static struct platform_device mipi_dsi_renesas_hd_panel_device = {
	.name = "mipi_renesas_hd",
	.id = 0,
};
#endif
#elif defined(CONFIG_MACH_MSM8960_EF45K) || defined(CONFIG_MACH_MSM8960_EF47S)
#if(BOARD_VER < WS10)
static struct platform_device mipi_dsi_sony_panel_device = {
	.name = "mipi_sony",
	.id = 0,
};
#else
static struct platform_device mipi_dsi_rohm_panel_device = {
	.name = "mipi_rohm",
	.id = 0,
};
#endif
#elif defined(CONFIG_MACH_MSM8960_EF46L) 
#if(BOARD_VER < WS10)
static struct platform_device mipi_dsi_sony_panel_device = {
	.name = "mipi_sony",
	.id = 0,
};
#else
static struct platform_device mipi_dsi_ortus_panel_device = {
	.name = "mipi_ortus",
	.id = 0,
};
#endif

#elif defined(CONFIG_MACH_MSM8960_EF44S)
#if(BOARD_VER < WS10)
static struct platform_device mipi_dsi_sony_panel_device = {
	.name = "mipi_sony",
	.id = 0,
};
#else
static struct platform_device mipi_dsi_sony_wm_panel_device = {
	.name = "mipi_sony_wm",
	.id = 0,
};
#endif

#elif defined(CONFIG_MACH_MSM8960_RACERJ)
static struct platform_device mipi_dsi_himax_panel_device = {
	.name = "mipi_himax",
	.id = 0,
};
#elif defined(CONFIG_MACH_MSM8960_OSCAR)
static struct platform_device mipi_dsi_samsung_oled_panel_octa_device = {
		.name = "mipi_oscar",
		.id = 0,
	};

#elif defined(CONFIG_MACH_MSM8960_MAGNUS)
static struct platform_device mipi_dsi_rohm_panel_device = {
	.name = "mipi_rohm",
	.id = 0,
};
#endif

#ifndef CONFIG_FB_MSM_MIPI_PANEL_DETECT
/**
 * Set MDP clocks to high frequency to avoid DSI underflow
 * when using high resolution 1200x1920 WUXGA panels
 */
static void set_mdp_clocks_for_wuxga(void)
{
	int i;

	mdp_ui_vectors[0].ab = 2000000000;
	mdp_ui_vectors[0].ib = 2000000000;
	mdp_vga_vectors[0].ab = 2000000000;
	mdp_vga_vectors[0].ib = 2000000000;
	mdp_720p_vectors[0].ab = 2000000000;
	mdp_720p_vectors[0].ib = 2000000000;
	mdp_1080p_vectors[0].ab = 2000000000;
	mdp_1080p_vectors[0].ib = 2000000000;

	mdp_pdata.mdp_core_clk_rate = 200000000;

	for (i = 0; i < ARRAY_SIZE(mdp_core_clk_rate_table); i++)
		mdp_core_clk_rate_table[i] = 200000000;

}
#endif

void __init msm8960_mdp_writeback(struct memtype_reserve* reserve_table)
{
	mdp_pdata.ov0_wb_size = MSM_FB_OVERLAY0_WRITEBACK_SIZE;
	mdp_pdata.ov1_wb_size = MSM_FB_OVERLAY1_WRITEBACK_SIZE;
#if defined(CONFIG_ANDROID_PMEM) && !defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov0_wb_size;
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov1_wb_size;
#endif
}

static struct platform_device mipi_dsi_renesas_panel_device = {
	.name = "mipi_renesas",
	.id = 0,
};

static struct platform_device mipi_dsi_simulator_panel_device = {
	.name = "mipi_simulator",
	.id = 0,
};

#if defined(CONFIG_FB_MSM_MIPI_DSI_TOSHIBA)
#define LPM_CHANNEL0 0
static int toshiba_gpio[] = {LPM_CHANNEL0};

static struct mipi_dsi_panel_platform_data toshiba_pdata = {
	.gpio = toshiba_gpio,
	.dsi_pwm_cfg = mipi_dsi_panel_pwm_cfg,
};

static struct platform_device mipi_dsi_toshiba_panel_device = {
	.name = "mipi_toshiba",
	.id = 0,
	.dev = {
		.platform_data = &toshiba_pdata,
	}
};
#endif

#define FPGA_3D_GPIO_CONFIG_ADDR	0xB5
static int dsi2lvds_gpio[2] = {
	0,/* Backlight PWM-ID=0 for PMIC-GPIO#24 */
	0x1F08 /* DSI2LVDS Bridge GPIO Output, mask=0x1f, out=0x08 */
	};

static struct msm_panel_common_pdata mipi_dsi2lvds_pdata = {
	.gpio_num = dsi2lvds_gpio,
};

static struct mipi_dsi_phy_ctrl dsi_novatek_cmd_mode_phy_db = {

/* DSI_BIT_CLK at 500MHz, 2 lane, RGB888 */
	{0x0F, 0x0a, 0x04, 0x00, 0x20},	/* regulator */
	/* timing   */
	{0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
	0x0c, 0x03, 0x04, 0xa0},
	{0x5f, 0x00, 0x00, 0x10},	/* phy ctrl */
	{0xff, 0x00, 0x06, 0x00},	/* strength */
	/* pll control */
	{0x40, 0xf9, 0x30, 0xda, 0x00, 0x40, 0x03, 0x62,
	0x40, 0x07, 0x03,
	0x00, 0x1a, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01},
};

static struct mipi_dsi_panel_platform_data novatek_pdata = {
	.fpga_3d_config_addr  = FPGA_3D_GPIO_CONFIG_ADDR,
	.fpga_ctrl_mode = FPGA_SPI_INTF,
	.phy_ctrl_settings = &dsi_novatek_cmd_mode_phy_db,
};

static struct platform_device mipi_dsi_novatek_panel_device = {
	.name = "mipi_novatek",
	.id = 0,
	.dev = {
		.platform_data = &novatek_pdata,
	}
};

static struct platform_device mipi_dsi2lvds_bridge_device = {
	.name = "mipi_tc358764",
	.id = 0,
	.dev.platform_data = &mipi_dsi2lvds_pdata,
};

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct resource hdmi_msm_resources[] = {
	{
		.name  = "hdmi_msm_qfprom_addr",
		.start = 0x00700000,
		.end   = 0x007060FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_hdmi_addr",
		.start = 0x04A00000,
		.end   = 0x04A00FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_irq",
		.start = HDMI_IRQ,
		.end   = HDMI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_cec_power(int on);
#ifdef CONFIG_PANTECH_FB_MSM_MHL_SII9244
int mhl_power_ctrl(int on);
#endif
static struct msm_hdmi_platform_data hdmi_msm_data = {
	.irq = HDMI_IRQ,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_cec_power,
};

static struct platform_device hdmi_msm_device = {
	.name = "hdmi_msm",
	.id = 0,
	.num_resources = ARRAY_SIZE(hdmi_msm_resources),
	.resource = hdmi_msm_resources,
	.dev.platform_data = &hdmi_msm_data,
};
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
static struct platform_device wfd_panel_device = {
	.name = "wfd_panel",
	.id = 0,
	.dev.platform_data = NULL,
};

static struct platform_device wfd_device = {
	.name          = "msm_wfd",
	.id            = -1,
};
#endif

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors dtv_bus_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 566092800 * 2,
		.ib = 707616000 * 2,
	},
};

static struct msm_bus_paths dtv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(dtv_bus_init_vectors),
		dtv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(dtv_bus_def_vectors),
		dtv_bus_def_vectors,
	},
};
static struct msm_bus_scale_pdata dtv_bus_scale_pdata = {
	dtv_bus_scale_usecases,
	ARRAY_SIZE(dtv_bus_scale_usecases),
	.name = "dtv",
};

static struct lcdc_platform_data dtv_pdata = {
	.bus_scale_table = &dtv_bus_scale_pdata,
};
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static int hdmi_enable_5v(int on)
{
	/* TBD: PM8921 regulator instead of 8901 */
	static struct regulator *reg_8921_hdmi_mvs;	/* HDMI_5V */
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!reg_8921_hdmi_mvs)
		reg_8921_hdmi_mvs = regulator_get(&hdmi_msm_device.dev,
			"hdmi_mvs");

	if (on) {
		rc = regulator_enable(reg_8921_hdmi_mvs);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
			return rc;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_hdmi_mvs);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"8921_hdmi_mvs", rc);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
}

static int hdmi_core_power(int on, int show)
{
	static struct regulator *reg_8921_l23, *reg_8921_s4;
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	/* TBD: PM8921 regulator instead of 8901 */
	if (!reg_8921_l23) {
		reg_8921_l23 = regulator_get(&hdmi_msm_device.dev, "hdmi_avdd");
		if (IS_ERR(reg_8921_l23)) {
			pr_err("could not get reg_8921_l23, rc = %ld\n",
				PTR_ERR(reg_8921_l23));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_l23, rc=%d\n", rc);
			return -EINVAL;
		}
	}
	if (!reg_8921_s4) {
		reg_8921_s4 = regulator_get(&hdmi_msm_device.dev, "hdmi_vcc");
		if (IS_ERR(reg_8921_s4)) {
			pr_err("could not get reg_8921_s4, rc = %ld\n",
				PTR_ERR(reg_8921_s4));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_s4, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_s4, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_8921_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_8921_l23);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_avdd", rc);
			return rc;
		}
		rc = regulator_enable(reg_8921_s4);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_vcc", rc);
			return rc;
		}
		rc = gpio_request(100, "HDMI_DDC_CLK");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_CLK", 100, rc);
			goto error1;
		}
		rc = gpio_request(101, "HDMI_DDC_DATA");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_DATA", 101, rc);
			goto error2;
		}
		rc = gpio_request(102, "HDMI_HPD");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_HPD", 102, rc);
			goto error3;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(100);
		gpio_free(101);
		gpio_free(102);

		rc = regulator_disable(reg_8921_l23);
		if (rc) {
			pr_err("disable reg_8921_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_8921_s4);
		if (rc) {
			pr_err("disable reg_8921_s4 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_8921_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;

error3:
	gpio_free(101);
error2:
	gpio_free(100);
error1:
	regulator_disable(reg_8921_l23);
	regulator_disable(reg_8921_s4);
	return rc;
}

static int hdmi_cec_power(int on)
{
	static int prev_on;
//	int rc;

	if (on == prev_on)
		return 0;

	if (on) {
	/*
		rc = gpio_request(99, "HDMI_CEC_VAR");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_CEC_VAR", 99, rc);
			goto error;
		}
		*/
		pr_debug("%s(on): success\n", __func__);
	} else {
	//	gpio_free(99);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
	/*
error:
	return rc;
	*/
}
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

#ifdef CONFIG_PANTECH_F_HDMI_PW_CTRL_ON_LCD
int hdmi_autodetect_control(int on)
{
int rc;
static int prev_on;


if (on == prev_on)
		return 0;

if (on)
	
{	rc = hdmi_enable_5v(on);
	if(rc) goto error;
	rc = hdmi_core_power(on, 1);
	if(rc) goto error;
	rc = hdmi_cec_power(on);
	if(rc) goto error;
}
else
{
	rc = hdmi_enable_5v(on);
	if(rc) goto error;
	rc = hdmi_core_power(on, 1);
	if(rc) goto error;
	rc = hdmi_cec_power(on);
	if(rc) goto error;

}
	prev_on = on;
	return 0;
	error:
		return rc;
}
EXPORT_SYMBOL(hdmi_autodetect_control);
#endif
#ifdef CONFIG_PANTECH_FB_MSM_MHL_SII9244
int mhl_power_ctrl(int on)
{


    static struct regulator *reg_8058_l12;
#ifdef CONFIG_PANTECH_MHL_VCC_3P3
	static struct regulator *reg_8058_l17;
#endif
    int gpio14=0;
	static int prev_on;
	int rc=0;

	if (on == prev_on)
		return 0;

	printk(KERN_ERR "[SKY_MHL]+%s 1st needed mhl-power-on\n", __FUNCTION__);

	if (!reg_8058_l12)
		reg_8058_l12 = regulator_get(&hdmi_msm_device.dev, "mhl_vcc");
	
	if (IS_ERR(reg_8058_l12)) {
			pr_err("%s:get regulator reg_8058_l12 failed\n",__func__);
			return -EINVAL;
	}

#ifdef CONFIG_PANTECH_MHL_VCC_3P3
if (!reg_8058_l17)
		reg_8058_l17 = regulator_get(&hdmi_msm_device.dev, "mhl_vcc_3p3");
	
	if (IS_ERR(reg_8058_l17)) {
			pr_err("%s:get regulator reg_8058_l17 failed\n",__func__);
			return -EINVAL;
	}

#else
gpio14 = PM8921_GPIO_PM_TO_SYS(14);
		if(!gpio14)
		rc = gpio_request(gpio14, "mhl_vcc_en");

		if (rc) {
			pr_err("request gpio 14 failed, rc=%d\n", rc);
			return -EINVAL;
		}
#endif		
	if (on) {
		hdmi_core_power(1, 0);
		hdmi_cec_power(1);
		hdmi_enable_5v(0); /* hdmi 5v is unused */
		
		rc = regulator_set_voltage(reg_8058_l12, 1200000, 1200000);
		if (!rc)
			rc = regulator_enable(reg_8058_l12);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",

					"8058_l12", rc);
			return rc;
		}




		if (rc) {
			pr_err("gpio_config 14 failed, rc=%d\n", rc);
			return -rc;
		}
#ifdef CONFIG_PANTECH_MHL_VCC_3P3

	rc = regulator_set_voltage(reg_8058_l17, 3300000, 3300000);
	if (!rc)
		rc = regulator_enable(reg_8058_l17);
	if (rc) {
		pr_err("'%s' regulator enable failed, rc=%d\n",

				"8058_l17", rc);
		return rc;
	}

#else
		gpio_set_value_cansleep(gpio14, 1);
#endif		
		pr_info("%s(on): success\n", __func__);
		
	} else {

		hdmi_core_power(0, 0);
		hdmi_cec_power(0);
		hdmi_enable_5v(0);
		
		rc = regulator_disable(reg_8058_l12);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
					"8921_l12", rc);
#ifdef CONFIG_PANTECH_MHL_VCC_3P3		
		rc = regulator_disable(reg_8058_l17);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
					"8921_l17", rc);
#else
		gpio_set_value_cansleep(gpio14, 0);

#endif
		printk("gpio14!!!=%d",gpio14);
		gpio14=0;
	}

	prev_on = on;

return 0;

}
EXPORT_SYMBOL(mhl_power_ctrl);
#endif

static struct platform_device * oem_panel_devices[] = {

#if defined(CONFIG_MACH_MSM8960_CHEETAH)
	&mipi_dsi_samsung_oled_panel_device,
#elif defined(CONFIG_MACH_MSM8960_STARQ)
	&mipi_dsi_lgd_panel_device,
#elif defined(CONFIG_MACH_MSM8960_VEGAPVW) 
	&mipi_dsi_samsung_oled_hd_panel_device,
#elif defined(CONFIG_MACH_MSM8960_SIRIUSLTE)
#if (BOARD_VER == PT11) || (BOARD_VER == PT10)
    &mipi_dsi_samsung_oled_hd_panel_device,
#else
	&mipi_dsi_renesas_hd_panel_device
#endif
#elif defined(CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF47S)
#if(BOARD_VER < WS10)
	&mipi_dsi_sony_panel_device,
#else
	&mipi_dsi_rohm_panel_device,
#endif
#elif defined(CONFIG_MACH_MSM8960_EF46L)
#if(BOARD_VER < WS10)
	&mipi_dsi_sony_panel_device,
#else
	&mipi_dsi_ortus_panel_device,
#endif
#elif defined (CONFIG_MACH_MSM8960_EF44S)
#if(BOARD_VER < WS10)
	&mipi_dsi_sony_panel_device,
#else
	&mipi_dsi_sony_wm_panel_device,
#endif
#elif defined(CONFIG_MACH_MSM8960_RACERJ)
	&mipi_dsi_himax_panel_device,
#elif defined(CONFIG_MACH_MSM8960_OSCAR)
	&mipi_dsi_samsung_oled_panel_octa_device,
#elif defined(CONFIG_MACH_MSM8960_MAGNUS)
	&mipi_dsi_rohm_panel_device,
#elif defined(CONFIG_FB_MSM_MIPI_DSI_TOSHIBA) /* not used default device */
	&mipi_dsi_toshiba_panel_device,
#endif
};


void __init msm8960_init_fb(void)
{
#if defined(CONFIG_MACH_MSM8960_MAGNUS) /* VCI_LDO GPIO Initialized for MAGNUS shinjg */
	msm_gpiomux_install(msm8960_magnus_ldo_configs,
			ARRAY_SIZE(msm8960_magnus_ldo_configs));
	msm_gpiomux_install(msm8960_magnus_lcd_en_configs,
			ARRAY_SIZE(msm8960_magnus_lcd_en_configs));
#elif defined(CONFIG_MACH_MSM8960_VEGAPVW)
	msm_gpiomux_install(msm8960_vegapvw_oled_det_configs,
			ARRAY_SIZE(msm8960_vegapvw_oled_det_configs));
#endif
	platform_device_register(&msm_fb_device);

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
	platform_device_register(&wfd_panel_device);
	platform_device_register(&wfd_device);
#endif

	if (machine_is_msm8960_sim())
		platform_device_register(&mipi_dsi_simulator_panel_device);

	if (machine_is_msm8960_rumi3())
		platform_device_register(&mipi_dsi_renesas_panel_device);

	if (!machine_is_msm8960_sim() && !machine_is_msm8960_rumi3()) {
		platform_device_register(&mipi_dsi_novatek_panel_device);

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
		platform_device_register(&hdmi_msm_device);
#endif
	}

	if (machine_is_msm8960_liquid())
		platform_device_register(&mipi_dsi2lvds_bridge_device);
	else
	/* Add routine for LS2 LCD Part */
		platform_add_devices(oem_panel_devices, ARRAY_SIZE(oem_panel_devices));

	if (machine_is_msm8x60_rumi3()) {
		msm_fb_register_device("mdp", NULL);
		mipi_dsi_pdata.target_type = 1;
	} else
		msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#ifdef CONFIG_MSM_BUS_SCALING
	msm_fb_register_device("dtv", &dtv_pdata);
#endif

//	platform_add_devices(oem_panel_devices, ARRAY_SIZE(oem_panel_devices));

}

void __init msm8960_allocate_fb_region(void)
{
	void *addr;
	unsigned long size;

	size = MSM_FB_SIZE;
	addr = alloc_bootmem_align(size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
			size, addr, __pa(addr));
}

void __init msm8960_set_display_params(char *prim_panel, char *ext_panel)
{
	if (strnlen(prim_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.prim_panel_name, prim_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.prim_panel_name %s\n",
			msm_fb_pdata.prim_panel_name);

		if (!strncmp((char *)msm_fb_pdata.prim_panel_name,
			HDMI_PANEL_NAME, strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
			pr_debug("HDMI is the primary display by"
				" boot parameter\n");
			hdmi_is_primary = 1;
		}
	}
	if (strnlen(ext_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.ext_panel_name, ext_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.ext_panel_name %s\n",
			msm_fb_pdata.ext_panel_name);
	}
}