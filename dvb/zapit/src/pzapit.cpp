/*
 * $Id: pzapit.cpp,v 1.18 2002/05/08 16:10:26 obi Exp $
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
	std::cout << "bouquet list: " << basename << " [-ra]" << std::endl;
	std::cout << "channel list: " << basename << " [-ra] <bouquet-number>" << std::endl;
	std::cout << "zap by number: " << basename << " [-ra] <bouquet-number> <channel-number>" << std::endl;
	std::cout << "zap by name: " << basename << " [-ra] <channel-name>" << std::endl;
	std::cout << "set diseqc type: " << basename << " -dt <type>" << std::endl;
	std::cout << "set diseqc repeats: " << basename << " -dr <count>" << std::endl;
	std::cout << "(-ra toggles radio mode)" << std::endl;
	std::cout << "switch record mode on/off: " << basename << " -re" << std::endl;
	std::cout << "start/stop playback: " << basename << " -p" << std::endl;
	std::cout << std::endl;
	std::cout << "change audio pid: " << basename << " -a <audio-number>" << std::endl;
	std::cout << std::endl;
	std::cout << "reload channels bouquets: " << basename << " -c" << std::endl;
	std::cout << std::endl;
	std::cout << "show satellites: " << basename << " -sh" << std::endl;
	std::cout << "select satellites: " << basename << " -se <satmask> <diseqc order>" << std::endl;
	std::cout << "start transponderscan: " << basename << " -st" << std::endl;
	std::cout << std::endl;
	std::cout << "mute audio: " << basename << " -mute" << std::endl;
	std::cout << "unmute audio: " << basename << " -unmute" << std::endl;
	std::cout << "set volume: " << basename << " -vol <0..64>" << std::endl;
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
	int diseqcRepeats = -1;
	int diseqcType = -1;
	int satmask = 0;
	int audio = 0;
	int mute = -1;
	int volume = -1;
	char* channelName = NULL;

	bool playback = false;
	bool recordmode = false;
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
		else if (!strncmp(argv[i], "-dr", 3))
		{
			if (i < argc - 1)
			{
				sscanf(argv[++i], "%d", &diseqcRepeats);
				continue;
			}
			else
			{
				return usage(argv[0]);
			}
		}
		else if (!strncmp(argv[i], "-dt", 3))
		{
			if (i < argc - 1)
			{
				sscanf(argv[++i], "%d", &diseqcType);
				continue;
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
		else if (!strncmp(argv[i], "-mute", 5))
		{
			mute = 1;
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
		else if (!strncmp(argv[i], "-p", 2))
		{
			playback = true;
			continue;
		}
		else if (!strncmp(argv[i], "-ra", 3))
		{
			radio = true;
			continue;
		}
		else if (!strncmp(argv[i], "-re", 3))
		{
			recordmode = true;
			continue;
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
		else if (!strncmp(argv[i], "-unmute", 7))
		{
			mute = 0;
			continue;
		}
		else if (!strncmp(argv[i], "-vol", 4))
		{
			if (i < argc - 1)
			{
				sscanf(argv[++i], "%d", &volume);
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

	/* audio mute */
	if (mute != -1)
	{
		std::cout << "mute/unmute" << std::endl;
		zapit->muteAudio(mute);
		delete zapit;
		return 0;
	}

	if (volume != -1)
	{
		std::cout << "set volume" << std::endl;
		zapit->setVolume(volume, volume);
		delete zapit;
		return 0;
	}

	/* reload services */
	if (reload)
	{
		std::cout << "reloading channels" << std::endl;
		zapit->reinitChannels();
		delete zapit;
		return 0;
	}

	if (diseqcType != -1)
	{
		zapit->setDiseqcType((diseqc_t) diseqcType);

		if (diseqcRepeats == -1)
		{
			delete zapit;
			return 0;
		}
	}

	if (diseqcRepeats != -1)
	{
		zapit->setDiseqcRepeat(diseqcRepeats);
		delete zapit;
		return 0;
	}

	if (playback)
	{
		if (zapit->isPlayBackActive())
		{
			zapit->stopPlayBack();
		}
		else
		{
			zapit->startPlayBack();
		}

		if (!recordmode)
		{
			delete zapit;
			return 0;
		}
	}

	if (recordmode)
	{
		zapit->setRecordMode(!zapit->isRecordModeActive());
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

		zapit->startScan();

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

