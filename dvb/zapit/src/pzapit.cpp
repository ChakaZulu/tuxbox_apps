/*
 * $Id: pzapit.cpp,v 1.8 2002/04/10 18:36:21 obi Exp $
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

int usage (std::string basename)
{
	std::cout << "bouquet list: " << basename << std::endl;
	std::cout << "channel list: " << basename << " [-r] <bouquet-number>" << std::endl;
	std::cout << "zap: " << basename << " [-r] <bouquet-number> <channel-number>" << std::endl;
	std::cout << "-r enables radio mode" << std::endl;
	std::cout << std::endl;
	std::cout << "change audio pid: " << basename << " -a <audio-number>" << std::endl;
	std::cout << std::endl;
	std::cout << "reload channels bouquets: " << basename << " -c" << std::endl;
	std::cout << std::endl;
	std::cout << "transponderscan: " << basename << " -s <sat-mask>" << std::endl;
	std::cout << "to get your satmask, simply add your supported satellites:" << std::endl;
	std::cout << "astra19 = 1, hotbird = 2, kopernikus = 4" << std::endl;
	std::cout << "astra28 = 8, sirius = 16, thor = 32, tuerksat = 64" << std::endl;
	std::cout << "leave bouquets untouched = 256" << std::cout;
	return -1;
}

int main (int argc, char** argv)
{
	int i;

	unsigned int bouquet = 0;
	unsigned int channel = 0;
	unsigned int count = 0;
	int satmask = 0;
	int audio = 0;

	bool radio = false;
	bool reload = false;

	CZapitClient *zapit;
	std::vector<CZapitClient::responseGetBouquets> bouquets;
	std::vector<CZapitClient::responseGetBouquetChannels> channels;

	/* command line */
	for (i = 1; i < argc; i++)
	{
		if (!strncmp(argv[i], "-a", 2))
		{
			if (i < argc - 1)
			{
				sscanf(argv[++i], "%d", &audio);
			}
			else
			{
				return usage(argv[0]);
			}
		}
		else if (!strncmp(argv[i], "-c", 2))
		{
			reload = true;
			continue;
		}
		else if (!strncmp(argv[i], "-r", 2))
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
				cout << "bouquet: " << bouquet << ", channel: " << channel << std::endl;
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

	/* create zapit client */
	zapit = new (nothrow) CZapitClient();

	if (zapit == NULL)
	{
		perror("[pzapit] new");
		return -1;
	}

	/* reload services */
	if (reload)
	{
		std::cout << "reloading channels" << std::endl;
		zapit->reinitChannels();
		delete zapit;
		return 0;
	}

	/* choose source mode */
	if (radio)
	{
		zapit->setMode(CZapitClient::MODE_RADIO);
	}
	else
	{
		zapit->setMode(CZapitClient::MODE_TV);
	}

	/* transponderscan */
	if (satmask)
	{
		unsigned int satellite;
		unsigned int transponder;
		unsigned int services;

		zapit->startScan(satmask);

		while (zapit->isScanReady(satellite, transponder, services) == false)
		{
			std::cout << "satellite: " << satellite << ", transponder: " << transponder << ", services: " << services << std::endl;
			sleep(1);
		}

		delete zapit;
		return 0;
	}

	/* set audio channel */
	if (audio)
	{
		zapit->setAudioChannel(audio - 1);
		delete zapit;
		return 0;
	}

	/* read bouquet list */
	zapit->getBouquets(bouquets, true);

	/* read channel list */
	if (bouquet)
	{
		zapit->getBouquetChannels(bouquet, channels);
	}

	/* display bouquet list */
	else
	{
		std::vector<CZapitClient::responseGetBouquets>::iterator b_resp;
		for (b_resp = bouquets.begin(); b_resp < bouquets.end(); b_resp++)
			std::cout << b_resp->bouquet_nr << ": " << b_resp->name << std::endl;
		delete zapit;
		return 0;
	}

	/* display channel list */
	if (!channel)
	{
		std::vector<CZapitClient::responseGetBouquetChannels>::iterator ch_resp;
		for (ch_resp = channels.begin(), count = 1; ch_resp < channels.end(); ch_resp++, count++)
			std::cout << count << ": " << ch_resp->name << std::endl;
		delete zapit;
		return 0;
	}

	/* zap */
	else
	{
		CZapitClient::responseGetPIDs pids;

		std::cout << "zapping to bouquet " << bouquets[bouquet-1].name << ", channel " << channels[channel-1].name << "." << std::endl;
		zapit->zapTo(bouquet, channel);
		zapit->getPIDS(pids);

		std::cout << "vpid: 0x" << std::hex << pids.PIDs.vpid << std::endl;
		std::cout << "vtxtpid: 0x" << std::hex << pids.PIDs.vtxtpid << std::endl;
		std::cout << "pcrpid: 0x" << std::hex << pids.PIDs.pcrpid << std::endl;
		std::cout << "audio channels:" << std::endl;

		for (count = 0; count < pids.APIDs.size(); count++)
		{
			std::cout << count + 1 << ") " << "pid: 0x" << std::hex << pids.APIDs[count].pid << ", description: " << pids.APIDs[count].desc;
			if (pids.APIDs[count].is_ac3) std::cout << " (ac3)";
			std::cout << std::endl;
		}
	}

	/* cleanup and exit */
	delete zapit;
	return 0;
}

