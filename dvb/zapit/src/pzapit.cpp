/*
 * $Id: pzapit.cpp,v 1.12 2002/04/18 23:46:08 obi Exp $
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

#include <iostream>
#include <string.h>

#include "clientlib/zapitclient.h"

int usage (std::string basename)
{
	std::cout << "bouquet list: " << basename << std::endl;
	std::cout << "channel list: " << basename << " [-r] <bouquet-number>" << std::endl;
	std::cout << "zap by number: " << basename << " [-r] <bouquet-number> <channel-number>" << std::endl;
	std::cout << "zap by name: " << basename << "[-r] <channel-name>" << std::endl;
	std::cout << "-r enables radio mode" << std::endl;
	std::cout << std::endl;
	std::cout << "change audio pid: " << basename << " -a <audio-number>" << std::endl;
	std::cout << std::endl;
	std::cout << "reload channels bouquets: " << basename << " -c" << std::endl;
	std::cout << std::endl;
	std::cout << "show satellites: " << basename << " -sh" << std::endl;
	std::cout << "select satellites: " << basename << " -se <satmask> <diseqc order>" << std::endl;
	std::cout << "start transponderscan: " << basename << " -st" << std::endl;
	return -1;
}

int main (int argc, char** argv)
{
	int i;
	uint32_t j;
	uint32_t k;

	unsigned int bouquet = 0;
	unsigned int channel = 0;
	unsigned int count = 0;
	int satmask = 0;
	int audio = 0;
	char* channelName = NULL;

	bool radio = false;
	bool reload = false;
	bool show_satellites = false;
	bool scan = false;
	bool zapByName = false;
	uint32_t diseqc[5];

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
		else if (!strncmp(argv[i], "-n", 2))
		{
			if (i < argc - 1)
			{
				zapByName = true;
				channelName = argv[++i];
				continue;
			}
			else
			{
				return usage(argv[0]);
			}
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
		else if (!strncmp(argv[i], "-se", 3))
		{
			if (i < argc - 2)
			{
				sscanf(argv[++i], "%d", &satmask);
				diseqc[0] = strlen(argv[i+1]);
				for (i++, j = 0; j <= diseqc[0]; j++)
				{
					diseqc[j+1] = argv[i][j] - 48;
				}
				continue;
			}
			else
			{
				return usage(argv[0]);
			}
		}
		else if (!strncmp(argv[i], "-sh", 3))
		{
			show_satellites = true;
			continue;
		}
		else if (!strncmp(argv[i], "-st", 3))
		{
			scan = true;
			continue;
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

	if (show_satellites)
	{
		std::vector<CZapitClient::responseGetSatelliteList> satelliteList;
		zapit->getScanSatelliteList(satelliteList);

		std::vector<CZapitClient::responseGetSatelliteList>::iterator rI;
		for (i = 0, rI = satelliteList.begin(); rI < satelliteList.end(); i++, rI++)
			std::cout << (1 << i) << ": " << rI->satName << std::endl;

		delete zapit;
		return 0;
	}
	else if (satmask != 0)
	{
		std::vector<CZapitClient::responseGetSatelliteList> satelliteList;
		zapit->getScanSatelliteList(satelliteList);

		std::vector<CZapitClient::commandSetScanSatelliteList> newSatelliteList;
		CZapitClient::commandSetScanSatelliteList item;

		for (i = 1, j = 0, k = 0; j < satelliteList.size(); i = (i << 1), k++)
		{
			if (satmask & i)
			{
				j++;
				std::cout << "diseqc " << diseqc[j] << ": " << satelliteList[k].satName << std::endl;

				strcpy(item.satName, satelliteList[k].satName);
				item.diseqc = diseqc[j];
				newSatelliteList.insert(newSatelliteList.end(), item);
			}

			if ((j >= diseqc[0]) || (k >= satelliteList.size()))
			{
				break;
			}
		}

		zapit->setScanSatelliteList(newSatelliteList);

		delete zapit;
		return 0;
	}

	/* transponderscan */
	if (scan)
	{
		unsigned int satellite;
		unsigned int transponder;
		unsigned int services;

		zapit->startScan(0);

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

	if (zapByName)
	{
		zapit->getChannels(channels);

		std::vector<CZapitClient::responseGetBouquetChannels>::iterator ch_resp;
		for (ch_resp = channels.begin(), count = 1; ch_resp < channels.end(); ch_resp++, count++)
		{
			if (!strcasecmp(ch_resp->name, channelName))
			{
				channel = count;
			}
		}

		if (channel == 0)
		{
			std::cout << "channel not found." << std::endl;
			delete zapit;
			return 0;
		}
		else
		{
			std::cout << "found channel number: " << channel << std::endl;
		}
	}
	else /* zap by bouquet number and channel number */
	{
		/* read channel list */
		if (bouquet)
		{
			zapit->getBouquetChannels(bouquet, channels);
		}

		/* display bouquet list */
		else
		{
			zapit->getBouquets(bouquets, true);

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
	}

	/* zap */
	{
		CZapitClient::responseGetPIDs pids;

		std::cout << "zapping to  channel " << channels[channel-1].name << "." << std::endl;

		zapit->zapTo(channels[channel-1].nr);
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

