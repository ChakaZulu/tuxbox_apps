/*
 * $Id: getservices.h,v 1.47 2002/09/20 16:55:22 thegoodguy Exp $
 */

#ifndef __getservices_h__
#define __getservices_h__

#include <ost/frontend.h>

#include <stdint.h>
#include <string.h>

#include <iostream>
#include <string>

#include <eventserver.h>

#include "xmlinterface.h"
#include <zapci/ci.h>
#include <zapsi/descriptors.h>
#include <zapsi/sdt.h>

#define zapped_chan_is_nvod 0x80

#define NONE 0x0000
#define INVALID 0x1FFF

void ParseTransponders (XMLTreeNode * xmltransponder, unsigned char DiSEqC);
void ParseChannels (XMLTreeNode * node, unsigned short transport_stream_id, unsigned short original_network_id, unsigned char DiSEqC);
void FindTransponder (XMLTreeNode * root);
void LoadSortList ();
int LoadServices ();

struct transponder
{
	unsigned short transport_stream_id;
	FrontendParameters feparams;
	unsigned char polarization;
	unsigned char DiSEqC;
	unsigned short original_network_id;

	transponder (unsigned short p_transport_stream_id, FrontendParameters p_feparams)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		polarization = 0;
		DiSEqC = 0;
		original_network_id = 0;
	}

	transponder (unsigned short p_transport_stream_id, FrontendParameters p_feparams, unsigned short p_polarization, unsigned char p_DiSEqC, unsigned short p_original_network_id)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		polarization = p_polarization;
		DiSEqC = p_DiSEqC;
		original_network_id = p_original_network_id;
	}
};

#endif /* __getservices_h__ */
