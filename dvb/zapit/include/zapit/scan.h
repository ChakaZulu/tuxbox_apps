/*
 *  $Id: scan.h,v 1.13 2002/04/19 14:53:29 obi Exp $
 */

#ifndef __scan_h__
#define __scan_h__

#include <ost/frontend.h>
#include <stdint.h>

#include <map>
#include <string>

struct scanchannel
{
	std::string name;
	uint16_t sid;
	uint16_t tsid;
	uint8_t service_type;
	uint16_t pmt;
	uint16_t onid;

	scanchannel(std::string Name, uint16_t Sid, uint16_t Tsid, uint16_t Onid, uint8_t Service_type)
	{
		name = Name;
		sid = Sid;
		tsid = Tsid;
		onid = Onid;
		service_type = Service_type;
		pmt = 0;
	}
	scanchannel(std::string Name, uint16_t Sid, uint16_t Tsid, uint16_t Onid)
	{
		name = Name;
		sid = Sid;
		tsid = Tsid;
		onid = Onid;
		service_type = 1;
		pmt = 0;
	}

	scanchannel(uint16_t Sid, uint16_t Tsid, uint16_t Pmt)
	{
		sid = Sid;
		tsid = Tsid;
		pmt = Pmt;
		onid = 0;
		service_type = 0;
	}
};

struct transpondermap
{
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	FrontendParameters feparams;
	uint8_t polarization;
	uint8_t DiSEqC;

	transpondermap(uint16_t p_transport_stream_id, uint16_t p_original_network_id, FrontendParameters p_feparams)
	{
		transport_stream_id = p_transport_stream_id;
		original_network_id = p_original_network_id;
		feparams = p_feparams;
		polarization = 0;
		DiSEqC = 0;
	}

	transpondermap(uint16_t p_transport_stream_id, uint16_t p_original_network_id, FrontendParameters p_feparams, uint8_t p_polarization, uint8_t p_DiSEqC)
	{
		transport_stream_id = p_transport_stream_id;
		original_network_id = p_original_network_id;
		feparams = p_feparams;
		polarization = p_polarization;
		DiSEqC = p_DiSEqC;
	}
};

struct bouquet_mulmap
{
	std::string provname;
	std::string servname;
	uint16_t sid;
	uint16_t onid;

	bouquet_mulmap(std::string Provname, std::string Servname, uint16_t Sid, uint16_t Onid)
	{
		provname = Provname;
		servname = Servname;
		sid = Sid;
		onid = Onid;
	}
};

extern std::map <uint32_t, transpondermap> scantransponders;
extern std::map <uint32_t, scanchannel> scanchannels;
extern std::multimap <std::string, bouquet_mulmap> scanbouquets;

#endif /* __scan_h__ */
