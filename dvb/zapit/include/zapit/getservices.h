/*
 * $Id: getservices.h,v 1.66 2009/09/30 17:12:39 seife Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#ifndef __getservices_h__
#define __getservices_h__

#ifdef HAVE_TRIPLEDRAGON
#include "td-frontend-compat.h"
#elif HAVE_DVB_API_VERSION < 3
#include <ost/frontend.h>
#define fe_type_t	FrontendType
#else
#include <linux/dvb/frontend.h>
#endif

#include <eventserver.h>

#include "ci.h"
#include "descriptors.h"
#include "sdt.h"
#include "types.h"
#include "xmlinterface.h"
#include "channel.h"

#include <map>

#define zapped_chan_is_nvod 0x80

#define NONE 0x0000
#define INVALID 0x1FFF

void parse_static_pids(CZapitChannel* channel);
void ParseTransponders(xmlNodePtr xmltransponder, const unsigned char DiSEqC, t_satellite_position satellitePosition);
void ParseChannels    (xmlNodePtr node, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, const unsigned char DiSEqC, t_satellite_position satellitePosition, const uint32_t frequency);
void FindTransponder  (xmlNodePtr root);
void LoadSortList     ();
int LoadServices      (fe_type_t, diseqc_t, bool);

struct transponder
{
	t_transport_stream_id   transport_stream_id;
	t_original_network_id   original_network_id;
	dvb_frontend_parameters feparams;
	uint8_t                 polarization;
	uint8_t                 DiSEqC;

	inline transponder(const t_transport_stream_id p_transport_stream_id, const t_original_network_id p_original_network_id, const dvb_frontend_parameters p_feparams, const uint8_t p_polarization = 0, const uint8_t p_DiSEqC = 0)
	{
		transport_stream_id = p_transport_stream_id;
		original_network_id = p_original_network_id;
		feparams            = p_feparams;
		polarization        = p_polarization;
		DiSEqC              = p_DiSEqC;
	}
};

typedef std::map<transponder_id_t, transponder> transponder_list_t;

#endif /* __getservices_h__ */
