/* drivers/input/touchscreen/max11871.c
 *
 * Copyright (c)2012 Maxim Integrated Products, Inc.
 *
 * Driver Version: 3.0.1
 * Release Date: June 17, 2012
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#else
#include <linux/suspend.h>
#endif
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
//#include <linux/max11871.h>
#include <linux/input/mt.h>
#include <linux/gpio.h>
#include "max11871.h"
#include <linux/miscdevice.h>
#include <asm/uaccess.h> //TOUCH_REACTION_RATE
#include <linux/kthread.h>
#include <linux/firmware.h>
#include <linux/crc16.h>
#include <asm/byteorder.h>  /* must include this header to get byte order */

#define SKY_PROCESS_CMD_KEY
#define TOUCH_IO

#define ESD_PROTECT

#define ERROR(d, e...) printk(KERN_ERR MAX11871_NAME "(E:%s:%d): " d "\n", \
				__func__, __LINE__, ##e)
#define CHECKI(a, d, e...) do {if (a) ERROR(d, ##e); } while (0)
#define CHECK(a, b, c, d, e...) do {if (a) {b; ERROR(d, ##e); return c; }; } \
				while (0)
#define CHECKB(a, b, d, e...) do {if (a) {b; ERROR(d, ##e); break; }; } \
				while (0)

#define PRINT(d, e...) printk(KERN_INFO MAX11871_NAME d "\n", ##e)
#define DEBUGL(a) ((1 << (a - 1)) & debug_mask)
#define DEBUG(a, d, e...) do {if DEBUGL(a) \
		printk(KERN_INFO MAX11871_NAME d "\n", ##e); } while (0)
#define DEBUGHD(a, d, e...) do {if DEBUGL(a) \
		printk(KERN_INFO MAX11871_NAME d, ##e); } while (0)
#define DEBUGNF(a, d, e...) do {if DEBUGL(a) printk(d, ##e); } while (0)

#define DEVFCA(n, imode, ishow, istore, a)                              \
do {                                                                    \
	int ii;                                                         \
	for (ii = 0; ii < n; ii++) {                                    \
		snprintf(dev_attr_##a##_name[ii],                       \
			sizeof(dev_attr_##a##_name[ii]), __stringify(a) \
			"%d", ii + 1);                                  \
		dev_attr_##a[ii].attr.name = dev_attr_##a##_name[ii];   \
		dev_attr_##a[ii].attr.mode = imode;                     \
		dev_attr_##a[ii].show = ishow;                          \
		dev_attr_##a[ii].store = istore;                        \
		CHECK(device_create_file(&client->dev,                  \
			&dev_attr_##a[ii]) < 0, , 0,                    \
			"failed to create sysfs file [%s]",             \
			dev_attr_##a##_name[ii]);                       \
	}                                                               \
} while (0)
#define DEVFRA(n, a)                                                    \
do {                                                                    \
	int ii;                                                         \
	for (ii = 0; ii < n; ii++)                                      \
		device_remove_file(&client->dev, &dev_attr_##a[ii]);    \
} while (0)

#define ENABLE_IRQ()                            \
do {                                            \
	mutex_lock(&ts->irq_mutex);             \
	if (ts->irq_disabled) {                 \
		enable_irq(ts->client->irq);    \
		ts->irq_disabled = 0;           \
	}                                       \
	mutex_unlock(&ts->irq_mutex);           \
} while (0)

#define DISABLE_IRQ()                           \
do {                                            \
	mutex_lock(&ts->irq_mutex);             \
	if (ts->irq_disabled == 0) {            \
		disable_irq(ts->client->irq);   \
		ts->irq_disabled = 1;           \
	}                                       \
	mutex_unlock(&ts->irq_mutex);           \
} while (0)

#define NWORDS(a)    (sizeof(a) / sizeof(u16))
#define BYTE_SIZE(a) ((a) * sizeof(u16))
#define BYTEH(a)     ((a) >> 8)
#define BYTEL(a)     ((a) & 0xFF)

#define CONFIG(a)      (ts->config->a)
#define COORDINATES(a) CONFIG(coordinates[CONFIG(coordinate_model)].a)
#define BUTTONXY(i, a) COORDINATES(button_xy[i]).a

#define MAXIM_TOUCH_ROWDATA_MODE 0x0000
#define MAXIM_TOUCH_REPORT_MODE 0x0001
#define MAXIM_TOUCH_ROWDATA_BASELINE 0x0000
#define MAXIM_TOUCH_AUTO_BASELINE 0x0002
#define MAX_REPORT_READERS  5
#define MAX_FWDL_RETRIES        5
#define MAX_FINGER_IDS          10

#define GPIO_TOUCH_CHG                  11
#define GPIO_TOUCH_RST                  50
#define GPIO_TOUCH_POWER                51

#define IRQ_TOUCH_INT		gpio_to_irq(GPIO_TOUCH_CHG)

#define MAX_TOUCHES_LIMIT 5 
#define SIZE_OF_RPT_BUFFER 1024

#define MAX11871_XLINE_NUM	12
#define MAX11871_YLINE_NUM	20

#define MAX11871_MIN_REFERENCE	-4000
#define MAX11871_MAX_REFERENCE	2000

static int max11871_init_gpio(struct max11871_pdata *pdata, int value);
static int max11871_reset(struct max11871_pdata *pdata, int value);
static int max11871_tirq(struct max11871_pdata *pdata);
static struct data *maxtsp_data = NULL;

#if defined(SKY_PROCESS_CMD_KEY) || defined(TOUCH_IO)
int16_t *rawDataBuffer;
#endif

#ifdef SKY_PROCESS_CMD_KEY 
// To be depreciated.
static long ts_fops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int ts_fops_open(struct inode *inode, struct file *filp);
#endif

#ifdef TOUCH_IO
static int open(struct inode *inode, struct file *file);
static int release(struct inode *inode, struct file *file);
static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos);
static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *ppos);
static long ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#endif

static unsigned max11871_gpio_table[] =
{
	GPIO_CFG(GPIO_TOUCH_POWER, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_TOUCH_RST, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
	GPIO_CFG(GPIO_TOUCH_CHG, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

#define MAX11871_GPIO_ERROR(ret, gpio, op)                             \
	if (ret < 0) {pr_err("%s: GPIO %d %s failed (%d)\n", __func__, \
				gpio, op, ret); return ret;}

#if MAX11871_BOARD_CONFIG
struct max11871_pdata max11871_pdata = {
	.config     = &max11871_config,
#else
struct max11871_pdata max11871_data = {
	.config     = NULL,
#endif
	.gpio_power = GPIO_TOUCH_POWER,
	.gpio_reset = GPIO_TOUCH_RST,
	.gpio_tirq  = GPIO_TOUCH_CHG,
	.init       = max11871_init_gpio,
	.reset      = max11871_reset,
	.tirq       = max11871_tirq,
};



#if !MAX11871_BOARD_CONFIG
struct max11871_config local_config = {
	.chip_configs = 1,
	.chip_config[0] = {.config_id = 0x0CFD, .fw_mappings = 2,
			.fw_mapping[0] = {.chip_id = 0x55, .fw_index = 0},
			.fw_mapping[1] = {.chip_id = 0x57, .fw_index = 1}},
	.fw_image[0] = {.file_name = "max11871_RevE_39.bin", .length = 0x8000,
			.config_boundary = 0x8000},
//			.config_boundary = 0x7A68},
	.fw_image[1] = {.file_name = "max11871_RevG_49.bin", .length = 0x8000,
                        .config_boundary = 0x8000},
	.defaults_allow = 1,
	.default_chip_config = 0x0CFD,
	.default_chip_id = 0x57,
	.i2c_words = 125,//MAX_WORDS_REPORT,
	.max_touches = 10,
	.events_per_sec = 120,
	.coordinate_settings = MAX11871_REVERSE_Y | MAX11871_SWAP_XY,
	.coordinate_model = MAX11871_OPTIMAL,
	.coordinates[MAX11871_OPTIMAL] = {
		.panel_mx_l = 0,
		.panel_mx_h = 0,
		.panel_my_l = 0,
		.panel_my_h = 0,
		.lcd_x = 720,
		.lcd_y = 1280},
	.input_protocol = MAX11871_PROTOCOL_B
};
#endif

struct report_reader {
	u16 report_id;
	u16 reports_passed;
	struct semaphore sem;
	int status;
};

struct data {
	struct max11871_config  *config;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct input_dev *key_input_dev;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
	u8  early_suspend_registered;
#endif
	struct mutex irq_mutex;
	struct mutex i2c_mutex;
	struct mutex report_mutex;
	struct semaphore report_sem;
	struct report_reader report_readers[MAX_REPORT_READERS];
	u8 irq_disabled;
	u16 nbuttons_original;
	u8 report_readers_outstanding;
	u16 report[MAX_WORDS_REPORT + 1];  /* with header */
	u16 rx_report[MAX_WORDS_REPORT + 1];  /* with header */
	u32 irq_counter;
	u8 got_report;
	int fw_index;
	u16 fw_crc16;
	u16 fw_version[MAX_WORDS_REPORT];
	u16 touch_config[MAX_WORDS_COMMAND_ALL];
//	int buttondown;
	int nobutton;
	u16 prev_fingers;
	u16 curr_fingers;
	char phys[32];
	char key_phys[32];
	/* firmware download decision */
	u8  fw_responsive;
	u8  have_fw;
	u8  have_touchcfg;
	u16 config_id;
	u16 controller_id;
};

#ifdef SKY_PROCESS_CMD_KEY
static struct file_operations ts_fops = {
	.owner = THIS_MODULE,
	.open = ts_fops_open,
	.unlocked_ioctl = ts_fops_ioctl, // mirinae
};

static struct miscdevice touch_event = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "touch_fops",
	.fops = &ts_fops,
};

typedef enum
{
	TOUCH_IOCTL_READ_LASTKEY=1001,	
	TOUCH_IOCTL_DO_KEY,	
	TOUCH_IOCTL_RELEASE_KEY, 
	TOUCH_IOCTL_CLEAN,
	TOUCH_IOCTL_DEBUG_,
	TOUCH_IOCTL_RESTART,
	TOUCH_IOCTL_PRESS_TOUCH,
	TOUCH_IOCTL_RELEASE_TOUCH,
	TOUCH_IOCTL_CHARGER_MODE,
	TOUCH_IOCTL_EARJACK_MODE,
	POWER_OFF,
	TOUCH_IOCTL_DELETE_ACTAREA = 2001,
	TOUCH_IOCTL_RECOVERY_ACTAREA,
	TOUCH_IOCTL_SENSOR_X = 2005,
	TOUCH_IOCTL_SENSOR_Y,
	TOUCH_IOCTL_CHECK_BASE,
	TOUCH_IOCTL_READ_IC_VERSION,
	TOUCH_IOCTL_READ_FW_VERSION,
	TOUCH_IOCTL_START_UPDATE,
	TOUCH_IOCTL_SELF_TEST,
	TOUCH_IOCTL_DIAGNOSTIC_MIN_DEBUG,
	TOUCH_IOCTL_DIAGNOSTIC_MAX_DEBUG,
	TOUCH_IOCTL_SELF_TEST_CONFIRM = 2100,//p14696 jhseo make for Maxim chip because Maxim don't provide self test mode
	TOUCH_IOCTL_INIT = 3001,	
	TOUCH_IOCTL_OFF  = 3002,
	
	TOUCH_IOCTL_SET_CONFIG = 3005,	
	TOUCH_IOCTL_GET_CONFIG = 3006,	
	TOUCH_IOCTL_READ_CONFIG = 3007,
	TOUCH_IOCTL_EVENT_TIME_GET  = 4001, //TOUCH_REACTION_RATE
} TOUCH_IOCTL_CMD;

#endif

#ifdef TOUCH_IO
#define RAW_DATA_WAIT_COUNT	10
static int rawDataEn = RAW_DATA_WAIT_COUNT;
	
static struct file_operations fops = 
{
	.owner =    THIS_MODULE,
	.unlocked_ioctl =    ioctl,  // mirinae
	.read =     read,
	.write =    write,
	.open =     open,
	.release =  release,
};

static struct miscdevice touch_io = 
{
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "qt602240",
	.fops =     &fops
};

typedef enum 
{
	//p13106 add touch_app
	APPLY_TOUCH_CONFIG = 501,
	DIAG_DEBUG = 502,
	RESET_TOUCH_CONFIG = 503,
	GET_TOUCH_CONFIG = 504,
	SET_TOUCH_CONFIG = 505,
	//READ_ITO_TYPE = 506,
	TOUCH_IOCTL_DEBUG = 507,
	TOUCH_CHARGER_MODE = 701
}CONFIG_CMD;
#endif



#ifdef CONFIG_HAS_EARLYSUSPEND
static void early_suspend(struct early_suspend *h);
static void late_resume(struct early_suspend *h);
#endif

static int device_init(struct i2c_client *client);
static int device_deinit(struct i2c_client *client);

static int bootloader_enter(struct data *ts);
static int bootloader_exit(struct data *ts);
static int bootloader_get_crc(struct data *ts, u16 *crc16, u16 len);
static int bootloader_set_byte_mode(struct data *ts);
static int bootloader_erase_flash(struct data *ts);
static int bootloader_write_flash(struct data *ts, u8 *image);

static void check_fw_and_config(struct data *ts, u16 request_slept,u16 panel_x, u16 panel_y);
static int change_touch_rpt(struct i2c_client *client, u16 to);
static int change_baseline_rpt(struct i2c_client *client, u16 to);
static int sreset(struct i2c_client *client);
static int get_touch_config(struct i2c_client *client);
static int get_fw_version(struct i2c_client *client);
static void propagate_report(struct data *ts, int status, u16 *report);
static int get_report(struct data *ts, u16 report_id, ulong timeout);
static void release_report(struct data *ts);

/* Bit 1 - I2C RX
   Bit 2 - I2C TX
   Bit 3 - I2C RX Header/TX Address
   Bit 4 - Touch and Key processing
   Bit 5 - Initialization */
static u16  debug_mask = 0x0010;
static u8   bootloader;
static u8   init_state;

static int max11871_init_gpio(struct max11871_pdata *pdata, int value)
{
	int  ret, i;

	if (value) {
		for (i = 0; i < ARRAY_SIZE(max11871_gpio_table); i++) {
			ret = gpio_tlmm_config(max11871_gpio_table[i], GPIO_CFG_ENABLE);
			MAX11871_GPIO_ERROR(ret, max11871_gpio_table[i], "config");
		}

		ret = gpio_request(pdata->gpio_power, "max11871_power");
		MAX11871_GPIO_ERROR(ret, pdata->gpio_power, "request");
		ret = gpio_direction_output(pdata->gpio_power, 1);
		MAX11871_GPIO_ERROR(ret, pdata->gpio_power, "direction");

		ret = gpio_request(pdata->gpio_reset, "max11871_reset");
		MAX11871_GPIO_ERROR(ret, pdata->gpio_reset, "request");
		ret = gpio_direction_output(pdata->gpio_reset, 1);
		MAX11871_GPIO_ERROR(ret, pdata->gpio_reset, "direction");

		ret = gpio_request(pdata->gpio_tirq, "max11871_tirq");
		MAX11871_GPIO_ERROR(ret, pdata->gpio_tirq, "request");
		ret = gpio_direction_input(pdata->gpio_tirq);
		MAX11871_GPIO_ERROR(ret, pdata->gpio_tirq, "direction");
	} else {
		gpio_free(pdata->gpio_power);
		gpio_free(pdata->gpio_reset);
		gpio_free(pdata->gpio_tirq);
	}

	return 0;
}

static int max11871_reset(struct max11871_pdata *pdata, int value)
{
	gpio_set_value(pdata->gpio_reset, !!value);
	return 0;
}

static int max11871_tirq(struct max11871_pdata *pdata)
{
	return gpio_get_value(pdata->gpio_tirq);
}

static int i2c_rx_bytes(struct data *ts, u8 *buf, u16 len)
{
	int  i, ret, level = (!bootloader && len == 2) ? 3 : 1;

	do {ret = i2c_master_recv(ts->client, (char *)buf, (int)len);
	} while (ret == -EAGAIN);
	CHECK(ret < 0, , ret, "I2C RX fail (%d)", ret);

	if (DEBUGL(1)) {
		DEBUGHD(level, "(RX): ");
		for (i = 0; i < len; i++)
			DEBUGNF(level, "%02X ", buf[i]);
		DEBUGNF(level, "\n");
	}

	return ret;
}

static int i2c_rx_words(struct data *ts, u16 *buf, u16 len)
{
	int  i, ret, level = (!bootloader && len == 1) ? 3 : 1;

	do {ret = i2c_master_recv(ts->client, (char *)buf, (int)(len * 2));
	} while (ret == -EAGAIN);
	CHECK(ret < 0, , ret, "I2C RX fail (%d)", ret);
	CHECK((ret % 2) != 0, , -1, "I2C words RX fail: odd number of bytes "
		"(%d)", ret);

#ifdef __BIG_ENDIAN
	for (i = 0; i < len; i++)
		buf[i] = (buf[i] << 8) | (buf[i] >> 8);
#endif
	if (DEBUGL(1)) {
		DEBUGHD(level, "(RX): ");
		for (i = 0; i < len; i++)
			DEBUGNF(level, "%04X ", buf[i]);
		DEBUGNF(level, "\n");
	}

	return ret / 2;
}

static int i2c_tx_bytes(struct data *ts, u8 *buf, u16 len)
{
	int  i, ret, level = (!bootloader && len == 2) ? 3 : 2;

	do {ret = i2c_master_send(ts->client, (char *)buf, (int)len);
	} while (ret == -EAGAIN);
	CHECK(ret < 0, , ret, "I2C TX fail (%d)", ret);

	if (DEBUGL(2)) {
		DEBUGHD(level, "(TX): ");
		if (len >= 1)
			DEBUGNF(bootloader ? 2 : 3, "%02X ", buf[0]);
		if (len >= 2)
			DEBUGNF(bootloader ? 2 : 3, "%02X ", buf[1]);
		for (i = 2; i < len; i++)
			DEBUGNF(level, "%02X ", buf[i]);
		DEBUGNF(level, "\n");
	}

	return ret;
}

static int i2c_tx_words(struct data *ts, u16 *buf, u16 len)
{
	int  i, ret, level = (!bootloader && len == 1) ? 3 : 2;

#ifdef __BIG_ENDIAN
	for (i = 0; i < len; i++)
		buf[i] = (buf[i] << 8) | (buf[i] >> 8);
#endif
	do {ret = i2c_master_send(ts->client, (char *)buf, (int)(len * 2));
	} while (ret == -EAGAIN);
	CHECK(ret < 0, , ret, "I2C TX fail (%d)", ret);
	CHECK((ret % 2) != 0, , -1, "I2C words TX fail: odd number of bytes "
		"(%d)", ret);

	if (DEBUGL(2)) {
		DEBUGHD(level, "(TX): ");
		if (len >= 1)
			DEBUGNF(bootloader ? 2 : 3, "%04X ", buf[0]);
		for (i = 1; i < len; i++)
			DEBUGNF(level, "%04X ", buf[i]);
		DEBUGNF(level, "\n");
	}

	return ret / 2;
}

static int read_mtp_report(struct data *ts, u16 *buf)
{
	int  words = 1, words_rx, i, ret = 0, recover = 0, remainder = 0;
	u16  address = 0x000A;

	mutex_lock(&ts->i2c_mutex);
	/* read header, get size, read entire report */
	for (i = 1; i <= 2; i++) {
		if (!ts->got_report) {
			words = i2c_tx_words(ts, &address, 1);
			CHECK(words != 1, mutex_unlock(&ts->i2c_mutex), -1,
				"Report RX fail: failed to set address");
			ts->got_report = 1;
		}

		words_rx = i2c_rx_words(ts, buf, words);
		if (words_rx != words || BYTEH(buf[0]) != 0x11 ||
			BYTEL(buf[0]) > MAX_WORDS_REPORT ||
			(i == 2 && buf[1] < 0x0100)) {
			if (recover == 0) {
				ts->got_report = 0;
				i = 0;
				recover = 1;
				continue;
			}
			ret = -1;
			ERROR("Report RX fail: received (%d) expected (%d) "
				"words, header (%04X)", words_rx, words,
				buf[0]);
			break;
		}
		words = BYTEL(buf[0]) + 1;
		if (words > CONFIG(i2c_words))
			remainder = words - CONFIG(i2c_words);
		if (remainder > 0)
			words = CONFIG(i2c_words);
		if (i == 2 && remainder > 0) {
			ts->got_report = 0;
			address += CONFIG(i2c_words);
			words = i2c_tx_words(ts, &address, 1);
			CHECK(words != 1, mutex_unlock(&ts->i2c_mutex), -1,
				"Report RX fail: failed to set address 0x%X",
				address);
			words_rx = i2c_rx_words(ts, &buf[CONFIG(i2c_words)],
						remainder);
			CHECK(words_rx != remainder,
				mutex_unlock(&ts->i2c_mutex),
				-1, "Report RX fail 0x%X: received (%d) "
				"expected (%d) words", address, words_rx,
				remainder);
		}
	}
	mutex_unlock(&ts->i2c_mutex);

	return ret;
}

static int send_mtp_command(struct data *ts, u16 *buf, u16 len)
{
	u16  tx_buf[MAX_WORDS_COMMAND + 2]; /* with address and header */
	u16  packets, words, words_tx, csum = 0;
	int  i, ret = 0;

	/* check basics */
	CHECK(len < 2, , -1, "Command too short (%d); 2 words minimum", len);
	CHECK((buf[1] + 2) != len, , -1, "Inconsistent command length: "
		"expected (%d) given (%d)", (buf[1] + 2), len);
	CHECK(len > MAX_WORDS_COMMAND_ALL, , -1, "Command too long (%d); "
		"maximum (%d) words", len, MAX_WORDS_COMMAND_ALL);

	/* calculate checksum for certain commands */
	if (buf[0] == 0x0001 || buf[0] == 0x0010 || buf[0] == 0x0003 ||
		buf[0] == 0x0030) {
		for (i = 2; i < (len - 1); i++)
			csum += buf[i];
		buf[len - 1] = csum;
	}

	/* packetize and send */
	packets = len / MAX_WORDS_COMMAND;
	if (len % MAX_WORDS_COMMAND)
		packets++;
	tx_buf[0] = 0x0000;

	mutex_lock(&ts->i2c_mutex);
	for (i = 0; i < packets; i++) {
		words = (i == (packets - 1)) ? len : MAX_WORDS_COMMAND;
		tx_buf[1] = (packets << 12) | ((i + 1) << 8) | words;
		memcpy(&tx_buf[2], &buf[i * MAX_WORDS_COMMAND],
			BYTE_SIZE(words));
		words_tx = i2c_tx_words(ts, tx_buf, words + 2);
		CHECKB(words_tx != (words + 2), ret = -1, "Command TX fail: "
			"transmitted (%d) expected (%d) words, packet (%d)",
			words_tx, words + 2, i);
		len -= MAX_WORDS_COMMAND;
	}
	ts->got_report = 0;
	mutex_unlock(&ts->i2c_mutex);

	return ret;
}
#if 0
static int report_button_xy(const struct data *ts, u16 x, u16 y)
{

	u16  i;

	if (!ts->key_input_dev)
		return -1;

	for (i = 0; i < CONFIG(buttons); i++) {
		if (x >= (BUTTONXY(i, x) - BUTTONXY(i, size_x) / 2) &&
		    x <= (BUTTONXY(i, x) + BUTTONXY(i, size_x) / 2) &&
		    y >= (BUTTONXY(i, y) - BUTTONXY(i, size_y) / 2) &&
		    y <= (BUTTONXY(i, y) + BUTTONXY(i, size_y) / 2)) {
			input_report_key(ts->key_input_dev,
					CONFIG(button_code[i]), 1);
			input_sync(ts->key_input_dev);
			return CONFIG(button_code[i]);
		}
	}

	return -1;
}
#endif
static void IOCTL_Touch_Test(u8 press)// 0 : release , 1 : press
{
	struct data *ts = (struct data *)maxtsp_data;
	u16  panel_x, panel_y;
	u8 i=0;

	DEBUG(5, "IOCTL_Touch_Test press : %d",press);

	if (CONFIG(coordinate_model) == MAX11871_OPTIMAL) {
		panel_x = COORDINATES(panel_mx_l) + COORDINATES(lcd_x) +
				COORDINATES(panel_mx_h) - 1;
		panel_y = COORDINATES(panel_my_l) + COORDINATES(lcd_y) +
				COORDINATES(panel_my_h) - 1;
	} else {
		panel_x = 720;
		panel_y = 1280;
	}


	if(press==0)
	{
		switch (CONFIG(input_protocol)) {
		case MAX11871_PROTOCOL_A:
		case MAX11871_PROTOCOL_A_TRACK:
			input_mt_sync(ts->input_dev);
			input_sync(ts->input_dev);
			break;
		case MAX11871_PROTOCOL_B:
			input_mt_slot(ts->input_dev, 0);
			input_mt_report_slot_state(ts->input_dev,
						MT_TOOL_FINGER, 0);
			input_sync(ts->input_dev);
			break;
		case MAX11871_PROTOCOL_CUSTOM1:
	#if defined(ABS_MT_AMPLITUDE) && defined(ABS_MT_POSITION)
			input_report_abs(ts->input_dev, ABS_MT_AMPLITUDE, 0);
			input_report_abs(ts->input_dev, ABS_MT_POSITION, 1 << 31);
	#endif
			break;
		}
	}
	else if(press==1)
	{
		switch (CONFIG(input_protocol)) {
			case MAX11871_PROTOCOL_A:
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, panel_x/2);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, panel_y/2);
				input_report_abs(ts->input_dev,
			#ifdef ABS_MT_PRESSURE
							ABS_MT_PRESSURE, 10);
			#else
							ABS_MT_TOUCH_MAJOR, 10);
			#endif
				input_mt_sync(ts->input_dev);
				break;
			case MAX11871_PROTOCOL_A_TRACK:
				input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, 0);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, panel_x/2);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, panel_x/2);
				input_report_abs(ts->input_dev,
			#ifdef ABS_MT_PRESSURE
							ABS_MT_PRESSURE, 10);
			#else
							ABS_MT_TOUCH_MAJOR, 10);
			#endif
				input_mt_sync(ts->input_dev);
				break;
			case MAX11871_PROTOCOL_B:
				ts->curr_fingers |= 1 << 0;
				input_mt_slot(ts->input_dev, 0);
				input_mt_report_slot_state(
						ts->input_dev,
						MT_TOOL_FINGER, 1);
				input_report_abs(ts->input_dev,
						ABS_MT_POSITION_X, panel_x/2);
				input_report_abs(ts->input_dev,
						ABS_MT_POSITION_Y, panel_y/2);
				input_report_abs(ts->input_dev,
#ifdef ABS_MT_PRESSURE
						ABS_MT_PRESSURE, 10);
#else
						ABS_MT_TOUCH_MAJOR,
						10);
#endif
				break;
			case MAX11871_PROTOCOL_CUSTOM1:
		#if defined(ABS_MT_AMPLITUDE) && defined(ABS_MT_POSITION)
				input_report_abs(ts->input_dev, ABS_MT_AMPLITUDE, (10 << 16) | 0x0A);
				input_report_abs(ts->input_dev, ABS_MT_POSITION, (1 << 31) |(panel_x/2 << 16) | panel_y/2);
		#endif
				break;
		}

		switch (CONFIG(input_protocol)) {
		case MAX11871_PROTOCOL_A:
		case MAX11871_PROTOCOL_A_TRACK:
			input_sync(ts->input_dev);
			break;
	#if 0
		case MAX11871_PROTOCOL_B:
			for (i = 0; i < MAX_FINGER_IDS; i++) {
				if ((ts->prev_fingers & (1 << i)) == 1 &&
					(ts->curr_fingers & (1 << i)) == 0) {
	                                input_mt_slot(ts->input_dev, i);
	                                input_mt_report_slot_state(
							ts->input_dev,
                                                        MT_TOOL_FINGER, 0);
				}
                        }
			ts->prev_fingers = ts->curr_fingers;
			input_sync(ts->input_dev);
			break;
	#endif
		case MAX11871_PROTOCOL_CUSTOM1:
			break;
		}
	}
	else
	{
		for(i=0;i<MAX_FINGER_IDS;i++)
		{
				switch (CONFIG(input_protocol)) {
				case MAX11871_PROTOCOL_A:
				case MAX11871_PROTOCOL_A_TRACK:
					input_mt_sync(ts->input_dev);
					input_sync(ts->input_dev);
					break;
				case MAX11871_PROTOCOL_B:
					input_mt_slot(ts->input_dev, i);
					input_mt_report_slot_state(ts->input_dev,
								MT_TOOL_FINGER, 0);
					break;
				case MAX11871_PROTOCOL_CUSTOM1:
			#if defined(ABS_MT_AMPLITUDE) && defined(ABS_MT_POSITION)
					input_report_abs(ts->input_dev, ABS_MT_AMPLITUDE, 0);
					input_report_abs(ts->input_dev, ABS_MT_POSITION, 1 << 31);
			#endif
					break;
			}
		}
		switch (CONFIG(input_protocol)) {
			case MAX11871_PROTOCOL_B:
				input_sync(ts->input_dev);
				break;
			default:
				break;
			}
	}
}
#ifdef SKY_PROCESS_CMD_KEY
static int ts_fops_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct timespec timespecs; //TOUCH_REACTION_RATE

// To be depreciated.
static long ts_fops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	long event_time;
	struct data *ts = (struct data *)maxtsp_data;
	struct max11871_pdata *pdata = &max11871_data;

	if(maxtsp_data == NULL){
		ERROR("NULL Device \n");
		return 0;
	}
	
//	maxinfo("ts_fops_ioctl(%d, %d) \n",(int)cmd,(int)arg);
	DEBUG(5, "ts_fops_ioctl(%d, %d) \n", (int)cmd,(int)arg);

	switch (cmd) 
	{
		case TOUCH_IOCTL_DO_KEY://p14696 jhseo
			if ( (int)argp == KEY_NUMERIC_STAR )
				input_report_key(maxtsp_data->key_input_dev, 0xe3, 1);
			else if ( (int)argp == KEY_NUMERIC_POUND )
				input_report_key(maxtsp_data->key_input_dev, 0xe4, 1);
			else
				input_report_key(maxtsp_data->key_input_dev, (int)argp, 1);
			input_sync(maxtsp_data->key_input_dev);
			break;
		case TOUCH_IOCTL_RELEASE_KEY://p14696 jhseo
			if ( (int)argp == KEY_NUMERIC_STAR )
				input_report_key(maxtsp_data->key_input_dev, 0xe3, 0);
			else if ( (int)argp == KEY_NUMERIC_POUND )
				input_report_key(maxtsp_data->key_input_dev, 0xe4, 0);
			else
				input_report_key(maxtsp_data->key_input_dev, (int)argp, 0);
			input_sync(maxtsp_data->key_input_dev);
			break;

		case TOUCH_IOCTL_DEBUG_:
			break;
		case TOUCH_IOCTL_CLEAN:
			break;
		case TOUCH_IOCTL_RESTART:
			rawDataEn=0;
			change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_AUTO_BASELINE); //row data baseline
			change_touch_rpt(maxtsp_data->client,MAXIM_TOUCH_REPORT_MODE); //basic data mode

			ENABLE_IRQ();
			break;

		case TOUCH_IOCTL_START_UPDATE:
			check_fw_and_config(ts,0,720,1280);
			break;

		case TOUCH_IOCTL_CHARGER_MODE:
			break;

		case TOUCH_IOCTL_EARJACK_MODE:			
			break;            

		case POWER_OFF:
			pm_power_off();
			break;			

		case TOUCH_IOCTL_SET_CONFIG:
			return 0;			
			break;

		case TOUCH_IOCTL_GET_CONFIG:
			return 0;
			break;
			
		case TOUCH_IOCTL_READ_CONFIG:
			return 0;
			break;

		case TOUCH_IOCTL_SENSOR_X:
			{
				int send_data;
				send_data = MAX11871_XLINE_NUM;

				if (copy_to_user(argp, &send_data, sizeof(send_data)))
					return false;
			}
			break;

		case TOUCH_IOCTL_SENSOR_Y:
			{
				int send_data;
				send_data = MAX11871_YLINE_NUM;

				if (copy_to_user(argp, &send_data, sizeof(send_data)))
					return false;
			}
			break;

		case TOUCH_IOCTL_SELF_TEST:
			{
#if 1
				int send_byte[240];
				int i=0;


				if(rawDataEn>=RAW_DATA_WAIT_COUNT)
				{
					DEBUG(5, "copy to user first \n");
					DISABLE_IRQ();

					change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_ROWDATA_BASELINE); //row data baseline
					change_touch_rpt(maxtsp_data->client,MAXIM_TOUCH_ROWDATA_MODE); //basic data mode

					mdelay(50);
					read_mtp_report(maxtsp_data, maxtsp_data->rx_report);
					rawDataBuffer=(int16_t *) maxtsp_data->rx_report;
				}
				else{
					DEBUG(5, "copy to user second \n");
					if(rawDataEn==0)
					{
						change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_AUTO_BASELINE); //row data baseline
						change_touch_rpt(maxtsp_data->client,MAXIM_TOUCH_REPORT_MODE); //basic data mode
					}
					rawDataEn++;
				}

				if(rawDataBuffer==NULL)
				{
					ERROR("rawDataBuffer is NULL \n");
				}
				else
				{
					DEBUG(5, "copy to user in \n");

					for(i=0;i<(MAX11871_XLINE_NUM*MAX11871_YLINE_NUM);i++)
						send_byte[i]=rawDataBuffer[5+i];

					if (copy_to_user(argp,send_byte, sizeof(int) * (MAX11871_XLINE_NUM*MAX11871_YLINE_NUM)))
					{
						change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_ROWDATA_BASELINE); //row data baseline
						change_touch_rpt(maxtsp_data->client,MAXIM_TOUCH_ROWDATA_MODE); //basic data mode
						return 0;
					}
				}

//				change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_AUTO_BASELINE); //row data baseline
//				change_touch_rpt(maxtsp_data->client,MAXIM_TOUCH_REPORT_MODE); //basic data mode

//				ENABLE_IRQ();
				DEBUG(5, "end copy to user\n");
				return 1;
#endif
			}
			break;
		//p14696 jhseo make 
		case TOUCH_IOCTL_SELF_TEST_CONFIRM:
#if 1
			{
				int send_byte[240];
				int i=0;

				DEBUG(5, "Self test start \n");
				change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_ROWDATA_BASELINE); //row data baseline

				if(rawDataBuffer==NULL)
				{
					ERROR("rawDataBuffer is NULL \n");
				}
				else
				{
					DEBUG(5, "copy to user in \n");

					for(i=0;i<(MAX11871_XLINE_NUM*MAX11871_YLINE_NUM);i++)
						send_byte[i]=rawDataBuffer[5+i];

					if (copy_to_user(argp,send_byte, sizeof(int) * (MAX11871_XLINE_NUM*MAX11871_YLINE_NUM)))
					{
						DEBUG(5, "Self test confirm False\n");
						change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_AUTO_BASELINE); //row data baseline
						change_touch_rpt(maxtsp_data->client,MAXIM_TOUCH_REPORT_MODE); //basic data mode
						return 0;
					}
				}
				change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_AUTO_BASELINE); //row data baseline
				change_touch_rpt(maxtsp_data->client,MAXIM_TOUCH_REPORT_MODE); //basic data mode
				DEBUG(5, "Self test confirm True\n");
				return 1;
			}
#endif
			break;

		case TOUCH_IOCTL_DIAGNOSTIC_MIN_DEBUG:
			return MAX11871_MIN_REFERENCE;
			break;

		case TOUCH_IOCTL_DIAGNOSTIC_MAX_DEBUG:
			return MAX11871_MAX_REFERENCE;
			break;

		case TOUCH_IOCTL_PRESS_TOUCH:
			IOCTL_Touch_Test(1);
			break;
		case TOUCH_IOCTL_RELEASE_TOUCH:		
			IOCTL_Touch_Test(0);
			break;			

		case TOUCH_IOCTL_DELETE_ACTAREA:
			break;

		case TOUCH_IOCTL_RECOVERY_ACTAREA:
			break;

		case TOUCH_IOCTL_INIT:
			max11871_init_gpio(pdata,0);
			msleep(100);
			max11871_init_gpio(pdata,1);
			msleep(400);
			break;

		case TOUCH_IOCTL_EVENT_TIME_GET:  //TOUCH_REACTION_RATE
			event_time = (timespecs.tv_sec * USEC_PER_MSEC) + (timespecs.tv_nsec / NSEC_PER_MSEC);	 
			if (copy_to_user(argp, &event_time, sizeof(event_time))){
				return 0;
			}
			DEBUG(5, "Report Kernel event time: %ld \n", (long)event_time);
			break;

		case TOUCH_IOCTL_OFF:
			max11871_init_gpio(pdata,0);
			msleep(100);
			break;

		default:
			break;
	}

	return true;
}
#endif

#ifdef TOUCH_IO
static int open(struct inode *inode, struct file *file) 
{
	return 0; 
}
static int release(struct inode *inode, struct file *file) 
{
	return 0; 
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
       int nBufSize = 0;
	int ret;   
	ret=0;
	if((size_t)(*ppos) > 0) return 0;   

	if(buf != NULL)
	{
		nBufSize=strlen(buf);

		DEBUG(5, "cmd is %s", buf);
#if 0		
		if(strncmp(buf, "maxerr", 6)==0)
		{
		       max11871_dbg_level = DEBUG_LEVEL_ERROR;
		}
		else if(strncmp(buf, "maxcr", 5)==0)
		{
		       max11871_dbg_level = DEBUG_LEVEL_CRITICAL;
		}
		else if(strncmp(buf, "maxwarning", 10)==0)
		{
		       max11871_dbg_level = DEBUG_LEVEL_WARNING;
		}
		else if(strncmp(buf, "maxinfo", 7)==0)
		{
		       max11871_dbg_level = DEBUG_LEVEL_INFO;
		}
		else if(strncmp(buf, "maxtrace", 8)==0)
		{
		       max11871_dbg_level = DEBUG_LEVEL_TRACE;  
              }
		else if(strncmp(buf, "maxoff", 6)==0)
		{
		       max11871_dbg_level = DEBUG_LEVEL_LOGOFF;
		}
		else if(strncmp(buf, "maxpostraceon", 13)==0 )
		{
		      max11871_pos_trace=true;
		}
		else if(strncmp(buf, "maxpostraceoff", 14)==0 )
		{
		       max11871_pos_trace=false;
		}
		else if(strncmp(buf, "upgrade", 7)==0 )
		{
		       maxcr("firmware update start!!!  \n");
			ret = max11871_firmware_dload(maxtsp_data->client);
			if(ret< 0)
				maxerr("Failed Firmware update, ret %d \n", ret);
		}
		else if(strncmp(buf, "maxall", 6)==0 )
		{
			read_touch_config(1);
		}
		else if(strncmp(buf, "readv", 5)==0 )
		{
			maxinfo("chip id : %x,%x,%x,%x \n",(int)maxtsp_config.chipid[0],(int)maxtsp_config.chipid[1],(int)maxtsp_config.chipid[2],(int)maxtsp_config.chipid[3]);
			maxinfo("'touch rpt mode : %x \n",(int)maxtsp_config.tch_rpt_mode );
			maxinfo("'power mode : %x \n",(int)maxtsp_config.power_mode );
			maxinfo("'sensitivity mode : %x \n",(int)maxtsp_config.sensitivity);
			maxinfo("'framerate mode : %x \n",(int)maxtsp_config.framerate );
			maxinfo("'reset_baseline mode : %x \n",(int)maxtsp_config.reset_baseline );
			maxinfo("'fw_version : %x,%x \n",(int)maxtsp_config.fw_version_major,(int)maxtsp_config.fw_version_minor);
			maxinfo("'fw_controller_id : %d \n",(int)maxtsp_config.fw_controller_id);
			maxinfo("'fw_build_version : %d \n",(int)maxtsp_config.fw_build_version);
			maxinfo("'scan_freq mode : %x \n",(int)maxtsp_config.scan_freq );
			maxinfo("'movement_threshold mode : %x \n",(int)maxtsp_config.movement_threshold );
			maxinfo("'sys_status mode : %x \n",(int)maxtsp_config.sys_status );
		}
		else if(strncmp(buf, "readi", 5)==0 )
		{
			maxinfo("chip id : %x,%x,%x,%x \n",(int)maxtsp_reset_config.chipid[0],(int)maxtsp_reset_config.chipid[1],(int)maxtsp_reset_config.chipid[2],(int)maxtsp_reset_config.chipid[3]);
			maxinfo("'touch rpt mode : %x \n",(int)maxtsp_reset_config.tch_rpt_mode );
			maxinfo("'power mode : %x \n",(int)maxtsp_reset_config.power_mode );
			maxinfo("'sensitivity mode : %x \n",(int)maxtsp_reset_config.sensitivity);
			maxinfo("'framerate mode : %x \n",(int)maxtsp_reset_config.framerate );
			maxinfo("'reset_baseline mode : %x \n",(int)maxtsp_reset_config.reset_baseline );
			maxinfo("'fw_version : %x,%x \n",(int)maxtsp_reset_config.fw_version_major,(int)maxtsp_reset_config.fw_version_minor);
			maxinfo("'fw_controller_id : %d \n",(int)maxtsp_reset_config.fw_controller_id);
			maxinfo("'fw_build_version : %d \n",(int)maxtsp_reset_config.fw_build_version);
			maxinfo("'scan_freq mode : %x \n",(int)maxtsp_reset_config.scan_freq );
			maxinfo("'movement_threshold mode : %x \n",(int)maxtsp_reset_config.movement_threshold );
			maxinfo("'sys_status mode : %x \n",(int)maxtsp_reset_config.sys_status );
		}
		else if(strncmp(buf, "init", 4)==0 )
		{
			read_touch_config(0);
			apply_config_values(1);
		}
		else if(strncmp(buf, "apply", 5)==0 )
		{
			read_touch_config(0);
			apply_config_values(0);
		}
		else if(strncmp(buf, "pwroff", 6)==0 )
		{
			max11871_power_onoff(0);
		}
		else if(strncmp(buf, "pwron", 5)==0 )
		{
			max11871_power_onoff(1);
		}
		else if(strncmp(buf, "force", 5)==0 )
		{
			max11871_force_init();
		}
		else if(strncmp(buf, "sleep", 5)==0 )
		{
			max11871_suspend(maxtsp_data->client, PMSG_SUSPEND);
		}
		else if(strncmp(buf, "resume", 6)==0 )
		{
			max11871_resume(maxtsp_data->client);
		}
		else
		{
		       maxerr("Bad cmd ! \n");
		}		
#endif
	}
	else
	{
	   ERROR("Buffer is NULL! \n");
	}

	*ppos +=nBufSize;
	
 	return nBufSize;
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	return 0; 
}


static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
//	int return_value = -1;
//	int object_type, field_index;
//	int channels = (MAX11871_XLINE_NUM*MAX11871_YLINE_NUM);
//	int i=0;
//	int set_cmd =0;
	struct max11871_pdata *pdata = &max11871_data;


	DEBUG(5,"ts_ioctl(%d, %d) \n",(int)cmd,(int)arg);
	switch (cmd)
	{
		case SET_TOUCH_CONFIG:
			break;

		case GET_TOUCH_CONFIG:
			break;

		case APPLY_TOUCH_CONFIG:
			break;
		case RESET_TOUCH_CONFIG:
			break;
		case TOUCH_IOCTL_DEBUG:
			break;
		case TOUCH_CHARGER_MODE:
			break;
			//Factory Cmd
		case TOUCH_IOCTL_READ_LASTKEY:
			break;
		case TOUCH_IOCTL_DO_KEY:
			if ( (int)arg == KEY_NUMERIC_STAR )
				input_report_key(maxtsp_data->key_input_dev, 0xe3, 1);
			else if ( (int)arg == KEY_NUMERIC_POUND )
				input_report_key(maxtsp_data->key_input_dev, 0xe4, 1);
			else
				input_report_key(maxtsp_data->key_input_dev, (int)arg, 1);
			input_sync(maxtsp_data->key_input_dev);
			break;
		case TOUCH_IOCTL_RELEASE_KEY:
			if ( (int)arg == KEY_NUMERIC_STAR )
				input_report_key(maxtsp_data->key_input_dev, 0xe3, 0);
			else if ( (int)arg == KEY_NUMERIC_POUND )
				input_report_key(maxtsp_data->key_input_dev, 0xe4, 0);
			else
				input_report_key(maxtsp_data->key_input_dev, (int)arg, 0);
			input_sync(maxtsp_data->key_input_dev);
			break;
		case TOUCH_IOCTL_INIT:
			DEBUG(5,"Touch init\n");
			max11871_init_gpio(pdata,0);
			msleep(100);
			max11871_init_gpio(pdata,1);
			msleep(400);
			break;
		case TOUCH_IOCTL_OFF:
			DEBUG(5,"Touch off\n");
			max11871_init_gpio(pdata,0);
			msleep(100);
			break;    

		case DIAG_DEBUG:
			/*
			 * Run Diag and save result into reference_data array when arg. is 5010 or 5011. 
			 * Returns diag result when the arg. is in range of 0~223. 
			 */
		#if 0//don't provide
			if (arg == 5010) 
			{				
				if(rawDataEn>=RAW_DATA_WAIT_COUNT)
				{
					change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_AUTO_BASELINE); //auto data baseline
					get_raw_data();
				}
				else{
					if(rawDataEn==0)
					{
						change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_AUTO_BASELINE); //auto data baseline
						change_touch_rpt(maxtsp_data->client,MAXIM_TOUCH_REPORT_MODE); //basic data mode
					}

					rawDataEn ++;
				}

					
				return 0;
			}
			if (arg == 5011) 
			{
				if(rawDataEn>=RAW_DATA_WAIT_COUNT)
				{
					change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_ROWDATA_BASELINE); //row data baseline
					get_raw_data();
				}
				else{
					if(rawDataEn==0)
					{
						change_baseline_rpt(maxtsp_data->client,MAXIM_TOUCH_AUTO_BASELINE); //auto data baseline
						change_touch_rpt(maxtsp_data->client,MAXIM_TOUCH_REPORT_MODE); //basic data mode
					}
					rawDataEn ++;
				}
					
				return 0;
			}
			else if (arg > (channels-1))
			{
				return 0;
			}

			if(rawDataBuffer == NULL){
				return 0;
			}
			else
				return rawDataBuffer[5+arg];
		#endif
			return 0;
		default:
			break;
	}
	return 0;
}
#endif /*TOUCH_IO*/



static void Clear_Touch_Event(void)
{
	struct data *ts = (struct data *)maxtsp_data;
	u8 i=0;

	for(i=0;i<MAX_FINGER_IDS;i++)
	{
		switch (CONFIG(input_protocol)) {
			case MAX11871_PROTOCOL_A:
			case MAX11871_PROTOCOL_A_TRACK:
				input_mt_sync(ts->input_dev);
				input_sync(ts->input_dev);
				break;
			case MAX11871_PROTOCOL_B:
				input_mt_slot(ts->input_dev, i);
				input_mt_report_slot_state(ts->input_dev,
							MT_TOOL_FINGER, 0);
				break;
			case MAX11871_PROTOCOL_CUSTOM1:
		#if defined(ABS_MT_AMPLITUDE) && defined(ABS_MT_POSITION)
				input_report_abs(ts->input_dev, ABS_MT_AMPLITUDE, 0);
				input_report_abs(ts->input_dev, ABS_MT_POSITION, 1 << 31);
		#endif
				break;
		}
	}
		switch (CONFIG(input_protocol)) {
			case MAX11871_PROTOCOL_B:
				input_sync(ts->input_dev);
				break;
			default:
				break;
			}
}
static void process_touch_report(struct data *ts, u16 *buf)
{
	int touch_count = 0;
	int i = 0;
	int x = 0, y = 0, z = 0, finger_id = 0;
	int data_size, swap;

	if (!ts->input_dev)
		return;

	switch (buf[1]) {
	case 0x0801:
		data_size = 4;  break;
	case 0x0802:
		data_size = 12; break;
//	case 0x800:
//		rawDataBuffer=(int16_t *) buf;
	default:
		return;
	}

	touch_count = buf[3] & 0x000F;
	if(touch_count>MAX_FINGER_IDS)
		touch_count=MAX_FINGER_IDS;
	CHECK(touch_count < 0 || touch_count > MAX_FINGER_IDS, , ,
		"Touch count == %i, out of bounds [0,10]!", touch_count);

	if (!DEBUGL(1))
		DEBUG(4, "(TOUCH): -------------------------------");
	if (touch_count == 0) {
	#if 0
		if (ts->buttondown != -1) {
			input_report_key(ts->key_input_dev, ts->buttondown,
					0);
			input_sync(ts->key_input_dev);
			ts->buttondown = -1;
		}
	#endif
		ts->nobutton = 0;
		switch (CONFIG(input_protocol)) {
		case MAX11871_PROTOCOL_A:
		case MAX11871_PROTOCOL_A_TRACK:
			input_mt_sync(ts->input_dev);
			input_sync(ts->input_dev);
			break;
		case MAX11871_PROTOCOL_B:
			ts->prev_fingers = ts->curr_fingers = 0;
			for (i = 0; i < MAX_FINGER_IDS; i++) {
				input_mt_slot(ts->input_dev, i);
				input_mt_report_slot_state(ts->input_dev,
							MT_TOOL_FINGER, 0);
			}
			input_sync(ts->input_dev);
			break;
		case MAX11871_PROTOCOL_CUSTOM1:
#if defined(ABS_MT_AMPLITUDE) && defined(ABS_MT_POSITION)
			input_report_abs(ts->input_dev, ABS_MT_AMPLITUDE, 0);
			input_report_abs(ts->input_dev, ABS_MT_POSITION,
					1 << 31);
#endif
			break;
		}
		DEBUG(4, "(TOUCH): Fingers up");
	} else {
		ts->curr_fingers = 0;
		for (i = 0; i < touch_count; i++) {
			x = buf[6 + i * data_size + 1] & 0x0FFF;
			y = buf[6 + i * data_size + 2] & 0x0FFF;
			if (CONFIG(coordinate_settings) & MAX11871_SWAP_XY) {
				swap = x;
				x = y;
				y = swap;
			}
			z = BYTEH(buf[6 + i * data_size + 3]);
			if (z == 0)
				z++;
			finger_id = buf[6 + i * data_size] & 0x000F;

			DEBUG(4, "(TOUCH): Finger %d: X(%.3d) Y(%.3d) "
				"Z(%.3d)", finger_id, x, y, z);
		#if 0
			if (ts->nobutton == 0 && ts->buttondown == -1)
				ts->buttondown = report_button_xy(ts, x, y);
			if (ts->buttondown == -1) 
		#endif
				{
				ts->nobutton = 1;
				switch (CONFIG(input_protocol)) {
				case MAX11871_PROTOCOL_A:
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION_X, x);
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION_Y, y);
					input_report_abs(ts->input_dev,
#ifdef ABS_MT_PRESSURE
							ABS_MT_PRESSURE, z);
#else
							ABS_MT_TOUCH_MAJOR,
							z);
#endif
					input_mt_sync(ts->input_dev);
					break;
				case MAX11871_PROTOCOL_A_TRACK:
					input_report_abs(ts->input_dev,
							ABS_MT_TRACKING_ID,
							finger_id);
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION_X, x);
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION_Y, y);
					input_report_abs(ts->input_dev,
#ifdef ABS_MT_PRESSURE
							ABS_MT_PRESSURE, z);
#else
							ABS_MT_TOUCH_MAJOR,
							z);
#endif
					input_mt_sync(ts->input_dev);
					break;
				case MAX11871_PROTOCOL_B:
					ts->curr_fingers |= 1 << finger_id;
					input_mt_slot(ts->input_dev, finger_id);
					input_mt_report_slot_state(
							ts->input_dev,
							MT_TOOL_FINGER, 1);
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION_X, x);
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION_Y, y);
					input_report_abs(ts->input_dev,
#ifdef ABS_MT_PRESSURE
							ABS_MT_PRESSURE, z);
#else
							ABS_MT_TOUCH_MAJOR,
							z);
#endif
					break;
				case MAX11871_PROTOCOL_CUSTOM1:
#if defined(ABS_MT_AMPLITUDE) && defined(ABS_MT_POSITION)
					input_report_abs(ts->input_dev,
							ABS_MT_AMPLITUDE,
							(z << 16) | 0x0A);
					input_report_abs(ts->input_dev,
							ABS_MT_POSITION,
							((i == (touch_count -
								1)) << 31) |
							 (x << 16) | y);
#endif
					break;
				}
			}
		}
		switch (CONFIG(input_protocol)) {
		case MAX11871_PROTOCOL_A:
		case MAX11871_PROTOCOL_A_TRACK:
			input_sync(ts->input_dev);
			break;
		case MAX11871_PROTOCOL_B:
			for (i = 0; i < MAX_FINGER_IDS; i++) {
				if ((ts->prev_fingers & (1 << i)) != 0 &&
					(ts->curr_fingers & (1 << i)) == 0) {
	                                input_mt_slot(ts->input_dev, i);
	                                input_mt_report_slot_state(
							ts->input_dev,
                                                        MT_TOOL_FINGER, 0);
				}
                        }
			ts->prev_fingers = ts->curr_fingers;
			input_sync(ts->input_dev);
			break;
		case MAX11871_PROTOCOL_CUSTOM1:
			break;
		}
	}
}

static irqreturn_t irq_handler(int irq, void *context)
{
	struct data *ts = (struct data *)context;

	if (read_mtp_report(ts, ts->rx_report) == 0) {
		process_touch_report(ts, ts->rx_report);
		propagate_report(ts, 0, ts->rx_report);
	}
	ts->irq_counter++;

	return IRQ_HANDLED;
}

static ssize_t init_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", init_state);
}

static ssize_t init_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	int value, ret;

	CHECK(sscanf(buf, "%d", &value) != 1, , -EINVAL, "bad parameter");
	switch (value) {
	case 0:
		if (init_state == 0)
			break;
		ret = device_deinit(to_i2c_client(dev));
		CHECK(ret != 0, , ret, "deinit error (%d)", ret);
		break;
	case 1:
		if (init_state == 1)
			break;
		ret = device_init(to_i2c_client(dev));
		CHECK(ret != 0, , ret, "init error (%d)", ret);
		break;
	case 2:
		if (init_state == 1) {
			ret = device_deinit(to_i2c_client(dev));
			CHECK(ret != 0, , ret, "deinit error (%d)", ret);
		}
		ret = device_init(to_i2c_client(dev));
		CHECK(ret != 0, , ret, "init error (%d)", ret);
		break;
	default:
		ERROR("bad value");
		return -EINVAL;
	}

	return count;
}

static ssize_t hreset_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);
//	struct max11871_pdata *pdata = client->dev.platform_data;
	struct max11871_pdata *pdata = &max11871_data;

	if (!pdata->reset)
		return count;

	DISABLE_IRQ();
	mutex_lock(&ts->i2c_mutex);
	pdata->reset(pdata, 0);
	usleep_range(10000, 11000);
	pdata->reset(pdata, 1);
	bootloader = 0;
	ts->got_report = 0;
	mutex_unlock(&ts->i2c_mutex);
	CHECK(get_report(ts, 0x01A0, 3000) != 0, , count, "Failed to receive "
		"system status report");
	release_report(ts);

	return count;
}

static ssize_t sreset_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);

	DISABLE_IRQ();
	CHECK(sreset(client) != 0, , count, "Failed to do soft reset.");
	CHECK(get_report(ts, 0x01A0, 3000) != 0, , count, "Failed to receive "
		"system status report");
	release_report(ts);
	return count;
}

static ssize_t irq_count_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE, "%u\n", ts->irq_counter);
}

static ssize_t irq_count_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);

	ts->irq_counter = 0;
	return count;
}

static ssize_t dflt_cfg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE, "%u %u %u\n", CONFIG(defaults_allow),
			CONFIG(default_chip_config), CONFIG(default_chip_id));
}

static ssize_t dflt_cfg_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	(void)sscanf(buf, "%hu %hu %hu", &CONFIG(defaults_allow),
			&CONFIG(default_chip_config),
			&CONFIG(default_chip_id));
	return count;
}

static ssize_t panel_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE, "%u %u %u %u %u %u\n",
			COORDINATES(panel_mx_l), COORDINATES(panel_mx_h),
			COORDINATES(panel_my_l), COORDINATES(panel_my_h),
			COORDINATES(lcd_x), COORDINATES(lcd_y));
}

static ssize_t panel_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	(void)sscanf(buf, "%hu %hu %hu %hu %hu %hu", &COORDINATES(panel_mx_l),
			&COORDINATES(panel_mx_h), &COORDINATES(panel_my_l),
			&COORDINATES(panel_my_h), &COORDINATES(lcd_x),
			&COORDINATES(lcd_y));
	return count;
}

static ssize_t buttons_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	return snprintf(buf, PAGE_SIZE, "%u %u %u\n", CONFIG(buttons_enabled),
			CONFIG(buttons_type), CONFIG(buttons));
}

static ssize_t buttons_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);

	(void)sscanf(buf, "%hu %hu %hu", &CONFIG(buttons_enabled),
			&CONFIG(buttons_type), &CONFIG(buttons));
	CONFIG(buttons_enabled) = !!CONFIG(buttons_enabled);
	if (CONFIG(buttons_type) != MAX11871_BUTTONS_XY &&
	    CONFIG(buttons_type) != MAX11871_BUTTONS_SENSE)
		CONFIG(buttons_type) = MAX11871_BUTTONS_XY;
	if (CONFIG(buttons) > MAX11871_MAX_BUTTONS)
		CONFIG(buttons) = MAX11871_MAX_BUTTONS;
	return count;
}

static ssize_t button_show(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);
	u16                index;

	sscanf(attr->attr.name, "button%hu", &index);
	index--;
	if (CONFIG(buttons_type) == MAX11871_BUTTONS_SENSE)
		return snprintf(buf, PAGE_SIZE, "%u\n",
				CONFIG(button_code[index]));
	else
		return snprintf(buf, PAGE_SIZE, "%u %u %u %u %u\n",
				BUTTONXY(index, x), BUTTONXY(index, y),
				BUTTONXY(index, size_x),
				BUTTONXY(index, size_y),
				CONFIG(button_code[index]));
}

static ssize_t button_store(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client  *client = to_i2c_client(dev);
	struct data        *ts = i2c_get_clientdata(client);
	u16                index;

	sscanf(attr->attr.name, "button%hu", &index);
	index--;
	if (CONFIG(buttons_type) == MAX11871_BUTTONS_SENSE)
		(void)sscanf(buf, "%u", &CONFIG(button_code[index]));
	else
		(void)sscanf(buf, "%hu %hu %hu %hu %u",
				&BUTTONXY(index, x), &BUTTONXY(index, y),
				&BUTTONXY(index, size_x),
				&BUTTONXY(index, size_y),
				&CONFIG(button_code[index]));
	return count;
}

static ssize_t fw_ver_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);
	u16 build_number = 0;
	u8 branch = BYTEL(ts->fw_version[3]) >> 6;

	if (ts->fw_version[1] >= 3)
		build_number = ts->fw_version[4];
	return snprintf(buf, PAGE_SIZE, "%u.%u.%u p%u%c "
		"(CRC16 0x%04X=>0x%04X) Chip ID 0x%02X\n",
		BYTEH(ts->fw_version[2]), BYTEL(ts->fw_version[2]),
		build_number, BYTEL(ts->fw_version[3]) & 0x3F,
		(branch == 0) ? ' ' : (branch - 1 + 'a'), (ts->fw_index != -1)
			? CONFIG(fw_image[ts->fw_index]).config_boundary : 0,
		ts->fw_crc16, BYTEH(ts->fw_version[3]));
}

static ssize_t driver_ver_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "3.0.1: June 17, 2012\n");
}

static ssize_t debug_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%04X\n", debug_mask);
}

static ssize_t debug_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	CHECK(sscanf(buf, "%hx", &debug_mask) != 1, , -EINVAL,
		"bad parameter");
	return count;
}

static ssize_t command_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct data *ts = i2c_get_clientdata(client);
	u16 buffer[MAX_WORDS_COMMAND_ALL];
	char scan_buf[5];
	int i;

	count--;  /* ignore carriage return */
	CHECK((count % 4) != 0, , -EINVAL, "words not properly defined");
	scan_buf[4] = '\0';
	for (i = 0; i < count; i += 4) {
		memcpy(scan_buf, &buf[i], 4);
		CHECK(sscanf(scan_buf, "%hx", &buffer[i / 4]) != 1, , -EINVAL,
			"bad word (%s)", scan_buf);
	}
	CHECKI(send_mtp_command(ts, buffer, count / 4), "MTP command failed");
	return ++count;
}

static ssize_t report_read(struct file *file, struct kobject *kobj,
			struct bin_attribute *attr, char *buf, loff_t off,
			size_t count)
{
	struct i2c_client *client = kobj_to_i2c_client(kobj);
	struct data *ts = i2c_get_clientdata(client);
	int printed, i, offset = 0, payload;

	if (get_report(ts, 0xFFFF, 0xFFFFFFFF))
		return 0;

	payload = BYTEL(ts->report[0]);
	if (count < (4 * payload + 1))
		return -EIO;
	if (count > (4 * payload + 1))
		count = 4 * payload + 1;

	for (i = 1; i <= payload; i++) {
		printed = snprintf(&buf[offset], PAGE_SIZE, "%04X\n",
							ts->report[i]);
		if (printed <= 0)
			return -EIO;
		offset += printed - 1;
	}
	snprintf(&buf[offset], PAGE_SIZE, "\n");
	release_report(ts);

	return count;
}

static DEVICE_ATTR(init,         0666, init_show,         init_store);
static DEVICE_ATTR(hreset,       0222, NULL,              hreset_store);
static DEVICE_ATTR(sreset,       0222, NULL,              sreset_store);
static DEVICE_ATTR(irq_count,    0666, irq_count_show,    irq_count_store);
static DEVICE_ATTR(dflt_cfg,     0666, dflt_cfg_show,     dflt_cfg_store);
static DEVICE_ATTR(panel,        0666, panel_show,        panel_store);
static DEVICE_ATTR(buttons,      0666, buttons_show,      buttons_store);
static DEVICE_ATTR(fw_ver,       0444, fw_ver_show,       NULL);
static DEVICE_ATTR(driver_ver,   0444, driver_ver_show,   NULL);
static DEVICE_ATTR(debug,        0666, debug_show,        debug_store);
static DEVICE_ATTR(command,      0222, NULL,              command_store);
static struct bin_attribute dev_attr_report = {
	.attr = {.name = "report", .mode = 0444}, .read = report_read};
static struct device_attribute dev_attr_button[MAX11871_MAX_BUTTONS];
static char                    dev_attr_button_name[MAX11871_MAX_BUTTONS][10];

static void collect_chip_data(struct data *ts)
{
	int  ret;

	ret = get_report(ts, 0x01A0, 3000);
	if (ret != 0) {
		ERROR("Failed to receive system status report");
		if (CONFIG(defaults_allow) == 0)
			msleep(5000);
	} else {
		release_report(ts);
		ts->fw_responsive = 1;
	}
	DISABLE_IRQ();
	ret = get_fw_version(ts->client);
	CHECKI(ret < 0, "Failed to retrieve firmware version");
	if (ret == 0) {
		ret = get_report(ts, 0x0140, 100);
		CHECKI(ret != 0, "Failed to receive firmware version report");
		if (ret == 0) {
			memcpy(ts->fw_version, &ts->report[1],
				BYTE_SIZE(ts->report[2] + 2));
			release_report(ts);
			ts->have_fw = 1;
		}
	}
	DISABLE_IRQ();
	ret = get_touch_config(ts->client);
	CHECKI(ret < 0, "Failed to retrieve touch config");
	if (ret == 0) {
		ret = get_report(ts, 0x0102, 100);
		CHECKI(ret != 0, "Failed to receive touch config report");
		if (ret == 0) {
			memcpy(ts->touch_config, &ts->report[1],
				BYTE_SIZE(ts->report[2] + 2));
			release_report(ts);
			ts->have_touchcfg = 1;
		}
	}
	ENABLE_IRQ();
	DEBUG(5, "(INIT): firmware responsive: (%u)", ts->fw_responsive);
	if (ts->fw_responsive) {
		if (ts->have_fw)
			DEBUG(5, "(INIT): firmware version: %u.%u Chip ID: "
				"0x%02X", BYTEH(ts->fw_version[2]),
				BYTEL(ts->fw_version[2]),
				BYTEH(ts->fw_version[3]));
		if (ts->have_touchcfg)
			DEBUG(5, "(INIT): configuration ID: 0x%04X",
				ts->touch_config[2]);
	}
}

static int device_fw_load(struct data *ts, const struct firmware *fw,
			u16 fw_index)
{
	u16 fw_crc16, chip_crc16;

	fw_crc16 = crc16(0, fw->data,
			CONFIG(fw_image[fw_index]).config_boundary);
	DEBUG(5, "(INIT): firmware size (%d) CRC16(0x%04X)", fw->size,
		fw_crc16);
	CHECK(bootloader_enter(ts), bootloader_exit(ts), -1,
		"Failed to enter bootloader");
	CHECK(bootloader_get_crc(ts, &chip_crc16,
		CONFIG(fw_image[fw_index]).config_boundary),
		bootloader_exit(ts), -1,
		"Failed to get CRC16 from the chip");
	DEBUG(5, "(INIT): chip CRC16(0x%04X)", chip_crc16);
	ts->fw_index = fw_index;
	ts->fw_crc16 = chip_crc16;
	if (fw_crc16 != chip_crc16) {
		DEBUG(5, "(INIT): will reprogram chip");
		CHECK(bootloader_erase_flash(ts), bootloader_exit(ts), -1,
			"Failed to erase chip flash");
		DEBUG(5, "(INIT): flash erase OK");
		CHECK(bootloader_set_byte_mode(ts), bootloader_exit(ts), -1,
			"Failed to set byte mode");
		DEBUG(5, "(INIT): byte mode OK");
		CHECK(bootloader_write_flash(ts, (u8 *)fw->data),
			bootloader_exit(ts), -1, "Failed to write flash");
		DEBUG(5, "(INIT): flash write OK");
		fw_crc16 = crc16(0, fw->data,
				CONFIG(fw_image[fw_index]).length);
		CHECK(bootloader_get_crc(ts, &chip_crc16,
			CONFIG(fw_image[fw_index]).length),
			bootloader_exit(ts), -1,
			"Failed to get CRC16 from the chip");
		CHECK(fw_crc16 != chip_crc16, bootloader_exit(ts), -1,
			"Failed to verify programming! (0x%04X)", chip_crc16);
		DEBUG(5, "(INIT): chip programmed successfully");
		CHECK(bootloader_get_crc(ts, &chip_crc16,
			CONFIG(fw_image[fw_index]).config_boundary),
			bootloader_exit(ts), -1, "Failed to get CRC16 from "
			"the chip");
		DEBUG(5, "(INIT): new chip CRC16(0x%04X)", chip_crc16);
		ts->fw_crc16 = chip_crc16;
	}
	CHECK(bootloader_exit(ts), bootloader_exit(ts), -1,
		"Failed to exit bootloader");
	return 0;
}

static int is_booting(void)
{
	unsigned long long t;
	unsigned long nanosec_rem;

	t = cpu_clock(smp_processor_id());
	nanosec_rem = do_div(t, 1000000000);
	return (t < 30) ? 1 : 0;
}

static void check_fw_and_config(struct data *ts, u16 request_slept,
				u16 panel_x, u16 panel_y)
{
	const struct firmware *fw;
#if 0
	u16 x_range, y_range;
#endif
	u16 config_id, chip_id;
	int i, j, ret;

	collect_chip_data(ts);
	CHECK((ts->have_fw == 0 || ts->have_touchcfg == 0) &&
		CONFIG(defaults_allow) == 0, , , "firmware is unresponsive "
		"or inconsistent and default selections are disabled");
	config_id = ts->have_touchcfg ? ts->touch_config[2] :
						CONFIG(default_chip_config);
	chip_id = ts->have_fw ? BYTEH(ts->fw_version[3]) :
						CONFIG(default_chip_id);
	for (i = 0; i < CONFIG(chip_configs); i++)
		if (CONFIG(chip_config[i]).config_id == config_id)
			break;
	CHECK(i == CONFIG(chip_configs), , , "configuration is not found for "
		"ID 0x%04X", config_id);
	for (j = 0; j < CONFIG(chip_config[i]).fw_mappings; j++)
		if (CONFIG(chip_config[i]).fw_mapping[j].chip_id == chip_id)
			break;
	CHECK(j == CONFIG(chip_config[i]).fw_mappings, , , "firmware image is"
		" not found for configuration 0x%04X and chip 0x%02X",
		config_id, chip_id);
	j = CONFIG(chip_config[i]).fw_mapping[j].fw_index;
	if (request_slept == 0 && is_booting() &&
		CONFIG(cfgfw_request_delay) > 0)
		msleep(CONFIG(cfgfw_request_delay));
	DEBUG(5, "(INIT): firmware file (%s)", CONFIG(fw_image[j]).file_name);
	ret = request_firmware(&fw, CONFIG(fw_image[j]).file_name,
				&ts->client->dev);
	CHECK(ret || fw == NULL, , , "firmware request failed (%d,%p)", ret,
		fw);
	CHECK(fw->size != CONFIG(fw_image[j]).length, release_firmware(fw), ,
		"firmware size %d is different from expected %d", fw->size,
		CONFIG(fw_image[j]).length);
	for (i = 0; i < MAX_FWDL_RETRIES; i++)
		if (device_fw_load(ts, fw, j) == 0)
			break;
	release_firmware(fw);
	CHECK(i == MAX_FWDL_RETRIES, , , "firmware download failed after %d "
		"retries", MAX_FWDL_RETRIES);
	collect_chip_data(ts);
	CHECK(ts->have_fw == 0 || ts->have_touchcfg == 0, , , "firmware is "
		"unresponsive or inconsistent after firmware download");
	DEBUG(5, "(INIT): firmware download OK");

	/* configure the chip */
	/* do not do this for Pantech */
#if 0
	x_range = (CONFIG(coordinate_settings) & MAX11871_SWAP_XY) ? panel_y :
								panel_x;
	if (CONFIG(coordinate_settings) & MAX11871_REVERSE_X)
		x_range |= 0x8000;
	y_range = (CONFIG(coordinate_settings) & MAX11871_SWAP_XY) ? panel_x :
								panel_y;
	if (CONFIG(coordinate_settings) & MAX11871_REVERSE_Y)
		y_range |= 0x8000;
	DEBUG(5, "(INIT): Touch Config: X(%04X) Y(%04X) xR(%04X) yR(%04X)",
		ts->touch_config[27], ts->touch_config[28], x_range, y_range);
	if (ts->touch_config[27] != x_range ||
					ts->touch_config[28] != y_range) {
		ts->touch_config[27] = x_range;
		ts->touch_config[28] = y_range;
		ts->touch_config[0] = 0x0001;
#endif
#if 0
	if (BYTEH(ts->touch_config[6]) != CONFIG(max_touches)) {
		ts->touch_config[0] = 0x0001;
		ts->touch_config[6] = (CONFIG(max_touches) << 8) |
										BYTEL(ts->touch_config[6]);
		send_mtp_command(ts, ts->touch_config,
						ts->touch_config[1] + 2);
		DISABLE_IRQ();
		sreset(ts->client);
		CHECK(get_report(ts, 0x01A0, 1000) != 0, , , "Failed to "
			"receive system status report");
		release_report(ts);
		DISABLE_IRQ();
		CHECK(get_touch_config(ts->client) < 0, ENABLE_IRQ(), ,
			"Failed to retrieve touch config");
		CHECK(get_report(ts, 0x0102, 100) != 0, , , "Failed to "
			"receive touch config report");
		memcpy(ts->touch_config, &ts->report[1],
						BYTE_SIZE(ts->report[2] + 2));
		release_report(ts);

		DEBUG(5, "(INIT): Configuration is OK");

	}
#endif
	CHECK(change_touch_rpt(ts->client, MAXIM_TOUCH_REPORT_MODE) < 0, , ,
		"Failed to set up touch report mode");
}

static int device_init_thread(void *arg)
{
	return device_init((struct i2c_client *)arg);
}

static int load_done=0;
static int device_init(struct i2c_client *client)
{
	struct data *ts = NULL;
//	struct max11871_pdata *pdata = client->dev.platform_data;
	struct max11871_pdata *pdata = &max11871_data;
	const struct firmware *fw;
	u16 request_slept = 0, panel_x, panel_y;
	int ret;

	init_state = 1;
	PRINT("(INIT): Start");

	/* if I2C functionality is not present we are done */
	CHECK(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C), , 0,
		"I2C core driver does not support I2C functionality");
	DEBUG(5, "(INIT): I2C functionality OK");

	/* if platform data is missing we are also done */
	CHECK(!pdata, , 0, "Platform data is missing");
	DEBUG(5, "(INIT): platform data OK");

	/* allocate control block; nothing more to do if we can't */
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	CHECK(!ts, , 0, "Failed to allocate control block memory");
#if MAX11871_BOARD_CONFIG
	ts->config = pdata->config;
	CHECK(!ts->config, , 0, "Configuration data is missing");
#else
	ts->config = &local_config;
#endif
	ts->client = client;
	i2c_set_clientdata(client, ts);
	mutex_init(&ts->irq_mutex);
	mutex_init(&ts->i2c_mutex);
	mutex_init(&ts->report_mutex);
	sema_init(&ts->report_sem, 1);
	ts->nbuttons_original = CONFIG(buttons);
	ts->fw_index = -1;
//	ts->buttondown = -1;
	DEBUG(5, "(INIT): memory allocation OK");

	/* request configuration from file */
	if (CONFIG(cfg_request) && CONFIG(cfg_file_name)) {
		if (is_booting() && CONFIG(cfgfw_request_delay) > 0) {
			msleep(CONFIG(cfgfw_request_delay));
			request_slept = 1;
		}
		ret = request_firmware(&fw, CONFIG(cfg_file_name),
					&ts->client->dev);
		if (ret || fw == NULL) {
			ERROR("configuration file [%s] cannot be acquired, "
				"%d %p", CONFIG(cfg_file_name), ret, fw);
		} else {
			if (fw->size >= 10) {
				/* parse configuration here */
				DEBUG(5, "(INIT): configuration parsed OK");
			} else
				DEBUG(5, "configuration file size is small "
					"(%d bytes)", fw->size);
			release_firmware(fw);
		}
	}
	/* need to validate configuration here */

	/* initialize GPIO pins */
	if (pdata->init)
		CHECK(pdata->init(pdata, 1) < 0, , 0, "GPIO init failed");
	msleep(40);
	DEBUG(5, "(INIT): chip init OK");

	/* set up IRQ and handler */
	client->irq=IRQ_TOUCH_INT;
	CHECK(request_threaded_irq(client->irq, NULL, irq_handler,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, ts) != 0, ,
		0, "Failed to set up IRQ");
	DEBUG(5, "(INIT): IRQ handler OK");

	/* start with known state - reset the chip */
	CHECK(bootloader_enter(ts), , 0, "Failed to enter bootloader");
	CHECK(bootloader_exit(ts), , 0, "Failed to exit bootloader");
	DEBUG(5, "(INIT): chip reset OK");

	/* evaluate configuration parameters */
	if (CONFIG(coordinate_model) == MAX11871_OPTIMAL) {
		panel_x = COORDINATES(panel_mx_l) + COORDINATES(lcd_x) +
				COORDINATES(panel_mx_h) - 1;
		panel_y = COORDINATES(panel_my_l) + COORDINATES(lcd_y) +
				COORDINATES(panel_my_h) - 1;
	} else {
		panel_x = 0x3E8;
		panel_y = 0x3EC;
	}

	/* collect controller ID and configuration ID data from firmware   */
	/* and perform firmware comparison/download if we have valid image */
	check_fw_and_config(ts, request_slept, panel_x, panel_y);

	/* allocate and register touch device */
	ts->input_dev = input_allocate_device();
	CHECK(!ts->input_dev, , 0, "Failed to allocate touch input device");
	snprintf(ts->phys, sizeof(ts->phys), "%s/input0",
		dev_name(&ts->client->dev));
	ts->input_dev->name = "max11871-touchscreen0";
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;
	__set_bit(EV_SYN, ts->input_dev->evbit);
	__set_bit(EV_ABS, ts->input_dev->evbit);

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
				COORDINATES(panel_mx_l),
				panel_x - COORDINATES(panel_mx_h), 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
				COORDINATES(panel_my_l),
				panel_y - COORDINATES(panel_my_h), 0, 0);
#ifdef ABS_MT_PRESSURE
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, 0xFF, 0,
#else
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xFF, 0,
#endif
				0);
	if (CONFIG(input_protocol) == MAX11871_PROTOCOL_A_TRACK)
		input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0,
					MAX_FINGER_IDS, 0, 0);
	if (CONFIG(input_protocol) == MAX11871_PROTOCOL_B)
		input_mt_init_slots(ts->input_dev, MAX_FINGER_IDS);
	if (CONFIG(input_protocol) == MAX11871_PROTOCOL_CUSTOM1) {
#if defined(ABS_MT_AMPLITUDE) && defined(ABS_MT_POSITION)
		input_set_abs_params(ts->input_dev, ABS_MT_AMPLITUDE, 0,
					0xFF14, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_POSITION, 0,
					((1 << 31) | (999 << 16) | 1003), 0,
					0);
#endif
	}

	CHECK(input_register_device(ts->input_dev),
		{input_free_device(ts->input_dev); ts->input_dev = NULL; }, 0,
		"Failed to register touch input device");
	DEBUG(5, "(INIT): input touch device OK");

	maxtsp_data = ts;

	/* allocate and register key device */
#if 0
	if (CONFIG(buttons_enabled)) {
		ts->key_input_dev = input_allocate_device();
		CHECK(!ts->key_input_dev, , 0, "Failed to allocate key input "
			"device");
		snprintf(ts->key_phys, sizeof(ts->phys), "%s/input1",
			dev_name(&client->dev));
		ts->key_input_dev->name = MAX11871_KEY;
		ts->key_input_dev->phys = ts->key_phys;
		ts->key_input_dev->id.bustype = BUS_I2C;
		__set_bit(EV_KEY,    ts->key_input_dev->evbit);
		set_bit(KEY_HOME, ts->key_input_dev->keybit);
		set_bit(KEY_MENU, ts->key_input_dev->keybit);
		set_bit(KEY_BACK, ts->key_input_dev->keybit);
		set_bit(KEY_SEARCH, ts->key_input_dev->keybit);

		CHECK(input_register_device(ts->key_input_dev),
			{input_free_device(ts->key_input_dev);
			ts->key_input_dev = NULL; }, 0,
			"Failed to register key input device");
		DEBUG(5, "(INIT): input key device OK");
	}
#else
		ts->key_input_dev = input_allocate_device();
		CHECK(!ts->key_input_dev, , 0, "Failed to allocate key input "
			"device");
		snprintf(ts->key_phys, sizeof(ts->phys), "%s/input1",
			dev_name(&client->dev));
		ts->key_input_dev->name = "max11871-touchscreen";
		ts->key_input_dev->phys = ts->key_phys;
		ts->key_input_dev->id.bustype = BUS_I2C;
		__set_bit(EV_KEY,    ts->key_input_dev->evbit);
		set_bit(KEY_HOME, ts->key_input_dev->keybit);
		set_bit(KEY_MENU, ts->key_input_dev->keybit);
		set_bit(KEY_BACK, ts->key_input_dev->keybit);
		set_bit(KEY_SEARCH, ts->key_input_dev->keybit);
		set_bit(BTN_TOUCH, ts->key_input_dev->keybit);
		set_bit(BTN_2, ts->key_input_dev->keybit);


		set_bit(KEY_0, ts->key_input_dev->keybit);
		set_bit(KEY_1, ts->key_input_dev->keybit);
		set_bit(KEY_2, ts->key_input_dev->keybit);
		set_bit(KEY_3, ts->key_input_dev->keybit);
		set_bit(KEY_4, ts->key_input_dev->keybit);
		set_bit(KEY_5, ts->key_input_dev->keybit);
		set_bit(KEY_6, ts->key_input_dev->keybit);
		set_bit(KEY_7, ts->key_input_dev->keybit);
		set_bit(KEY_8, ts->key_input_dev->keybit);
		set_bit(KEY_9, ts->key_input_dev->keybit);
		set_bit(0xe3, ts->key_input_dev->keybit); /* '*' */
		set_bit(0xe4, ts->key_input_dev->keybit); /* '#' */
		set_bit(0xe5, ts->key_input_dev->keybit); /* 'KEY_END */

		set_bit(KEY_LEFTSHIFT, ts->key_input_dev->keybit);
		set_bit(KEY_RIGHTSHIFT, ts->key_input_dev->keybit);

		set_bit(KEY_LEFT, ts->key_input_dev->keybit);
		set_bit(KEY_RIGHT, ts->key_input_dev->keybit);
		set_bit(KEY_UP, ts->key_input_dev->keybit);
		set_bit(KEY_DOWN, ts->key_input_dev->keybit);
		set_bit(KEY_ENTER, ts->key_input_dev->keybit);

		set_bit(KEY_SEND, ts->key_input_dev->keybit);
		set_bit(KEY_END, ts->key_input_dev->keybit);

		set_bit(KEY_VOLUMEUP, ts->key_input_dev->keybit);
		set_bit(KEY_VOLUMEDOWN, ts->key_input_dev->keybit);

		set_bit(KEY_CLEAR, ts->key_input_dev->keybit);
		set_bit(KEY_BACK, ts->key_input_dev->keybit);

		set_bit(KEY_CAMERA, ts->key_input_dev->keybit);
		set_bit(KEY_F3, ts->key_input_dev->keybit);

		CHECK(input_register_device(ts->key_input_dev),
			{input_free_device(ts->key_input_dev);
			ts->key_input_dev = NULL; }, 0,
			"Failed to register key input device");
		DEBUG(5, "(INIT): input key device OK");
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	/* configure suspend/resume */
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	ts->early_suspend.suspend = early_suspend;
	ts->early_suspend.resume = late_resume;
	register_early_suspend(&ts->early_suspend);
	ts->early_suspend_registered = 1;
	DEBUG(5, "(INIT): suspend/resume registration OK");
#endif

#ifdef SKY_PROCESS_CMD_KEY
	ret = misc_register(&touch_event);
	if (ret) {
		ERROR("::::::::: can''t register touch_fops\n");
	}
	DEBUG(5, "(INIT): touch_event OK");
#endif

#ifdef TOUCH_IO  
	ret = misc_register(&touch_io);
	if (ret) 
	{
		ERROR("::::::::: can''t register qt602240 misc\n");
	}
	DEBUG(5, "(INIT): touch_io OK");
#endif
	/* set up debug interface */
	CHECK(device_create_file(&client->dev, &dev_attr_hreset) < 0, , 0,
		"failed to create sysfs file [hreset]");
	CHECK(device_create_file(&client->dev, &dev_attr_sreset) < 0, , 0,
		"failed to create sysfs file [sreset]");
	CHECK(device_create_file(&client->dev, &dev_attr_irq_count) < 0, , 0,
		"failed to create sysfs file [irq_count]");
	CHECK(device_create_file(&client->dev, &dev_attr_dflt_cfg) < 0, , 0,
		"failed to create sysfs file [dflt_cfg]");
	CHECK(device_create_file(&client->dev, &dev_attr_panel) < 0, , 0,
		"failed to create sysfs file [panel]");
	CHECK(device_create_file(&client->dev, &dev_attr_buttons) < 0, , 0,
		"failed to create sysfs file [buttons]");
	DEVFCA(CONFIG(buttons), 0666, button_show, button_store, button);
	CHECK(device_create_file(&client->dev, &dev_attr_fw_ver) < 0, , 0,
		"failed to create sysfs file [fw_ver]");
	CHECK(device_create_file(&client->dev, &dev_attr_driver_ver) < 0, , 0,
		"failed to create sysfs file [driver_ver]");
	CHECK(device_create_file(&client->dev, &dev_attr_debug) < 0, , 0,
		"failed to create sysfs file [debug]");
	CHECK(device_create_file(&client->dev, &dev_attr_command) < 0, , 0,
		"failed to create sysfs file [command]");
	CHECK(device_create_bin_file(&client->dev, &dev_attr_report) < 0, , 0,
		"failed to create sysfs file [report]");

	PRINT("(INIT): Done");

	load_done=1;
	return 0;
}

static int device_deinit(struct i2c_client *client)
{
	struct data *ts = i2c_get_clientdata(client);
//	struct max11871_pdata *pdata = client->dev.platform_data;
	struct max11871_pdata *pdata = &max11871_data;

	propagate_report(ts, -1, NULL);

	init_state = 0;
	device_remove_bin_file(&client->dev, &dev_attr_report);
	device_remove_file(&client->dev, &dev_attr_command);
	device_remove_file(&client->dev, &dev_attr_debug);
	device_remove_file(&client->dev, &dev_attr_driver_ver);
	device_remove_file(&client->dev, &dev_attr_fw_ver);
	DEVFRA(ts->nbuttons_original, button);
	device_remove_file(&client->dev, &dev_attr_buttons);
	device_remove_file(&client->dev, &dev_attr_panel);
	device_remove_file(&client->dev, &dev_attr_dflt_cfg);
	device_remove_file(&client->dev, &dev_attr_irq_count);
	device_remove_file(&client->dev, &dev_attr_sreset);
	device_remove_file(&client->dev, &dev_attr_hreset);

#ifdef CONFIG_HAS_EARLYSUSPEND
	if (ts->early_suspend_registered)
		unregister_early_suspend(&ts->early_suspend);
#endif
	if (ts->input_dev)
		input_unregister_device(ts->input_dev);
#if 0
	if (ts->key_input_dev)
		input_unregister_device(ts->key_input_dev);
#endif
	if (client->irq)
		free_irq(client->irq, ts);

	kfree(ts);
	if (pdata && pdata->init)
		(void)pdata->init(pdata, 0);

	PRINT("(INIT): Deinitialized");
	return 0;
}

static int probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	CHECK(device_create_file(&client->dev, &dev_attr_init) < 0, , 0,
		"failed to create sysfs file [init]");
	if (!is_booting())
		return device_init(client);
	CHECK(IS_ERR(kthread_run(device_init_thread, (void *)client,
		MAX11871_NAME)), , -1, "failed to start kernel thread");

	while(!load_done) msleep(1);
	DEBUG(5, "(INIT): Probe OK");

	return 0;
}

static int remove(struct i2c_client *client)
{
	int ret = device_deinit(client);

	device_remove_file(&client->dev, &dev_attr_init);
	return ret;
}

/*
	COMMANDS
*/
static int sreset(struct i2c_client *client)
{
	struct data *ts = i2c_get_clientdata(client);
	u16 data[] = {0x00E9, 0x0000};
	return send_mtp_command(ts, data, NWORDS(data));
}

static int get_touch_config(struct i2c_client *client)
{
	struct data *ts = i2c_get_clientdata(client);
	u16 data[] = {0x0002, 0x0000};
	return send_mtp_command(ts, data, NWORDS(data));
}

static int get_fw_version(struct i2c_client *client)
{
	struct data *ts = i2c_get_clientdata(client);
	u16 data[] = {0x0040, 0x0000};
	return send_mtp_command(ts, data, NWORDS(data));
}

static int change_touch_rpt(struct i2c_client *client, u16 to)
{
	struct data *ts = i2c_get_clientdata(client);
	u16 data[] = {0x0018, 0x0001, to & 0x0003};
	return send_mtp_command(ts, data, NWORDS(data));
}

static int change_baseline_rpt(struct i2c_client *client, u16 to)
{
	struct data *ts = i2c_get_clientdata(client);
	u16 data[] = {0x0028, 0x0001, to & 0x0003};
	return send_mtp_command(ts, data, NWORDS(data));
}

static void propagate_report(struct data *ts, int status, u16 *report)
{
	int i;

	down(&ts->report_sem);
	if (report)
		memcpy(ts->report, report, BYTE_SIZE(BYTEL(report[0]) + 1));

	mutex_lock(&ts->report_mutex);
	for (i = 0; i < MAX_REPORT_READERS; i++) {
		if (status == 0) {
			if (ts->report_readers[i].report_id == 0xFFFF ||
			    ts->report_readers[i].report_id ==
							ts->report[1]) {
				up(&ts->report_readers[i].sem);
				ts->report_readers[i].reports_passed++;
				ts->report_readers_outstanding++;
			}
		} else {
			if (ts->report_readers[i].report_id != 0) {
				ts->report_readers[i].status = status;
				up(&ts->report_readers[i].sem);
			}
		}
	}
	if (ts->report_readers_outstanding == 0)
		up(&ts->report_sem);
	mutex_unlock(&ts->report_mutex);
}

static int get_report(struct data *ts, u16 report_id, ulong timeout)
{
	int i, ret, status;

	mutex_lock(&ts->report_mutex);
	for (i = 0; i < MAX_REPORT_READERS; i++)
		if (ts->report_readers[i].report_id == 0)
			break;
	CHECK(i == MAX_REPORT_READERS, {mutex_unlock(&ts->report_mutex);
			ENABLE_IRQ(); }, -1, "maximum readers reached");
	ts->report_readers[i].report_id = report_id;
	sema_init(&ts->report_readers[i].sem, 1);
	down(&ts->report_readers[i].sem);
	ts->report_readers[i].status = 0;
	ts->report_readers[i].reports_passed = 0;
	mutex_unlock(&ts->report_mutex);
	ENABLE_IRQ();

	if (timeout == 0xFFFFFFFF)
		ret = down_interruptible(&ts->report_readers[i].sem);
	else
		ret = down_timeout(&ts->report_readers[i].sem,
					(timeout * HZ) / 1000);

	mutex_lock(&ts->report_mutex);
	if (ret && ts->report_readers[i].reports_passed > 0)
		if (--ts->report_readers_outstanding == 0)
			up(&ts->report_sem);
	status = ts->report_readers[i].status;
	ts->report_readers[i].report_id = 0;
	mutex_unlock(&ts->report_mutex);

	return (status == 0) ? ret : status;
}

static void release_report(struct data *ts)
{
	mutex_lock(&ts->report_mutex);
	if (--ts->report_readers_outstanding == 0)
		up(&ts->report_sem);
	mutex_unlock(&ts->report_mutex);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void early_suspend(struct early_suspend *h)
{
	u16 data[] = {0x0020, 0x0001, 0x0000};
	struct data *ts;
	ts = container_of(h, struct data, early_suspend);

#ifdef TOUCH_IO
	rawDataEn = 0;
#endif
	DISABLE_IRQ();

	Clear_Touch_Event();

	(void)send_mtp_command(ts, data, NWORDS(data));
//	ENABLE_IRQ();
}

static void late_resume(struct early_suspend *h)
{
#ifndef ESD_PROTECT
	u16 data[] = {0x0020, 0x0001, 0x0002};
#endif
	struct data *ts;
	ts = container_of(h, struct data, early_suspend);

	rawDataEn=0;

#ifdef ESD_PROTECT
	DEBUG(5, "(late_resume): ESD Protect");
	sreset(ts->client);
	msleep(20); 
#else
	/* previous_fingers = current_fingers = 0; */
	(void)send_mtp_command(ts, data, NWORDS(data));
#endif

	(void)change_touch_rpt(ts->client, MAXIM_TOUCH_REPORT_MODE);

	ENABLE_IRQ();//p14696 jhseo move from early_suspend
}
#endif

#define STATUS_ADDR_H 0x00
#define STATUS_ADDR_L 0xFF
#define DATA_ADDR_H   0x00
#define DATA_ADDR_L   0xFE
#define STATUS_READY_H 0xAB
#define STATUS_READY_L 0xCC
#define RXTX_COMPLETE_H 0x54
#define RXTX_COMPLETE_L 0x32
static int bootloader_read_status_reg(struct data *ts, const u8 byteL,
					const u8 byteH)
{
	u8 buffer[] = {STATUS_ADDR_L, STATUS_ADDR_H}, i;

	for (i = 0; i < 3; i++) {
		CHECK(i2c_tx_bytes(ts, buffer, 2) != 2, , -1, "TX fail");
		CHECK(i2c_rx_bytes(ts, buffer, 2) != 2, , -1, "RX fail");
		if (buffer[0] == byteL && buffer[1] == byteH)
			break;
	}
	CHECK(i == 3, , -1, "Unexpected status => %02X%02X vs %02X%02X",
		buffer[0], buffer[1], byteL, byteH);

	return 0;
}

static int bootloader_write_status_reg(struct data *ts, const u8 byteL,
					const u8 byteH)
{
	u8 buffer[] = {STATUS_ADDR_L, STATUS_ADDR_H, byteL, byteH};

	CHECK(i2c_tx_bytes(ts, buffer, 4) != 4, , -1, "TX fail");
	return 0;
}

static int bootloader_rxtx_complete(struct data *ts)
{
	return bootloader_write_status_reg(ts, RXTX_COMPLETE_L,
						RXTX_COMPLETE_H);
}

static int bootloader_read_data_reg(struct data *ts, u8* byteL, u8* byteH)
{
	u8 buffer[] = {DATA_ADDR_L, DATA_ADDR_H, 0x00, 0x00};

	CHECK(i2c_tx_bytes(ts, buffer, 2) != 2, , -1, "TX fail");
	CHECK(i2c_rx_bytes(ts, buffer, 4) != 4, , -1, "RX fail");
	CHECK(buffer[2] != 0xCC && buffer[3] != 0xAB, , -1,
		"Status is not ready");

	*byteL = buffer[0];
	*byteH = buffer[1];
	return bootloader_rxtx_complete(ts);
}

static int bootloader_write_data_reg(struct data *ts, const u8 byteL,
					const u8 byteH)
{
	u8 buffer[6] = {DATA_ADDR_L, DATA_ADDR_H, byteL, byteH,
			RXTX_COMPLETE_L, RXTX_COMPLETE_H};

	CHECK(bootloader_read_status_reg(ts, STATUS_READY_L,
		STATUS_READY_H) < 0, , -1, "read status register fail");
	CHECK(i2c_tx_bytes(ts, buffer, 6) != 6, , -1, "TX fail");
	return 0;
}

static int bootloader_rxtx(struct data *ts, u8* byteL, u8* byteH, const int tx)
{
	if (tx > 0) {
		CHECK(bootloader_write_data_reg(ts, *byteL, *byteH) < 0, , -1,
			"write data register fail");
		return 0;
	}

	CHECK(bootloader_read_data_reg(ts, byteL, byteH) < 0, , -1, "read "
		"data register fail");
	return 0;
}

static int bootloader_get_cmd_conf(struct data *ts, int retries)
{
	u8 byteL, byteH;

	do {
		if (bootloader_read_data_reg(ts, &byteL, &byteH) >= 0) {
			if (byteH == 0x00 && byteL == 0x3E)
				return 0;
		}
		retries--;
	} while (retries > 0);

	return -1;
}

static int bootloader_write_buffer(struct data *ts, u8* buffer, int size)
{
	u8 byteH = 0x00;
	int k;

	for (k = 0; k < size; k++) {
		CHECK(bootloader_rxtx(ts, &buffer[k], &byteH, 1) < 0, , -1,
			"bootloader RX-TX fail");
	}
	return 0;
}

static int bootloader_enter(struct data *ts)
{
	int i;
	u16 enter[3][2] = {{0x7F00, 0x0047}, {0x7F00, 0x00C7},
			{0x7F00, 0x0007} };

	DISABLE_IRQ();
	mutex_lock(&ts->i2c_mutex);
	for (i = 0; i < 3; i++) {
		CHECK(i2c_tx_words(ts, enter[i], 2) != 2,
			{mutex_unlock(&ts->i2c_mutex); ENABLE_IRQ(); }, -1,
			"Failed to enter bootloader");
	}

	CHECK(bootloader_get_cmd_conf(ts, 5) < 0,
		{mutex_unlock(&ts->i2c_mutex); ENABLE_IRQ(); }, -1,
		"Failed to enter bootloader mode");
	bootloader = 1;
	return 0;
}

static int bootloader_exit(struct data *ts)
{
	u16 exit[] = {0x00FE, 0x0001, 0x5432};

	bootloader = 0;
	ts->got_report = 0;
	CHECK(i2c_tx_words(ts, exit, NWORDS(exit)) != NWORDS(exit),
		mutex_unlock(&ts->i2c_mutex), -1,
		"Failed to exit bootloader");
	mutex_unlock(&ts->i2c_mutex);
	return 0;
}

static int bootloader_get_crc(struct data *ts, u16 *crc16, u16 len)
{
	u8 crc_command[] = {0x30, 0x02, 0x00, 0x00, BYTEL(len), BYTEH(len)};
	u8 byteL = 0, byteH = 0;
	u16 rx_crc16 = 0;

	CHECK(bootloader_write_buffer(ts, crc_command, 6) < 0, , -1,
		"write buffer fail");
	msleep(200); /* wait 200ms for it to get done */

	/* reads low 8bits (crcL) */
	CHECK(bootloader_rxtx(ts, &byteL, &byteH, 0) < 0, , -1,
		"Failed to read low byte of crc response!");
	rx_crc16 = (u16)byteL;

	/* reads high 8bits (crcH) */
	CHECK(bootloader_rxtx(ts, &byteL, &byteH, 0) < 0, , -1,
		"Failed to read high byte of crc response!");
	rx_crc16 = (u16)(byteL << 8) | rx_crc16;

	CHECK(bootloader_get_cmd_conf(ts, 5) < 0, , -1, "CRC get failed");
	*crc16 = rx_crc16;

	return 0;
}

static int bootloader_set_byte_mode(struct data *ts)
{
	u8 buffer[2] = {0x0A, 0x00};

	CHECK(bootloader_write_buffer(ts, buffer, 2) < 0, , -1,
		"write buffer fail");
	CHECK(bootloader_get_cmd_conf(ts, 10) < 0, , -1,
		"command confirm fail");
	return 0;
}

static int bootloader_erase_flash(struct data *ts)
{
	u8 byteL = 0x02, byteH = 0x00;
	int i, verify = 0;

	CHECK(bootloader_rxtx(ts, &byteL, &byteH, 1) < 0, , -1,
		"bootloader RX-TX fail");

	for (i = 0; i < 10; i++) {
		msleep(60); /* wait 60ms */

		if (bootloader_get_cmd_conf(ts, 0) < 0)
			continue;

		verify = 1;
		break;
	}

	CHECK(verify != 1, , -1, "Flash Erase failed");

	return 0;
}

static int bootloader_write_flash(struct data *ts, u8 *image)
{
	u8 command[] = {0xF0, 0x00, 0x80, 0x00, 0x00}, buffer[130];
	int i, j;

	CHECK(bootloader_write_buffer(ts, command, 5) < 0, , -1,
		"write buffer fail");

	for (i = 0; i < 256; i++) {
		for (j = 0; j < 100; j++) {
			usleep_range(1500, 2000);
			if (bootloader_read_status_reg(ts, STATUS_READY_L,
						STATUS_READY_H) == 0)
				break;
		}
		CHECK(j == 100, , -1, "Failed to read Status register!");

		buffer[0] = ((i % 2) == 0) ? 0x00 : 0x40;
		buffer[1] = 0x00;
		memcpy(buffer + 2, image + i * 128, 128);

		CHECK(i2c_tx_bytes(ts, buffer, 130) != 130, , -1,
			"Failed to write data (%d)", i);
		CHECK(bootloader_rxtx_complete(ts) < 0, , -1,
			"Transfer failure (%d)", i);
	}

	usleep_range(10000, 11000);
	CHECK(bootloader_get_cmd_conf(ts, 5) < 0, , -1,
		"Flash programming failed");
	return 0;
}

/****************************************
 *
 * Standard Driver Structures/Functions
 *
 ****************************************/
static const struct i2c_device_id id[] = {
	{ MAX11871_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, id);

static struct i2c_driver driver = {
	.probe          = probe,
	.remove         = remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend        = suspend,
	.resume         = resume,
#endif
	.id_table       = id,
	.driver = {
		.name   = MAX11871_NAME,
	},
};

static int __devinit max11871_init(void)
{
	return i2c_add_driver(&driver);
}

static void __exit max11871_exit(void)
{
	i2c_del_driver(&driver);
}

module_init(max11871_init);
module_exit(max11871_exit);

MODULE_AUTHOR("Maxim Integrated Products, Inc.");
MODULE_DESCRIPTION("MAX11871 Touchscreen Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("3.0.1");

