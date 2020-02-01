/**************************************************************
 * Copyright (c)  2008- 2030  Oppo Mobile communication Corp.ltd.£¬
 * VENDOR_EDIT
 * File       : touchpanel_common_driver.c
 * Description: Source file for Touch common driver
 * Version   : 1.0
 * Date        : 2016-09-02
 * Author    : Tong.han@Bsp.Group.Tp
 * TAG         : BSP.TP.Init
 * ---------------- Revision History: --------------------------
 *   <version>    <date>          < author >                            <desc>
 * Revision 1.1, 2016-09-09, Tong.han@Bsp.Group.Tp, modify based on gerrit review result(http://gerrit.scm.adc.com:8080/#/c/223721/)
 ****************************************************************/
#ifndef _TOUCHPANEL_COMMON_H_
#define _TOUCHPANEL_COMMON_H_

/*********PART1:Head files**********************/
#include <linux/input/mt.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/kthread.h>
#include <asm/uaccess.h>
#include <soc/oppo/device_info.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include "util_interface/touch_interfaces.h"
#include "tp_devices.h"

#ifdef CONFIG_TOUCHPANEL_MTK_PLATFORM
#include<mt-plat/mtk_boot_common.h>
#else
#include <soc/oppo/boot_mode.h>
#endif

#define EFTM (250)
#define FW_UPDATE_COMPLETE_TIMEOUT  msecs_to_jiffies(40*1000)

/*********PART2:Define Area**********************/
#define TPD_USE_EINT
#define TYPE_B_PROTOCOL

#define PAGESIZE 512
#define MAX_GESTURE_COORD 6

#define UnkownGesture       0
#define DouTap              1	// double tap
#define UpVee               2	// V
#define DownVee             3	// ^
#define LeftVee             4	// >
#define RightVee            5	// <
#define Circle              6	// O
#define DouSwip             7	// ||
#define Left2RightSwip      8	// -->
#define Right2LeftSwip      9	// <--
#define Up2DownSwip         10	// |v
#define Down2UpSwip         11	// |^
#define Mgestrue            12	// M
#define Wgestrue            13	// W
#define FingerprintDown     14
#define FingerprintUp       15

#define FINGERPRINT_DOWN_DETECT 0X0f
#define FINGERPRINT_UP_DETECT 0X1f

/* bit operation */
#define SET_BIT(data, flag) ((data) |= (flag))
#define CLR_BIT(data, flag) ((data) &= ~(flag))
#define CHK_BIT(data, flag) ((data) & (flag))
#define VK_TAB {KEY_MENU, KEY_HOMEPAGE, KEY_BACK, KEY_SEARCH}

#define TOUCH_BIT_CHECK           0x3FF	//max support 10 point report.using for detect non-valid points
#define MAX_FW_NAME_LENGTH        60
#define MAX_EXTRA_NAME_LENGTH     60

#define MAX_DEVICE_VERSION_LENGTH 16
#define MAX_DEVICE_MANU_LENGTH    16

#define SYNAPTICS_PREFIX    "SY_"
#define GOODIX_PREFIX       "GT_"
#define FOCAL_PREFIX        "FT_"

#define SMART_GESTURE_THRESHOLD 0x0A
#define SMART_GESTURE_LOW_VALUE 0x05

#define GESTURE_RATE_MODE 0
#define GESTURE_COORD_GET 0
#define FW_UPDATE_DELAY        msecs_to_jiffies(2*1000)
/*********PART3:Struct Area**********************/
typedef enum {
	TYPE_DELTA_IDLE,	/*means not in reading delta */
	TYPE_DELTA_BUSY,	/*reading delta data */
} delta_state;

typedef enum {
	TYPE_PROPERTIES = 1,	/*using board_properties */
	TYPE_AREA_SEPRATE,	/*using same IC (button zone &&  touch zone are seprate) */
	TYPE_DIFF_IC,		/*using diffrent IC (button zone &&  touch zone are seprate) */
	TYPE_NO_NEED,		/*No need of virtual key process */
} vk_type;

typedef enum {
	HEALTH_NONE,
	HEALTH_INTERACTIVE_CLEAR = 1,	/*INTERACTIVE SW CLAR DATA */
	HEALTH_FORMAL_CLEAR,	/*FORMAL SW CLAR DATA */
	HEALTH_UPDATARP,	/*trigger chip to update those data */
	HEALTH_INFO_GET,	/*health_monitor node show */
	HEALTH_LASTEXCP_GET,	/*health_monitor node show */
} health_ctl;

typedef enum {
	AREA_NOTOUCH,
	AREA_EDGE,
	AREA_CRITICAL,
	AREA_NORMAL,
	AREA_CORNER,
} touch_area;

typedef enum {
	CORNER_TOPLEFT,		/*When Phone Face you in portrait top left corner */
	CORNER_TOPRIGHT,	/*When Phone Face you in portrait top right corner */
	CORNER_BOTTOMLEFT,	/*When Phone Face you in portrait bottom left corner */
	CORNER_BOTTOMRIGHT,	/*When Phone Face you in portrait 7bottom right corner */
} corner_type;

typedef enum {
	MODE_NORMAL,
	MODE_SLEEP,
	MODE_EDGE,
	MODE_GESTURE,
	MODE_GLOVE,
	MODE_CHARGE,
	MODE_GAME,
	MODE_EARSENSE,
	MODE_PALM_REJECTION,
	MODE_FACE_DETECT,
	MODE_HEADSET,
} work_mode;

typedef enum {
	FW_NORMAL,		/*fw might update, depend on the fw id */
	FW_ABNORMAL,		/*fw abnormal, need update */
} fw_check_state;

typedef enum {
	FW_UPDATE_SUCCESS,
	FW_NO_NEED_UPDATE,
	FW_UPDATE_ERROR,
	FW_UPDATE_FATAL,
} fw_update_state;

typedef enum {
	TP_SUSPEND_EARLY_EVENT,
	TP_SUSPEND_COMPLETE,
	TP_RESUME_EARLY_EVENT,
	TP_RESUME_COMPLETE,
	TP_SPEEDUP_RESUME_COMPLETE,
} suspend_resume_state;

typedef enum IRQ_TRIGGER_REASON {
	IRQ_IGNORE = 0x00,
	IRQ_TOUCH = 0x01,
	IRQ_GESTURE = 0x02,
	IRQ_BTN_KEY = 0x04,
	IRQ_EXCEPTION = 0x08,
	IRQ_FW_CONFIG = 0x10,
	IRQ_FW_HEALTH = 0x20,
	IRQ_FW_AUTO_RESET = 0x40,
	IRQ_FACE_STATE = 0x80,
	IRQ_FINGERPRINT = 0x0100,
} irq_reason;

typedef enum vk_bitmap {
	BIT_reserve = 0x08,
	BIT_BACK = 0x04,
	BIT_HOME = 0x02,
	BIT_MENU = 0x01,
} vk_bitmap;

typedef enum finger_protect_status {
	FINGER_PROTECT_TOUCH_UP,
	FINGER_PROTECT_TOUCH_DOWN,
	FINGER_PROTECT_NOTREADY,
} fp_touch_state;

typedef enum debug_level {
	LEVEL_BASIC,		/*printk basic tp debug info */
	LEVEL_DETAIL,		/*printk tp detail log for stress test */
	LEVEL_DEBUG,		/*printk all tp debug info */
} tp_debug_level;

typedef enum resume_order {
	TP_LCD_RESUME,
	LCD_TP_RESUME,
} tp_resume_order;

typedef enum suspend_order {
	TP_LCD_SUSPEND,
	LCD_TP_SUSPEND,
} tp_suspend_order;

typedef enum lcd_power {
	LCD_POWER_OFF,
	LCD_POWER_ON,
} lcd_power_status;

typedef enum {
	OEM_VERIFIED_BOOT_STATE_UNLOCKED,
	OEM_VERIFIED_BOOT_STATE_LOCKED,
} oem_verified_boot_state;

struct Coordinate {
	int x;
	int y;
};

typedef enum interrupt_mode {
	BANNABLE,
	UNBANNABLE,
	INTERRUPT_MODE_MAX,
} tp_interrupt_mode;

typedef enum switch_mode_type {
	SEQUENCE,
	SINGLE,
} tp_switch_mode;

enum touch_direction {
	VERTICAL_SCREEN,
	LANDSCAPE_SCREEN_90,
	LANDSCAPE_SCREEN_270,
};

struct gesture_info {
	uint32_t gesture_type;
	uint32_t clockwise;
	struct Coordinate Point_start;
	struct Coordinate Point_end;
	struct Coordinate Point_1st;
	struct Coordinate Point_2nd;
	struct Coordinate Point_3rd;
	struct Coordinate Point_4th;
};

struct point_info {
	uint16_t x;
	uint16_t y;
	uint16_t z;
	uint8_t width_major;
	uint8_t touch_major;
	uint8_t status;
	touch_area type;
};

struct corner_info {
	uint8_t id;
	bool flag;
	struct point_info point;
};

struct firmware_headfile {
	const uint8_t *firmware_data;
	size_t firmware_size;
};

struct panel_info {
	char *fw_name;		/*FW name */
	char *test_limit_name;	/*test limit name */
	char *extra;		/*for some ic, may need other information */
	const char *chip_name;	/*chip name the panel is controlled by */
	uint32_t TP_FW;		/*FW Version Read from IC */
	tp_dev tp_type;
	struct firmware_headfile firmware_headfile;	/*firmware headfile for noflash ic */
	struct manufacture_info manufacture_info;	/*touchpanel device info */
};

struct hw_resource {
	//gpio
	int id1_gpio;
	int id2_gpio;
	int id3_gpio;

	int irq_gpio;		/*irq GPIO num */
	int reset_gpio;		/*Reset GPIO */

	int enable2v8_gpio;	/*vdd_2v8 enable GPIO */
	int enable1v8_gpio;	/*vcc_1v8 enable GPIO */

	//TX&&RX Num
	int TX_NUM;
	int RX_NUM;
	int key_TX;		/*the tx num occupied by touchkey */
	int key_RX;		/*the rx num occupied by touchkey */
	int EARSENSE_TX_NUM;	/*for earsense function data reading */
	int EARSENSE_RX_NUM;	/*for earsense function data reading */

	//power
	struct regulator *vdd_2v8;	/*power 2v8 */
	struct regulator *vcc_1v8;	/*power 1v8 */
	uint32_t vdd_volt;	/*avdd specific volt */

	//pinctrl
	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_set_high;
	struct pinctrl_state *pin_set_low;
	struct pinctrl_state *pin_set_nopull;
};

struct edge_limit {
	int limit_area;
	int left_x1;
	int right_x1;
	int left_x2;
	int right_x2;
	int left_x3;
	int right_x3;
	int left_y1;
	int right_y1;
	int left_y2;
	int right_y2;
	int left_y3;
	int right_y3;
	touch_area in_which_area;
};

struct touch_major_limit {
	int width_range;
	int height_range;
};

struct button_map {
	int width_x;		/*width of each key area */
	int height_y;		/*height of each key area */
	struct Coordinate coord_menu;	/*Menu centre coordinates */
	struct Coordinate coord_home;	/*Home centre coordinates */
	struct Coordinate coord_back;	/*Back centre coordinates */
};

struct resolution_info {
	uint32_t max_x;		/*touchpanel width */
	uint32_t max_y;		/*touchpanel height */
	uint32_t LCD_WIDTH;	/*LCD WIDTH        */
	uint32_t LCD_HEIGHT;	/*LCD HEIGHT       */
};

struct esd_information {
	bool esd_running_flag;
	int esd_work_time;
	struct mutex esd_lock;
	struct workqueue_struct *esd_workqueue;
	struct delayed_work esd_check_work;
};

struct freq_hop_info {
	struct workqueue_struct *freq_hop_workqueue;
	struct delayed_work freq_hop_work;
	bool freq_hop_simulating;
	int freq_hop_freq;	/*save frequency-hopping frequency, trigger frequency-hopping every freq_hop_freq seconds */
};

struct spurious_fp_touch {
	bool fp_trigger;	/*thread only turn into runnning state by fingerprint kick proc/touchpanel/finger_protect_trigger */
	bool lcd_resume_ok;
	bool lcd_trigger_fp_check;
	fp_touch_state fp_touch_st;	/*provide result to fingerprint of touch status data */
	struct task_struct *thread;	/*tread use for fingerprint susprious touch check */
};

struct register_info {
	uint8_t reg_length;
	uint16_t reg_addr;
	uint8_t *reg_result;
};

struct black_gesture_test {
	bool gesture_backup;	/*store the gesture enable flag */
	bool flag;		/* indicate do black gesture test or not */
	char *message;		/* failure information if gesture test failed */
};

struct monitor_data {
	unsigned long monitor_down;
	unsigned long monitor_up;
	health_ctl ctl_type;

	int bootup_test;
	int repeat_finger;
	int miss_irq;
	int grip_report;
	int baseline_err;
	int noise_count;
	int shield_palm;
	int shield_edge;
	int shield_metal;
	int shield_water;
	int shield_esd;
	int hard_rst;
	int inst_rst;
	int parity_rst;
	int wd_rst;
	int other_rst;
	int reserve1;
	int reserve2;
	int reserve3;
	int reserve4;
	int reserve5;

	int fw_download_retry;
	int fw_download_fail;

	int eli_size;
	int eli_ver_range;
	int eli_hor_range;
	int *eli_ver_pos;
	int *eli_hor_pos;
};

struct fp_underscreen_info {
	uint8_t touch_state;
	uint8_t area_rate;
	uint16_t x;
	uint16_t y;
};

struct debug_info_proc_operations;
struct earsense_proc_operations;
struct touchpanel_data {
	bool register_is_16bit;	/*register is 16bit */
	bool glove_mode_support;	/*glove_mode support feature */
	bool black_gesture_support;	/*black_gesture support feature */
	bool charger_pump_support;	/*charger_pump support feature */
	bool headset_pump_support;	/*headset_pump support feature */
	bool edge_limit_support;	/*edge_limit support feature */
	bool fw_edge_limit_support;	/*edge_limit by FW support feature */
	bool drlimit_remove_support;	/*remove driver side edge limit control */
	bool esd_handle_support;	/*esd handle support feature */
	bool spurious_fp_support;	/*avoid fingerprint spurious trrigger feature */
	bool gesture_test_support;	/*indicate test black gesture or not */
	bool game_switch_support;	/*indicate game switch support or not */
	bool ear_sense_support;	/*touch porximity function */
	bool smart_gesture_support;	/*feature used to controltouch_major report */
	bool pressure_report_support;	/*feature use to control ABS_MT_PRESSURE report */
	bool face_detect_support;	/*touch porximity function */
	bool fingerprint_underscreen_support;	/*fingerprint underscreen support */
	bool sec_long_low_trigger;	/*samsung s6d7ate ic int feature */
	bool suspend_gesture_cfg;
	bool auto_test_force_pass_support;	/*auto test force pass in early project */
	bool freq_hop_simulate_support;	/*frequency hopping simulate feature */
	bool external_touch_support;	/*external key used for touch point report */

	bool external_touch_status;	/*shows external key status */
	bool i2c_ready;		/*i2c resume status */
	bool is_headset_checked;	/*state of headset or usb */
	bool is_usb_checked;	/*state of charger or usb */
	bool loading_fw;	/*touchpanel FW updating */
	bool is_incell_panel;	/*touchpanel is incell */
	bool is_noflash_ic;	/*noflash ic */
	bool lcd_trigger_load_tp_fw_support;	/*trigger load tp fw by lcd driver after lcd reset */
	bool has_callback;	/*whether have callback method to invoke common */
	bool use_resume_notify;	/*notify speed resume process */
	bool fw_update_app_support;	/*bspFwUpdate is used */
	bool health_monitor_support;	/*bspFwUpdate is used */
	bool irq_trigger_hdl_support;	/*some no-flash ic (such as TD4330) need irq to trigger hdl */
	bool in_test_process;	/*flag whether in test process */
	bool noise_modetest_support;	/*noise mode test is used */
	u8 vk_bitmap;		/*every bit declear one state of key "reserve(keycode)|home(keycode)|menu(keycode)|back(keycode)" */
	vk_type vk_type;	/*virtual_key type */
	delta_state delta_state;

	uint32_t irq_flags;	/*irq setting flag */
	int irq;		/*irq num */

	uint32_t irq_flags_cover;	/*cover irq setting flag */

	int gesture_enable;	/*control state of black gesture */
#if GESTURE_RATE_MODE
	int geature_ignore;
#endif
	int palm_enable;
	int es_enable;
	int fd_enable;
	int fp_enable;
	int touch_count;
	int glove_enable;	/*control state of glove gesture */
	int limit_enable;	/*control state of limit ebale */
	int limit_edge;		/*control state of limit edge */
	int limit_corner;	/*control state of limit corner */
	int default_hor_area;	/*parse the horizontal area configed in dts */
	int limit_valid;	/*show current app in whitlist or not */
	int is_suspended;	/*suspend/resume flow exec flag */
	suspend_resume_state suspend_state;	/*detail suspend/resume state */

	int boot_mode;		/*boot up mode */
	int view_area_touched;	/*view area touched flag */
	int force_update;	/*force update flag */
	int max_num;		/*max muti-touch num supportted */
	int irq_slot;		/*debug use, for print all finger's first touch log */
	int firmware_update_type;	/*firmware_update_type: 0=check firmware version 1=force update; 2=for FAE debug */

	tp_resume_order tp_resume_order;
	tp_suspend_order tp_suspend_order;
	tp_interrupt_mode int_mode;	/*whether interrupt and be disabled */
	tp_switch_mode mode_switch_type;	/*used for switch mode */
	bool skip_reset_in_resume;	/*some incell ic is reset by lcd reset */
	bool skip_suspend_operate;	/*LCD and TP is in one chip,lcd power off in suspend at first,
					   can not operate i2c when tp suspend */
	bool ps_status;		/*save ps status, ps near = 1, ps far = 0 */
	int noise_level;	/*save ps status, ps near = 1, ps far = 0 */

#if defined(TPD_USE_EINT)
	struct hrtimer timer;	/*using polling instead of IRQ */
#endif
#if defined(CONFIG_FB)
	struct notifier_block fb_notif;	/*register to control suspend/resume */
#endif
	struct monitor_data monitor_data;
	struct mutex mutex;	/*mutex for lock i2c related flow */
	struct mutex mutex_earsense;
	struct completion pm_complete;	/*completion for control suspend and resume flow */
	struct completion fw_complete;	/*completion for control fw update */
	struct completion resume_complete;	/*completion for control fw update */
	struct panel_info panel_data;	/*GPIO control(id && pinctrl && tp_type) */
	struct hw_resource hw_res;	/*hw resourc information */
	struct edge_limit edge_limit;	/*edge limit */
	struct button_map button_map;	/*virtual_key button area */
	struct resolution_info resolution_info;	/*resolution of touchpanel && LCD */
	struct gesture_info gesture;	/*gesture related info */
	struct touch_major_limit touch_major_limit;	/*used for control touch major reporting area */
	struct fp_underscreen_info fp_info;	/*tp info used for underscreen fingerprint */

	struct work_struct speed_up_work;	/*using for speedup resume */
	struct workqueue_struct *speedup_resume_wq;	/*using for touchpanel speedup resume wq */
	struct work_struct lcd_trigger_load_tp_fw_work;	/*trigger load tp fw by lcd driver after lcd reset */
	struct workqueue_struct *lcd_trigger_load_tp_fw_wq;	/*trigger laod tp fw by lcd driver after lcd reset */

	struct work_struct read_delta_work;	/*using for read delta */
	struct workqueue_struct *delta_read_wq;

	struct work_struct async_work;
	struct workqueue_struct *async_workqueue;
	struct work_struct fw_update_work;	/*using for fw update */

	struct esd_information esd_info;
	struct freq_hop_info freq_hop_info;
	struct spurious_fp_touch spuri_fp_touch;	/*spurious_finger_support */

	struct device *dev;	/*used for i2c->dev */
	struct i2c_client *client;
	struct spi_device *s_client;
	struct input_dev *input_dev;
	struct input_dev *kpd_input_dev;
	struct input_dev *ps_input_dev;

	struct oppo_touchpanel_operations *ts_ops;	/*call_back function */
	struct proc_dir_entry *prEntry_tp;	/*struct proc_dir_entry of "/proc/touchpanel" */
	struct proc_dir_entry *prEntry_debug_tp;	/*struct proc_dir_entry of "/proc/touchpanel/debug_info" */
	struct debug_info_proc_operations *debug_info_ops;	/*debug info data */
	struct earsense_proc_operations *earsense_ops;
	struct register_info reg_info;	/*debug node for register length */
	struct black_gesture_test gesture_test;	/*gesture test struct */

	void *chip_data;	/*Chip Related data */
	void *private_data;	/*Reserved Private data */
	char *earsense_delta;
};

struct oppo_touchpanel_operations {
	int (*get_chip_info) (void *chip_data);	/*return 0:success;other:failed */
	int (*mode_switch) (void *chip_data, work_mode mode, bool flag);	/*return 0:success;other:failed */
	int (*get_touch_points) (void *chip_data, struct point_info * points, int max_num);	/*return point bit-map */
	int (*get_gesture_info) (void *chip_data, struct gesture_info * gesture);	/*return 0:success;other:failed */
	int (*ftm_process) (void *chip_data);	/*ftm boot mode process */
	int (*get_vendor) (void *chip_data, struct panel_info * panel_data);	/*distingush which panel we use, (TRULY/OFLIM/BIEL/TPK) */
	int (*reset) (void *chip_data);	/*Reset Touchpanel */
	int (*reinit_device) (void *chip_data);
	 fw_check_state(*fw_check) (void *chip_data, struct resolution_info * resolution_info, struct panel_info * panel_data);	/*return < 0 :failed; 0 sucess */
	 fw_update_state(*fw_update) (void *chip_data, const struct firmware * fw, bool force);	/*return 0 normal; return -1:update failed; */
	int (*power_control) (void *chip_data, bool enable);	/*return 0:success;other:abnormal, need to jump out */
	int (*reset_gpio_control) (void *chip_data, bool enable);	/*used for reset gpio */
	 u8(*trigger_reason) (void *chip_data, int gesture_enable, int is_suspended);	/*clear innterrupt reg && detect irq trigger reason */
	 u32(*u32_trigger_reason) (void *chip_data, int gesture_enable,
				   int is_suspended);
	 u8(*get_keycode) (void *chip_data);	/*get touch-key code */
	int (*esd_handle) (void *chip_data);
	int (*fw_handle) (void *chip_data);	/*return 0 normal; return -1:update failed; */
	void (*resume_prepare) (void *chip_data);	/*using for operation before resume flow,
							   eg:incell 3320 need to disable gesture to release inter pins for lcd resume */
	 fp_touch_state(*spurious_fp_check) (void *chip_data);	/*spurious fingerprint check */
	void (*finger_proctect_data_get) (void *chip_data);	/*finger protect data get */
	void (*exit_esd_mode) (void *chip_data);	/*add for s4322 exit esd mode */
	void (*register_info_read) (void *chip_data, uint16_t register_addr, uint8_t * result, uint8_t length);	/*add for read registers */
	void (*write_ps_status) (void *chip_data, int ps_status);	/*when detect iron plate, if ps is near ,enter iron plate mode;if ps is far, can not enter; exit esd mode when ps is far */
	void (*specific_resume_operate) (void *chip_data);	/*some ic need specific opearation in resuming */
	void (*resume_timedout_operate) (void *chip_data);	/*some ic need opearation if resume timed out */
	int (*get_usb_state) (void);	/*get current usb state */
	void (*black_screen_test) (void *chip_data, char *msg);	/*message of black gesture test */
	int (*irq_handle_unlock) (void *chip_info);	/*irq handler without mutex */
	int (*async_work) (void *chip_info);	/*async work */
	int (*get_face_state) (void *chip_info);	/*get face detect state */
	void (*health_report) (void *chip_data, struct monitor_data * mon_data);	/*data logger get */
	void (*bootup_test) (void *chip_data, const struct firmware * fw, struct monitor_data * mon_data, struct hw_resource * hw_res);	/*boot_up test */
	void (*get_gesture_coord) (void *chip_data, uint32_t gesture_type);
	void (*enable_fingerprint) (void *chip_data, uint32_t enable);
	void (*enable_gesture_mask) (void *chip_data, uint32_t enable);
	void (*set_touch_direction) (void *chip_data, uint8_t dir);
	 uint8_t(*get_touch_direction) (void *chip_data);
	void (*screenon_fingerprint_info) (void *chip_data, struct fp_underscreen_info * fp_tpinfo);	/*get gesture info of fingerprint underscreen when screen on */
	void (*freq_hop_trigger) (void *chip_data);	/*trigger frequency-hopping */
	void (*set_noise_modetest) (void *chip_data, bool enable);
	 uint8_t(*get_noise_modetest) (void *chip_data);
};

struct debug_info_proc_operations {
	void (*limit_read) (struct seq_file * s, struct touchpanel_data * ts);
	void (*delta_read) (struct seq_file * s, void *chip_data);
	void (*self_delta_read) (struct seq_file * s, void *chip_data);
	void (*self_raw_read) (struct seq_file * s, void *chip_data);
	void (*baseline_read) (struct seq_file * s, void *chip_data);
	void (*baseline_blackscreen_read) (struct seq_file * s,
					   void *chip_data);
	void (*main_register_read) (struct seq_file * s, void *chip_data);
	void (*reserve_read) (struct seq_file * s, void *chip_data);
	void (*abs_doze_read) (struct seq_file * s, void *chip_data);
	void (*RT251) (struct seq_file * s, void *chip_data);
	void (*RT76) (struct seq_file * s, void *chip_data);
	void (*RT254) (struct seq_file * s, void *chip_data);
	void (*DRT) (struct seq_file * s, void *chip_data);
	void (*gesture_rate) (struct seq_file * s, u16 * coord_arg,
			      void *chip_data);
};

struct invoke_method {
	void (*invoke_common) (void);
	void (*async_work) (void);
};

struct earsense_proc_operations {
	void (*rawdata_read) (void *chip_data, char *earsense_baseline,
			      int read_length);
	void (*delta_read) (void *chip_data, char *earsense_delta,
			    int read_length);
	void (*self_data_read) (void *chip_data, char *earsense_self_data,
				int read_length);
};

/*********PART3:function or variables for other files**********************/
extern unsigned int tp_debug;	/*using for print debug log */

struct touchpanel_data *common_touch_data_alloc(void);

int common_touch_data_free(struct touchpanel_data *pdata);
int register_common_touch_device(struct touchpanel_data *pdata);

void tp_i2c_suspend(struct touchpanel_data *ts);
void tp_i2c_resume(struct touchpanel_data *ts);

int tp_powercontrol_1v8(struct hw_resource *hw_res, bool on);
int tp_powercontrol_2v8(struct hw_resource *hw_res, bool on);

void operate_mode_switch(struct touchpanel_data *ts);
void input_report_key_oppo(struct input_dev *dev, unsigned int code, int value);
void esd_handle_switch(struct esd_information *esd_info, bool on);
void clear_view_touchdown_flag(void);
void tp_touch_btnkey_release(void);
extern int tp_util_get_vendor(struct hw_resource *hw_res,
			      struct panel_info *panel_data);
extern bool tp_judge_ic_match(char *tp_ic_name);
__attribute__ ((weak))
int request_firmware_select(const struct firmware **firmware_p,
			    const char *name, struct device *device)
{
	return 1;
}

__attribute__ ((weak))
int opticalfp_irq_handler(struct fp_underscreen_info *fp_tpinfo)
{
	return 0;
}

bool is_oem_unlocked(void);
int __init get_oem_verified_boot_state(void);

#endif
