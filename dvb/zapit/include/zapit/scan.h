/*
 *  $Id: scan.h,v 1.20 2002/09/24 10:11:12 thegoodguy Exp $
 */

#ifndef __scan_h__
#define __scan_h__

#include <ost/frontend.h>

#include <stdint.h>

#include <map>
#include <string>

#include "bouquets.h"

struct scanchannel
{
	std::string           name;
	t_service_id          service_id;
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	uint8_t               service_type;
	uint16_t              pmt;

	scanchannel(std::string Name, t_service_id Sid, t_transport_stream_id Tsid, t_original_network_id Onid, uint8_t Service_type)
	{
		name = Name;
		service_id = Sid;
		transport_stream_id = Tsid;
		original_network_id = Onid;
		service_type = Service_type;
		pmt = 0;
	}
	scanchannel(std::string Name, t_service_id Sid, t_transport_stream_id Tsid, t_original_network_id Onid)
	{
		name = Name;
		service_id = Sid;
		transport_stream_id = Tsid;
		original_network_id = Onid;
		service_type = 1;
		pmt = 0;
	}

	scanchannel(t_service_id Sid, t_transport_stream_id Tsid, uint16_t Pmt)
	{
		service_id = Sid;
		transport_stream_id = Tsid;
		pmt = Pmt;
		original_network_id = 0;
		service_type = 0;
	}
};

struct transpondermap
{
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	FrontendParameters feparams;
	uint8_t polarization;
	uint8_t DiSEqC;

	transpondermap(t_transport_stream_id p_transport_stream_id, t_original_network_id p_original_network_id, FrontendParameters p_feparams)
	{
		transport_stream_id = p_transport_stream_id;
		original_network_id = p_original_network_id;
		feparams = p_feparams;
		polarization = 0;
		DiSEqC = 0;
	}

	transpondermap(t_transport_stream_id p_transport_stream_id, t_original_network_id p_original_network_id, FrontendParameters p_feparams, uint8_t p_polarization, uint8_t p_DiSEqC)
	{
		transport_stream_id = p_transport_stream_id;
		original_network_id = p_original_network_id;
		feparams = p_feparams;
		polarization = p_polarization;
		DiSEqC = p_DiSEqC;
	}
};

extern std::map <t_channel_id, scanchannel> scanchannels;
typedef std::map <t_channel_id, scanchannel>::iterator sciterator;

extern std::map <uint32_t, transpondermap> scantransponders;
typedef std::map <uint32_t, transpondermap>::iterator stiterator;

extern CBouquetManager* scanBouquetManager;

#endif /* __scan_h__ */
