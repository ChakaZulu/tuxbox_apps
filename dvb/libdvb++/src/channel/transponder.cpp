/*
 * $Id: transponder.cpp,v 1.2 2004/09/03 13:50:33 mws Exp $
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

#include <dvb/channel/transponder.h>
#include <dvb/debug/debug.h>

Transponder::Transponder(
	const uint16_t pTransportStreamId,
	const uint16_t pOriginalNetworkId,
	const SatelliteDeliverySystemDescriptor * const d)
{
	type = SATELLITE;
	
	transportStreamId = pTransportStreamId;
	originalNetworkId = pOriginalNetworkId;

	if (d == NULL)
		DVB_FATAL("d == NULL");

	p.s.frequency = (nim_frequency_t) (d->getFrequency() * 10);
	p.s.orbitalPosition = (nim_orbital_position_t) d->getOrbitalPosition();
	p.s.westEastFlag = (nim_west_east_flag_t) d->getWestEastFlag();
	p.s.polarization = (nim_polarization_t) d->getPolarization();
	p.s.modulation = (nim_modulation_satellite_t) d->getModulation();
	p.s.symbolRate = (nim_symbol_rate_t) (d->getSymbolRate() * 100);
	p.s.fecInner = (nim_fec_inner_t) d->getFecInner();
}

Transponder::Transponder(
	const uint16_t pTransportStreamId,
	const uint16_t pOriginalNetworkId,
	const nim_frequency_t frequency,
	const nim_orbital_position_t orbitalPosition,
	const nim_west_east_flag_t westEastFlag,
	const nim_polarization_t polarization,
	const nim_symbol_rate_t symbolRate,
	const nim_fec_inner_t fecInner)
{
	type = SATELLITE;
	
	transportStreamId = pTransportStreamId;
	originalNetworkId = pOriginalNetworkId;

	p.s.frequency = frequency;
	p.s.orbitalPosition = orbitalPosition;
	p.s.westEastFlag = westEastFlag;
	p.s.polarization = polarization;
	p.s.modulation = NIM_MODULATION_S_QPSK;
	p.s.symbolRate = symbolRate;
	p.s.fecInner = fecInner;
}

Transponder::Transponder(
	const uint16_t pTransportStreamId,
	const uint16_t pOriginalNetworkId,
	const CableDeliverySystemDescriptor * const d)
{
	type = CABLE;
	
	transportStreamId = pTransportStreamId;
	originalNetworkId = pOriginalNetworkId;

	if (d == NULL)
		DVB_FATAL("d == NULL");

	p.c.frequency = (nim_frequency_t) (d->getFrequency() * 100);
	p.c.fecOuter = (nim_fec_outer_t) d->getFecOuter();
	p.c.modulation = (nim_modulation_cable_t) d->getModulation();
	p.c.symbolRate = (nim_symbol_rate_t) (d->getSymbolRate() * 100);
	p.c.fecInner = (nim_fec_inner_t) d->getFecInner();
}

Transponder::Transponder(
	const uint16_t pTransportStreamId,
	const uint16_t pOriginalNetworkId,
	const nim_frequency_t frequency,
	const nim_fec_outer_t fecOuter,
	const nim_modulation_cable_t modulation,
	const nim_symbol_rate_t symbolRate,
	const nim_fec_inner_t fecInner)
{
	type = CABLE;
	
	transportStreamId = pTransportStreamId;
	originalNetworkId = pOriginalNetworkId;

	p.c.frequency = frequency;
	p.c.fecOuter = fecOuter;
	p.c.modulation = modulation;
	p.c.symbolRate = symbolRate;
	p.c.fecInner = fecInner;
}

Transponder::Transponder(
	const uint16_t pTransportStreamId,
	const uint16_t pOriginalNetworkId,
	const TerrestrialDeliverySystemDescriptor * const d)
{
	type = TERRESTRIAL;
	
	transportStreamId = pTransportStreamId;
	originalNetworkId = pOriginalNetworkId;

	if (d == NULL)
		DVB_FATAL("d == NULL");

	p.t.centreFrequency = (nim_frequency_t) d->getCentreFrequency();
	p.t.bandwidth = (nim_bandwidth_t) d->getBandwidth();
	p.t.code_rate_HP = (nim_fec_inner_t) d->getCodeRateHpStream();
	p.t.code_rate_LP = (nim_fec_inner_t) d->getCodeRateLpStream();
	p.t.constellation = (nim_constellation_t) d->getConstellation();
	p.t.guard_interval = (nim_guard_interval_t) d->getGuardInterval(); 
	p.t.hierarchy_information = (nim_hierarchy_t) d->getHierarchyInformation();
	p.t.transmission_mode = (nim_transmit_mode_t) d->getTransmissionMode();
	p.t.otherFrequencyFlag = (nim_otherfrequencyflag_t) d->getOtherFrequencyFlag();
}

Transponder::Transponder(
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
	const nim_otherfrequencyflag_t pOtherFrequencyFlag)
{
	type = TERRESTRIAL;
	
	transportStreamId = pTransportStreamId;
	originalNetworkId = pOriginalNetworkId;

	p.t.centreFrequency = pCentreFrequency;
	p.t.bandwidth = pBandwidth;
	p.t.code_rate_HP = pCodeRateHpStream;
	p.t.code_rate_LP = pCodeRateLpStream;
	p.t.constellation = pConstellation;
	p.t.guard_interval = pGuardInterval; 
	p.t.hierarchy_information = pHierarchyInformation;
	p.t.transmission_mode = pTransmissionMode;
	p.t.otherFrequencyFlag = pOtherFrequencyFlag;
}

uint16_t Transponder::getTransportStreamId(void) const
{
	return transportStreamId;
}

uint16_t Transponder::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

nim_frequency_t Transponder::getFrequency(void) const
{
	switch (type) {
	case CABLE:
		return p.c.frequency;
	case SATELLITE:
		return p.s.frequency;
	case TERRESTRIAL:
		return p.t.centreFrequency;
	default:
		return 0;
	}
}

nim_symbol_rate_t Transponder::getSymbolRate(void) const
{
	switch (type) {
	case CABLE:
		return p.c.symbolRate;
	case SATELLITE:
		return p.s.symbolRate;
	default:
		return 0;
	}
}

nim_fec_inner_t Transponder::getFecInner(void) const
{
	switch (type) {
	case CABLE:
		return p.c.fecInner;
	case SATELLITE:
		return p.s.fecInner;
	default:
		DVB_FATAL("type == TERRESTRIAL");
		return NIM_FEC_I_UNKNOWN;
	}
}

nim_modulation_cable_t Transponder::getModulation(void) const
{
	if (type != CABLE)
		DVB_FATAL("type != CABLE");

	return p.c.modulation;
}

nim_polarization_t Transponder::getPolarization(void) const
{
	if (type != SATELLITE)
		DVB_FATAL("type != SATELLITE");

	return p.s.polarization;
}

uint64_t Transponder::getUniqueId(void) const
{
	// 48 bit id for satellite systems
	if (type == SATELLITE)
		return ((((uint64_t)p.s.orbitalPosition) << 32) |
			(((uint64_t)transportStreamId) << 16) |
			 ((uint64_t)originalNetworkId));

	// 32 bit id for cable and terrestrial
	return (transportStreamId << 16) | originalNetworkId;
}

void Transponder::insertService(Service *service)
{
	ServiceMapIterator smi = services.find(service->getId());
	
	if (smi != services.end()) {
		delete smi->second;
		smi->second = service;
	}
	else {
		services[service->getId()] = service;
	}
}

const ServiceMap *Transponder::getServiceMap(void) const
{
	return &services;
}

const Service *Transponder::getService(const uint16_t serviceId) const
{
	ServiceMapConstIterator smci = services.find(serviceId);

	if (smci == services.end())
		return NULL;

	return smci->second;
}

uint8_t Transponder::getType(void) const
{
	return type;
}

