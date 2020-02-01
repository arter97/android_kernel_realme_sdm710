/* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/firmware.h>
#include <cam_sensor_cmn_header.h>
#include "cam_ois_core.h"
#include "cam_ois_soc.h"
#include "cam_sensor_util.h"
#include "cam_debug_util.h"
#include "cam_res_mgr_api.h"

#ifdef VENDOR_EDIT
/*Added by zhengrong.zhang@Camera.Drv, 20180821, for lc898123f40 firmware update*/
#include "PhoneUpdate.h"

extern struct cam_ois_ctrl_t *g_ois_ctrl;
extern UINT_8 WrGyroGain( UINT_32 ul_gain_x, UINT_32 ul_gain_y );
extern void F40_IOWrite32A( UINT_32 IOadrs, UINT_32 IOdata );

//****************************************************
//	CUSTOMER NECESSARY CREATING FUNCTION LIST
//****************************************************
/* for I2C communication */
void RamWrite32A( UINT_16 addr, UINT_32 data)
{
    int32_t rc = 0;
    struct cam_sensor_i2c_reg_array i2c_write_setting = {
    	.reg_addr = addr,
    	.reg_data = data,
    	.delay = 0x00,
    	.data_mask = 0x00,
    };

    struct cam_sensor_i2c_reg_setting i2c_write = {
    	.reg_setting = &i2c_write_setting,
    	.size = 1,
    	.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD,
    	.data_type = CAMERA_SENSOR_I2C_TYPE_DWORD,
    	.delay = 0x00,
    };

	rc = camera_io_dev_write(&(g_ois_ctrl->io_master_info),
		&i2c_write);

	if (rc < 0) {
		CAM_ERR(CAM_OIS, "write 0x%x fail", addr);
	}
}

void RamRead32A( UINT_32 addr, UINT_32 *data)
{
    int32_t rc = 0;

	rc = camera_io_dev_read(&(g_ois_ctrl->io_master_info), (uint32_t)addr, (uint32_t *)data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_DWORD);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "read fail");
	}
}


/* for I2C Multi Translation : Burst Mode*/
void CntWrt( INT_8 *data, UINT_16 size)
{
    int32_t rc = 0;
    int i = 0;
    int reg_data_cnt = size - 2;
    int continue_cnt = 0;
    struct cam_sensor_i2c_reg_array *i2c_write_setting = NULL;
    struct cam_sensor_i2c_reg_setting i2c_write;

    i2c_write_setting = (struct cam_sensor_i2c_reg_array *)kzalloc( sizeof(struct cam_sensor_i2c_reg_array) * reg_data_cnt, GFP_KERNEL);

    if (!i2c_write_setting) {
        CAM_ERR(CAM_OIS, "alloc i2c_write_setting fail");
        return;
    }
    memset(i2c_write_setting, 0, sizeof(struct cam_sensor_i2c_reg_array) * reg_data_cnt);
    memset(&i2c_write, 0, sizeof(struct cam_sensor_i2c_reg_setting));

    for (i = 0; i < reg_data_cnt; i++) {
        if (i == 0) {
            i2c_write_setting[continue_cnt].reg_addr = ((data[0] << 8) & 0xFF00) + (data[1] & 0xFF);
            i2c_write_setting[continue_cnt].reg_data = (data[2] & 0xFF);
            i2c_write_setting[continue_cnt].delay = 0x00;
            i2c_write_setting[continue_cnt].data_mask = 0x00;
        } else {
            //i2c_write_setting[continue_cnt].reg_addr = 0x00;
            i2c_write_setting[continue_cnt].reg_data = (data[i + 2] & 0xFF);
            i2c_write_setting[continue_cnt].delay = 0x00;
            i2c_write_setting[continue_cnt].data_mask = 0x00;
        }
        continue_cnt++;
    }

    i2c_write.reg_setting = i2c_write_setting;
    i2c_write.size = continue_cnt;
    i2c_write.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
    i2c_write.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
    i2c_write.delay = 0x00;

	rc = camera_io_dev_write_continuous(
		&(g_ois_ctrl->io_master_info), &i2c_write, 1);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "continue write fail");
	}

    if (i2c_write_setting) {
        kfree(i2c_write_setting);
        i2c_write_setting = NULL;
    }
}

/*
void CntRd3( UINT_32, void *, UINT_16 )
{
    return;
}
*/

/* WPB control for LC898123F40*/
void WPBCtrl( UINT_8 ctrl)
{
    return;
}

/* for Wait timer [Need to adjust for your system] */
void WitTim( UINT_16 delay)
{
    msleep(delay);
}
#endif

int32_t cam_ois_construct_default_power_setting(
	struct cam_sensor_power_ctrl_t *power_info)
{
	int rc = 0;

	power_info->power_setting_size = 1;
	power_info->power_setting =
		(struct cam_sensor_power_setting *)
		kzalloc(sizeof(struct cam_sensor_power_setting),
			GFP_KERNEL);
	if (!power_info->power_setting)
		return -ENOMEM;

	power_info->power_setting[0].seq_type = SENSOR_VAF;
	power_info->power_setting[0].seq_val = CAM_VAF;
	power_info->power_setting[0].config_val = 1;
	power_info->power_setting[0].delay = 2;

	power_info->power_down_setting_size = 1;
	power_info->power_down_setting =
		(struct cam_sensor_power_setting *)
		kzalloc(sizeof(struct cam_sensor_power_setting),
			GFP_KERNEL);
	if (!power_info->power_down_setting) {
		rc = -ENOMEM;
		goto free_power_settings;
	}

	power_info->power_down_setting[0].seq_type = SENSOR_VAF;
	power_info->power_down_setting[0].seq_val = CAM_VAF;
	power_info->power_down_setting[0].config_val = 0;

	return rc;

free_power_settings:
	kfree(power_info->power_setting);
	return rc;
}


/**
 * cam_ois_get_dev_handle - get device handle
 * @o_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
static int cam_ois_get_dev_handle(struct cam_ois_ctrl_t *o_ctrl,
	void *arg)
{
	struct cam_sensor_acquire_dev    ois_acq_dev;
	struct cam_create_dev_hdl        bridge_params;
	struct cam_control              *cmd = (struct cam_control *)arg;

	if (o_ctrl->bridge_intf.device_hdl != -1) {
		CAM_ERR(CAM_OIS, "Device is already acquired");
		return -EFAULT;
	}
	if (copy_from_user(&ois_acq_dev, (void __user *) cmd->handle,
		sizeof(ois_acq_dev)))
		return -EFAULT;

	bridge_params.session_hdl = ois_acq_dev.session_handle;
	bridge_params.ops = &o_ctrl->bridge_intf.ops;
	bridge_params.v4l2_sub_dev_flag = 0;
	bridge_params.media_entity_flag = 0;
	bridge_params.priv = o_ctrl;

	ois_acq_dev.device_handle =
		cam_create_device_hdl(&bridge_params);
	o_ctrl->bridge_intf.device_hdl = ois_acq_dev.device_handle;
	o_ctrl->bridge_intf.session_hdl = ois_acq_dev.session_handle;

	CAM_DBG(CAM_OIS, "Device Handle: %d", ois_acq_dev.device_handle);
	if (copy_to_user((void __user *) cmd->handle, &ois_acq_dev,
		sizeof(struct cam_sensor_acquire_dev))) {
		CAM_ERR(CAM_OIS, "ACQUIRE_DEV: copy to user failed");
		return -EFAULT;
	}
	return 0;
}

static int cam_ois_power_up(struct cam_ois_ctrl_t *o_ctrl)
{
	int                             rc = 0;
	struct cam_hw_soc_info          *soc_info =
		&o_ctrl->soc_info;
	struct cam_ois_soc_private *soc_private;
	struct cam_sensor_power_ctrl_t  *power_info;

	soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;

#ifndef VENDOR_EDIT
	/*Jinshui.Liu@Camera.Driver, 2018/03/20, modify for [oppo ois]*/
	if ((power_info->power_setting == NULL) &&
		(power_info->power_down_setting == NULL)) {
		CAM_INFO(CAM_OIS,
			"Using default power settings");
		rc = cam_ois_construct_default_power_setting(power_info);
		if (rc < 0) {
			CAM_ERR(CAM_OIS,
				"Construct default ois power setting failed.");
			return rc;
		}
	}

	/* Parse and fill vreg params for power up settings */
	rc = msm_camera_fill_vreg_params(
		soc_info,
		power_info->power_setting,
		power_info->power_setting_size);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"failed to fill vreg params for power up rc:%d", rc);
		return rc;
	}

	/* Parse and fill vreg params for power down settings*/
	rc = msm_camera_fill_vreg_params(
		soc_info,
		power_info->power_down_setting,
		power_info->power_down_setting_size);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"failed to fill vreg params for power down rc:%d", rc);
		return rc;
	}

	power_info->dev = soc_info->dev;

	rc = cam_sensor_core_power_up(power_info, soc_info);
	if (rc) {
		CAM_ERR(CAM_OIS, "failed in ois power up rc %d", rc);
		return rc;
	}
#else
	if (soc_info->num_rgltr > 0) {
		if ((power_info->power_setting == NULL) &&
			(power_info->power_down_setting == NULL)) {
			CAM_INFO(CAM_OIS,
				"Using default power settings");
			rc = cam_ois_construct_default_power_setting(power_info);
			if (rc < 0) {
				CAM_ERR(CAM_OIS,
					"Construct default ois power setting failed.");
				return rc;
			}
		}

		/* Parse and fill vreg params for power up settings */
		rc = msm_camera_fill_vreg_params(
			soc_info,
			power_info->power_setting,
			power_info->power_setting_size);
		if (rc) {
			CAM_ERR(CAM_OIS,
				"failed to fill vreg params for power up rc:%d", rc);
			return rc;
		}

		/* Parse and fill vreg params for power down settings*/
		rc = msm_camera_fill_vreg_params(
			soc_info,
			power_info->power_down_setting,
			power_info->power_down_setting_size);
		if (rc) {
			CAM_ERR(CAM_OIS,
				"failed to fill vreg params for power down rc:%d", rc);
			return rc;
		}

		power_info->dev = soc_info->dev;

		rc = cam_sensor_core_power_up(power_info, soc_info);
		if (rc) {
			CAM_ERR(CAM_OIS, "failed in ois power up rc %d", rc);
			return rc;
		}
	}
#endif

	rc = camera_io_init(&o_ctrl->io_master_info);
	if (rc)
		CAM_ERR(CAM_OIS, "cci_init failed: rc: %d", rc);

	return rc;
}

/**
 * cam_ois_power_down - power down OIS device
 * @o_ctrl:     ctrl structure
 *
 * Returns success or failure
 */
static int cam_ois_power_down(struct cam_ois_ctrl_t *o_ctrl)
{
	int32_t                         rc = 0;
	struct cam_sensor_power_ctrl_t  *power_info;
	struct cam_hw_soc_info          *soc_info =
		&o_ctrl->soc_info;
	struct cam_ois_soc_private *soc_private;

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "failed: o_ctrl %pK", o_ctrl);
		return -EINVAL;
	}

	soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;
	soc_info = &o_ctrl->soc_info;

#ifndef VENDOR_EDIT
	/*Jinshui.Liu@Camera.Driver, 2018/03/20, modify for [oppo ois]*/
	if (!power_info) {
		CAM_ERR(CAM_OIS, "failed: power_info %pK", power_info);
		return -EINVAL;
	}

	rc = msm_camera_power_down(power_info, soc_info);
	if (rc) {
		CAM_ERR(CAM_OIS, "power down the core is failed:%d", rc);
		return rc;
	}
#else
	if (soc_info->num_rgltr > 0) {
		if (!power_info) {
			CAM_ERR(CAM_OIS, "failed: power_info %pK", power_info);
			return -EINVAL;
		}

		rc = msm_camera_power_down(power_info, soc_info);
		if (rc) {
			CAM_ERR(CAM_OIS, "power down the core is failed:%d", rc);
			return rc;
		}
	}
#endif

	camera_io_release(&o_ctrl->io_master_info);

	return rc;
}

static int cam_ois_apply_settings(struct cam_ois_ctrl_t *o_ctrl,
	struct i2c_settings_array *i2c_set)
{
	struct i2c_settings_list *i2c_list;
	int32_t rc = 0;
	uint32_t i, size;

	if (o_ctrl == NULL || i2c_set == NULL) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	if (i2c_set->is_settings_valid != 1) {
		CAM_ERR(CAM_OIS, " Invalid settings");
		return -EINVAL;
	}

	list_for_each_entry(i2c_list,
		&(i2c_set->list_head), list) {
		if (i2c_list->op_code ==  CAM_SENSOR_I2C_WRITE_RANDOM) {
			rc = camera_io_dev_write(&(o_ctrl->io_master_info),
				&(i2c_list->i2c_settings));
			if (rc < 0) {
				CAM_ERR(CAM_OIS,
					"Failed in Applying i2c wrt settings");
				return rc;
			}
		} else if (i2c_list->op_code == CAM_SENSOR_I2C_POLL) {
			size = i2c_list->i2c_settings.size;
			for (i = 0; i < size; i++) {
				rc = camera_io_dev_poll(
				&(o_ctrl->io_master_info),
				i2c_list->i2c_settings.reg_setting[i].reg_addr,
				i2c_list->i2c_settings.reg_setting[i].reg_data,
				i2c_list->i2c_settings.reg_setting[i].data_mask,
				i2c_list->i2c_settings.addr_type,
				i2c_list->i2c_settings.data_type,
				i2c_list->i2c_settings.reg_setting[i].delay);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
						"i2c poll apply setting Fail");
					return rc;
				}
			}
		}
	}

	return rc;
}

static int cam_ois_slaveInfo_pkt_parser(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t *cmd_buf)
{
	int32_t rc = 0;
	struct cam_cmd_ois_info *ois_info;

	if (!o_ctrl || !cmd_buf) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	ois_info = (struct cam_cmd_ois_info *)cmd_buf;
	if (o_ctrl->io_master_info.master_type == CCI_MASTER) {
		o_ctrl->io_master_info.cci_client->i2c_freq_mode =
			ois_info->i2c_freq_mode;
		o_ctrl->io_master_info.cci_client->sid =
			ois_info->slave_addr >> 1;
		o_ctrl->ois_fw_flag = ois_info->ois_fw_flag;
		o_ctrl->is_ois_calib = ois_info->is_ois_calib;
		memcpy(o_ctrl->ois_name, ois_info->ois_name, 32);
		o_ctrl->io_master_info.cci_client->retries = 3;
		o_ctrl->io_master_info.cci_client->id_map = 0;
		memcpy(&(o_ctrl->opcode), &(ois_info->opcode),
			sizeof(struct cam_ois_opcode));
		CAM_DBG(CAM_OIS, "Slave addr: 0x%x Freq Mode: %d",
			ois_info->slave_addr, ois_info->i2c_freq_mode);
	} else if (o_ctrl->io_master_info.master_type == I2C_MASTER) {
		o_ctrl->io_master_info.client->addr = ois_info->slave_addr;
		CAM_DBG(CAM_OIS, "Slave addr: 0x%x", ois_info->slave_addr);
	} else {
		CAM_ERR(CAM_OIS, "Invalid Master type : %d",
			o_ctrl->io_master_info.master_type);
		rc = -EINVAL;
	}

	return rc;
}

static int cam_ois_fw_download(struct cam_ois_ctrl_t *o_ctrl)
{
	uint16_t                           total_bytes = 0;
	uint8_t                           *ptr = NULL;
	int32_t                            rc = 0, cnt;
	uint32_t                           fw_size;
	const struct firmware             *fw = NULL;
	const char                        *fw_name_prog = NULL;
	const char                        *fw_name_coeff = NULL;
	char                               name_prog[32] = {0};
	char                               name_coeff[32] = {0};
	struct device                     *dev = &(o_ctrl->pdev->dev);
	struct cam_sensor_i2c_reg_setting  i2c_reg_setting;
	struct page                       *page = NULL;

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	snprintf(name_coeff, 32, "%s.coeff", o_ctrl->ois_name);

	snprintf(name_prog, 32, "%s.prog", o_ctrl->ois_name);

	/* cast pointer as const pointer*/
	fw_name_prog = name_prog;
	fw_name_coeff = name_coeff;

	/* Load FW */
	rc = request_firmware(&fw, fw_name_prog, dev);
	if (rc) {
		CAM_ERR(CAM_OIS, "Failed to locate %s", fw_name_prog);
		return rc;
	}

	total_bytes = fw->size;
#ifndef VENDOR_EDIT
/*Jinshui.Liu@Camera.Driver, 2018/03/21, modify for [oppo ois]*/
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
#else
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
#endif
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.size = total_bytes;
	i2c_reg_setting.delay = 0;
	fw_size = PAGE_ALIGN(sizeof(struct cam_sensor_i2c_reg_array) *
		total_bytes) >> PAGE_SHIFT;
	page = cma_alloc(dev_get_cma_area((o_ctrl->soc_info.dev)),
		fw_size, 0);
	if (!page) {
		CAM_ERR(CAM_OIS, "Failed in allocating i2c_array");
		release_firmware(fw);
		return -ENOMEM;
	}

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *)(
		page_address(page));

#ifndef VENDOR_EDIT
	/*Jinshui.Liu@Camera.Driver, 2018/03/21, modify for [oppo ois]*/
	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;
		cnt++, ptr++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.prog;
		i2c_reg_setting.reg_setting[cnt].reg_data = *ptr;
		i2c_reg_setting.reg_setting[cnt].delay = 0;
		i2c_reg_setting.reg_setting[cnt].data_mask = 0;
	}

	rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
		&i2c_reg_setting, 1);
#else
	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;
		cnt++, ptr++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.prog + cnt;
		i2c_reg_setting.reg_setting[cnt].reg_data = *ptr;
		i2c_reg_setting.reg_setting[cnt].delay = 0;
		i2c_reg_setting.reg_setting[cnt].data_mask = 0;
	}
	i2c_reg_setting.delay = 0;
	rc = camera_io_dev_write(&(o_ctrl->io_master_info),
		&i2c_reg_setting);
#endif
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);
		goto release_firmware;
	}
	cma_release(dev_get_cma_area((o_ctrl->soc_info.dev)),
		page, fw_size);
	page = NULL;
	fw_size = 0;
	release_firmware(fw);

	rc = request_firmware(&fw, fw_name_coeff, dev);
	if (rc) {
		CAM_ERR(CAM_OIS, "Failed to locate %s", fw_name_coeff);
		return rc;
	}

	total_bytes = fw->size;
#ifndef VENDOR_EDIT
	/*Jinshui.Liu@Camera.Driver, 2018/03/21, modify for [oppo ois]*/
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
#else
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
#endif
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.size = total_bytes;
	i2c_reg_setting.delay = 0;
	fw_size = PAGE_ALIGN(sizeof(struct cam_sensor_i2c_reg_array) *
		total_bytes) >> PAGE_SHIFT;
	page = cma_alloc(dev_get_cma_area((o_ctrl->soc_info.dev)),
		fw_size, 0);
	if (!page) {
		CAM_ERR(CAM_OIS, "Failed in allocating i2c_array");
		release_firmware(fw);
		return -ENOMEM;
	}

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *)(
		page_address(page));

#ifndef VENDOR_EDIT
	/*Jinshui.Liu@Camera.Driver, 2018/03/21, modify for [oppo ois]*/
	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;
		cnt++, ptr++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.coeff;
		i2c_reg_setting.reg_setting[cnt].reg_data = *ptr;
		i2c_reg_setting.reg_setting[cnt].delay = 0;
		i2c_reg_setting.reg_setting[cnt].data_mask = 0;
	}

	rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
		&i2c_reg_setting, 1);
#else
	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;
		cnt++, ptr++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.coeff + cnt;
		i2c_reg_setting.reg_setting[cnt].reg_data = *ptr;
		i2c_reg_setting.reg_setting[cnt].delay = 0;
		i2c_reg_setting.reg_setting[cnt].data_mask = 0;
	}
	i2c_reg_setting.delay = 0;

	rc = camera_io_dev_write(&(o_ctrl->io_master_info),
		&i2c_reg_setting);
#endif
	if (rc < 0)
		CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);

release_firmware:
	cma_release(dev_get_cma_area((o_ctrl->soc_info.dev)),
		page, fw_size);
	release_firmware(fw);

	return rc;
}

/**
 * cam_ois_pkt_parse - Parse csl packet
 * @o_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
static int cam_ois_pkt_parse(struct cam_ois_ctrl_t *o_ctrl, void *arg)
{
	int32_t                         rc = 0;
	int32_t                         i = 0;
	uint32_t                        total_cmd_buf_in_bytes = 0;
	struct common_header           *cmm_hdr = NULL;
	uint64_t                        generic_ptr;
	struct cam_control             *ioctl_ctrl = NULL;
	struct cam_config_dev_cmd       dev_config;
	struct i2c_settings_array      *i2c_reg_settings = NULL;
	struct cam_cmd_buf_desc        *cmd_desc = NULL;
	uint64_t                        generic_pkt_addr;
	size_t                          pkt_len;
	struct cam_packet              *csl_packet = NULL;
	size_t                          len_of_buff = 0;
	uint32_t                       *offset = NULL, *cmd_buf;
	struct cam_ois_soc_private     *soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t  *power_info = &soc_private->power_info;
	#ifdef VENDOR_EDIT
	/*Added by Zhengrong.Zhang@Cam.Drv, 20180721, for ois*/
	uint32_t ois_information = 0;
	uint32_t ois_sub_version = 0;
	uint32_t x_gain = 0;
	uint32_t y_gain = 0;
	#endif

	ioctl_ctrl = (struct cam_control *)arg;
	if (copy_from_user(&dev_config, (void __user *) ioctl_ctrl->handle,
		sizeof(dev_config)))
		return -EFAULT;
	rc = cam_mem_get_cpu_buf(dev_config.packet_handle,
		(uint64_t *)&generic_pkt_addr, &pkt_len);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"error in converting command Handle Error: %d", rc);
		return rc;
	}

	if (dev_config.offset > pkt_len) {
		CAM_ERR(CAM_OIS,
			"offset is out of bound: off: %lld len: %zu",
			dev_config.offset, pkt_len);
		return -EINVAL;
	}

	csl_packet = (struct cam_packet *)
		(generic_pkt_addr + dev_config.offset);
	switch (csl_packet->header.op_code & 0xFFFFFF) {
	case CAM_OIS_PACKET_OPCODE_INIT:
		offset = (uint32_t *)&csl_packet->payload;
		offset += (csl_packet->cmd_buf_offset / sizeof(uint32_t));
		cmd_desc = (struct cam_cmd_buf_desc *)(offset);

		/* Loop through multiple command buffers */
		for (i = 0; i < csl_packet->num_cmd_buf; i++) {
			total_cmd_buf_in_bytes = cmd_desc[i].length;
			if (!total_cmd_buf_in_bytes)
				continue;

			rc = cam_mem_get_cpu_buf(cmd_desc[i].mem_handle,
				(uint64_t *)&generic_ptr, &len_of_buff);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "Failed to get cpu buf");
				return rc;
			}
			cmd_buf = (uint32_t *)generic_ptr;
			if (!cmd_buf) {
				CAM_ERR(CAM_OIS, "invalid cmd buf");
				return -EINVAL;
			}
			cmd_buf += cmd_desc[i].offset / sizeof(uint32_t);
			cmm_hdr = (struct common_header *)cmd_buf;

			switch (cmm_hdr->cmd_type) {
			case CAMERA_SENSOR_CMD_TYPE_I2C_INFO:
				rc = cam_ois_slaveInfo_pkt_parser(
					o_ctrl, cmd_buf);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
					"Failed in parsing slave info");
					return rc;
				}
				break;
			case CAMERA_SENSOR_CMD_TYPE_PWR_UP:
			case CAMERA_SENSOR_CMD_TYPE_PWR_DOWN:
				CAM_DBG(CAM_OIS,
					"Received power settings buffer");
				rc = cam_sensor_update_power_settings(
					cmd_buf,
					total_cmd_buf_in_bytes,
					power_info);
				if (rc) {
					CAM_ERR(CAM_OIS,
					"Failed: parse power settings");
					return rc;
				}
				break;
			default:
			if (o_ctrl->i2c_init_data.is_settings_valid == 0) {
				CAM_DBG(CAM_OIS,
				"Received init settings");
				i2c_reg_settings =
					&(o_ctrl->i2c_init_data);
				i2c_reg_settings->is_settings_valid = 1;
				i2c_reg_settings->request_id = 0;
				rc = cam_sensor_i2c_command_parser(
					&o_ctrl->io_master_info,
					i2c_reg_settings,
					&cmd_desc[i], 1);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
					"init parsing failed: %d", rc);
					return rc;
				}
			} else if ((o_ctrl->is_ois_calib != 0) &&
				(o_ctrl->i2c_calib_data.is_settings_valid ==
				0)) {
				CAM_DBG(CAM_OIS,
					"Received calib settings");
				i2c_reg_settings = &(o_ctrl->i2c_calib_data);
				i2c_reg_settings->is_settings_valid = 1;
				i2c_reg_settings->request_id = 0;
				rc = cam_sensor_i2c_command_parser(
					&o_ctrl->io_master_info,
					i2c_reg_settings,
					&cmd_desc[i], 1);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
						"Calib parsing failed: %d", rc);
					return rc;
				}
			}
			break;
			}
		}

		if (o_ctrl->cam_ois_state != CAM_OIS_CONFIG) {
			rc = cam_ois_power_up(o_ctrl);
			if (rc) {
				CAM_ERR(CAM_OIS, " OIS Power up failed");
				return rc;
			}
			o_ctrl->cam_ois_state = CAM_OIS_CONFIG;
			#ifdef VENDOR_EDIT
			/*Added by Zhengrong.Zhang@Cam.Drv, 20180721, for ois*/
			msleep(5);

			rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x8000, &ois_information,
				CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_DWORD);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "read ois_information fail before FW update");
			} else {
				CAM_INFO(CAM_OIS, "ois_information = 0x%x before FW update", ois_information);
				ois_information = ois_information & 0xFF;

				rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x8008, &ois_sub_version,
					CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_DWORD);
				if (rc < 0) {
					CAM_ERR(CAM_OIS, "read ois_sub_version fail before FW update");
				} else {
					CAM_INFO(CAM_OIS, "ois_sub_version = 0x%x before FW update", ois_sub_version);
				}

				if (ois_information != 0xE6 || (ois_information == 0xE6 && ois_sub_version != 0x5)) {
					UINT_8 update_status = F40_FlashDownload(0x1, 0x01, 0x00);
					CAM_INFO(CAM_OIS, "FW update finish: update_status = 0x%x", update_status);
					rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x8000, &ois_information,
						CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_DWORD);
					if (rc >= 0) {
						CAM_INFO(CAM_OIS, "ois_information = 0x%x after FW update", ois_information);
						ois_information = ois_information & 0xFF;
					}
				}

				if (ois_information == 0xE6) {
					int need_remap = 0;
					RamRead32A(0x82B8, &x_gain);
					RamRead32A(0x8318, &y_gain);
					if (x_gain < 0x40000000/*0.5*/ || x_gain > 0x59999980/*0.7*/) {
						x_gain = 0x4C8FDA00;/*aver*/
						need_remap = 1;
					}

					if (y_gain > 0xC0000000/*0.5*/ || y_gain < 0xA6666680/*0.7*/) {
						y_gain = 0xB6D2D480;/*aver*/
						need_remap = 1;
					}

					if (need_remap) {
						WrGyroGain(x_gain, y_gain);
						F40_IOWrite32A(0xD000AC, 0x00001000) ;
						msleep(1000);
					}
					RamRead32A(0x82B8, &x_gain);
					RamRead32A(0x8318, &y_gain);
					CAM_INFO(CAM_OIS, "x_gain = 0x%x y_gain = 0x%x,need_remap=%d",
						x_gain, y_gain, need_remap);
				}
			}
			#endif
		}

#ifdef VENDOR_EDIT
		/*Jinshui.Liu@Camera.Driver, 2018/03/20, add for [ois sequence]*/
		rc = cam_ois_apply_settings(o_ctrl, &o_ctrl->i2c_init_data);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "Cannot apply Init settings");
			goto pwr_dwn;
		}
#endif

		if (o_ctrl->ois_fw_flag) {
			rc = cam_ois_fw_download(o_ctrl);
			if (rc) {
				CAM_ERR(CAM_OIS, "Failed OIS FW Download");
				goto pwr_dwn;
			}
		}

#ifndef VENDOR_EDIT
		/*Jinshui.Liu@Camera.Driver, 2018/03/20, delete for [ois sequence]*/
		rc = cam_ois_apply_settings(o_ctrl, &o_ctrl->i2c_init_data);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "Cannot apply Init settings");
			goto pwr_dwn;
		}
#endif

		if (o_ctrl->is_ois_calib) {
			rc = cam_ois_apply_settings(o_ctrl,
				&o_ctrl->i2c_calib_data);
			if (rc) {
				CAM_ERR(CAM_OIS, "Cannot apply calib data");
				goto pwr_dwn;
			}
		}

		rc = delete_request(&o_ctrl->i2c_init_data);
		if (rc < 0) {
			CAM_WARN(CAM_OIS,
				"Fail deleting Init data: rc: %d", rc);
			rc = 0;
		}
		rc = delete_request(&o_ctrl->i2c_calib_data);
		if (rc < 0) {
			CAM_WARN(CAM_OIS,
				"Fail deleting Calibration data: rc: %d", rc);
			rc = 0;
		}
		break;
	case CAM_OIS_PACKET_OPCODE_OIS_CONTROL:
		if (o_ctrl->cam_ois_state < CAM_OIS_CONFIG) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
				"Not in right state to control OIS: %d",
				o_ctrl->cam_ois_state);
			return rc;
		}
		offset = (uint32_t *)&csl_packet->payload;
		offset += (csl_packet->cmd_buf_offset / sizeof(uint32_t));
		cmd_desc = (struct cam_cmd_buf_desc *)(offset);
		i2c_reg_settings = &(o_ctrl->i2c_mode_data);
		i2c_reg_settings->is_settings_valid = 1;
		i2c_reg_settings->request_id = 0;
		rc = cam_sensor_i2c_command_parser(&o_ctrl->io_master_info,
			i2c_reg_settings,
			cmd_desc, 1);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "OIS pkt parsing failed: %d", rc);
			return rc;
		}

		rc = cam_ois_apply_settings(o_ctrl, i2c_reg_settings);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "Cannot apply mode settings");
			return rc;
		}

		rc = delete_request(i2c_reg_settings);
		if (rc < 0)
			CAM_ERR(CAM_OIS,
				"Fail deleting Mode data: rc: %d", rc);
		break;
	default:
		break;
	}
	return rc;
pwr_dwn:
	cam_ois_power_down(o_ctrl);
	return rc;
}

void cam_ois_shutdown(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc;

	if (o_ctrl->cam_ois_state == CAM_OIS_INIT)
		return;

	if (o_ctrl->cam_ois_state >= CAM_OIS_CONFIG) {
		rc = cam_ois_power_down(o_ctrl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "OIS Power down failed");
	}

	if (o_ctrl->cam_ois_state >= CAM_OIS_ACQUIRE) {
		rc = cam_destroy_device_hdl(o_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "destroying the device hdl");
		o_ctrl->bridge_intf.device_hdl = -1;
		o_ctrl->bridge_intf.link_hdl = -1;
		o_ctrl->bridge_intf.session_hdl = -1;
	}

	o_ctrl->cam_ois_state = CAM_OIS_INIT;
}

/**
 * cam_ois_driver_cmd - Handle ois cmds
 * @e_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
int cam_ois_driver_cmd(struct cam_ois_ctrl_t *o_ctrl, void *arg)
{
	int                            rc = 0;
	struct cam_ois_query_cap_t     ois_cap = {0};
	struct cam_control            *cmd = (struct cam_control *)arg;

	if (!o_ctrl || !arg) {
		CAM_ERR(CAM_OIS, "Invalid arguments");
		return -EINVAL;
	}

	if (cmd->handle_type != CAM_HANDLE_USER_POINTER) {
		CAM_ERR(CAM_OIS, "Invalid handle type: %d",
			cmd->handle_type);
		return -EINVAL;
	}

	mutex_lock(&(o_ctrl->ois_mutex));
	switch (cmd->op_code) {
	case CAM_QUERY_CAP:
		ois_cap.slot_info = o_ctrl->soc_info.index;

		if (copy_to_user((void __user *) cmd->handle,
			&ois_cap,
			sizeof(struct cam_ois_query_cap_t))) {
			CAM_ERR(CAM_OIS, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}
		CAM_DBG(CAM_OIS, "ois_cap: ID: %d", ois_cap.slot_info);
		break;
	case CAM_ACQUIRE_DEV:
		rc = cam_ois_get_dev_handle(o_ctrl, arg);
		if (rc) {
			CAM_ERR(CAM_OIS, "Failed to acquire dev");
			goto release_mutex;
		}

		o_ctrl->cam_ois_state = CAM_OIS_ACQUIRE;
		break;
	case CAM_START_DEV:
		if (o_ctrl->cam_ois_state != CAM_OIS_CONFIG) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
			"Not in right state for start : %d",
			o_ctrl->cam_ois_state);
			goto release_mutex;
		}
		o_ctrl->cam_ois_state = CAM_OIS_START;
		break;
	case CAM_CONFIG_DEV:
		rc = cam_ois_pkt_parse(o_ctrl, arg);
		if (rc) {
			CAM_ERR(CAM_OIS, "Failed in ois pkt Parsing");
#ifdef VENDOR_EDIT
			/*Jinshui.Liu@Camera.Driver, 2018/03/28, add for [dont notify config err]*/
			rc = 0;
#endif
			goto release_mutex;
		}
		break;
	case CAM_RELEASE_DEV:
		if (o_ctrl->cam_ois_state == CAM_OIS_START) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
				"Cant release ois: in start state");
			goto release_mutex;
		}

		if (o_ctrl->cam_ois_state == CAM_OIS_CONFIG) {
			rc = cam_ois_power_down(o_ctrl);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "OIS Power down failed");
				goto release_mutex;
			}
		}

		if (o_ctrl->bridge_intf.device_hdl == -1) {
			CAM_ERR(CAM_OIS, "link hdl: %d device hdl: %d",
				o_ctrl->bridge_intf.device_hdl,
				o_ctrl->bridge_intf.link_hdl);
			rc = -EINVAL;
			goto release_mutex;
		}
		rc = cam_destroy_device_hdl(o_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "destroying the device hdl");
		o_ctrl->bridge_intf.device_hdl = -1;
		o_ctrl->bridge_intf.link_hdl = -1;
		o_ctrl->bridge_intf.session_hdl = -1;
		o_ctrl->cam_ois_state = CAM_OIS_INIT;

		#ifdef VENDOR_EDIT
		/*Feng.Hu@Camera.Driver 20180515 modify for ois init costs too long time*/
		if (o_ctrl->i2c_mode_data.is_settings_valid == 1)
		{
			delete_request(&o_ctrl->i2c_mode_data);
		}

		if (o_ctrl->i2c_calib_data.is_settings_valid == 1)
		{
			delete_request(&o_ctrl->i2c_calib_data);
		}

		if (o_ctrl->i2c_init_data.is_settings_valid == 1)
		{
			delete_request(&o_ctrl->i2c_init_data);
		}
		#endif
		break;
	case CAM_STOP_DEV:
		if (o_ctrl->cam_ois_state != CAM_OIS_START) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
			"Not in right state for stop : %d",
			o_ctrl->cam_ois_state);
		}
		o_ctrl->cam_ois_state = CAM_OIS_CONFIG;
		break;

#ifdef VENDOR_EDIT
	/*Added by Zhengrong.Zhang@Cam.Drv, 20180421, for [ois calibration]*/
	case CAM_GET_OIS_GYRO_OFFSET: {
		uint32_t gyro_offset = 0;
		uint32_t gyro_offset_x = 0;
		uint32_t gyro_offset_y = 0;

		rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x6040, &gyro_offset_x,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
        	if (rc < 0) {
			CAM_ERR(CAM_OIS, "read gyro offset_x fail");
		}
		gyro_offset_x = (gyro_offset_x & 0xFF) << 8 | (gyro_offset_x & 0xFF00) >> 8;

		rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x6042, &gyro_offset_y,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
        	if (rc < 0) {
			CAM_ERR(CAM_OIS, "read gyro offset_y fail");
		}
		gyro_offset_y = (gyro_offset_y & 0xFF) << 8 | (gyro_offset_y & 0xFF00) >> 8;

		gyro_offset = ((gyro_offset_y & 0xFFFF) << 16) | (gyro_offset_x & 0xFFFF);
		CAM_INFO(CAM_OIS, "final gyro_offset = 0x%x; gyro_x=0x%x, gyro_y=0x%x",
			gyro_offset, gyro_offset_x, gyro_offset_y);

		if (copy_to_user((void __user *) cmd->handle, &gyro_offset,
			sizeof(gyro_offset))) {
			CAM_ERR(CAM_OIS, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}
		break;
	}

	case CAM_GET_OIS_HALL_POSITION: {
		uint32_t hall_position = 0;
		uint32_t hall_position_x = 0;
		uint32_t hall_position_y = 0;

		rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x6058, &hall_position_x,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "read hall_position_x fail");
		}

		rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x6059, &hall_position_y,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "read hall_position_y fail");
		}

		hall_position = ((hall_position_y & 0xFF) << 16) | (hall_position_x & 0xFF);
		CAM_INFO(CAM_OIS, "final hall_position = 0x%x; hall_position_x=0x%x, hall_position_y=0x%x",
			hall_position, hall_position_x, hall_position_y);

		if (copy_to_user((void __user *) cmd->handle, &hall_position,
			sizeof(hall_position))) {
			CAM_ERR(CAM_OIS, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}
		break;
	}
#endif

	default:
		CAM_ERR(CAM_OIS, "invalid opcode");
		goto release_mutex;
	}
release_mutex:
	mutex_unlock(&(o_ctrl->ois_mutex));
	return rc;
}
