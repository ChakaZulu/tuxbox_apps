/*
 * $Id: network_interface_module.h,v 1.1 2003/07/17 01:07:32 obi Exp $
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

#ifndef __dvb_hardware_network_interface_module_h__
#define __dvb_hardware_network_interface_module_h__

#include <linux/dvb/frontend.h>
#include <dvb/channel/transponder.h>
#include <dvb/types.h>

class NetworkInterfaceModule
{
	protected:
		int fd;
		struct dvb_frontend_info info;

		static const nim_lnb_lof_t default_lof;
		nim_polarization_t pol;
		nim_band_t band;
		nim_sec_type_t sec_type;
		uint8_t repeat_count;

		void setSec(nim_polarization_t, nim_diseqc_addr_t, nim_band_t);

	public:
		NetworkInterfaceModule(const uint8_t adapter = 0, const uint8_t frontend = 0);
		~NetworkInterfaceModule(void);

		uint32_t getBitErrorRate(void) const;
		nim_dvb_type_t getType(void) const;
		uint16_t getSignalNoiseRatio(void) const;
		uint16_t getSignalStrength(void) const;
		nim_status_t getStatus(void) const;

		void setSecType(nim_sec_type_t, uint8_t count = 0);

		bool tune(Transponder *ts);
		bool tune(struct dvb_frontend_parameters *);
		bool tuneQam(nim_frequency_t, nim_symbol_rate_t, nim_fec_inner_t = NIM_FEC_I_UNKNOWN, nim_modulation_cable_t = NIM_MODULATION_C_QAM64);
		bool tuneQpsk(nim_frequency_t, nim_symbol_rate_t, nim_polarization_t, nim_fec_inner_t = NIM_FEC_I_UNKNOWN, nim_diseqc_addr_t = NIM_DISEQC_ADDR1);
};

#endif /* __dvb_hardware_network_interface_module_h__ */
