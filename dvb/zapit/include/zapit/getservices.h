/*
 * $Id: getservices.h,v 1.43 2002/05/15 20:47:07 obi Exp $
 */

#ifndef __getservices_h__
#define __getservices_h__

#if (DVB_API_VERSION == 1)
#include <ost/frontend.h>
#else
#include <linux/dvb/frontend.h>
#endif

#include <stdint.h>
#include <string.h>

#include <iostream>
#include <string>

#include <eventserver.h>

#include <xml/xmltree.h>
#include <zapci/ci.h>
#include <zapsi/descriptors.h>
#include <zapsi/sdt.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define CONFIGDIR "/var/tuxbox/config"
#endif

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

typedef struct bouquet_msg_struct
{
	unsigned int bouquet_nr;
	char name[30];

} bouquet_msg;

typedef struct channel_msg_struct
{
	unsigned int chan_nr;
	char name[30];
	char mode;

} channel_msg;

typedef struct channel_msg_struct_2
{
	unsigned int chan_nr;
	char name[30];
	char mode;
	unsigned int onid_tsid;

} channel_msg_2;

#endif /* __getservices_h__ */
