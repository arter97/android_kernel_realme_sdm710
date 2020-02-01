/************************************************************************************
** File:  \\192.168.144.3\Linux_Share\12015\ics2\development\mediatek\custom\oppo77_12015\kernel\battery\battery
** VENDOR_EDIT
** Copyright (C), 2008-2012, OPPO Mobile Comm Corp., Ltd
**
** Description:
**          for dc-dc sn111008 charg
**
** Version: 1.0
** Date created: 21:03:46, 05/04/2012
** Author: Fanhong.Kong@ProDrv.CHG
**
** --------------------------- Revision History: ------------------------------------------------------------
* <version>           <date>                <author>                                <desc>
* Revision 1.0        2015-06-22        Fanhong.Kong@ProDrv.CHG            Created for new architecture
* Revision 1.1        2016-05-09        wenbin.liu@SW.Bsp.Driver           Modify for code review
************************************************************************************************************/

#define VOOC_MCU_STM8S

#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#ifdef CONFIG_OPPO_CHARGER_MTK
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

#include <linux/xlog.h>

//#include <upmu_common.h>
//#include <mt-plat/mtk_gpio.h>
#include <linux/dma-mapping.h>

//#include <mt-plat/battery_meter.h>
#include <linux/module.h>
#include <soc/oppo/device_info.h>

#else
#include <linux/i2c.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <soc/oppo/device_info.h>
#include "../gauge_ic/oppo_bq27541.h"
#include <soc/oppo/device_info.h>
#endif
#include "oppo_vooc_fw.h"
#ifdef VENDOR_EDIT
//OuYangBaiLi@BSP.CHG.Basic 2018/12/21 modify for vooc charge and normal charge compatibility
#include "../gauge_ic/oppo_bq27541.h"
extern int bq27541_i2c_check(void);
#endif /* VENDOR_EDIT */

#if defined(CONFIG_OPPO_CHARGER_MTK6763) || defined(CONFIG_OPPO_CHARGER_MTK6771)
#define I2C_MASK_FLAG	(0x00ff)
#define I2C_ENEXT_FLAG (0x0200)
#define I2C_DMA_FLAG	(0xdead2000)
#endif
#define ERASE_COUNT                   192        /*0x8800-0x9FFF*/
#define READ_COUNT                    191

#define BYTE_OFFSET                                  2
#define BYTES_TO_WRITE                               16
#define FW_CHECK_FAIL                                0
#define FW_CHECK_SUCCESS                             1
static struct oppo_vooc_chip *the_chip = NULL;

#ifdef CONFIG_OPPO_CHARGER_MTK

#define GTP_SUPPORT_I2C_DMA   			0

#define I2C_MASTER_CLOCK                              300
#define GTP_DMA_MAX_TRANSACTION_LENGTH  255           /* for DMA mode */

DEFINE_MUTEX(dma_wr_access_stm);

#if GTP_SUPPORT_I2C_DMA
static int i2c_dma_write(struct i2c_client *client, u8 addr, s32 len, u8 const *txbuf);
static int i2c_dma_read(struct i2c_client *client, u8 addr, s32 len, u8 *txbuf);
static u8 *gpDMABuf_va = NULL;
static dma_addr_t gpDMABuf_pa = 0;
#endif

#if GTP_SUPPORT_I2C_DMA
static int i2c_dma_read(struct i2c_client *client, u8 addr, s32 len, u8 *rxbuf)
{
        int ret;
        s32 retry = 0;
        u8 buffer[1];

        struct i2c_msg msg[2] =
        {
                {
                        .addr = (client->addr & I2C_MASK_FLAG),
                        .flags = 0,
                        .buf = buffer,
                        .len = 1,
                        .timing = I2C_MASTER_CLOCK
                },
                {
                        .addr = (client->addr & I2C_MASK_FLAG),
                        .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
                        .flags = I2C_M_RD,
                        .buf = (__u8 *)gpDMABuf_pa,   /*modified by PengNan*/
                        .len = len,
                        .timing = I2C_MASTER_CLOCK
                },
        };
        mutex_lock(&dma_wr_access_stm);
        /*buffer[0] = (addr >> 8) & 0xFF;*/
        buffer[0] = addr & 0xFF;
        if (rxbuf == NULL) {
                mutex_unlock(&dma_wr_access_stm);
                return -1;
        }
        /*chg_err("vooc dma i2c read: 0x%x, %d bytes(s)\n", addr, len);*/
        for (retry = 0; retry < 5; ++retry) {
                ret = i2c_transfer(client->adapter, &msg[0], 2);
                if (ret < 0) {
                        continue;
                }
                memcpy(rxbuf, gpDMABuf_va, len);
                mutex_unlock(&dma_wr_access_stm);
                return 0;
        }
        /*chg_err(" Error: 0x%04X, %d byte(s), err-code: %d\n", addr, len, ret);*/
        mutex_unlock(&dma_wr_access_stm);
        return ret;
}

static int i2c_dma_write(struct i2c_client *client, u8 addr, s32 len, u8 const *txbuf)
{
    int ret = 0;
        s32 retry = 0;
        u8 *wr_buf = gpDMABuf_va;
        struct i2c_msg msg =
        {
                .addr = (client->addr & I2C_MASK_FLAG),
                .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG),
                .flags = 0,
                .buf = (__u8 *)gpDMABuf_pa,        /*modified by PengNan*/
                .len = 1 + len,
                .timing = I2C_MASTER_CLOCK
        };
        mutex_lock(&dma_wr_access_stm);
        wr_buf[0] = (u8)(addr & 0xFF);
        if (txbuf == NULL) {
                mutex_unlock(&dma_wr_access_stm);
                return -1;
        }
        memcpy(wr_buf+1, txbuf, len);
        for (retry = 0; retry < 5; ++retry) {
                ret = i2c_transfer(client->adapter, &msg, 1);
                if (ret < 0) {
                        continue;
                }
                mutex_unlock(&dma_wr_access_stm);
                return 0;
        }
        /*chg_err(" Error: 0x%04X, %d byte(s), err-code: %d\n", addr, len, ret);*/
        mutex_unlock(&dma_wr_access_stm);
        return ret;
}
#endif /*GTP_SUPPORT_I2C_DMA*/
#endif

static int oppo_vooc_i2c_read(struct i2c_client *client, u8 addr, s32 len, u8 *rxbuf)
{
#ifdef CONFIG_OPPO_CHARGER_MTK
#if GTP_SUPPORT_I2C_DMA
        return i2c_dma_read(client, addr, len, rxbuf);
#else
        return i2c_smbus_read_i2c_block_data(client, addr, len, rxbuf);
#endif
#else
	return i2c_smbus_read_i2c_block_data(client, addr, len, rxbuf);
#endif
}

static int oppo_vooc_i2c_write(struct i2c_client *client, u8 addr, s32 len, u8 const *txbuf)
{
#ifdef CONFIG_OPPO_CHARGER_MTK
#if GTP_SUPPORT_I2C_DMA
        return i2c_dma_write(client, addr, len, txbuf);
#else
        return i2c_smbus_write_i2c_block_data(client, addr, len, txbuf);
#endif
#else
	return i2c_smbus_write_i2c_block_data(client, addr, len, txbuf);
#endif

}

static bool stm8s_fw_check_frontline(struct oppo_vooc_chip *chip)
{
        unsigned char addr_buf[2] = {0x88, 0x00};
        unsigned char data_buf[32] = {0x0};
        int rc, i, j, addr;
        int fw_line = 0;

        if (!chip->firmware_data) {
                chg_err("Stm8s_fw_data Null, Return\n");
                return FW_CHECK_FAIL;
        }
        /*first:set address*/
        /*rc = i2c_smbus_write_i2c_block_data(chip->client, 0x01, 2, &addr_buf[0]);*/
        rc = oppo_vooc_i2c_write(chip->client, 0x01, 2, &addr_buf[0]);
        if (rc < 0) {
                chg_err(" i2c_write 0x01 error\n");
                goto i2c_err;
        }
        msleep(10);
        for (i = 0;i < READ_COUNT;i++) {        /*1508:448, 1503:192*/
                oppo_vooc_i2c_read(chip->client, 0x03, 16, &data_buf[0]);
                msleep(2);
                oppo_vooc_i2c_read(chip->client, 0x03, 16, &data_buf[16]);
                addr = 0x8800 + i * 32;

                /*compare recv_buf with Stm8s_firmware_data[] begin*/
                /*#ifdef CONFIG_OPPO_CHARGER_MTK*/
                /*#if 1 check  all bytes in every line*/
                if (addr == ((chip->firmware_data[fw_line * 34 + 1] << 8) | chip->firmware_data[fw_line * 34])) {
                        for (j = 0;j < 32;j++) {
                                if (data_buf[j] != chip->firmware_data[fw_line * 34 + 2 + j]) {
                                        chg_err(" fail, data_buf[%d]:0x%x != Stm8s_firmware_data[%d]:0x%x\n",
                                                j, data_buf[j], (fw_line * 34 + 2 + j), chip->firmware_data[fw_line * 34 + 2 + j]);
                                        chg_err("addr = 0x%x, %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x "
                                                "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
                                                addr, data_buf[0], data_buf[1], data_buf[2], data_buf[3], data_buf[4], data_buf[5], data_buf[6], data_buf[7],
                                                data_buf[8], data_buf[9], data_buf[10], data_buf[11], data_buf[12], data_buf[13], data_buf[14],
                                                data_buf[15], data_buf[16], data_buf[17], data_buf[18], data_buf[19], data_buf[20], data_buf[21], data_buf[22],
                                                data_buf[23], data_buf[24], data_buf[25], data_buf[26], data_buf[27], data_buf[28], data_buf[29], data_buf[30],
                                                data_buf[31]);
                                        return FW_CHECK_FAIL;
                                }
                        }
                        fw_line++;
                } else {
                /*chg_err(" addr dismatch, addr:0x%x, stm_data:0x%x\n" ,*/
                                /*addr, (Stm8s_firmware_data[fw_line * 34 + 1] << 8) | Stm8s_firmware_data[fw_line * 34]);*/
                }
/*#else*/
/***only check the first byte in every line  */
/*                j = 0;
                if (addr == ((Stm8s_firmware_data[fw_line * 34 + 1] << 8) | Stm8s_firmware_data[fw_line * 34])){
                                if (data_buf[0] != Stm8s_firmware_data[fw_line * 34 + 2]){
                                        chg_err("  fail, data_buf[%d]:0x%x != Stm8s_firmware_data[%d]:0x%x\n" ,
                                                j, data_buf[j], (fw_line * 34 + 2 + j), Stm8s_firmware_data[fw_line * 34 + 2 + j]);
                                        chg_err("  addr = 0x%x, %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n" , addr,
                                                data_buf[0], data_buf[1], data_buf[2], data_buf[3], data_buf[4], data_buf[5], data_buf[6], data_buf[7],
                                                data_buf[8], data_buf[9], data_buf[10], data_buf[11], data_buf[12], data_buf[13], data_buf[14],
                                                data_buf[15], data_buf[16], data_buf[17], data_buf[18], data_buf[19], data_buf[20], data_buf[21], data_buf[22],
                                                data_buf[23], data_buf[24], data_buf[25], data_buf[26], data_buf[27], data_buf[28], data_buf[29], data_buf[30],
                                                data_buf[31]);
                                        return FW_CHECK_FAIL;
                                }
                        fw_line++;
                } else {
                chg_err(" addr dismatch, addr:0x%x, stm_data:0x%x\n",
                                addr, (Stm8s_firmware_data[fw_line * 34 + 1] << 8) | Stm8s_firmware_data[fw_line * 34]);
                }
*/
/*#endif*/
        /*compare recv_buf with Stm8s_firmware_data[] end*/
        }
        /*pr_info("  success\n"  );*/
        return FW_CHECK_SUCCESS;

i2c_err:
        chg_err(" failed\n");
        return FW_CHECK_FAIL;
}

static bool stm8s_fw_check_lastline(struct oppo_vooc_chip *chip)
{
        unsigned char addr_buf[2] = {0x9F, 0xE0};
        unsigned char data_buf[32] = {0x0};
        int i = 0, rc = 0;

        if (!chip->firmware_data) {
                chg_err("Stm8s_fw_data Null, Return\n");
                return FW_CHECK_FAIL;
        }

        /*first:set address*/
        rc = oppo_vooc_i2c_write(chip->client, 0x01, 2, &addr_buf[0]);
        if (rc < 0) {
                chg_err(" i2c_write 0x01 error\n");
                return FW_CHECK_FAIL;
        }
        msleep(2);
        oppo_vooc_i2c_read(chip->client, 0x03, 16, &data_buf[0]);
        msleep(2);
        oppo_vooc_i2c_read(chip->client, 0x03, 16, &data_buf[16]);
		chip->fw_mcu_version = data_buf[28];
/*#ifdef CONFIG_OPPO_CHARGER_MTK*/
/*#if 1   check  all bytes in last line*/
        for (i = 0;i < 32;i++) {
                if (data_buf[i] != chip->firmware_data[chip->fw_data_count - 32 + i]) {
                        chg_err("  fail, data_buf[%d]:0x%x != Stm8s_firmware_data[]:0x%x\n" ,
                                i, data_buf[i], chip->firmware_data[chip->fw_data_count - 32 + i]);
                        chg_err(" data %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n" ,
                                data_buf[0],  data_buf[1],  data_buf[2],  data_buf[3],  data_buf[4],  data_buf[5],  data_buf[6], data_buf[7],
                                data_buf[8],  data_buf[9],  data_buf[10], data_buf[11], data_buf[12], data_buf[13], data_buf[14],data_buf[15],
					data_buf[16], data_buf[17], data_buf[18], data_buf[19], data_buf[20], data_buf[21], data_buf[22],data_buf[23],
                                data_buf[24], data_buf[25], data_buf[26], data_buf[27], data_buf[28], data_buf[29], data_buf[30],data_buf[31]);

                        return FW_CHECK_FAIL;
                }
        }
/*#else*/
/**only check the first byte in last line  */
/*
        if (data_buf[0] != Stm8s_firmware_data[sizeof(Stm8s_firmware_data) - 32]) {
                chg_err(" fail, data_buf[0]:0x%x != Stm8s_firmware_data[]:0x%x\n" ,
                        data_buf[0], Stm8s_firmware_data[sizeof(Stm8s_firmware_data) - 32]);
                chg_err(" data %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n" ,
                        data_buf[0], data_buf[1], data_buf[2], data_buf[3], data_buf[4], data_buf[5], data_buf[6], data_buf[7],
                        data_buf[8], data_buf[9], data_buf[10], data_buf[11], data_buf[12], data_buf[13], data_buf[14],
                        data_buf[15], data_buf[16], data_buf[17], data_buf[18], data_buf[19], data_buf[20], data_buf[21], data_buf[22],
                        data_buf[23], data_buf[24], data_buf[25], data_buf[26], data_buf[27], data_buf[28], data_buf[29], data_buf[30],
                        data_buf[31]);
                return FW_CHECK_FAIL;
        }
**/
/*#endif*/
/*
		if(chip->vooc_fw_update_newmethod) {
			chip->fw_data_version = data_buf[28];
			chg_debug("fw_version:0x%x\n",chip->fw_data_version);
		}
*/
        return FW_CHECK_SUCCESS;
}

static int stm8s_fw_write(struct oppo_vooc_chip *chip, const unsigned char *data_buf, unsigned int offset, unsigned int length)
{
        unsigned int count = 0;
        unsigned char zero_buf[1] = {0};
        unsigned char temp_buf[1] = {0};
        unsigned char addr_buf[2] = {0x88, 0x00};
        int rc;

        if (!chip->firmware_data) {
                chg_err("Stm8s_fw_data Null, Return\n");
                return -1;
        }

        count = offset;
        /*write data begin*/
        while (count < (offset + length)) {
                addr_buf[0] = data_buf[count + 1];
                addr_buf[1] = data_buf[count];
                /*chg_err(" write data addr_buf[0]:0x%x, addr_buf[1]:0x%x\n", addr_buf[0], addr_buf[1]);*/
                        rc = oppo_vooc_i2c_write(chip->client, 0x01, 2, &addr_buf[0]);
                if (rc < 0) {
                        chg_err(" i2c_write 0x01 error\n");
                        return -1;
                }

                /*swap low byte and high byte begin*/

                /*swap low byte and high byte end*/
                /*write 16 bytes data to stm8s*/
                oppo_vooc_i2c_write(chip->client, 0x02, BYTES_TO_WRITE, &data_buf[count+BYTE_OFFSET]);
                oppo_vooc_i2c_write(chip->client, 0x05, 1, &zero_buf[0]);
                oppo_vooc_i2c_read(chip->client, 0x05, 1, &temp_buf[0]);
                /*chg_err("lfc read 0x05, temp_buf[0]:0x%x\n", temp_buf[0]);*/

                /*write 16 bytes data to stm8s again*/
                oppo_vooc_i2c_write(chip->client, 0x02, BYTES_TO_WRITE, &data_buf[count+BYTE_OFFSET+BYTES_TO_WRITE]);
                oppo_vooc_i2c_write(chip->client, 0x05, 1, &zero_buf[0]);
                oppo_vooc_i2c_read(chip->client, 0x05, 1, &temp_buf[0]);
                /*chg_err("lfc read again 0x05, temp_buf[0]:0x%x\n", temp_buf[0]);*/

                count = count + BYTE_OFFSET + 2 * BYTES_TO_WRITE;

                msleep(2);
                /*chg_err("  count:%d, offset:%d, length:%d\n" , count, offset, length);*/
                if (count > (offset + length - 1)) {
                        break;
                }
        }
        return 0;
}

static int stm8s_fw_update(struct oppo_vooc_chip *chip)
{
        unsigned char zero_buf[1] = {0};
        unsigned char addr_buf[2] = {0x88, 0x00};
        unsigned char temp_buf[1] = {0};
        int i, rc = 0;
        unsigned int addr = 0x8800;
        int download_again = 0;

        if (!chip->firmware_data) {
                chg_err("Stm8s_fw_data Null, Return\n");
                return -1;
        } else {
                chg_debug(" start, erase data ing.......\n");
        }

update_fw:
        /*erase address 0x200-0x7FF*/
        for (i = 0;i < ERASE_COUNT;i++) {
                /*first:set address*/
                /*rc = i2c_smbus_write_i2c_block_data(chip->client, 0x01, 2, &addr_buf[0]);*/
                rc = oppo_vooc_i2c_write(chip->client, 0x01, 2, &addr_buf[0]);
                if (rc < 0) {
                        chg_err(" stm8s_update_fw, i2c_write 0x01 error\n");
                        goto update_fw_err;
                }

                /*erase data:0x10 words once*/
                oppo_vooc_i2c_write(chip->client, 0x04, 1, &zero_buf[0]);
                msleep(1);
                oppo_vooc_i2c_read(chip->client, 0x04, 1, &temp_buf[0]);
                /*chg_err("lfc read 0x04, temp_buf[0]:0x%x\n", temp_buf[0]);*/

                /*erase data:0x10 words once*/
                addr = addr + 0x10;
                addr_buf[0] = addr >> 8;
                addr_buf[1] = addr & 0xFF;
                /*chg_err("lfc addr_buf[0]:0x%x, addr_buf[1]:0x%x\n", addr_buf[0], addr_buf[1]);*/
        }
        msleep(10);

        stm8s_fw_write(chip, chip->firmware_data, 0, chip->fw_data_count - 34);

        /*fw check begin:read data from stm1503/1508, and compare it with Stm8s_firmware_data[]*/
        rc = stm8s_fw_check_frontline(chip);
        if (rc == FW_CHECK_FAIL) {
                download_again++;
                if (download_again > 3) {
                        goto update_fw_err;
                }
                chip->mcu_update_ing = false;
                opchg_set_reset_active(chip);
                chip->mcu_update_ing = true;
                msleep(1000);
                chg_err(" fw check fail, download fw again\n");
                goto update_fw;
        }
        /*fw check end*/

        /*write 0x7F0~0x7FF(0x7FF = 0x3455)*/
        rc = stm8s_fw_write(chip, chip->firmware_data, chip->fw_data_count - 34, 34);
        if (rc < 0) {
                goto update_fw_err;
        }
        /*write 0x7F0~0x7FF end*/

        msleep(2);
        /*jump to app code begin*/
        oppo_vooc_i2c_write(chip->client, 0x06, 1, &zero_buf[0]);
        oppo_vooc_i2c_read(chip->client, 0x06, 1, &temp_buf[0]);
        /*jump to app code end*/
        chip->have_updated = 1;
        chip->mcu_update_ing = false;
        /*pull down GPIO96 to power off MCU1503/1508*/
        chip->vooc_fw_check = true;
		chip->fw_mcu_version = chip->fw_data_version;
        chg_debug(" success\n");
        return 0;

update_fw_err:
        chip->mcu_update_ing = false;
        chg_err("  fail\n");
        return 1;
}
static int stm8s_get_fw_verion_from_ic(struct oppo_vooc_chip *chip)
{
	unsigned char addr_buf[2] = {0x9F,0xFC};
	unsigned char data_buf[4] = {0};
	int rc = 0;
	int update_result = 0;

//	if (!chip->firmware_data) {
//		chg_err("Stm8s_fw_data Null, Return\n");
//		return FW_CHECK_FAIL;
//	}
	if(oppo_is_power_off_charging(chip) == true || oppo_is_charger_reboot(chip)== true) {
		chip->mcu_update_ing = true;
		update_result = stm8s_fw_update(chip);
		chip->mcu_update_ing = false;
		if(update_result) {
			msleep(30);
			opchg_set_clock_sleep(chip);
			opchg_set_reset_active(chip);
		}
	} else {
		opchg_set_clock_active(chip);
		chip->mcu_boot_by_gpio = true;
		msleep(10);
		opchg_set_reset_active(chip);
		chip->mcu_update_ing = true;
		msleep(2500);
		chip->mcu_boot_by_gpio = false;
		opchg_set_clock_sleep(chip);

	//first:set address
	rc = oppo_vooc_i2c_write(chip->client,0x01,2,&addr_buf[0]);
	if (rc < 0) {
		chg_err(" i2c_write 0x01 error\n" );
		return FW_CHECK_FAIL;
	}
	msleep(2);
	oppo_vooc_i2c_read(chip->client,0x03,4,data_buf);
	//	strcpy(ver,&data_buf[0]);
	chg_err("data:%x %x %x %x, fw_ver:%x\n",data_buf[0], data_buf[1], data_buf[2], data_buf[3],data_buf[0]);

		chip->mcu_update_ing = false;
		msleep(5);
		opchg_set_reset_active(chip);
	}


	return data_buf[0];
}
static void stm8s_fw_check_then_recover(struct oppo_vooc_chip *chip)
{
        int update_result = 0;

        if (!chip->firmware_data) {
                chg_err("Stm8s_fw_data Null, Return\n");
                return;
        } else {
                chg_debug(" begin\n");
        }

        if (oppo_is_power_off_charging(chip) == true || oppo_is_charger_reboot(chip)== true) {
                chip->mcu_update_ing = true;
                update_result = stm8s_fw_update(chip);
                chip->mcu_update_ing = false;
                if (update_result) {
                        msleep(30);
                        opchg_set_clock_sleep(chip);
                        opchg_set_reset_active(chip);
                }
        } else {
                opchg_set_clock_active(chip);
                chip->mcu_boot_by_gpio = true;
                msleep(10);
                opchg_set_reset_active(chip);
                chip->mcu_update_ing = true;
                msleep(2500);
                chip->mcu_boot_by_gpio = false;
                opchg_set_clock_sleep(chip);
                if (stm8s_fw_check_frontline(chip) == FW_CHECK_FAIL || stm8s_fw_check_lastline(chip) == FW_CHECK_FAIL) {
                        stm8s_fw_update(chip);
                } else {
                        chip->vooc_fw_check = true;
                        chg_debug("  fw check ok\n");
                }
                chip->mcu_update_ing = false;
                msleep(5);
                opchg_set_reset_active(chip);
        }
}

struct oppo_vooc_operations oppo_stm8s_ops = {
                .fw_update = stm8s_fw_update,
                .fw_check_then_recover = stm8s_fw_check_then_recover,
                .set_switch_mode = opchg_set_switch_mode,
                .eint_regist = oppo_vooc_eint_register,
                .eint_unregist = oppo_vooc_eint_unregister,
                .set_data_active = opchg_set_data_active,
                .set_data_sleep = opchg_set_data_sleep,
                .set_clock_active = opchg_set_clock_active,
                .set_clock_sleep = opchg_set_clock_sleep,
                .get_gpio_ap_data = opchg_get_gpio_ap_data,
                .read_ap_data = opchg_read_ap_data,
                .reply_mcu_data = opchg_reply_mcu_data,
                .reset_fastchg_after_usbout = reset_fastchg_after_usbout,
                .switch_fast_chg = switch_fast_chg,
                .reset_mcu = opchg_set_reset_active,
                .is_power_off_charging = oppo_is_power_off_charging,
                .get_reset_gpio_val = oppo_vooc_get_reset_gpio_val,
                .get_switch_gpio_val = oppo_vooc_get_switch_gpio_val,
                .get_ap_clk_gpio_val = oppo_vooc_get_ap_clk_gpio_val,
				.get_fw_version		= stm8s_get_fw_verion_from_ic,
				.get_clk_gpio_num = opchg_get_clk_gpio_num,
				.get_data_gpio_num = opchg_get_data_gpio_num,
};

static void register_vooc_devinfo(void)
{
        int ret = 0;
        char *version;
        char *manufacture;

        version = "stm8s";
        manufacture = "ST";

        ret = register_device_proc("vooc", version, manufacture);
        if (ret) {
                chg_err(" fail\n");
        }
}

static void stm8s_shutdown(struct i2c_client *client)
{
        if (!the_chip) {
                return;
        }
        opchg_set_switch_mode(the_chip, NORMAL_CHARGER_MODE);
        msleep(10);
        if (oppo_vooc_get_fastchg_started() == true) {
                opchg_set_clock_sleep(the_chip);
                msleep(10);
                opchg_set_reset_active(the_chip);
        }
        msleep(80);
        return;
}

static ssize_t vooc_fw_check_read(struct file *filp, char __user *buff, size_t count, loff_t *off)
{
        char page[256] = {0};
        char read_data[32] = {0};
        int len = 0;

        if (the_chip && the_chip->vooc_fw_check == true) {
                read_data[0] = '1';
        } else {
                read_data[0] = '0';
        }
        len = sprintf(page, "%s", read_data);
        if (len > *off) {
                len -= *off;
        } else {
                len = 0;
        }
        if (copy_to_user(buff, page, (len < count ? len : count))) {
                return -EFAULT;
        }
        *off += len < count ? len : count;
        return (len < count ? len : count);
}

static const struct file_operations vooc_fw_check_proc_fops = {
        .read = vooc_fw_check_read,
        .llseek = noop_llseek,
};

static int init_proc_vooc_fw_check(void)
{
        struct proc_dir_entry *p = NULL;

        p = proc_create("vooc_fw_check", 0444, NULL, &vooc_fw_check_proc_fops);
        if (!p) {
                chg_err("proc_create vooc_fw_check_proc_fops fail!\n");
        }
        return 0;
}

static int stm8s_driver_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        struct oppo_vooc_chip *chip;
		#ifdef VENDOR_EDIT
		//OuYangBaiLi@BSP.CHG.Basic 2018/12/21 modify for vooc charge and normal charge compatibility
		int ret=0;
		unsigned char pcb_version = get_PCB_Version();
		//pr_err("%s:get_PCB_Version, pcb_version:%d\n", __func__,pcb_version);
		ret = bq27541_i2c_check();
		if(ret&&(pcb_version < 4)){
			chg_err("[Error] bq27541 i2c read failed!\n");
			return -ENOMEM;
		}
		//chg_err("bq27541_i2c_check pass!\n");
		#endif /* VENDOR_EDIT */
        chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
        if (!chip) {
                dev_err(&client->dev, "Couldn't allocate memory\n");
                return -ENOMEM;
        }

        chip->client = client;
        chip->dev = &client->dev;
        i2c_set_clientdata(client, chip);

#ifdef CONFIG_OPPO_CHARGER_MTK
#if GTP_SUPPORT_I2C_DMA
        client->dev.coherent_dma_mask = DMA_BIT_MASK(32);
        gpDMABuf_va = (u8 *)dma_alloc_coherent(&client->dev, GTP_DMA_MAX_TRANSACTION_LENGTH, &gpDMABuf_pa, GFP_KERNEL);
        if (!gpDMABuf_va) {
                chg_err("[Error] Allocate DMA I2C Buffer failed!\n");
        } else {
                chg_debug(" ppp dma_alloc_coherent success\n");
        }
        memset(gpDMABuf_va, 0, GTP_DMA_MAX_TRANSACTION_LENGTH);
#endif
#endif
       if(get_vooc_mcu_type(chip) != OPPO_VOOC_MCU_HWID_STM8S){
               chg_err("It is not stm8s\n");
               return -ENOMEM;
       }

        chip->pcb_version = g_hw_version;
        chip->vooc_fw_check = false;
		mutex_init(&chip->pinctrl_mutex);

/* wenbin.liu@BSP.CHG.Vooc, 2016/10/20
**    Modify for vooc batt 4.40   */
        oppo_vooc_fw_type_dt(chip);
        if (chip->batt_type_4400mv) {
                chip->firmware_data = Stm8s_firmware_data_4400mv;
                chip->fw_data_count = sizeof(Stm8s_firmware_data_4400mv);
                chip->fw_data_version = Stm8s_firmware_data_4400mv[chip->fw_data_count - 4];
        } else {
                chip->firmware_data = Stm8s_firmware_data_4350mv;
                chip->fw_data_count = sizeof(Stm8s_firmware_data_4350mv);
                chip->fw_data_version = Stm8s_firmware_data_4350mv[chip->fw_data_count - 4];
        }
        if (chip->vooc_fw_type == VOOC_FW_TYPE_STM8S_4400_AVOID_FAKE_ADAPTER) {
                chip->firmware_data = Stm8s_fw_data_4400_avoid_fake_adapter;
                chip->fw_data_count = sizeof(Stm8s_fw_data_4400_avoid_fake_adapter);
                chip->fw_data_version = Stm8s_fw_data_4400_avoid_fake_adapter[chip->fw_data_count - 4];
        } else if (chip->vooc_fw_type == VOOC_FW_TYPE_STM8S_4400_AVOID_FG_I2C_ERR) {
                chip->firmware_data = Stm8s_fw_data_4400_avoid_fg_i2c_err;
                chip->fw_data_count = sizeof(Stm8s_fw_data_4400_avoid_fg_i2c_err);
                chip->fw_data_version = Stm8s_fw_data_4400_avoid_fg_i2c_err[chip->fw_data_count - 4];
        } else if (chip->vooc_fw_type == VOOC_FW_TYPE_STM8S_4400_AVOID_OVER_TEMP) {
                chip->firmware_data = Stm8s_fw_data_4400_avoid_over_temp;
                chip->fw_data_count = sizeof(Stm8s_fw_data_4400_avoid_over_temp);
                chip->fw_data_version = Stm8s_fw_data_4400_avoid_over_temp[chip->fw_data_count - 4];
        } else if (chip->vooc_fw_type == VOOC_FW_TYPE_STM8S_4400_AVOID_OVER_TEMP_NTC61C) {
                chip->firmware_data = Stm8s_fw_data_4400_avoid_over_temp_ntc61c;
                chip->fw_data_count = sizeof(Stm8s_fw_data_4400_avoid_over_temp_ntc61c);
                chip->fw_data_version = Stm8s_fw_data_4400_avoid_over_temp_ntc61c[chip->fw_data_count - 4];
        } else if (chip->vooc_fw_type == VOOC_FW_TYPE_STM8S_4400_VOOC_FFC_09C) {
                chip->firmware_data = Stm8s_fw_data_4400_vooc_ffc_09c;
                chip->fw_data_count = sizeof(Stm8s_fw_data_4400_vooc_ffc_09c);
                chip->fw_data_version = Stm8s_fw_data_4400_vooc_ffc_09c[chip->fw_data_count - 4];
        } else if (chip->vooc_fw_type == VOOC_FW_TYPE_STM8S_4400_VOOC_FFC_15C) {
                chip->firmware_data = Stm8s_fw_data_4400_vooc_ffc_15c;
                chip->fw_data_count = sizeof(Stm8s_fw_data_4400_vooc_ffc_15c);
                chip->fw_data_version = Stm8s_fw_data_4400_vooc_ffc_15c[chip->fw_data_count - 4];
        } else if (chip->vooc_fw_type == VOOC_FW_TYPE_STM8S_4400_VOOC_FFC_15C_FV4450) {
                chip->firmware_data = Stm8s_fw_data_4400_vooc_ffc_15c_fv4450;
                chip->fw_data_count = sizeof(Stm8s_fw_data_4400_vooc_ffc_15c_fv4450);
                chip->fw_data_version = Stm8s_fw_data_4400_vooc_ffc_15c_fv4450[chip->fw_data_count - 4];
        } else if (chip->vooc_fw_type == VOOC_FW_TYPE_STM8S_4400_SVOOC_3500MA) {
			chip->firmware_data = Stm8s_fw_data_4400_svooc_3500MA;
				chip->fw_data_count = sizeof(Stm8s_fw_data_4400_svooc_3500MA);
				chip->fw_data_version = Stm8s_fw_data_4400_svooc_3500MA[chip->fw_data_count - 4];
		} else if (chip->vooc_fw_type == VOOC_FW_TYPE_STM8S_4400_SVOOC_1000MA) {
			chip->firmware_data = Stm8s_fw_data_4400_svooc_1000MA;
				chip->fw_data_count = sizeof(Stm8s_fw_data_4400_svooc_1000MA);
				chip->fw_data_version = Stm8s_fw_data_4400_svooc_1000MA[chip->fw_data_count - 4];
		} else if (chip->vooc_fw_type == VOOC_FW_TYPE_STM8S_4400_SVOOC_5000MA) {
			chip->firmware_data = Stm8s_fw_data_4400_svooc_5000MA;
				chip->fw_data_count = sizeof(Stm8s_fw_data_4400_svooc_5000MA);
				chip->fw_data_version = Stm8s_fw_data_4400_svooc_5000MA[chip->fw_data_count - 4];
		}

        chip->vops = &oppo_stm8s_ops;
		chip->fw_mcu_version = 0;
        oppo_vooc_gpio_dt_init(chip);
        opchg_set_clock_sleep(chip);
		oppo_vooc_delay_reset_mcu_init(chip);
		if(chip->vooc_fw_update_newmethod) {
			if(oppo_is_rf_ftm_mode()){
				oppo_vooc_fw_update_work_init(chip);
			}
		} else {
			oppo_vooc_fw_update_work_init(chip);
		}
        oppo_vooc_init(chip);
        register_vooc_devinfo();
        init_proc_vooc_fw_check();
        the_chip = chip;
        chg_debug("stm8s probe success,fw_type = 0x%x, fw_version = 0x%x, mcu_version = 0x%x\n",chip->vooc_fw_type,chip->fw_data_version,chip->fw_mcu_version);
        return 0;
}

/**********************************************************
  *
  *   [platform_driver API]
  *
  *********************************************************/
static const struct of_device_id stm8s_match[] = {
        { .compatible = "oppo,stm8s-fastcg"},
        { },
};

static const struct i2c_device_id stm8s_id[] = {
        { "stm8s-fastcg", 0},
        {},
};
MODULE_DEVICE_TABLE(i2c, pic16f_id);

struct i2c_driver stm8s_i2c_driver = {
        .driver        = {
                .name = "stm8s-fastcg",
                .owner        = THIS_MODULE,
                .of_match_table = stm8s_match,
        },
        .probe = stm8s_driver_probe,
        .shutdown        = stm8s_shutdown,
        .id_table = stm8s_id,
};

static int __init stm8s_subsys_init(void)
{
        int ret = 0;
        chg_debug(" init start\n");
        init_hw_version();

        if (i2c_add_driver(&stm8s_i2c_driver) != 0) {
                chg_err(" failed to register stm8s i2c driver.\n");
        } else {
                chg_debug(" Success to register stm8s i2c driver.\n");
        }
        return ret;
}
/*
static void  pic16f_exit(void)
{
        i2c_del_driver(&pic16f_i2c_driver);
}
*/
subsys_initcall(stm8s_subsys_init);
MODULE_DESCRIPTION("Driver for oppo vooc stm8s fast mcu");
MODULE_LICENSE("GPL v2");
