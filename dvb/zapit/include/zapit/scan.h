/*
 *  $Id: scan.h,v 1.22 2002/09/24 10:40:13 thegoodguy Exp $
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

	scanchannel(const std::string Name, const t_service_id Sid, const t_transport_stream_id Tsid, const t_original_network_id Onid, const uint8_t Service_type)
	{
		name                = Name;
		service_id          = Sid;
		transport_stream_id = Tsid;
		original_network_id = Onid;
		service_type        = Service_type;
	}
};

struct transpondermap
{
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	FrontendParameters    feparams;
	uint8_t               polarization;
	uint8_t               DiSEqC;

	transpondermap(const t_transport_stream_id p_transport_stream_id, const t_original_network_id p_original_network_id, const FrontendParameters p_feparams, const uint8_t p_polarization = 0, const uint8_t p_DiSEqC = 0)
	{
		transport_stream_id = p_transport_stream_id;
		original_network_id = p_original_network_id;
		feparams            = p_feparams;
		polarization        = p_polarization;
		DiSEqC              = p_DiSEqC;
	}
};

extern std::map <t_channel_id, scanchannel> scanchannels;
typedef std::map <t_channel_id, scanchannel>::iterator sciterator;

extern std::map <uint32_t, transpondermap> scantransponders;
typedef std::map <uint32_t, transpondermap>::iterator stiterator;

extern CBouquetManager* scanBouquetManager;

#endif /* __scan_h__ */
