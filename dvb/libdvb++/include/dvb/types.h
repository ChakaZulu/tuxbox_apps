/*
 * $Id: types.h,v 1.2 2004/09/03 13:50:33 mws Exp $
 *
 * Copyright (C) 2002, 2003 Andreas Oberritter <obi@saftware.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __dvb_types_h__
#define __dvb_types_h__

#include <inttypes.h>

/*
 * do never rely on *_UNKNOWN values!
 * they are used as defaults and indicate
 * errors if made public.
 */

typedef enum nim_dvb_type {
	NIM_DVB_UNKNOWN,
	NIM_DVB_C,
	NIM_DVB_S,
	NIM_DVB_T
} nim_dvb_type_t;

typedef enum nim_sec_type {
	NIM_SEC_UNKNOWN		= 0x00000000UL,
	NIM_SEC_SIMPLE		= 0x00000100UL,	/* 13/14V, 0/22kHz             */
	NIM_SEC_TONEBURST	= 0x00000200UL,	/* Mini-DiSEqC A/B             */
	NIM_SEC_SMATV		= 0x00000400UL,
	NIM_SEC_VOLTAGE_SWITCH	= 0x00001000UL,	/* use voltage as sat switch   */
	NIM_SEC_TONE_SWITCH	= 0x00002000UL,	/* use tone as sat switch      */
	NIM_SEC_DISEQC_1_0	= 0x00010000UL,	/* 1-way pure DiSEqC           */
	NIM_SEC_DISEQC_1_1	= 0x00020000UL,	/* 1-way pure DiSEqC cascaded  */
	NIM_SEC_DISEQC_1_2	= 0x00030000UL,	/* 1-way pure DiSEqC motorized */
	NIM_SEC_DISEQC_1_3	= 0x00040000UL,
	NIM_SEC_DISEQC_1_MASK	= 0x00070000UL,
	NIM_SEC_DISEQC_2_0	= 0x01000000UL,	/* 2-way pure DiSEqC           */
	NIM_SEC_DISEQC_2_1	= 0x02000000UL,	/* 2-way pure DiSEqC cascaded  */
	NIM_SEC_DISEQC_2_2	= 0x03000000UL,	/* 2-way pure DiSEqC motorized */
	NIM_SEC_DISEQC_2_3	= 0x04000000UL,
	NIM_SEC_DISEQC_2_MASK	= 0x07000000UL,
	NIM_SEC_DISEQC_MASK	= 0x07070000UL
} nim_sec_type_t;

typedef enum nim_modulation_cable {
	NIM_MODULATION_C_UNKNOWN,
	NIM_MODULATION_C_QAM16,
	NIM_MODULATION_C_QAM32,
	NIM_MODULATION_C_QAM64,
	NIM_MODULATION_C_QAM128,
	NIM_MODULATION_C_QAM256
} nim_modulation_cable_t;

typedef enum nim_modulation_satellite {
	NIM_MODULATION_S_UNKNOWN,
	NIM_MODULATION_S_QPSK
} nim_modulation_satellite_t;

typedef enum nim_fec_outer {
	NIM_FEC_O_UNKNOWN,
	NIM_FEC_O_NONE,
	NIM_FEC_O_RS
} nim_fec_outer_t;

typedef enum nim_fec_inner {
	NIM_FEC_I_UNKNOWN,
	NIM_FEC_I_1_2,
	NIM_FEC_I_2_3,
	NIM_FEC_I_3_4,
	NIM_FEC_I_5_6,
	NIM_FEC_I_7_8,
	NIM_FEC_I_NONE = 0x0f
} nim_fec_inner_t;

typedef enum nim_west_east_flag {
	NIM_WEST,
	NIM_EAST
} nim_west_east_flag_t;

typedef enum nim_polarization {
	NIM_POLARIZATION_H,
	NIM_POLARIZATION_V,
	NIM_POLARIZATION_L,
	NIM_POLARIZATION_R
} nim_polarization_t;

typedef enum nim_band {
	NIM_BAND_UNKNOWN,
	NIM_BAND_LOW,
	NIM_BAND_HIGH
} nim_band_t;

typedef enum nim_bandwidth {
	NIM_BANDWIDTH_8MHZ,
	NIM_BANDWIDTH_7MHZ,
	NIM_BANDWIDTH_6MHZ
} nim_bandwidth_t;

typedef enum nim_constellation {
	NIM_CONSTELLATION_QPSK,
	NIM_CONSTELLATION_QAM16,
	NIM_CONSTELLATION_QAM64
} nim_constellation_t;

typedef enum nim_guard_interval {
	NIM_GUARD_INTERVAL_1_32,
	NIM_GUARD_INTERVAL_1_16,
	NIM_GUARD_INTERVAL_1_8,
	NIM_GUARD_INTERVAL_1_4,
	NIM_GUARD_INTERVAL_AUTO
} nim_guard_interval_t;

typedef enum nim_transmit_mode {
	NIM_TRANSMISSION_MODE_2K,
	NIM_TRANSMISSION_MODE_8K,
	NIM_TRANSMISSION_MODE_AUTO
} nim_transmit_mode_t;

typedef enum nim_hierarchy {
	NIM_HIERARCHY_NONE,
	NIM_HIERARCHY_1,
	NIM_HIERARCHY_2,
	NIM_HIERARCHY_4,
	NIM_HIERARCHY_AUTO
} nim_hierarchy_t;

typedef enum nim_otherfrequencyflag {
	NIM_OTHERFREQ_NO = 0x00,
	NIM_OTHERFREQ_YES
} nim_otherfrequencyflag_t;

typedef enum nim_diseqc_addr {
	NIM_DISEQC_ADDR_UNKNOWN,
	NIM_DISEQC_ADDR1,
	NIM_DISEQC_ADDR2,
	NIM_DISEQC_ADDR3,
	NIM_DISEQC_ADDR4
} nim_diseqc_addr_t;

typedef enum nim_status {
	NIM_STATUS_UNKNOWN = 0x00,
	NIM_STATUS_SYNC = 0x01,
	NIM_STATUS_LOCK = 0x02
} nim_status_t;

typedef struct nim_lnb_lof {
	uint32_t lof_low;
	uint32_t lof_high;
	uint32_t lof_offset;
} nim_lnb_lof_t;

typedef uint32_t nim_frequency_t;
typedef uint32_t nim_symbol_rate_t;
typedef uint16_t nim_orbital_position_t;

struct ts_parameters_cable {
	nim_frequency_t			frequency;
	nim_fec_outer_t			fecOuter;
	nim_modulation_cable_t		modulation;
	nim_symbol_rate_t		symbolRate;
	nim_fec_inner_t			fecInner;
};

struct ts_parameters_satellite {
	nim_frequency_t			frequency;
	nim_orbital_position_t		orbitalPosition;
	nim_west_east_flag_t		westEastFlag;
	nim_polarization_t		polarization;
	nim_modulation_satellite_t	modulation;
	nim_symbol_rate_t		symbolRate;
	nim_fec_inner_t			fecInner;
};

struct ts_parameters_terrestrial {
	nim_frequency_t			centreFrequency;
	nim_bandwidth_t			bandwidth;
	nim_fec_inner_t			code_rate_HP;
	nim_fec_inner_t			code_rate_LP;
	nim_transmit_mode_t		transmission_mode;
	nim_constellation_t		constellation;
	nim_guard_interval_t		guard_interval;
	nim_hierarchy_t			hierarchy_information;
	nim_otherfrequencyflag_t	otherFrequencyFlag;
};

#endif /* __dvb_types_h__ */
