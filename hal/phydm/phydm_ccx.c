/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#include "mp_precomp.h"
#include "phydm_precomp.h"

void
phydm_ccx_hw_restart(
	void			*dm_void
)/*Will Restart NHM/CLM/FAHM simultaneously*/
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	u32	reg1 = (dm->support_ic_type & ODM_IC_11AC_SERIES) ? R_0x994 : R_0x890;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);
	odm_set_bb_reg(dm, reg1, 0x7, 0x0); /*disable NHM,CLM, FAHM*/

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		odm_set_bb_reg(dm, R_0x994, BIT(8), 0x0);
		odm_set_bb_reg(dm, R_0x994, BIT(8), 0x1);

	} else if (dm->support_ic_type & ODM_IC_11N_SERIES) {
		odm_set_bb_reg(dm, R_0x890, BIT(8), 0x0);
		odm_set_bb_reg(dm, R_0x890, BIT(8), 0x1);
	}
}

#ifdef FAHM_SUPPORT

u16
phydm_hw_divider(
	void	*dm_void,
	u16	numerator,
	u16	denumerator
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	u16	result = DEVIDER_ERROR;
	u32	tmp_u32 = ((numerator << 16) | denumerator);
	u32	reg_devider_input;
	u32	reg_devider_rpt;
	u8	i;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __FUNCTION__);

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		reg_devider_input =  0x1cbc;
		reg_devider_rpt = 0x1f98;
	} else {
		reg_devider_input =  0x980;
		reg_devider_rpt = 0x9f0;
	}

	odm_set_bb_reg(dm, reg_devider_input, MASKDWORD, tmp_u32);

	for (i = 0; i < 10; i++) {
		ODM_delay_ms(1);
		if (odm_get_bb_reg(dm, reg_devider_rpt, BIT(24))) { /*Chk HW rpt is ready*/

			result = (u16)odm_get_bb_reg(dm, reg_devider_rpt, MASKBYTE2);
			break;
		}
	}
	return	result;
}

void
phydm_fahm_trigger(
	void		*dm_void,
	u16		trigger_period	/*unit (4us)*/
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info			*ccx_info = &dm->dm_ccx_info;
	u32		fahm_reg1;
	u32		fahm_reg2;

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		odm_set_bb_reg(dm, R_0x1cf8, 0xffff00, trigger_period);

		fahm_reg1 =  0x994;
	} else {

		odm_set_bb_reg(dm, R_0x978, 0xff000000, (trigger_period & 0xff));
		odm_set_bb_reg(dm, R_0x97c, 0xff, (trigger_period & 0xff00)>>8);

		fahm_reg1 =  0x890;
	}

	odm_set_bb_reg(dm, fahm_reg1, BIT(2), 0);
	odm_set_bb_reg(dm, fahm_reg1, BIT(2), 1);
}

void
phydm_fahm_set_valid_cnt(
	void		*dm_void,
	u8		numerator_sel,
	u8		denumerator_sel
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info			*ccx_info = &dm->dm_ccx_info;
	u32		fahm_reg1;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __FUNCTION__);

	if ((ccx_info->fahm_nume_sel == numerator_sel) &&
		(ccx_info->fahm_denum_sel == denumerator_sel)) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "no need to update\n");
		return;
	}

	ccx_info->fahm_nume_sel = numerator_sel;
	ccx_info->fahm_denum_sel = denumerator_sel;

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		fahm_reg1 =  0x994;
	} else {
		fahm_reg1 =  0x890;
	}

	odm_set_bb_reg(dm, fahm_reg1, 0xe0, numerator_sel);
	odm_set_bb_reg(dm, fahm_reg1, 0x7000, denumerator_sel);
}

void
phydm_fahm_get_result(
	void		*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info			*ccx_info = &dm->dm_ccx_info;
	u16		fahm_rpt_cnt[12];	/*packet count*/
	u16		fahm_rpt[12];		/*percentage*/
	u16		fahm_denumerator;	/*packet count*/
	u32		reg_rpt, reg_rpt_2;
	u32		reg_val_tmp;
	boolean	is_ready = false;
	u8		i;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __FUNCTION__);

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		reg_rpt =  0x1f80;
		reg_rpt_2 = 0x1f98;
	} else {
		reg_rpt =  0x9d8;
		reg_rpt_2 = 0x9f0;
	}

	for (i = 0; i < 3; i++) {

		if (odm_get_bb_reg(dm, reg_rpt_2, BIT(31))) { /*Chk HW rpt is ready*/

			is_ready = true;
			break;
		}
		ODM_delay_ms(1);
	}

	if (is_ready == false)
		return;

	/*Get Denumerator*/
	fahm_denumerator = (u16)odm_get_bb_reg(dm, reg_rpt_2, MASKLWORD);

	PHYDM_DBG(dm, DBG_ENV_MNTR, "Reg[0x%x] fahm_denmrtr = %d\n", reg_rpt_2, fahm_denumerator);


	/*Get nemerator*/
	for (i = 0; i<6; i++) {
		reg_val_tmp = odm_get_bb_reg(dm, reg_rpt + (i<<2), MASKDWORD);

		PHYDM_DBG(dm, DBG_ENV_MNTR, "Reg[0x%x] fahm_denmrtr = %d\n", reg_rpt + (i*4), reg_val_tmp);

		fahm_rpt_cnt[i*2] = (u16)(reg_val_tmp & MASKLWORD);
		fahm_rpt_cnt[i*2 +1] = (u16)((reg_val_tmp & MASKHWORD)>>16);
	}

	for (i = 0; i<12; i++) {
		fahm_rpt[i] = phydm_hw_divider(dm, fahm_rpt_cnt[i], fahm_denumerator);
	}

	PHYDM_DBG(dm, DBG_ENV_MNTR,
		  "FAHM_RPT_cnt[10:0]=[%d, %d, %d, %d, %d(IGI), %d, %d, %d, %d, %d, %d, %d]\n",
		  fahm_rpt_cnt[11], fahm_rpt_cnt[10], fahm_rpt_cnt[9],
		  fahm_rpt_cnt[8], fahm_rpt_cnt[7], fahm_rpt_cnt[6],
		  fahm_rpt_cnt[5], fahm_rpt_cnt[4], fahm_rpt_cnt[3],
		  fahm_rpt_cnt[2], fahm_rpt_cnt[1], fahm_rpt_cnt[0]);

	PHYDM_DBG(dm, DBG_ENV_MNTR,
		  "FAHM_RPT[10:0]=[%d, %d, %d, %d, %d(IGI), %d, %d, %d, %d, %d, %d, %d]\n",
		  fahm_rpt[11], fahm_rpt[10], fahm_rpt[9], fahm_rpt[8],
		  fahm_rpt[7], fahm_rpt[6],
		  fahm_rpt[5], fahm_rpt[4], fahm_rpt[3], fahm_rpt[2],
		  fahm_rpt[1], fahm_rpt[0]);

}

void
phydm_fahm_set_th_by_igi(
	void		*dm_void,
	u8		igi
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info			*ccx_info = &dm->dm_ccx_info;
	u8	fahm_th[11];
	u8	rssi_th[11];	/*in RSSI scale*/
	u8	th_gap = 2 * IGI_TO_NHM_TH_MULTIPLIER;	/*beacuse unit is 0.5dB for FAHM*/
	u8	i;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __FUNCTION__);

	if (ccx_info->env_mntr_igi == igi) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "No need to update FAHM_th, IGI=0x%x\n", ccx_info->env_mntr_igi);
		return;
	}

	ccx_info->env_mntr_igi = igi;	/*bkp IGI*/

	if (igi >= CCA_CAP)
		fahm_th[0] = (igi - CCA_CAP) * IGI_TO_NHM_TH_MULTIPLIER;
	else
		fahm_th[0] = 0;

	rssi_th[0] = igi -10 - CCA_CAP;

	for (i = 1; i <= 10; i++) {
		fahm_th[i] = fahm_th[0] + th_gap * i;
		rssi_th[i] = rssi_th[0] +  (i<<1);
	}

	PHYDM_DBG(dm, DBG_ENV_MNTR,
		  "FAHM_RSSI_th[10:0]=[%d, %d, %d, (IGI)%d, %d, %d, %d, %d, %d, %d, %d]\n",
		  rssi_th[10], rssi_th[9], rssi_th[8], rssi_th[7], rssi_th[6],
		  rssi_th[5], rssi_th[4], rssi_th[3], rssi_th[2], rssi_th[1],
		  rssi_th[0]);

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {

		odm_set_bb_reg(dm, R_0x1c38, 0xffffff00, ((fahm_th[2]<<24) |(fahm_th[1]<<16) | (fahm_th[0]<<8)));
		odm_set_bb_reg(dm, R_0x1c78, 0xffffff00, ((fahm_th[5]<<24) |(fahm_th[4]<<16) | (fahm_th[3]<<8)));
		odm_set_bb_reg(dm, R_0x1c7c, 0xffffff00, ((fahm_th[7]<<24) |(fahm_th[6]<<16)));
		odm_set_bb_reg(dm, R_0x1cb8, 0xffffff00, ((fahm_th[10]<<24) |(fahm_th[9]<<16) | (fahm_th[8]<<8)));
	} else {
		odm_set_bb_reg(dm, R_0x970, MASKDWORD, ((fahm_th[3]<<24) |(fahm_th[2]<<16) | (fahm_th[1]<<8) | fahm_th[0]));
		odm_set_bb_reg(dm, R_0x974, MASKDWORD, ((fahm_th[7]<<24) |(fahm_th[6]<<16) | (fahm_th[5]<<8) | fahm_th[4]));
		odm_set_bb_reg(dm, R_0x978, MASKDWORD, ((fahm_th[10]<<16) | (fahm_th[9]<<8) | fahm_th[8]));
	}
}

void
phydm_fahm_init(
	void			*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info			*ccx_info = &dm->dm_ccx_info;
	u32	fahm_reg1;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __FUNCTION__);
	PHYDM_DBG(dm, DBG_ENV_MNTR, "IGI=0x%x\n", dm->dm_dig_table.cur_ig_value);

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		fahm_reg1 =  0x994;
	} else {
		fahm_reg1 =  0x890;
	}

	ccx_info->fahm_period = 65535;

	odm_set_bb_reg(dm, fahm_reg1, 0x6, 3);	/*FAHM HW block enable*/

	phydm_fahm_set_valid_cnt(dm, FAHM_INCLD_FA, (FAHM_INCLD_FA| FAHM_INCLD_CRC_OK |FAHM_INCLD_CRC_ER));
	phydm_fahm_set_th_by_igi(dm, dm->dm_dig_table.cur_ig_value);
}

void
phydm_fahm_dbg(
	void		*dm_void,
	char		input[][16],
	u32		*_used,
	char		*output,
	u32		*_out_len,
	u32		input_num
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info	*ccx_info = &dm->dm_ccx_info;
	char		help[] = "-h";
	u32		var1[10] = {0};
	u32		used = *_used;
	u32		out_len = *_out_len;
	u32		i;

	for (i=0; (i<2 && strnlen(input[i+1], 1)); i++)
		PHYDM_SSCANF(input[i+1], DCMD_DECIMAL, &var1[i]);

	if ((strcmp(input[1], help) == 0)) {
		PDM_SNPF(out_len, used, output + used, out_len - used, "{1: trigger, 2:get result}\n");
		PDM_SNPF(out_len, used, output + used, out_len - used, "{3: MNTR mode sel} {1: driver, 2. FW}\n");
		return;
	} else if (var1[0] == 1) { /* Set & trigger CLM */

		phydm_fahm_set_th_by_igi(dm, dm->dm_dig_table.cur_ig_value);
		phydm_fahm_trigger(dm, ccx_info->fahm_period);
		PDM_SNPF(out_len, used, output + used, out_len - used, "Monitor FAHM for %d * 4us\n",
			       ccx_info->fahm_period);

	} else if (var1[0] == 2) { /* Get CLM results */

		phydm_fahm_get_result(dm);
		PDM_SNPF(out_len, used, output + used, out_len - used,"FAHM_result=%d us\n",
			       (ccx_info->clm_result<<2));

	} else {
		PDM_SNPF(out_len, used, output + used, out_len - used, "Error\n");
	}

	*_used = used;
	*_out_len = out_len;
}


#endif /*#ifdef FAHM_SUPPORT*/

#ifdef NHM_SUPPORT

void
phydm_nhm_racing_release(
	void			*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	u32	value32;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);
	PHYDM_DBG(dm, DBG_ENV_MNTR, "lv:(%d)->(0)\n", ccx->nhm_set_lv);

	ccx->nhm_ongoing = false;
	ccx->nhm_set_lv = NHM_RELEASE;

	if (!((ccx->nhm_app == NHM_BACKGROUND) || (ccx->nhm_app == NHM_ACS)))
		phydm_pause_func(dm, F00_DIG, PHYDM_RESUME, PHYDM_PAUSE_LEVEL_1, 1, &value32);

	ccx->nhm_app = NHM_BACKGROUND;
}

u8
phydm_nhm_racing_ctrl(
	void			*dm_void,
	enum phydm_nhm_level	 nhm_lv
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	u8	set_result = PHYDM_SET_SUCCESS;
	/*acquire to control NHM API*/

	PHYDM_DBG(dm, DBG_ENV_MNTR, "nhm_ongoing=%d, lv:(%d)->(%d)\n",
		  ccx->nhm_ongoing, ccx->nhm_set_lv, nhm_lv);
	if (ccx->nhm_ongoing) {
		if (nhm_lv <= ccx->nhm_set_lv) {
			set_result = PHYDM_SET_FAIL;
		} else {
			phydm_ccx_hw_restart(dm);
			ccx->nhm_ongoing = false;
		}
	}

	if (set_result)
		ccx->nhm_set_lv = nhm_lv;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "nhm racing success=%d\n", set_result);
	return set_result;
}


void
phydm_nhm_trigger(
	void		*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	u32	nhm_reg1 = (dm->support_ic_type & ODM_IC_11AC_SERIES) ? 0x994 : 0x890;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);

	/*Trigger NHM*/
	pdm_set_reg(dm, nhm_reg1, BIT(1), 0);
	pdm_set_reg(dm, nhm_reg1, BIT(1), 1);
	ccx->nhm_trigger_time = dm->phydm_sys_up_time;
	ccx->nhm_rpt_stamp++;
	ccx->nhm_ongoing = true;
}

boolean
phydm_nhm_check_rdy(
	void		*dm_void
)
{
	struct dm_struct		*dm = (struct dm_struct *)dm_void;
	boolean	is_ready = false;
	u32		reg1 = 0, reg1_bit = 0;

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		reg1 =  0xfb4;
		reg1_bit = 16;
	} else {
		reg1 =  0x8b4;
		if (dm->support_ic_type == ODM_RTL8710B) {
			reg1_bit = 25;
		} else {
			reg1_bit = 17;
		}
	}

	if (odm_get_bb_reg(dm, reg1, BIT(reg1_bit)))
		is_ready = true;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "NHM rdy=%d\n", is_ready);
	return is_ready;
}

void
phydm_nhm_get_utility(
	void		*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;

	if (ccx->nhm_rpt_sum >= ccx->nhm_result[0])
		ccx->nhm_ratio = (u8)(((ccx->nhm_rpt_sum - ccx->nhm_result[0]) * 100) >> 8);
	else {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "[warning] nhm_rpt_sum invalid\n");
		ccx->nhm_ratio = 0;
	}

	PHYDM_DBG(dm, DBG_ENV_MNTR, "nhm_ratio=%d\n", ccx->nhm_ratio);
}

boolean
phydm_nhm_get_result(
	void		*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info			*ccx = &dm->dm_ccx_info;
	u32			value32;
	u8			i;
	u32	nhm_reg1 = (dm->support_ic_type & ODM_IC_11AC_SERIES) ? 0x994 : 0x890;
	u16	nhm_rpt_sum_tmp = 0;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);
	pdm_set_reg(dm, nhm_reg1, BIT(1), 0);

	if (phydm_nhm_check_rdy(dm) == false) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "Get NHM report Fail\n");
		phydm_nhm_racing_release(dm);
		return false;
	}

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		value32 = odm_read_4byte(dm, 0xfa8);
		odm_move_memory(dm, &ccx->nhm_result[0], &value32, 4);

		value32 = odm_read_4byte(dm, 0xfac);
		odm_move_memory(dm, &ccx->nhm_result[4], &value32, 4);

		value32 = odm_read_4byte(dm, 0xfb0);
		odm_move_memory(dm, &ccx->nhm_result[8], &value32, 4);

		/*Get NHM duration*/
		value32 = odm_read_4byte(dm, 0xfb4);
		ccx->nhm_duration = (u16)(value32 & MASKLWORD);
	} else {
		value32 = odm_read_4byte(dm, 0x8d8);
		odm_move_memory(dm, &ccx->nhm_result[0], &value32, 4);

		value32 = odm_read_4byte(dm, 0x8dc);
		odm_move_memory(dm, &ccx->nhm_result[4], &value32, 4);

		value32 = odm_get_bb_reg(dm, R_0x8d0, 0xffff0000);
		odm_move_memory(dm, &ccx->nhm_result[8], &value32, 2);

		value32 = odm_read_4byte(dm, 0x8d4);
		/*odm_move_memory(dm, &ccx->nhm_result[10], (&value32 + 2), 2);*/
		ccx->nhm_result[10] = (u8)((value32 & MASKBYTE2) >> 16);
		ccx->nhm_result[11] = (u8)((value32 & MASKBYTE3) >> 24);

		/*Get NHM duration*/
		ccx->nhm_duration = (u16)(value32 & MASKLWORD);
	}

	/* sum all nhm_result */
	if (ccx->nhm_period >= 65530) {
		value32 = (ccx->nhm_duration * 100) >> 16;
		PHYDM_DBG(dm, DBG_ENV_MNTR, "NHM valid time = %d, valid: %d percent\n", ccx->nhm_duration, value32);
	}

	for (i = 0; i < NHM_RPT_NUM; i++)
		nhm_rpt_sum_tmp += (u16)ccx->nhm_result[i];

	ccx->nhm_rpt_sum = (u8)nhm_rpt_sum_tmp;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "NHM_Rpt[%d](H->L)[%d %d %d %d %d %d %d %d %d %d %d %d]\n",
		ccx->nhm_rpt_stamp,
		ccx->nhm_result[11], ccx->nhm_result[10], ccx->nhm_result[9],
		ccx->nhm_result[8], ccx->nhm_result[7], ccx->nhm_result[6],
		ccx->nhm_result[5], ccx->nhm_result[4], ccx->nhm_result[3],
		ccx->nhm_result[2], ccx->nhm_result[1], ccx->nhm_result[0]);

	phydm_nhm_racing_release(dm);

	if (nhm_rpt_sum_tmp > 255) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "[Warning] Invalid NHM RPT, total=%d\n",
			  nhm_rpt_sum_tmp);
		return false;
	}

	return true;
}

void
phydm_nhm_set_th_reg(
	void		*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info	*ccx = &dm->dm_ccx_info;
	u32	reg1 = 0,  reg2 = 0, reg3 = 0, reg4 = 0;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		reg1 = 0x994;
		reg2 = 0x998;
		reg3 = 0x99c;
		reg4 = 0x9a0;
	} else {
		reg1 = 0x890;
		reg2 = 0x898;
		reg3 = 0x89c;
		reg4 = 0xe28;
	}

	/*Set NHM threshold*/ /*Unit: PWdB U(8,1)*/
	pdm_set_reg(dm, reg2, MASKDWORD, BYTE_2_DWORD(ccx->nhm_th[3], ccx->nhm_th[2], ccx->nhm_th[1], ccx->nhm_th[0]));
	pdm_set_reg(dm, reg3, MASKDWORD, BYTE_2_DWORD(ccx->nhm_th[7], ccx->nhm_th[6], ccx->nhm_th[5], ccx->nhm_th[4]));
	pdm_set_reg(dm, reg4, MASKBYTE0, ccx->nhm_th[8]);
	pdm_set_reg(dm, reg1, 0xffff0000, BYTE_2_DWORD(0, 0, ccx->nhm_th[10], ccx->nhm_th[9]));

	PHYDM_DBG(dm, DBG_ENV_MNTR, "Update NHM_th[H->L]=[%d %d %d %d %d %d %d %d %d %d %d]\n",
		ccx->nhm_th[10], ccx->nhm_th[9], ccx->nhm_th[8],ccx->nhm_th[7],
		ccx->nhm_th[6], ccx->nhm_th[5],ccx->nhm_th[4], ccx->nhm_th[3],
		ccx->nhm_th[2], ccx->nhm_th[1], ccx->nhm_th[0]);
}

boolean
phydm_nhm_th_update_chk(
	void		*dm_void,
	enum nhm_application	nhm_app,
	u8	*nhm_th,
	u32	*igi_new
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info	*ccx = &dm->dm_ccx_info;
	boolean	is_update = false;
	u8	igi_curr = dm->dm_dig_table.cur_ig_value;
	u8	nhm_igi_th_11k_low[NHM_TH_NUM] = {0x12, 0x15, 0x18, 0x1b, 0x1e, 0x23, 0x28, 0x2c, 0x78, 0x78, 0x78};
	u8	nhm_igi_th_11k_high[NHM_TH_NUM] = {0x1e, 0x23, 0x28, 0x2d, 0x32, 0x37, 0x78, 0x78, 0x78, 0x78, 0x78};
	u8	nhm_igi_th_xbox[NHM_TH_NUM] = {0x1a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3d};
	u8	i;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);
	PHYDM_DBG(dm, DBG_ENV_MNTR, "App=%d, nhm_igi=0x%x, igi_curr=0x%x\n", nhm_app, ccx->nhm_igi, igi_curr);

	if (igi_curr < 0x10)/* Protect for invalid IGI*/
		return	false;

	switch (nhm_app) {

	case NHM_BACKGROUND:	/*Get IGI form driver parameter(cur_ig_value)*/
	case NHM_ACS:
		if ((ccx->nhm_igi != igi_curr) || (ccx->nhm_app != nhm_app)) {
			is_update = true;
			*igi_new = (u32)igi_curr;
			nhm_th[0] = (u8)IGI_2_NHM_TH(igi_curr - CCA_CAP);
			for (i = 1; i <= 10; i++)
				nhm_th[i] = nhm_th[0] + IGI_2_NHM_TH(2 * i);
		}
		break;

	case IEEE_11K_HIGH:
		is_update = true;
		*igi_new = 0x2c;
		for (i = 0; i < NHM_TH_NUM; i++)
			nhm_th[i] = IGI_2_NHM_TH(nhm_igi_th_11k_high[i]);
		break;

	case IEEE_11K_LOW:
		is_update = true;
		*igi_new = 0x20;
		for (i = 0; i < NHM_TH_NUM; i++)
			nhm_th[i] = IGI_2_NHM_TH(nhm_igi_th_11k_low[i]);
		break;

	case INTEL_XBOX:
		is_update = true;
		*igi_new = 0x36;
		for (i = 0; i < NHM_TH_NUM; i++)
			nhm_th[i] = IGI_2_NHM_TH(nhm_igi_th_xbox[i]);
		break;

	case NHM_DBG:	/*Get IGI form register*/
		igi_curr = (u8)odm_get_bb_reg(dm, R_0xc50, MASKBYTE0);
		if ((ccx->nhm_igi != igi_curr) || (ccx->nhm_app != nhm_app)) {
			is_update = true;
			*igi_new = (u32)igi_curr;
			nhm_th[0] = (u8)IGI_2_NHM_TH(igi_curr - CCA_CAP);
			for (i = 1; i <= 10; i++)
				nhm_th[i] = nhm_th[0] + IGI_2_NHM_TH(2 * i);
		}
		break;
	}

	if (is_update) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "[Update NHM_TH] igi_RSSI=%d\n",
			  IGI_2_RSSI(*igi_new));

		for (i = 0; i < NHM_TH_NUM; i++) {
			PHYDM_DBG(dm, DBG_ENV_MNTR, "NHM_th[%d](RSSI) = %d\n",
				  i, NTH_TH_2_RSSI(nhm_th[i]));
		}
	} else {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "No need to update NHM_TH\n");
	}
	return is_update;
}

void
phydm_nhm_set(
	void		*dm_void,
	enum nhm_inexclude_txon_all	include_tx,
	enum nhm_inexclude_cca_all	include_cca,
	enum nhm_divider_opt_all	divi_opt,
	enum nhm_application		nhm_app,
	u16		period
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info	*ccx = &dm->dm_ccx_info;
	u8	nhm_th[NHM_TH_NUM] = {0};
	u8	i = 0;
	u32	igi = 0x20;
	u32	reg1 = 0, reg2 = 0;
	u32	val_tmp = 0;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);

	PHYDM_DBG(dm, DBG_ENV_MNTR, "incld{tx, cca}={%d, %d}, divi_opt=%d, period=%d\n",
		  include_tx, include_cca, divi_opt, period);

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		reg1 = 0x994;
		reg2 = 0x990;
	} else {
		reg1 = 0x890;
		reg2 = 0x894;
	}

	/*Set disable_ignore_cca, disable_ignore_txon, ccx_en*/
	if ((include_tx != ccx->nhm_include_txon) ||
		(include_cca != ccx->nhm_include_cca) ||
		(divi_opt != ccx->nhm_divider_opt)) {

		val_tmp = (u32)BIT_2_BYTE(divi_opt, include_tx, include_cca, 1);
		pdm_set_reg(dm, reg1, 0xf00, val_tmp);

		ccx->nhm_include_txon = include_tx;
		ccx->nhm_include_cca = include_cca;
		ccx->nhm_divider_opt = divi_opt;
		/*
		PHYDM_DBG(dm, DBG_ENV_MNTR, "val_tmp=%d, incld{tx, cca}={%d, %d}, divi_opt=%d, period=%d\n",
		  val_tmp, include_tx, include_cca, divi_opt, period);

		PHYDM_DBG(dm, DBG_ENV_MNTR, "0x994=0x%x\n", odm_get_bb_reg(dm, 0x994, 0xf00));
		*/

	}

	/*Set NHM period*/
	if (period != ccx->nhm_period) {
		pdm_set_reg(dm, reg2, MASKHWORD, period);
		PHYDM_DBG(dm, DBG_ENV_MNTR, "Update NHM period ((%d)) -> ((%d))\n",
			  ccx->nhm_period, period);

		ccx->nhm_period = period;
	}

	/*Set NHM threshold*/
	if (phydm_nhm_th_update_chk(dm, nhm_app, &(nhm_th[0]), &igi)) {

		/*Pause IGI*/
		if ((nhm_app == NHM_BACKGROUND) || (nhm_app == NHM_ACS)) {
			PHYDM_DBG(dm, DBG_ENV_MNTR, "DIG Free Run\n");
		} else if (phydm_pause_func(dm, F00_DIG, PHYDM_PAUSE, PHYDM_PAUSE_LEVEL_1, 1, &igi) == PAUSE_FAIL) {
			PHYDM_DBG(dm, DBG_ENV_MNTR, "pause DIG Fail\n");
			return;
		} else {
			PHYDM_DBG(dm, DBG_ENV_MNTR, "pause DIG=0x%x\n", igi);
		}
		ccx->nhm_app = nhm_app;
		ccx->nhm_igi = (u8)igi;
		odm_move_memory(dm, &ccx->nhm_th[0], &nhm_th, NHM_TH_NUM);

		/*Set NHM th*/
		phydm_nhm_set_th_reg(dm);
	}
}

u8
phydm_nhm_mntr_set(
	void			*dm_void,
	struct nhm_para_info	*nhm_para
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	u16	nhm_time = 0;	/*unit: 4us*/

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);

	if (nhm_para->mntr_time == 0)
		return PHYDM_SET_FAIL;

	if (nhm_para->nhm_lv >= NHM_MAX_NUM) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "Wrong LV=%d\n", nhm_para->nhm_lv);
		return PHYDM_SET_FAIL;
	}

	if (phydm_nhm_racing_ctrl(dm, nhm_para->nhm_lv) == PHYDM_SET_FAIL)
		return PHYDM_SET_FAIL;

	if (nhm_para->mntr_time >= 262)
		nhm_time = NHM_PERIOD_MAX;
	else
		nhm_time = nhm_para->mntr_time * MS_TO_4US_RATIO;

	phydm_nhm_set(dm, nhm_para->incld_txon, nhm_para->incld_cca, nhm_para->div_opt, nhm_para->nhm_app, nhm_time);

	return PHYDM_SET_SUCCESS;
}

/*Environment Monitor*/
boolean
phydm_nhm_mntr_chk(
	void	*dm_void,
	u16	monitor_time		/*unit ms*/
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	struct nhm_para_info	nhm_para = {0};
	boolean			nhm_chk_result = false;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);

	if (ccx->nhm_manual_ctrl) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "NHM in manual ctrl\n");
		return nhm_chk_result;
	}

	if (ccx->nhm_app != NHM_BACKGROUND &&
	   ((ccx->nhm_trigger_time + MAX_ENV_MNTR_TIME) > dm->phydm_sys_up_time)) {

		PHYDM_DBG(dm, DBG_ENV_MNTR, "nhm_app=%d, trigger_time %d, sys_time=%d\n",
			  ccx->nhm_app, ccx->nhm_trigger_time, dm->phydm_sys_up_time);

		return nhm_chk_result;
	}

	/*[NHM get result & calculate Utility----------------------------*/
	if (phydm_nhm_get_result(dm)) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "Get NHM_rpt success\n");
		phydm_nhm_get_utility(dm);
	}

	/*[NHM trigger]-------------------------------------------------*/
	nhm_para.incld_txon = NHM_EXCLUDE_TXON;
	nhm_para.incld_cca = NHM_EXCLUDE_CCA;
	nhm_para.div_opt = NHM_CNT_ALL;
	nhm_para.nhm_app = NHM_BACKGROUND;
	nhm_para.nhm_lv	= NHM_LV_1;
	nhm_para.mntr_time = monitor_time;

	nhm_chk_result = phydm_nhm_mntr_set(dm, &nhm_para);

	return nhm_chk_result;
}

void
phydm_nhm_init(
	void			*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __FUNCTION__);
	PHYDM_DBG(dm, DBG_ENV_MNTR, "cur_igi=0x%x\n", dm->dm_dig_table.cur_ig_value);

	ccx->nhm_app = NHM_BACKGROUND;
	ccx->nhm_igi = 0xff;

	/*Set NHM threshold*/
	ccx->nhm_ongoing = false;
	ccx->nhm_set_lv = NHM_RELEASE;

	if (phydm_nhm_th_update_chk(dm, ccx->nhm_app, &(ccx->nhm_th[0]), (u32*)(&ccx->nhm_igi))) {
		phydm_nhm_set_th_reg(dm);
	}
	ccx->nhm_period = 0;

	ccx->nhm_include_cca = NHM_CCA_INIT;
	ccx->nhm_include_txon = NHM_TXON_INIT;
	ccx->nhm_divider_opt = NHM_CNT_INIT;

	ccx->nhm_manual_ctrl = 0;
	ccx->nhm_rpt_stamp = 0;
}

void
phydm_nhm_dbg(
	void		*dm_void,
	char		input[][16],
	u32		*_used,
	char		*output,
	u32		*_out_len,
	u32		input_num
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	struct nhm_para_info	nhm_para;
	char		help[] = "-h";
	u32		var1[10] = {0};
	u32		used = *_used;
	u32		out_len = *_out_len;
	boolean		nhm_rpt_success = true;
	u8		i;

	PHYDM_SSCANF(input[1], DCMD_DECIMAL, &var1[0]);

	if ((strcmp(input[1], help) == 0)) {
		PDM_SNPF(out_len, used, output + used, out_len - used, "NHM Basic-Trigger 262ms: {1}\n");
		PDM_SNPF(out_len, used, output + used, out_len - used, "NHM Adv-Trigger: {2} {Include TXON} {Include CCA}\n{0:Cnt_all, 1:Cnt valid} {App} {LV} {0~262ms}\n");
		PDM_SNPF(out_len, used, output + used, out_len - used, "NHM Get Result: {100}\n");
	} else if (var1[0] == 100) { /*Get NHM results*/

		PDM_SNPF(out_len, used, output + used, out_len - used, "IGI=0x%x, rpt_stamp=%d\n", ccx->nhm_igi, ccx->nhm_rpt_stamp);

		nhm_rpt_success = phydm_nhm_get_result(dm);

		if (nhm_rpt_success) {
			for (i = 0; i <= 11; i++) {
				PDM_SNPF(out_len, used, output + used, out_len - used, "nhm_rpt[%d] = %d (%d percent)\n",
				 	i, ccx->nhm_result[i],
					 (((ccx->nhm_result[i] * 100) + 128) >> 8));
			}
		} else {
			PDM_SNPF(out_len, used, output + used, out_len - used, "Get NHM_rpt Fail\n");
		}
		ccx->nhm_manual_ctrl = 0;

	} else {	/* NMH trigger */

		ccx->nhm_manual_ctrl = 1;

		if (var1[0] == 1)
		{
			nhm_para.incld_txon = NHM_EXCLUDE_TXON;
			nhm_para.incld_cca = NHM_EXCLUDE_CCA;
			nhm_para.div_opt = NHM_CNT_ALL;
			nhm_para.nhm_app = NHM_DBG;
			nhm_para.nhm_lv = NHM_LV_4;
			nhm_para.mntr_time = 262;
		}
		else
		{
			for (i=0; (i<7 && strnlen(input[i+1], 1)); i++)
				PHYDM_SSCANF(input[i+1], DCMD_DECIMAL, &var1[i]);

			nhm_para.incld_txon = (enum nhm_inexclude_txon_all)var1[1];
			nhm_para.incld_cca = (enum nhm_inexclude_cca_all)var1[2];
			nhm_para.div_opt = (enum nhm_divider_opt_all)var1[3];
			nhm_para.nhm_app = (enum nhm_application)var1[4];
			nhm_para.nhm_lv = (enum phydm_nhm_level)var1[5];
			nhm_para.mntr_time = (u16)var1[6];
		}

		PDM_SNPF(out_len, used, output + used, out_len - used, " txon=%d, cca=%d, dev=%d, app=%d, lv=%d, time=%d ms\n",
			nhm_para.incld_txon, nhm_para.incld_cca,
			nhm_para.div_opt, nhm_para.nhm_app, nhm_para.nhm_lv,
			nhm_para.mntr_time);

		if (phydm_nhm_mntr_set(dm, &nhm_para) == PHYDM_SET_SUCCESS) {
			phydm_nhm_trigger(dm);
		}

		PDM_SNPF(out_len, used, output + used, out_len - used, "IGI=0x%x, rpt_stamp=%d\n",
			ccx->nhm_igi, ccx->nhm_rpt_stamp);

		for (i = 0; i <= 10; i++) {
			PDM_SNPF(out_len, used, output + used, out_len - used, "NHM_th[%d] RSSI = %d\n",
				 i, NTH_TH_2_RSSI(ccx->nhm_th[i]));
		}
	}

	*_used = used;
	*_out_len = out_len;
}
#endif	/*#ifdef NHM_SUPPORT*/

#if 1

void
phydm_set_nhm_th_by_igi(
	void			*dm_void,
	u8				igi
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx_info = &dm->dm_ccx_info;
	u8	th_gap = 2 * IGI_TO_NHM_TH_MULTIPLIER;
	u8	i;

	ccx_info->echo_igi = igi;
	ccx_info->nhm_th[0] = (ccx_info->echo_igi - CCA_CAP) * IGI_TO_NHM_TH_MULTIPLIER;
	for (i = 1; i <= 10; i++)
		ccx_info->nhm_th[i] = ccx_info->nhm_th[0] + th_gap * i;
}



void
phydm_nhm_setting(
	void		*dm_void,
	u8	nhm_setting
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info	*ccx_info = &dm->dm_ccx_info;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __FUNCTION__);


	PHYDM_DBG(dm, DBG_ENV_MNTR, "IGI=0x%x\n", ccx_info->echo_igi);

	if (nhm_setting == SET_NHM_SETTING) {
		PHYDM_DBG(dm, DBG_ENV_MNTR,
		"NHM_th[H->L]=[0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]\n",
		ccx_info->nhm_th[10], ccx_info->nhm_th[9], ccx_info->nhm_th[8],
		ccx_info->nhm_th[7], ccx_info->nhm_th[6], ccx_info->nhm_th[5],
		ccx_info->nhm_th[4], ccx_info->nhm_th[3], ccx_info->nhm_th[2],
		ccx_info->nhm_th[1], ccx_info->nhm_th[0]);
	}

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		if (nhm_setting == SET_NHM_SETTING) {
			/*Set inexclude_cca, inexclude_txon*/
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, BIT(9), ccx_info->nhm_include_cca);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, BIT(10), ccx_info->nhm_include_txon);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, BIT(11), ccx_info->nhm_divider_opt);

			/*Set NHM period*/
			odm_set_bb_reg(dm, ODM_REG_CCX_PERIOD_11AC, MASKHWORD, ccx_info->nhm_period);

			/*Set NHM threshold*/ /*Unit: PWdB U(8,1)*/
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE0, ccx_info->nhm_th[0]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE1, ccx_info->nhm_th[1]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE2, ccx_info->nhm_th[2]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE3, ccx_info->nhm_th[3]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE0, ccx_info->nhm_th[4]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE1, ccx_info->nhm_th[5]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE2, ccx_info->nhm_th[6]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE3, ccx_info->nhm_th[7]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH8_11AC, MASKBYTE0, ccx_info->nhm_th[8]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, MASKBYTE2, ccx_info->nhm_th[9]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, MASKBYTE3, ccx_info->nhm_th[10]);

			/*CCX EN*/
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, BIT(8), CCX_EN);

		} else if (nhm_setting == STORE_NHM_SETTING) {
			/*Store pervious disable_ignore_cca, disable_ignore_txon*/
			ccx_info->nhm_inexclude_cca_restore = (enum nhm_inexclude_cca_all)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, BIT(9));
			ccx_info->nhm_inexclude_txon_restore = (enum nhm_inexclude_txon_all)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, BIT(10));

			/*Store pervious NHM period*/
			ccx_info->nhm_period_restore = (u16)odm_get_bb_reg(dm, ODM_REG_CCX_PERIOD_11AC, MASKHWORD);

			/*Store NHM threshold*/
			ccx_info->nhm_th_restore[0] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE0);
			ccx_info->nhm_th_restore[1] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE1);
			ccx_info->nhm_th_restore[2] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE2);
			ccx_info->nhm_th_restore[3] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE3);
			ccx_info->nhm_th_restore[4] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE0);
			ccx_info->nhm_th_restore[5] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE1);
			ccx_info->nhm_th_restore[6] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE2);
			ccx_info->nhm_th_restore[7] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE3);
			ccx_info->nhm_th_restore[8] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH8_11AC, MASKBYTE0);
			ccx_info->nhm_th_restore[9] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, MASKBYTE2);
			ccx_info->nhm_th_restore[10] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, MASKBYTE3);
		} else if (nhm_setting == RESTORE_NHM_SETTING) {
			/*Set disable_ignore_cca, disable_ignore_txon*/
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, BIT(9), ccx_info->nhm_inexclude_cca_restore);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, BIT(10), ccx_info->nhm_inexclude_txon_restore);

			/*Set NHM period*/
			odm_set_bb_reg(dm, ODM_REG_CCX_PERIOD_11AC, MASKHWORD, ccx_info->nhm_period);

			/*Set NHM threshold*/
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE0, ccx_info->nhm_th_restore[0]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE1, ccx_info->nhm_th_restore[1]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE2, ccx_info->nhm_th_restore[2]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE3, ccx_info->nhm_th_restore[3]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE0, ccx_info->nhm_th_restore[4]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE1, ccx_info->nhm_th_restore[5]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE2, ccx_info->nhm_th_restore[6]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE3, ccx_info->nhm_th_restore[7]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH8_11AC, MASKBYTE0, ccx_info->nhm_th_restore[8]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, MASKBYTE2, ccx_info->nhm_th_restore[9]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, MASKBYTE3, ccx_info->nhm_th_restore[10]);
		} else
			return;
	}

	else if (dm->support_ic_type & ODM_IC_11N_SERIES) {
		if (nhm_setting == SET_NHM_SETTING) {
			/*Set disable_ignore_cca, disable_ignore_txon*/
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, BIT(9), ccx_info->nhm_include_cca);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, BIT(10), ccx_info->nhm_include_txon);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, BIT(11), ccx_info->nhm_divider_opt);

			/*Set NHM period*/
			odm_set_bb_reg(dm, ODM_REG_CCX_PERIOD_11N, MASKHWORD, ccx_info->nhm_period);

			/*Set NHM threshold*/
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE0, ccx_info->nhm_th[0]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE1, ccx_info->nhm_th[1]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE2, ccx_info->nhm_th[2]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE3, ccx_info->nhm_th[3]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE0, ccx_info->nhm_th[4]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE1, ccx_info->nhm_th[5]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE2, ccx_info->nhm_th[6]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE3, ccx_info->nhm_th[7]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH8_11N, MASKBYTE0, ccx_info->nhm_th[8]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, MASKBYTE2, ccx_info->nhm_th[9]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, MASKBYTE3, ccx_info->nhm_th[10]);

			/*CCX EN*/
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, BIT(8), CCX_EN);
		} else if (nhm_setting == STORE_NHM_SETTING) {
			/*Store pervious disable_ignore_cca, disable_ignore_txon*/
			ccx_info->nhm_inexclude_cca_restore = (enum nhm_inexclude_cca_all)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, BIT(9));
			ccx_info->nhm_inexclude_txon_restore = (enum nhm_inexclude_txon_all)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, BIT(10));

			/*Store pervious NHM period*/
			ccx_info->nhm_period_restore = (u16)odm_get_bb_reg(dm, ODM_REG_CCX_PERIOD_11N, MASKHWORD);

			/*Store NHM threshold*/
			ccx_info->nhm_th_restore[0] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE0);
			ccx_info->nhm_th_restore[1] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE1);
			ccx_info->nhm_th_restore[2] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE2);
			ccx_info->nhm_th_restore[3] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE3);
			ccx_info->nhm_th_restore[4] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE0);
			ccx_info->nhm_th_restore[5] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE1);
			ccx_info->nhm_th_restore[6] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE2);
			ccx_info->nhm_th_restore[7] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE3);
			ccx_info->nhm_th_restore[8] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH8_11N, MASKBYTE0);
			ccx_info->nhm_th_restore[9] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, MASKBYTE2);
			ccx_info->nhm_th_restore[10] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, MASKBYTE3);

		} else if (nhm_setting == RESTORE_NHM_SETTING) {
			/*Set disable_ignore_cca, disable_ignore_txon*/
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, BIT(9), ccx_info->nhm_inexclude_cca_restore);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, BIT(10), ccx_info->nhm_inexclude_txon_restore);

			/*Set NHM period*/
			odm_set_bb_reg(dm, ODM_REG_CCX_PERIOD_11N, MASKHWORD, ccx_info->nhm_period_restore);

			/*Set NHM threshold*/
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE0, ccx_info->nhm_th_restore[0]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE1, ccx_info->nhm_th_restore[1]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE2, ccx_info->nhm_th_restore[2]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE3, ccx_info->nhm_th_restore[3]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE0, ccx_info->nhm_th_restore[4]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE1, ccx_info->nhm_th_restore[5]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE2, ccx_info->nhm_th_restore[6]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE3, ccx_info->nhm_th_restore[7]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH8_11N, MASKBYTE0, ccx_info->nhm_th_restore[8]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, MASKBYTE2, ccx_info->nhm_th_restore[9]);
			odm_set_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, MASKBYTE3, ccx_info->nhm_th_restore[10]);
		} else
			return;

	}
}

void
phydm_get_nhm_result(
	void		*dm_void
)
{
	struct dm_struct		*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx_info = &dm->dm_ccx_info;
	u32			value32;
	u8			i;

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		value32 = odm_read_4byte(dm, ODM_REG_NHM_CNT_11AC);
		ccx_info->nhm_result[0] = (u8)(value32 & MASKBYTE0);
		ccx_info->nhm_result[1] = (u8)((value32 & MASKBYTE1) >> 8);
		ccx_info->nhm_result[2] = (u8)((value32 & MASKBYTE2) >> 16);
		ccx_info->nhm_result[3] = (u8)((value32 & MASKBYTE3) >> 24);

		value32 = odm_read_4byte(dm, ODM_REG_NHM_CNT7_TO_CNT4_11AC);
		ccx_info->nhm_result[4] = (u8)(value32 & MASKBYTE0);
		ccx_info->nhm_result[5] = (u8)((value32 & MASKBYTE1) >> 8);
		ccx_info->nhm_result[6] = (u8)((value32 & MASKBYTE2) >> 16);
		ccx_info->nhm_result[7] = (u8)((value32 & MASKBYTE3) >> 24);

		value32 = odm_read_4byte(dm, ODM_REG_NHM_CNT11_TO_CNT8_11AC);
		ccx_info->nhm_result[8] = (u8)(value32 & MASKBYTE0);
		ccx_info->nhm_result[9] = (u8)((value32 & MASKBYTE1) >> 8);
		ccx_info->nhm_result[10] = (u8)((value32 & MASKBYTE2) >> 16);
		ccx_info->nhm_result[11] = (u8)((value32 & MASKBYTE3) >> 24);

		/*Get NHM duration*/
		value32 = odm_read_4byte(dm, ODM_REG_NHM_DUR_READY_11AC);
		ccx_info->nhm_duration = (u16)(value32 & MASKLWORD);

	}

	else if (dm->support_ic_type & ODM_IC_11N_SERIES) {
		value32 = odm_read_4byte(dm, ODM_REG_NHM_CNT_11N);
		ccx_info->nhm_result[0] = (u8)(value32 & MASKBYTE0);
		ccx_info->nhm_result[1] = (u8)((value32 & MASKBYTE1) >> 8);
		ccx_info->nhm_result[2] = (u8)((value32 & MASKBYTE2) >> 16);
		ccx_info->nhm_result[3] = (u8)((value32 & MASKBYTE3) >> 24);

		value32 = odm_read_4byte(dm, ODM_REG_NHM_CNT7_TO_CNT4_11N);
		ccx_info->nhm_result[4] = (u8)(value32 & MASKBYTE0);
		ccx_info->nhm_result[5] = (u8)((value32 & MASKBYTE1) >> 8);
		ccx_info->nhm_result[6] = (u8)((value32 & MASKBYTE2) >> 16);
		ccx_info->nhm_result[7] = (u8)((value32 & MASKBYTE3) >> 24);

		value32 = odm_read_4byte(dm, ODM_REG_NHM_CNT9_TO_CNT8_11N);
		ccx_info->nhm_result[8] = (u8)((value32 & MASKBYTE2) >> 16);
		ccx_info->nhm_result[9] = (u8)((value32 & MASKBYTE3) >> 24);

		value32 = odm_read_4byte(dm, ODM_REG_NHM_CNT10_TO_CNT11_11N);
		ccx_info->nhm_result[10] = (u8)((value32 & MASKBYTE2) >> 16);
		ccx_info->nhm_result[11] = (u8)((value32 & MASKBYTE3) >> 24);

		/*Get NHM duration*/
		value32 = odm_read_4byte(dm, ODM_REG_NHM_CNT10_TO_CNT11_11N);
		ccx_info->nhm_duration = (u16)(value32 & MASKLWORD);

	}

	/* sum all nhm_result */
	ccx_info->nhm_rpt_sum = 0;
	for (i = 0; i <= 11; i++)
		ccx_info->nhm_rpt_sum += ccx_info->nhm_result[i];

	PHYDM_DBG(dm, DBG_ENV_MNTR,
	"NHM_result=(H->L)[%d %d %d %d (igi) %d %d %d %d %d %d %d %d]\n",
		ccx_info->nhm_result[11], ccx_info->nhm_result[10], ccx_info->nhm_result[9],
		ccx_info->nhm_result[8], ccx_info->nhm_result[7], ccx_info->nhm_result[6],
		ccx_info->nhm_result[5], ccx_info->nhm_result[4], ccx_info->nhm_result[3],
		ccx_info->nhm_result[2], ccx_info->nhm_result[1], ccx_info->nhm_result[0]);

}

boolean
phydm_check_nhm_rdy(
	void		*dm_void
)
{
	struct dm_struct		*dm = (struct dm_struct *)dm_void;
	u8			i;
	boolean			is_ready = false;

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		if (odm_get_bb_reg(dm, ODM_REG_NHM_DUR_READY_11AC, BIT(16)))
			is_ready = 1;
	} else if (dm->support_ic_type & ODM_IC_11N_SERIES) {

		if (dm->support_ic_type == ODM_RTL8710B) {
			if (odm_get_bb_reg(dm, R_0x8b4, BIT(25)))
				is_ready = 1;
		} else {
			if (odm_get_bb_reg(dm, R_0x8b4, BIT(17)))
				is_ready = 1;
		}
	}
	PHYDM_DBG(dm, DBG_ENV_MNTR, "NHM rdy=%d\n", is_ready);
	return is_ready;
}

void
phydm_ccx_monitor_trigger(
	void			*dm_void,
	u16			monitor_time		/*unit ms*/
)
{
	u8			nhm_th[11], i, igi;
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx_info = &dm->dm_ccx_info;
	u16 	monitor_time_4us = 0;

	if (!(dm->support_ability & ODM_BB_ENV_MONITOR))
		return;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __FUNCTION__);

	if (monitor_time == 0)
		return;

	if (monitor_time >= 262)
		monitor_time_4us = 65534;
	else
		monitor_time_4us = monitor_time * MS_TO_4US_RATIO;

	/* check if NHM threshold is changed */
	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {

		nhm_th[0] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE0);
		nhm_th[1] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE1);
		nhm_th[2] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE2);
		nhm_th[3] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11AC, MASKBYTE3);
		nhm_th[4] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE0);
		nhm_th[5] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE1);
		nhm_th[6] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE2);
		nhm_th[7] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11AC, MASKBYTE3);
		nhm_th[8] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH8_11AC, MASKBYTE0);
		nhm_th[9] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, MASKBYTE2);
		nhm_th[10] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11AC, MASKBYTE3);
	} else if (dm->support_ic_type & ODM_IC_11N_SERIES) {

		nhm_th[0] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE0);
		nhm_th[1] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE1);
		nhm_th[2] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE2);
		nhm_th[3] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH3_TO_TH0_11N, MASKBYTE3);
		nhm_th[4] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE0);
		nhm_th[5] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE1);
		nhm_th[6] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE2);
		nhm_th[7] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH7_TO_TH4_11N, MASKBYTE3);
		nhm_th[8] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH8_11N, MASKBYTE0);
		nhm_th[9] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, MASKBYTE2);
		nhm_th[10] = (u8)odm_get_bb_reg(dm, ODM_REG_NHM_TH9_TH10_11N, MASKBYTE3);
	}

	for (i = 0; i <= 10; i++) {

		if (nhm_th[i] != ccx_info->nhm_th[i]) {
			PHYDM_DBG(dm, DBG_ENV_MNTR,
				"nhm_th[%d] != ccx_info->nhm_th[%d]!!\n", i, i);
		}
	}
	/*[NHM]*/
	igi = (u8)odm_get_bb_reg(dm, R_0xc50, MASKBYTE0);
	phydm_set_nhm_th_by_igi(dm, igi);

	ccx_info->nhm_period = monitor_time_4us;
	ccx_info->nhm_include_cca = NHM_EXCLUDE_CCA;
	ccx_info->nhm_include_txon = NHM_EXCLUDE_TXON;
	ccx_info->nhm_divider_opt = NHM_CNT_ALL;

	phydm_nhm_setting(dm, SET_NHM_SETTING);
	phydm_nhm_trigger(dm);

	/*[CLM]*/
	ccx_info->clm_period = monitor_time_4us;

	if (ccx_info->clm_mntr_mode == CLM_DRIVER_MNTR) {
		phydm_clm_setting(dm, ccx_info->clm_period);
		phydm_clm_trigger(dm);
	} else if (ccx_info->clm_mntr_mode == CLM_FW_MNTR){
		phydm_clm_h2c(dm, monitor_time_4us, true);
	}

}

void
phydm_ccx_monitor_result(
	void			*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx_info = &dm->dm_ccx_info;
	u32					clm_result_tmp = 0;

	if (!(dm->support_ability & ODM_BB_ENV_MONITOR))
		return;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "%s ======>\n", __func__);

	if (phydm_check_nhm_rdy(dm)) {
		phydm_get_nhm_result(dm);

		if (ccx_info->nhm_rpt_sum != 0)
			ccx_info->nhm_ratio  = (u8)(((ccx_info->nhm_rpt_sum - ccx_info->nhm_result[0])*100) >> 8);
	}

	if (ccx_info->clm_mntr_mode == CLM_DRIVER_MNTR) {

		if (!phydm_clm_check_rdy(dm))
			goto out;

		phydm_clm_get_result(dm);

		if (ccx_info->clm_period != 0) {
			if (ccx_info->clm_period == 64000)
				ccx_info->clm_ratio = (u8)(((ccx_info->clm_result >> 6) + 5) /10);
			else if (ccx_info->clm_period == 65535) {
				clm_result_tmp = (u32)(ccx_info->clm_result * 100);
				ccx_info->clm_ratio = (u8)((clm_result_tmp + (1<<15)) >> 16);
			} else
				ccx_info->clm_ratio = (u8)((ccx_info->clm_result*100) / ccx_info->clm_period);
		}

	} else {
		if (ccx_info->clm_fw_result_cnt != 0)
			ccx_info->clm_ratio = (u8)(ccx_info->clm_fw_result_acc /ccx_info->clm_fw_result_cnt);
		else
			ccx_info->clm_ratio = 0;

		PHYDM_DBG(dm, DBG_ENV_MNTR, "clm_fw_result_acc=%d, clm_fw_result_cnt=%d\n",
			ccx_info->clm_fw_result_acc, ccx_info->clm_fw_result_cnt);

		ccx_info->clm_fw_result_acc = 0;
		ccx_info->clm_fw_result_cnt = 0;
	}

out:
	PHYDM_DBG(dm, DBG_ENV_MNTR, "IGI=0x%x, nhm_ratio=%d, clm_ratio=%d\n\n",
		ccx_info->echo_igi, ccx_info->nhm_ratio, ccx_info->clm_ratio);

}


#endif


#ifdef CLM_SUPPORT

void
phydm_clm_racing_release(
	void			*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	u32	value32;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);
	PHYDM_DBG(dm, DBG_ENV_MNTR, "lv:(%d)->(0)\n", ccx->clm_set_lv);

	ccx->clm_ongoing = false;
	ccx->clm_set_lv = CLM_RELEASE;
	ccx->clm_app = CLM_BACKGROUND;
}

u8
phydm_clm_racing_ctrl(
	void			*dm_void,
	enum phydm_nhm_level	 clm_lv
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	u8	set_result = PHYDM_SET_SUCCESS;
	/*acquire to control CLM API*/

	PHYDM_DBG(dm, DBG_ENV_MNTR, "clm_ongoing=%d, lv:(%d)->(%d)\n",
		  ccx->clm_ongoing, ccx->clm_set_lv, clm_lv);
	if (ccx->clm_ongoing) {
		if (clm_lv <= ccx->clm_set_lv) {
			set_result = PHYDM_SET_FAIL;
		} else {
			phydm_ccx_hw_restart(dm);
			ccx->clm_ongoing = false;
		}
	}

	if (set_result)
		ccx->clm_set_lv = clm_lv;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "clm racing success=%d\n", set_result);
	return set_result;
}


void
phydm_clm_c2h_report_handler(
	void	*dm_void,
	u8	*cmd_buf,
	u8	cmd_len
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info			*ccx_info = &dm->dm_ccx_info;
	u8	clm_report = cmd_buf[0];
	u8	clm_report_idx = cmd_buf[1];

	if (cmd_len >=12)
		return;

	ccx_info->clm_fw_result_acc += clm_report;
	ccx_info->clm_fw_result_cnt++;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%d] clm_report= %d\n", ccx_info->clm_fw_result_cnt, clm_report);

}

void
phydm_clm_h2c(
	void	*dm_void,
	u16	obs_time,
	u8	fw_clm_en
)
{
	struct dm_struct		*dm = (struct dm_struct *)dm_void;
	u8		h2c_val[H2C_MAX_LENGTH] = {0};
	u8		i = 0;
	u8		obs_time_idx = 0;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s] ======>\n", __func__);
	PHYDM_DBG(dm, DBG_ENV_MNTR, "obs_time_index=%d *4 us\n", obs_time);

	for (i =1; i<=16; i++) {
		if (obs_time & BIT(16 -i)) {
			obs_time_idx = 16-i;
			break;
		}
	}

	/*
	obs_time =(2^16 -1) ~ (2^15)  => obs_time_idx = 15  (65535 ~ 32768)
	obs_time =(2^15 -1) ~ (2^14)  => obs_time_idx = 14
	...
	...
	...
	obs_time =(2^1 -1) ~ (2^0)  => obs_time_idx = 0

	*/

	h2c_val[0] = obs_time_idx | (((fw_clm_en) ? 1 : 0)<< 7);
	h2c_val[1] = CLM_MAX_REPORT_TIME;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "PHYDM h2c[0x4d]=0x%x %x %x %x %x %x %x\n",
		h2c_val[6], h2c_val[5], h2c_val[4], h2c_val[3], h2c_val[2], h2c_val[1], h2c_val[0]);

	odm_fill_h2c_cmd(dm, PHYDM_H2C_FW_CLM_MNTR, H2C_MAX_LENGTH, h2c_val);

}

void
phydm_clm_setting(
	void			*dm_void,
	u16			clm_period	/*4us sample 1 time*/
)
{
	struct dm_struct		*dm = (struct dm_struct *)dm_void;
	struct ccx_info	*ccx = &dm->dm_ccx_info;

	if (ccx->clm_period != clm_period) {

		if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
			odm_set_bb_reg(dm, R_0x990, MASKLWORD, clm_period);

		} else if (dm->support_ic_type & ODM_IC_11N_SERIES) {
			odm_set_bb_reg(dm, R_0x894, MASKLWORD, clm_period);
		}

		ccx->clm_period = clm_period;
		PHYDM_DBG(dm, DBG_ENV_MNTR, "Update CLM period ((%d)) -> ((%d))\n",
			  ccx->clm_period, clm_period);
	}

	PHYDM_DBG(dm, DBG_ENV_MNTR, "Set CLM period=%d * 4us\n", ccx->clm_period);

}

void
phydm_clm_trigger(
	void			*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	u32	reg1 = (dm->support_ic_type & ODM_IC_11AC_SERIES) ? R_0x994 : R_0x890;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);

	odm_set_bb_reg(dm, reg1, BIT(0), 0x0);
	odm_set_bb_reg(dm, reg1, BIT(0), 0x1);

	ccx->clm_trigger_time = dm->phydm_sys_up_time;
	ccx->clm_rpt_stamp++;
	ccx->clm_ongoing = true;
}

boolean
phydm_clm_check_rdy(
	void			*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	boolean			is_ready = false;
	u32			reg1 = 0, reg1_bit = 0;

	if (dm->support_ic_type & ODM_IC_11AC_SERIES) {
		reg1 = ODM_REG_CLM_RESULT_11AC;
		reg1_bit = 16;
	} else if (dm->support_ic_type & ODM_IC_11N_SERIES) {
		if (dm->support_ic_type == ODM_RTL8710B) {
			reg1 = R_0x8b4;
			reg1_bit = 24;
		} else {
			reg1 = R_0x8b4;
			reg1_bit = 16;
		}
	}
	PHYDM_DBG(dm, DBG_ENV_MNTR, "CLM rdy=%d\n", is_ready);
	return is_ready;
}

void
phydm_clm_get_utility(
	void		*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	u32	clm_result_tmp;

	if (ccx->clm_period == 0) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "[warning] clm_period = 0\n");
		ccx->clm_ratio = 0;
	} else if (ccx->clm_period >= 65530) {
		clm_result_tmp = (u32)(ccx->clm_result * 100);
		ccx->clm_ratio = (u8)((clm_result_tmp + (1<<15)) >> 16);
	} else
		ccx->clm_ratio = (u8)((ccx->clm_result*100) / ccx->clm_period);
}

boolean
phydm_clm_get_result(
	void			*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx_info = &dm->dm_ccx_info;
	u32	reg1 = (dm->support_ic_type & ODM_IC_11AC_SERIES) ? R_0x994 : R_0x890;

	odm_set_bb_reg(dm, reg1, BIT(0), 0x0);
	if (phydm_clm_check_rdy(dm) == false) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "Get CLM report Fail\n");
		phydm_clm_racing_release(dm);
		return false;
	}

	if (dm->support_ic_type & ODM_IC_11AC_SERIES)
		ccx_info->clm_result = (u16)odm_get_bb_reg(dm, R_0xfa4, MASKLWORD);
	else if (dm->support_ic_type & ODM_IC_11N_SERIES)
		ccx_info->clm_result = (u16)odm_get_bb_reg(dm, R_0x8d0, MASKLWORD);


	PHYDM_DBG(dm, DBG_ENV_MNTR, "CLM result = %d *4 us\n", ccx_info->clm_result);
	phydm_clm_racing_release(dm);
	return true;
}

void
phydm_clm_mntr_fw(
	void	*dm_void,
	u16	monitor_time	/*unit ms*/
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	u32			clm_result_tmp = 0;

	/*[Get CLM report]*/
	if (ccx->clm_fw_result_cnt != 0)
		ccx->clm_ratio = (u8)(ccx->clm_fw_result_acc /ccx->clm_fw_result_cnt);
	else
		ccx->clm_ratio = 0;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "clm_fw_result_acc=%d, clm_fw_result_cnt=%d\n",
		ccx->clm_fw_result_acc, ccx->clm_fw_result_cnt);

	ccx->clm_fw_result_acc = 0;
	ccx->clm_fw_result_cnt = 0;


	/*[CLM trigger]*/
	if (monitor_time >= 262)
		ccx->clm_period = 65535;
	else
		ccx->clm_period = monitor_time * MS_TO_4US_RATIO;

	phydm_clm_h2c(dm, monitor_time, true);

}

u8
phydm_clm_mntr_set(
	void	*dm_void,
	struct	clm_para_info	*clm_para
)
{
	/*Driver Monitor CLM*/
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	u16			clm_period = 0;


	if (clm_para->mntr_time == 0)
		return PHYDM_SET_FAIL;

	if (clm_para->clm_lv >= CLM_MAX_NUM) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "[WARNING] Wrong LV=%d\n", clm_para->clm_lv);
		return PHYDM_SET_FAIL;
	}

	if (phydm_clm_racing_ctrl(dm, (enum phydm_nhm_level)clm_para->clm_lv) == PHYDM_SET_FAIL)
		return PHYDM_SET_FAIL;

	if (clm_para->mntr_time >= 262)
		clm_period = CLM_PERIOD_MAX;
	else
		clm_period = clm_para->mntr_time * MS_TO_4US_RATIO;

	ccx->clm_app = clm_para->clm_app;
	phydm_clm_setting(dm, clm_period);

	return PHYDM_SET_SUCCESS;
}

boolean
phydm_clm_mntr_chk(
	void	*dm_void,
	u16	monitor_time	/*unit ms*/
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	struct clm_para_info	clm_para = {0};
	u32			clm_result_tmp = 0;
	boolean			clm_chk_result = false;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s] ======>\n", __func__);

	if (ccx->clm_manual_ctrl) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "CLM in manual ctrl\n");
		return clm_chk_result;
	}

	if ((ccx->clm_app != CLM_BACKGROUND) &&
	    (ccx->clm_trigger_time + MAX_ENV_MNTR_TIME) > dm->phydm_sys_up_time) {

		PHYDM_DBG(dm, DBG_ENV_MNTR, "trigger_time %d, sys_time=%d\n",
			  ccx->clm_trigger_time, dm->phydm_sys_up_time);

		return clm_chk_result;
	}

	clm_para.clm_app = CLM_BACKGROUND;
	clm_para.clm_lv	= CLM_LV_1;
	clm_para.mntr_time = monitor_time;

	if (ccx->clm_mntr_mode == CLM_DRIVER_MNTR) {

		/*[Get CLM report]*/
		if (phydm_clm_get_result(dm)) {
			PHYDM_DBG(dm, DBG_ENV_MNTR, "Get CLM_rpt success\n");
			phydm_clm_get_utility(dm);
		}

		/*[CLM trigger]-------------------------------------------------*/
		if (phydm_clm_mntr_set(dm, &clm_para) == PHYDM_SET_SUCCESS) {
			clm_chk_result = true;
		}
	} else {
		phydm_clm_mntr_fw(dm, monitor_time);
	}

	PHYDM_DBG(dm, DBG_ENV_MNTR, "clm_ratio=%d\n", ccx->clm_ratio);
	return clm_chk_result;
}

void
phydm_set_clm_mntr_mode(
	void			*dm_void,
	enum clm_monitor_mode mode
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx_info = &dm->dm_ccx_info;

	if (ccx_info->clm_mntr_mode != mode) {

		ccx_info->clm_mntr_mode = mode;
		phydm_ccx_hw_restart(dm);

		if (mode == CLM_DRIVER_MNTR)
			phydm_clm_h2c(dm,0, 0);
	}
}

void
phydm_clm_init(
	void			*dm_void
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __FUNCTION__);

	ccx->clm_ongoing = false;
	ccx->clm_manual_ctrl = 0;
	ccx->clm_mntr_mode = CLM_DRIVER_MNTR;
	ccx->clm_period = 0;
	ccx->clm_rpt_stamp = 0;
	phydm_clm_setting(dm, 65535);
}

void
phydm_clm_dbg(
	void		*dm_void,
	char		input[][16],
	u32		*_used,
	char		*output,
	u32		*_out_len,
	u32		input_num
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info	*ccx = &dm->dm_ccx_info;
	char		help[] = "-h";
	u32		var1[10] = {0};
	u32		used = *_used;
	u32		out_len = *_out_len;
	struct clm_para_info	clm_para = {0};
	u32		i;

	for (i=0; (i<4 && strnlen(input[i+1], 1)); i++)
		PHYDM_SSCANF(input[i+1], DCMD_DECIMAL, &var1[i]);

	if ((strcmp(input[1], help) == 0)) {
		PDM_SNPF(out_len, used, output + used, out_len - used, "CLM Driver Basic-Trigger 262ms: {1}\n");
		PDM_SNPF(out_len, used, output + used, out_len - used, "CLM Driver Adv-Trigger: {2} {app} {LV} {0~262ms}\n");
		PDM_SNPF(out_len, used, output + used, out_len - used, "CLM FW Trigger: {3}\n");
		PDM_SNPF(out_len, used, output + used, out_len - used, "CLM Get Result: {100}\n");
	} else if (var1[0] == 100) { /* Get CLM results */

		if (phydm_clm_get_result(dm)) {
			phydm_clm_get_utility(dm);
		}

		PDM_SNPF(out_len, used, output + used, out_len - used, "clm_rpt_stamp=%d\n",
			 ccx->clm_rpt_stamp);

		PDM_SNPF(out_len, used, output + used, out_len - used, "clm_ratio:((%d percent)) = (%d us/ %d us)\n",
			ccx->clm_ratio, ccx->clm_result<<2, ccx->clm_period<<2);

		ccx->clm_manual_ctrl = 0;

	} else { /* Set & trigger CLM */
		ccx->clm_manual_ctrl = 1;

		if (var1[0] == 1) {
			clm_para.clm_app = CLM_BACKGROUND;
			clm_para.clm_lv	= CLM_LV_4;
			clm_para.mntr_time = 262;
			ccx->clm_mntr_mode = CLM_DRIVER_MNTR;

		} else if (var1[0] == 2) {
			clm_para.clm_app = (enum clm_application )var1[1];
			clm_para.clm_lv	= (enum phydm_clm_level )var1[2];
			ccx->clm_mntr_mode = CLM_DRIVER_MNTR;
			clm_para.mntr_time = (u16)var1[3];

		} else if (var1[0] == 3) {
			clm_para.clm_app = CLM_BACKGROUND;
			clm_para.clm_lv	= CLM_LV_4;
			ccx->clm_mntr_mode = CLM_FW_MNTR;
			clm_para.mntr_time = 262;
		}

		PDM_SNPF(out_len, used, output + used, out_len - used, "app=%d, lv=%d, mode=%s, time=%d ms\n",
			  clm_para.clm_app, clm_para.clm_lv,
			  ((ccx->clm_mntr_mode == CLM_FW_MNTR) ? "FW" : "driver"),
			  clm_para.mntr_time);

		if (phydm_clm_mntr_set(dm, &clm_para) == PHYDM_SET_SUCCESS) {
			phydm_clm_trigger(dm);
			/**/
		}

		PDM_SNPF(out_len, used, output + used, out_len - used, "clm_rpt_stamp=%d\n",
			 ccx->clm_rpt_stamp);

	}

	*_used = used;
	*_out_len = out_len;
}


#endif	/*#ifdef CLM_SUPPORT*/

u8
phydm_env_mntr_trigger(
	void			*dm_void,
	struct	nhm_para_info	*nhm_para,
	struct	clm_para_info	*clm_para,
	struct	env_trig_rpt	*trig_rpt
)
{
#if(defined(NHM_SUPPORT) && defined(CLM_SUPPORT))
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	boolean			nhm_set_ok = false;
	boolean			clm_set_ok = false;
	u8			trigger_result = 0;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s] ======>\n", __func__);
	/*[NHM]*/
	nhm_set_ok = phydm_nhm_mntr_set(dm, nhm_para);

	/*[CLM]*/
	if (ccx->clm_mntr_mode == CLM_DRIVER_MNTR) {
		clm_set_ok = phydm_clm_mntr_set(dm, clm_para);
	} else if (ccx->clm_mntr_mode == CLM_FW_MNTR){
		phydm_clm_h2c(dm, CLM_PERIOD_MAX, true);
		trigger_result |= CLM_SUCCESS;
	}

	if (nhm_set_ok) {
		phydm_nhm_trigger(dm);
		trigger_result |= NHM_SUCCESS;
	}

	if (clm_set_ok) {
		phydm_clm_trigger(dm);
		trigger_result |= CLM_SUCCESS;
	}

	trig_rpt->nhm_rpt_stamp = ccx->nhm_rpt_stamp;
	trig_rpt->clm_rpt_stamp = ccx->clm_rpt_stamp;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "nhm_rpt_stamp=%d, clm_rpt_stamp=%d,\n\n",
		  trig_rpt->nhm_rpt_stamp, trig_rpt->clm_rpt_stamp);

	return trigger_result;
#endif
}

u8
phydm_env_mntr_result(
	void			*dm_void,
	struct	env_mntr_rpt	*rpt
)
{
#if(defined(NHM_SUPPORT) && defined(CLM_SUPPORT))
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	u8			env_mntr_rpt = 0;
	u32			clm_result_tmp = 0;
	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s] ======>\n", __func__);
	/*Get NHM result*/
	if (phydm_nhm_get_result(dm)) {
		PHYDM_DBG(dm, DBG_ENV_MNTR, "Get NHM_rpt success\n");
		phydm_nhm_get_utility(dm);
		rpt->nhm_ratio = ccx->nhm_ratio;
		env_mntr_rpt |= NHM_SUCCESS;
		odm_move_memory(dm, &rpt->nhm_result[0], &ccx->nhm_result[0], NHM_RPT_NUM);
	} else {
		rpt->nhm_ratio = ENV_MNTR_FAIL;
	}

	/*Get CLM result*/
	if (ccx->clm_mntr_mode == CLM_DRIVER_MNTR) {

		if (phydm_clm_get_result(dm)) {
			PHYDM_DBG(dm, DBG_ENV_MNTR, "Get CLM_rpt success\n");
			phydm_clm_get_utility(dm);
			env_mntr_rpt |= CLM_SUCCESS;
			rpt->clm_ratio = ccx->clm_ratio;
		} else {
			rpt->clm_ratio = ENV_MNTR_FAIL;
		}

	} else {
		if (ccx->clm_fw_result_cnt != 0)
			ccx->clm_ratio = (u8)(ccx->clm_fw_result_acc /ccx->clm_fw_result_cnt);
		else
			ccx->clm_ratio = 0;

		rpt->clm_ratio = ccx->clm_ratio;
		PHYDM_DBG(dm, DBG_ENV_MNTR, "clm_fw_result_acc=%d, clm_fw_result_cnt=%d\n",
			ccx->clm_fw_result_acc, ccx->clm_fw_result_cnt);

		ccx->clm_fw_result_acc = 0;
		ccx->clm_fw_result_cnt = 0;
		env_mntr_rpt |= CLM_SUCCESS;
	}

	rpt->nhm_rpt_stamp = ccx->nhm_rpt_stamp;
	rpt->clm_rpt_stamp = ccx->clm_rpt_stamp;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "IGI=0x%x, nhm_ratio=%d, clm_ratio=%d, nhm_rpt_stamp=%d, clm_rpt_stamp=%d\n\n",
		ccx->nhm_igi, rpt->nhm_ratio, rpt->clm_ratio, rpt->nhm_rpt_stamp, rpt->clm_rpt_stamp);

	return env_mntr_rpt;
#endif
}

/*Environment Monitor*/
void
phydm_env_mntr_watchdog(
	void	*dm_void
)
{
#if(defined(NHM_SUPPORT) && defined(CLM_SUPPORT))
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info		*ccx = &dm->dm_ccx_info;
	boolean			nhm_chk_ok = false;
	boolean			clm_chk_ok = false;

	if (!(dm->support_ability & ODM_BB_ENV_MONITOR))
		return;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __func__);

	nhm_chk_ok = phydm_nhm_mntr_chk(dm, 262);/*monitor 262ms*/
	clm_chk_ok = phydm_clm_mntr_chk(dm, 262); /*monitor 262ms*/

	if (nhm_chk_ok)
		phydm_nhm_trigger(dm);

	if (clm_chk_ok)
		phydm_clm_trigger(dm);

	PHYDM_DBG(dm, DBG_ENV_MNTR, "Summary: nhm_ratio=((%d)) clm_ratio=((%d))\n\n",
		  ccx->nhm_ratio, ccx->clm_ratio);
#endif
}


void
phydm_env_monitor_init(
	void			*dm_void
)
{
#if(defined(NHM_SUPPORT) && defined(CLM_SUPPORT))
	struct dm_struct	*dm = (struct dm_struct *)dm_void;

	if (!(dm->support_ability & ODM_BB_ENV_MONITOR))
		return;

	PHYDM_DBG(dm, DBG_ENV_MNTR, "[%s]===>\n", __FUNCTION__);

	phydm_ccx_hw_restart(dm);
	phydm_nhm_init(dm);
	phydm_clm_init(dm);
#endif
}

void
phydm_env_mntr_dbg(
	void		*dm_void,
	char		input[][16],
	u32		*_used,
	char		*output,
	u32		*_out_len,
	u32		input_num
)
{
	struct dm_struct	*dm = (struct dm_struct *)dm_void;
	struct ccx_info	*ccx = &dm->dm_ccx_info;
	char		help[] = "-h";
	u32		var1[10] = {0};
	u32		used = *_used;
	u32		out_len = *_out_len;
	struct clm_para_info	clm_para = {0};
	struct nhm_para_info	nhm_para = {0};
	struct env_mntr_rpt	rpt = {0};
	struct env_trig_rpt	trig_rpt = {0};
	u8		set_result;
	u8		i;

	PHYDM_SSCANF(input[1], DCMD_DECIMAL, &var1[0]);

	if ((strcmp(input[1], help) == 0)) {
		PDM_SNPF(out_len, used, output + used, out_len - used, "Basic-Trigger 262ms: {1}\n");
		PDM_SNPF(out_len, used, output + used, out_len - used, "Get Result: {100}\n");
	} else if (var1[0] == 100) { /* Get CLM results */

		set_result = phydm_env_mntr_result(dm, &rpt);

		PDM_SNPF(out_len, used, output + used, out_len - used, "Set Result=%d\n nhm_ratio=%d clm_ratio=%d\n nhm_rpt_stamp=%d, clm_rpt_stamp=%d, \n",
			set_result, rpt.nhm_ratio, rpt.clm_ratio, rpt.nhm_rpt_stamp, rpt.clm_rpt_stamp);

		for (i = 0; i <= 11; i++) {
			PDM_SNPF(out_len, used, output + used, out_len - used, "nhm_rpt[%d] = %d (%d percent)\n",
			 	i, rpt.nhm_result[i],
				 (((rpt.nhm_result[i] * 100) + 128) >> 8));
		}

	} else { /* Set & trigger CLM */
		/*nhm para*/
		nhm_para.incld_txon = NHM_EXCLUDE_TXON;
		nhm_para.incld_cca = NHM_EXCLUDE_CCA;
		nhm_para.div_opt = NHM_CNT_ALL;
		nhm_para.nhm_app = NHM_ACS;
		nhm_para.nhm_lv	= NHM_LV_2;
		nhm_para.mntr_time = 262;

		/*clm para*/
		clm_para.clm_app = CLM_ACS;
		clm_para.clm_lv	= CLM_LV_2;
		clm_para.mntr_time = 262;

		set_result = phydm_env_mntr_trigger(dm, &nhm_para, &clm_para, &trig_rpt);

		PDM_SNPF(out_len, used, output + used, out_len - used, "Set Result=%d, nhm_rpt_stamp=%d, clm_rpt_stamp=%d\n",
			set_result, trig_rpt.nhm_rpt_stamp, trig_rpt.clm_rpt_stamp);
	}

	*_used = used;
	*_out_len = out_len;
}
