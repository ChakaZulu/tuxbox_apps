/*
 * $Id: getservices.h,v 1.58 2005/01/09 16:56:54 thegoodguy Exp $
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

#include <linux/dvb/frontend.h>

#include <eventserver.h>

#include "ci.h"
#include "descriptors.h"
#include "sdt.h"
#include "types.h"
#include "xmlinterface.h"

#include <map>

#define zapped_chan_is_nvod 0x80

#define NONE 0x0000
#define INVALID 0x1FFF

void ParseTransponders(xmlNodePtr xmltransponder, const unsigned char DiSEqC, t_satellite_position satellitePosition);
void ParseChannels    (xmlNodePtr node, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, const unsigned char DiSEqC, t_satellite_position satellitePosition);
void FindTransponder  (xmlNodePtr root);
void LoadSortList     ();
int LoadServices      (fe_type_t, diseqc_t);

struct transponder
{
	t_transport_stream_id transport_stream_id;
	dvb_frontend_parameters feparams;
	unsigned char polarization;
	unsigned char DiSEqC;
	t_original_network_id original_network_id;

	transponder (t_transport_stream_id p_transport_stream_id, dvb_frontend_parameters p_feparams)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		polarization = 0;
		DiSEqC = 0;
		original_network_id = 0;
	}

	transponder (t_transport_stream_id p_transport_stream_id, dvb_frontend_parameters p_feparams, unsigned short p_polarization, unsigned char p_DiSEqC, t_original_network_id p_original_network_id)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		polarization = p_polarization;
		DiSEqC = p_DiSEqC;
		original_network_id = p_original_network_id;
	}
};

typedef std::map<transponder_id_t, transponder> transponder_list_t;

#endif /* __getservices_h__ */
