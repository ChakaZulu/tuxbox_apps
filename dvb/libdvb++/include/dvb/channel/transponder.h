/*
 * $Id: transponder.h,v 1.3 2004/09/03 13:50:33 mws Exp $
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

#ifndef __dvb_channel_transponder_h__
#define __dvb_channel_transponder_h__

#include <map>
#include <dvbsi++/cable_delivery_system_descriptor.h>
#include <dvbsi++/satellite_delivery_system_descriptor.h>
#include <dvbsi++/terrestrial_delivery_system_descriptor.h>
#include <dvb/types.h>
#include "service.h"

class Transponder
{
	protected:
		uint16_t transportStreamId;
		uint16_t originalNetworkId;

		union {
			struct ts_parameters_cable c;
			struct ts_parameters_satellite s;
			struct ts_parameters_terrestrial t;
		} p;

		ServiceMap services;

	public:
		Transponder(
			const uint16_t pTransportStreamId,
			const uint16_t pOriginalNetworkId,
			const CableDeliverySystemDescriptor * const
		);
		Transponder(
			const uint16_t pTransportStreamId,
			const uint16_t pOriginalNetworkId,
			const nim_frequency_t,
			const nim_fec_outer_t,
			const nim_modulation_cable_t,
			const nim_symbol_rate_t,
			const nim_fec_inner_t
		);

		Transponder(
			const uint16_t pTransportStreamId,
			const uint16_t pOriginalNetworkId,
			const SatelliteDeliverySystemDescriptor * const
		);
		Transponder(
			const uint16_t pTransportStreamId,
			const uint16_t pOriginalNetworkId,
			const nim_frequency_t,
			const nim_orbital_position_t,
			const nim_west_east_flag_t,
			const nim_polarization_t,
			const nim_symbol_rate_t,
			const nim_fec_inner_t
		);

		Transponder(
			const uint16_t pTransportStreamId,
			const uint16_t pOriginalNetworkId,
			const TerrestrialDeliverySystemDescriptor * const
		);
		
		Transponder(
			const uint16_t pTransportStreamId,
			const uint16_t pOriginalNetworkId,
			const uint32_t pCentreFrequency,
			const nim_bandwidth_t pBandwidth,
			const nim_constellation_t pConstellation,
			const nim_hierarchy_t pHierarchyInformation,
			const nim_fec_inner_t pCodeRateHpStream,
			const nim_fec_inner_t pCodeRateLpStream,
			const nim_guard_interval_t pGuardInterval,
			const nim_transmit_mode_t pTransmissionMode,
			const nim_otherfrequencyflag_t pOtherFrequencyFlag
		);


		enum {
			UNKNOWN,
			CABLE,
			SATELLITE,
			TERRESTRIAL
		} type;
	
		//void addService(Service * const service);
		//void deleteService(const Service * const Service);
		//void deleteServiceList(void);

		uint16_t getTransportStreamId(void) const;
		uint16_t getOriginalNetworkId(void) const;
		nim_frequency_t getFrequency(void) const;
		nim_symbol_rate_t getSymbolRate(void) const;
		nim_polarization_t getPolarization(void) const;
		nim_fec_inner_t getFecInner(void) const;
		nim_modulation_cable_t getModulation(void) const;
		uint64_t getUniqueId(void) const;
		uint8_t getType(void) const;

		const ServiceMap *getServiceMap(void) const;
		const Service *getService(const uint16_t serviceId) const;
		void insertService(Service *service);

		//void setFrequency(const uint32_t frequency);

		//uint8_t getModulation(void) const;
		//void setModulation(const uint8_t modulation);

		//void setSymbolrate(const uint32_t symbolrate);

		//void setPolarization(const uint8_t polarization);

		//uint8_t getDiseqc(void) const;
		//void setDiseqc(const uint8_t diseqc);

		//uint8_t getInnerFec(void) const;
		//void setInnerFec(const uint8_t innerFec);

		//uint8_t getInversion(void) const;
		//void setInversion(const uint8_t inversion);

		//void setOriginalNetworkId(const uint16_t originalNetworkId);

		//void setTransportStreamId(const uint16_t transportStreamId);

		//uint16_t getDeliverySystemId(void) const;
		//void setDeliverySystemId(const uint16_t deliverySystemId);
};

typedef std::map<const uint64_t, Transponder*> TransponderMap;
typedef TransponderMap::iterator TransponderMapIterator;
typedef TransponderMap::const_iterator TransponderMapConstIterator;

#endif /* __dvb_channel_transponder_h__ */
