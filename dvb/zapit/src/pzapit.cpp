/*
 * $Id: pzapit.cpp,v 1.2 2002/03/24 13:28:21 obi Exp $
 *
 * simple commandline client for zapit
 *
 * Copyright (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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

#include "clientlib/zapitclient.h"
#include <iostream>

int main (int argc, char** argv)
{
	unsigned int bouquet;
	unsigned int channel;
	bool zap = true;

	/* commandline parameters */
	if ((argc < 3) || (argc > 4))
	{
		printf("usage: %s [-d] bouquet-number channel-number\n", argv[0]);
		return -1;
	}
	else if (argc == 3)
	{
		sscanf(argv[1], "%d", &bouquet);
        	sscanf(argv[2], "%d", &channel);
	}
	else if (argc == 4)
	{
		if (!strncmp(argv[1], "-d", 2))
			zap = false;
		else
			return -1;

		sscanf(argv[2], "%d", &bouquet);
        	sscanf(argv[3], "%d", &channel);
	}

	/* create objects */
	CZapitClient *zapit = new CZapitClient();
	CZapitClient::BouquetList *bouquets = new CZapitClient::BouquetList();
	CZapitClient::BouquetChannelList *channels = new CZapitClient::BouquetChannelList();

	/* get and show bouquets */
	std::vector<CZapitClient::responseGetBouquets>::iterator b_resp;
	zapit->getBouquets(*bouquets, true);
	for (b_resp = bouquets->begin(); b_resp < bouquets->end(); b_resp++)
		std::cout << b_resp->bouquet_nr << ": " << b_resp->name << std::endl;

	/* get and show channels */
	std::vector<CZapitClient::responseGetBouquetChannels>::iterator ch_resp;
	zapit->getBouquetChannels(bouquet, *channels);
	for (ch_resp = channels->begin(); ch_resp < channels->end(); ch_resp++)
		cout << ch_resp->nr << ": " << ch_resp->name << "(" << ch_resp->onid_sid << ")" << endl;

	/* zap */
	if (zap == true)
	{
		zapit->zapTo(bouquet, channel);
	}

	/* bla */
	std::cout << "this is the end... my only friend, the end..." << std::endl;

	return 0;
}

