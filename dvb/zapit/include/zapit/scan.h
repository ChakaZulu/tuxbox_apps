/*
 *  $Id: scan.h,v 1.26 2003/03/14 07:31:50 obi Exp $
 */

#ifndef __scan_h__
#define __scan_h__

#include <linux/dvb/frontend.h>

#include <inttypes.h>

#include <map>
#include <string>

#include "bouquets.h"

struct transpondermap
{
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	dvb_frontend_parameters    feparams;
	uint8_t               polarization;
	uint8_t               DiSEqC;

	transpondermap(const t_transport_stream_id p_transport_stream_id, const t_original_network_id p_original_network_id, const dvb_frontend_parameters p_feparams, const uint8_t p_polarization = 0, const uint8_t p_DiSEqC = 0)
	{
		transport_stream_id = p_transport_stream_id;
		original_network_id = p_original_network_id;
		feparams            = p_feparams;
		polarization        = p_polarization;
		DiSEqC              = p_DiSEqC;
	}
};

extern std::map <uint32_t, transpondermap> scantransponders;
typedef std::map <uint32_t, transpondermap>::iterator stiterator;

extern CBouquetManager* scanBouquetManager;

char * getFrontendName();

#endif /* __scan_h__ */
