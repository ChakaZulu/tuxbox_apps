/*
 * $Id: pzapit.cpp,v 1.1 2002/02/20 18:44:16 obi Exp $
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

	if (argc != 3)
	{
		printf("usage: %s <bouquet-nr> <channel-nr>\n", argv[0]);
		return -1;
	}

        sscanf(argv[1], "%d", &bouquet);
        sscanf(argv[2], "%d", &channel);

	CZapitClient *zapit = new CZapitClient();

#if 0
	CZapitClient::BouquetList *bouquets = new CZapitClient::BouquetList();
	CZapitClient::BouquetChannelList *channels = new CZapitClient::BouquetChannelList();

	zapit->getBouquets(*bouquets, true);

	struct CZapitClient::responseGetBouquets *b_resp;

	for (b_resp = bouquets->begin(); b_resp < bouquets->end(); b_resp++)
		std::cout << b_resp->bouquet_nr << ": " << b_resp->name << std::endl;

	zapit->getBouquetChannels(bouquet, *channels);

	struct CZapitClient::responseGetBouquetChannels *ch_resp;

	for (ch_resp = channels->begin(); ch_resp < channels->end(); ch_resp++)
		cout << ch_resp->nr << ": " << ch_resp->name << "(" << ch_resp->onid_sid << ")" << endl;
#endif

	zapit->zapTo(bouquet, channel);

	std::cout << "this is the end... my only friend, the end..." << std::endl;

	return 0;
}

