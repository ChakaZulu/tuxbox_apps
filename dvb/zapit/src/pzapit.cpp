/*
 * $Id: pzapit.cpp,v 1.53 2005/12/21 17:13:38 mws Exp $
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

#include <cstdio>
#include <cstring>
#include <iostream>
#include <unistd.h> /* sleep */

#include <zapit/client/zapitclient.h>

int usage (const char * basename)
{
	std::cout << "bouquet list: " << basename << " [-ra]" << std::endl;
	std::cout << "channel list: " << basename << " [-ra] <bouquet-number>" << std::endl;
	std::cout << "zap by number: " << basename << " [-ra] <bouquet-number> <channel-number>" << std::endl;
	std::cout << "zap by name: " << basename << " [-ra] -n <channel-name>" << std::endl;
	std::cout << "set diseqc type: " << basename << " -dt <type>" << std::endl;
	std::cout << "set diseqc repeats: " << basename << " -dr <count>" << std::endl;
	std::cout << "(-ra toggles radio mode)" << std::endl;
	std::cout << "switch record mode on/off: " << basename << " -re" << std::endl;
	std::cout << "start/stop playback: " << basename << " -p" << std::endl
		  << std::endl;
	std::cout << "change audio pid: " << basename << " -a <audio-number>" << std::endl
		  << std::endl;
	std::cout << "reload channels bouquets: " << basename << " -c" << std::endl;
	std::cout << "save bouquets: " << basename << " -sb" << std::endl
		  << std::endl;
	std::cout << "show satellites: " << basename << " -sh" << std::endl;
	std::cout << "select satellites: " << basename << " -se <satmask> <diseqc order>" << std::endl;
	std::cout << "start transponderscan: " << basename << " -st" << std::endl
		  << std::endl;
	std::cout << "mute audio: " << basename << " -mute" << std::endl;
	std::cout << "unmute audio: " << basename << " -unmute" << std::endl;
	std::cout << "set volume: " << basename << " -vol <0..64>" << std::endl;
	std::cout << "register neutrino as event client: " << basename << " -rn" << std::endl;
	std::cout << "shutdown zapit: " << basename << " -kill" << std::endl;
	std::cout << "enter standby: " << basename << " -esb" << std::endl;
	std::cout << "leave standby: " << basename << " -lsb" << std::endl;
        std::cout << "switch to ntsc mode: " << basename << " --ntsc" << std::endl;
        std::cout << "switch to pal mode: " << basename << " --pal" << std::endl;
	std::cout << "send diseqc 1.2 motor command: " << basename << "-m <cmdtype> <addr> <cmd> <number of parameters> <parameter 1> <parameter 2>" << std::endl;
	std::cout << "activate Iec (requires aviaEXT driver): " << basename << " --iecon>" << std::endl;
	std::cout << "deactivate Iec (requires aviaEXT driver): " << basename << " --iecoff>" << std::endl;
	std::cout << "get Iec state (0=off, 1=on): " << basename << " --iecstate>" << std::endl;
	std::cout << "set decoder to PES mode (requires aviaEXT driver): " << basename << " --pes>" << std::endl;
	std::cout << "set decoder to SPTS mode (requires aviaEXT driver): " << basename << " --spts>" << std::endl;
	std::cout << "get decoder mode (0=PES, 1=SPTS): " << basename << " --decmode>" << std::endl;
	std::cout << "get current PIDs: " << basename << " --getpids>" << std::endl;
	return -1;
}

int main (int argc, char** argv)
{
	int i;
	uint32_t j;
	uint32_t k;

	int bouquet = -1;
	unsigned int channel = 0;
	unsigned int count;
	int diseqcRepeats = -1;
	int diseqcType = -1;
	int satmask = 0;
	int audio = 0;
	int mute = -1;
	int volume = -1;
	int nvod = -1;
	const char * channelName = NULL;

	bool playback = false;
	bool recordmode = false;
	bool radio = false;
	bool reload = false;
	bool register_neutrino = false;
	bool savebouquets = false;
	bool show_satellites = false;
        bool set_pal = false;
        bool set_ntsc = false;
	bool scan = false;
	bool zapByName = false;
	bool killzapit = false;
	bool enterStandby = false;
	bool leaveStandby = false;
	bool sendMotorCommand = false;
	bool Iecon = false;
	bool Iecoff = false;
	bool Iecstate = false;
	bool pes = false;
	bool spts = false;
	bool decmode = false;
	bool getpids = false;
	uint8_t motorCmdType = 0;
	uint8_t motorCmd = 0;
	uint8_t motorNumParameters = 0;
	uint8_t motorParam1 = 0;
	uint8_t motorParam2 = 0;
	uint8_t motorAddr = 0;
	uint32_t diseqc[5];
	unsigned int tmp;

	/* command line */
	for (i = 1; i < argc; i++)
	{
		if (!strncmp(argv[i], "-a", 2))
		{
			if (i < argc - 1)
			{
				sscanf(argv[++i], "%d", &audio);
				continue;
			}
		}
		else if (!strncmp(argv[i], "-dr", 3))
		{
			if (i < argc - 1)
			{
				sscanf(argv[++i], "%d", &diseqcRepeats);
				continue;
			}
		}
		else if (!strncmp(argv[i], "-dt", 3))
		{
			if (i < argc - 1)
			{
				sscanf(argv[++i], "%d", &diseqcType);
				continue;
			}
		}
		else if (!strncmp(argv[i], "-c", 2))
		{
			reload = true;
			continue;
		}
		else if (!strncmp(argv[i], "-esb", 4))
		{
			enterStandby = true;
			continue;
		}
		else if (!strncmp(argv[i], "-kill", 5))
		{
			killzapit = true;
			continue;
		}
		else if (!strncmp(argv[i], "-lsb", 4))
		{
			leaveStandby = true;
			continue;
		}
		else if (!strncmp(argv[i], "-motor", 6))
		{
			if (i < argc - 6)
			{
				sscanf(argv[++i], "%x", &tmp);
				motorCmdType = tmp;
				sscanf(argv[++i], "%x", &tmp);
				motorAddr = tmp;
				sscanf(argv[++i], "%x", &tmp);
				motorCmd = tmp;
				sscanf(argv[++i], "%x", &tmp);
				motorNumParameters = tmp;
				sscanf(argv[++i], "%x", &tmp);
				motorParam1 = tmp;
				sscanf(argv[++i], "%x", &tmp);
				motorParam2 = tmp;
				printf("[pzapit] motor command = %x %x %x %x %x %x\n", motorCmdType, motorAddr, motorCmd, motorNumParameters, motorParam1, motorParam2);
				sendMotorCommand = true;
				continue;
			}
		}
		else if (!strncmp(argv[i], "-rn", 3))
		{
			register_neutrino = true;
			continue;
		}
		else if (!strncmp(argv[i], "-mute", 5))
		{
			mute = 1;
			continue;
		}
		else if (!strncmp(argv[i], "-nvod", 5))
		{
			if (i < argc - 1)
			{
				sscanf(argv[++i], "%d", &nvod);
				continue;
			}
		}
		else if (!strncmp(argv[i], "-n", 2))
		{
			if (i < argc - 1)
			{
				zapByName = true;
				channelName = argv[++i];
				continue;
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
		else if (!strncmp(argv[i], "-sb", 3))
		{
			savebouquets = true;
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
                else if (!strncmp(argv[i], "--pal", 4))
                {
                        set_pal = true;
                        continue;
                }
        
                else if (!strncmp(argv[i], "--ntsc", 5))
                {
                        set_ntsc = true;
                        continue;
                }
		else if (!strncmp(argv[i], "-unmute", 7))
		{
			mute = 0;
			continue;
		}
                else if (!strncmp(argv[i], "--iecon", 7))
                {
                        Iecon = true;
                        continue;
                }
                else if (!strncmp(argv[i], "--iecoff", 8))
                {
                        Iecoff = true;
                        continue;
                }
                else if (!strncmp(argv[i], "--iecstate", 10))
                {
                        Iecstate = true;
                        continue;
                }
                else if (!strncmp(argv[i], "--pes", 5))
                {
                        pes = true;
                        continue;
                }
                else if (!strncmp(argv[i], "--spts", 6))
                {
                        spts = true;
                        continue;
                }
                else if (!strncmp(argv[i], "--decmode", 9))
                {
                        decmode = true;
                        continue;
                }
		else if (!strncmp(argv[i], "-vol", 4))
		{
			if (i < argc - 1)
			{
				sscanf(argv[++i], "%d", &volume);
				continue;
			}
		}
		else if (!strncmp(argv[i], "--getpids", 9)) 
		{
			getpids = true;
			continue;
		}
		else if (i < argc - 1)
		{
			if ((sscanf(argv[i], "%d", &bouquet) > 0) && (sscanf(argv[++i], "%u", &channel) > 0))
				continue;
		}
		else if (sscanf(argv[i], "%d", &bouquet) > 0)
			continue;

		return usage(argv[0]);
	}

	/* create zapit client */
	CZapitClient zapit;

	/* send diseqc 1.2 motor command */
	if (sendMotorCommand)
	{
		zapit.sendMotorCommand(motorCmdType, motorAddr, motorCmd, motorNumParameters, motorParam1, motorParam2);
	}
	
	/* kill zapit*/
	if (killzapit)
	{
		zapit.shutdown();
		std::cout << "zapit shot down :)" << std::endl;
		return 0;
	}

	if (enterStandby)
	{
		zapit.setStandby(true);
		return 0;
	}

	if (leaveStandby)
	{
		zapit.setStandby(false);
		return 0;
	}

	/* audio mute */
	if (mute != -1)
	{
		std::cout << "mute/unmute" << std::endl;
		zapit.muteAudio(mute);
		return 0;
	}

	if (volume != -1)
	{
		std::cout << "set volume" << std::endl;
		zapit.setVolume(volume, volume);
		return 0;
	}

	if (Iecon)
	{
		std::cout << "Iec on" << std::endl;
		zapit.IecOn();
		return 0;
	}
	if (Iecoff)
	{
		std::cout << "Iec off" << std::endl;
		zapit.IecOff();
		return 0;
	}
	if (Iecstate)
	{
		std::cout << "Iec state = " << zapit.IecState() << std::endl;
		return 0;
	}

	if (pes)
	{
		std::cout << "set decoder to PES mode" << std::endl;
		zapit.PlaybackPES();
		return 0;
	}
	if (spts)
	{
		std::cout << "set decoder to SPTS mode" << std::endl;
		zapit.PlaybackSPTS();
		return 0;
	}
	if (decmode)
	{
		std::cout << "decoder mode = " << zapit.PlaybackState() << std::endl;
		return 0;
	}

	/* reload services */
	if (reload)
	{
		std::cout << "reloading channels" << std::endl;
		zapit.reinitChannels();
		return 0;
	}

	if (register_neutrino)
	{
#define NEUTRINO_UDS_NAME "/tmp/neutrino.sock"
		std::cout << "registering neutrino" << std::endl;
		for (int i = CZapitClient::FIRST_EVENT_MARKER; i < CZapitClient::LAST_EVENT_MARKER; i++)
			zapit.registerEvent(i, 222, NEUTRINO_UDS_NAME);
		return 0;
	}

	if (diseqcType != -1)
	{
		zapit.setDiseqcType((diseqc_t) diseqcType);

		if (diseqcRepeats == -1)
			return 0;
	}

	if (diseqcRepeats != -1)
	{
		zapit.setDiseqcRepeat(diseqcRepeats);
		return 0;
	}

	if (playback)
	{
		if (zapit.isPlayBackActive())
			zapit.stopPlayBack();
		else
			zapit.startPlayBack();

		if (!recordmode)
			return 0;
	}

	if (recordmode)
	{
		zapit.setRecordMode(!zapit.isRecordModeActive());
		return 0;
	}

	if (savebouquets)
	{
		zapit.saveBouquets();
		return 0;
	}

	if (show_satellites)
	{
		std::vector<CZapitClient::responseGetSatelliteList> satelliteList;
		zapit.getScanSatelliteList(satelliteList);

		std::vector<CZapitClient::responseGetSatelliteList>::const_iterator rI;
		for (i = 0, rI = satelliteList.begin(); rI != satelliteList.end(); i++, rI++)
			std::cout << (1 << i) << ": " << rI->satName << std::endl;

		return 0;
	}
	else if (satmask != 0)
	{
		std::vector<CZapitClient::responseGetSatelliteList> satelliteList;
		zapit.getScanSatelliteList(satelliteList);

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
				newSatelliteList.push_back(item);
			}

			if ((j >= diseqc[0]) || (k >= satelliteList.size()))
			{
				break;
			}
		}

		zapit.setScanSatelliteList(newSatelliteList);

		return 0;
	}

	/* transponderscan */
	if (scan)
	{
		unsigned int satellite;
		unsigned int processed_transponder;
		unsigned int transponder;
		unsigned int services;
		
		zapit.startScan(1);

		while (zapit.isScanReady(satellite, processed_transponder, transponder, services) == false)
		{
			std::cout << "satellite: " << satellite << ", transponder: " << processed_transponder <<", of: " << transponder << ", services: " << services << std::endl;
			sleep(1);
		}

		return 0;
	}

        if (set_pal)
        {
                zapit.stopPlayBack();
                zapit.setVideoSystem_a(PAL);
                zapit.startPlayBack();
                return 0;
        }
                 
        if (set_ntsc)
        {
                zapit.stopPlayBack();
                zapit.setVideoSystem_a(NTSC);
                zapit.startPlayBack();
                return 0;
        }


	/* choose source mode */
	zapit.setMode(radio ? CZapitClient::MODE_RADIO : CZapitClient::MODE_TV);

	/* set audio channel */
	if (audio)
	{
		zapit.setAudioChannel(audio - 1);
		return 0;
	}

	if (nvod != -1)
	{
		zapit.zaptoNvodSubService(nvod);
		return 0;
	}
	else
	{
		std::vector<CZapitClient::responseGetBouquetChannels> channels;

		if (zapByName)
		{
			zapit.getChannels(channels);

			std::vector<CZapitClient::responseGetBouquetChannels>::const_iterator ch_resp;
			for (ch_resp = channels.begin(), channel = 1; ch_resp != channels.end(); ch_resp++, channel++)
			{
				if (!strcasecmp(ch_resp->name, channelName))
				{
					std::cout << "found channel number: " << channel << std::endl;
					goto channel_found;
				}
			}
			
			std::cout << "channel not found." << std::endl;
			return 0;
		}
		else /* zap by bouquet number and channel number */
		{
			if (getpids && bouquet == -1) {
				goto getpids;
			}
			/* read channel list */
			if (bouquet != -1)
				zapit.getBouquetChannels(bouquet - 1, channels);

			/* display bouquet list */
			else
			{
				std::vector<CZapitClient::responseGetBouquets> bouquets;
				std::vector<CZapitClient::responseGetBouquets>::const_iterator b_resp;

				zapit.getBouquets(bouquets, false);

				for (b_resp = bouquets.begin(); b_resp != bouquets.end(); b_resp++)
					std::cout << (b_resp->bouquet_nr + 1) << ": " << b_resp->name << std::endl;

				return 0;
			}

			/* display channel list */
			if (!channel)
			{
				std::vector<CZapitClient::responseGetBouquetChannels>::const_iterator ch_resp;
				for (ch_resp = channels.begin(), channel = 1; ch_resp != channels.end(); ch_resp++, channel++)
					std::cout << channel << ": " << ch_resp->name << std::endl;
				return 0;
			}
		}

		/* zap */
		if (channel > channels.size())
		{
			std::cout << "Only " << channels.size() << " channels in bouquet " << bouquet << std::endl;
			return 0;
		}

	channel_found:
		zapit.zapTo(channels[channel-1].nr);
		std::cout << "zapped to " << channels[channel-1].name << std::endl;
	}

	
getpids:
	{	
		CZapitClient::responseGetPIDs pids;
		zapit.getPIDS(pids);

		if (pids.PIDs.vpid)
			std::cout << "   video: 0x" << std::hex << pids.PIDs.vpid << std::endl;

		if (pids.PIDs.ecmpid) 
			std::cout << "  ecmpid: 0x" << std::hex << pids.PIDs.ecmpid << std::endl;

		if (pids.PIDs.vtxtpid)
			std::cout << "teletext: 0x" << std::hex << pids.PIDs.vtxtpid << std::endl;

		if (pids.PIDs.pcrpid)
			std::cout << "     pcr: 0x" << std::hex << pids.PIDs.pcrpid << std::endl;

		if (pids.PIDs.pmtpid)
			std::cout << "     pmt: 0x" << std::hex << pids.PIDs.pmtpid << std::endl;

		if (pids.PIDs.privatepid)
			std::cout << " private: 0x" << std::hex << pids.PIDs.privatepid << std::endl;

		for (count = 0; count < pids.APIDs.size(); count++)
		{
			if (count == pids.PIDs.selected_apid)
				std::cout << "*";
			else 
				std::cout << " ";
	
			std::cout << "audio " << std::dec << count + 1 << ": 0x" << std::hex << pids.APIDs[count].pid << " (" << pids.APIDs[count].desc;
			if (pids.APIDs[count].is_ac3)
				std::cout << ", ac3";
			std::cout << ")" << std::endl;
		}
	}

	return 0;
}
