/*
 * $Id: pzapit.cpp,v 1.7 2002/04/07 12:54:11 obi Exp $
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

int usage(char *basename)
{
	printf("\n");
	printf("usage:\n");
	printf("\n");
	printf("start a transponderscan\n");
	printf("\t%s -s <sat-mask>\n", basename);
	printf("\n");
	printf("list bouquets\n");
	printf("\t%s\n", basename);
	printf("\n");
	printf("list channels of one bouquet\n");
	printf("\t%s [-r] <bouquet-number>\n", basename);
	printf("\n");
	printf("zap to a channel\n");
	printf("\t%s [-r] <bouquet-number> <channel-number>\n", basename);
	printf("\n");
	printf("\t-r enables radio mode\n");
	printf("\n");
	printf("\tto get your satmask, simply add your supported satellites:\n");
	printf("\tastra = 1, hotbird = 2, kopernikus = 4,\n");
	printf("\tdigiturk = 8, sirius = 16, thor = 32\n");
	printf("\tleave bouquets untouched = 256\n");

	return -1;
}

int main (int argc, char** argv)
{
	int i;

	unsigned int bouquet = 0;
	unsigned int channel = 0;
	unsigned int count = 0;
	int satmask = 0;

	bool radio = false;

	CZapitClient *zapit;
	std::vector<CZapitClient::responseGetBouquets> bouquets;
	std::vector<CZapitClient::responseGetBouquetChannels> channels;

	/* command line */
	for (i = 1; i < argc; i++)
	{
		if (!strncmp(argv[i], "-r", 2))
		{
			if (i < argc - 1)
			{
				radio = true;
				continue;
			}
			else
			{
				return usage(argv[0]);
			}
		}
		else if (!strncmp(argv[i], "-s", 2))
		{
			if (i < argc - 1)
			{
				sscanf(argv[++i], "%d", &satmask);
				continue;
			}
			else
			{
				return usage(argv[0]);
			}
		}
		else if (i < argc - 1)
		{
			if ((sscanf(argv[i], "%d", &bouquet) > 0) && (sscanf(argv[++i], "%d", &channel) > 0))
			{
				continue;
			}
			else
			{
				return usage(argv[0]);
			}
		}
		else if (sscanf(argv[i], "%d", &bouquet) > 0)
		{
			printf("selecting bouquet %d\n", bouquet);
			continue;
		}
		else
		{
			return usage(argv[0]);
		}
	}

	/* zap it */
	zapit = new (nothrow) CZapitClient();

	if (zapit == NULL)
	{
		perror("[pzapit] new");
		return -1;
	}

	if (radio)
	{
		zapit->setMode(CZapitClient::MODE_RADIO);
	}
	else
	{
		zapit->setMode(CZapitClient::MODE_TV);
	}

	if (satmask)
	{
		zapit->startScan(satmask);
	}
	else
	{
		zapit->getBouquets(bouquets, true);
	}

	if (bouquet)
	{
		zapit->getBouquetChannels(bouquet, channels);
	}

	if (!channel)
	{
		if (!bouquet)
		{
			std::vector<CZapitClient::responseGetBouquets>::iterator b_resp;
			for (b_resp = bouquets.begin(); b_resp < bouquets.end(); b_resp++)
				std::cout << b_resp->bouquet_nr << ": " << b_resp->name << std::endl;
		}
		else
		{
			std::vector<CZapitClient::responseGetBouquetChannels>::iterator ch_resp;
			for (ch_resp = channels.begin(), count = 1; ch_resp < channels.end(); ch_resp++, count++)
				cout << count << ": " << ch_resp->name << endl;
		}
	}
	else
	{
		std::cout << "zapping to bouquet " << bouquets[bouquet-1].name << ", channel " << channels[channel-1].name << "." << endl;
		zapit->zapTo(bouquet, channel);
	}

	delete zapit;
	return 0;
}

