/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: scan.cpp,v $
Revision 1.9  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.3  2001/12/17 01:00:41  tux
scan.cpp fix

Revision 1.7  2001/12/17 00:18:18  obi
readded revision 1.5

Revision 1.5  2001/12/07 23:12:31  rasc
scanfile.dat reorganized to handle more transponders,
some small fixes

Revision 1.3  2001/12/07 14:10:33  rasc
Fixes for SAT tuning and Diseqc. Diseqc doesn't work properly for me (diseqc 2.0 switch).
Someone should check this please..

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include "scan.h"

scan::scan(settings *s, pat *p1, pmt *p2, nit *n, sdt *s1, osd *o, tuner *t, channels *c)
{
        setting = s;
        pat_obj = p1;
        pmt_obj = p2;
        nit_obj = n;
        sdt_obj = s1;
        osd_obj = o;
        tuner_obj = t;
		channels_obj = c;
}

void scan::readUpdates()
{
	channels tmp_channels(setting, pat_obj, pmt_obj);
	sdt_obj->getChannels(&tmp_channels);

	bool changed = false;
	bool new_channels = false;

	int old_number = channels_obj->getCurrentChannelNumber();
	for (int i = 0; i < tmp_channels.numberChannels(); i++)
	{
		channel tmp_channel = tmp_channels.getChannelByNumber(i);
		int channelnumber = channels_obj->getChannelNumber(tmp_channel.TS, tmp_channel.ONID, tmp_channel.SID);
		if (channelnumber == -1)
		{
			channels_obj->addChannel(tmp_channel);
			changed = true;
			new_channels = true;
		}
		else
		{
			channel tmp_channel_old = channels_obj->getChannelByNumber(channelnumber);
			if (strcmp(tmp_channel.serviceName, tmp_channel_old.serviceName))
			{
				channels_obj->updateChannel(channelnumber, tmp_channel);
				if (channelnumber == old_number)
					osd_obj->setServiceName(tmp_channel.serviceName);
				changed = true;
			}
		}
	}
	channels_obj->setCurrentChannel(old_number);
	if (changed)
		channels_obj->saveDVBChannels();
	if (new_channels)
	{
		osd_obj->setPerspectiveName("New Channels found and added!!!!");
		osd_obj->addCommand("SHOW perspective");
		sleep(5);
		osd_obj->addCommand("HIDE perspective");
	}
}


channels scan::scanChannels(bool full = false, int start_frequency = -1, int start_symbol = -1, int start_polarization = -1, int start_fec = -1)
{
	int number;
	channels channels(setting, pat_obj, pmt_obj);
	bool badcable = false;

	//settings settings;
	osd_obj->createScan();
	osd_obj->addCommand("SHOW scan");
	sleep(1);

	osd_obj->setScanProgress(0);
	osd_obj->setScanChannelNumber(0);


	if (setting->boxIsCable())
	{
		start_frequency = 3460;
		start_symbol = 6900;
		osd_obj->createPerspective();
		

		while(channels.numberTransponders() < 1)
		{
			char message[100];
			sprintf(message, "Searching NIT on %d - %d", start_frequency, start_symbol);
			osd_obj->setPerspectiveName(message);
			osd_obj->addCommand("SHOW perspective");
			
			tuner_obj->tune(start_frequency, start_symbol);

			number = nit_obj->getTransportStreams(&channels);
			channels.dumpTS();
			start_frequency += 80;
			if (start_frequency > 4000)
				break;
		}

		if (channels.numberTransponders() < 1)
		{
			osd_obj->setPerspectiveName("Sorry, no NIT found! Check cables!!!");
			osd_obj->addCommand("SHOW perspective");
			
			exit(-1);
		}

		channels.setBeginTS();
		int test_frequ = channels.getCurrentFrequency();

		if (test_frequ > 10000)
		{
			osd_obj->setPerspectiveName("Your cable-company sucks! Manually searching...");
			badcable = true;
			osd_obj->addCommand("SHOW perspective");

			channels.clearTS();

			sleep(5);

			for (int i = 3300; i < 4600; i += 80)
			{
				char message[100];
				sprintf(message, "Checking %d - %d", i, 6900);
				osd_obj->setPerspectiveName(message);
				osd_obj->addCommand("SHOW perspective");

				tuner_obj->tune(i, 6900);

				if (pat_obj->readPAT())
				{
					channels.addTS(pat_obj->getTS(), pat_obj->getONID(), i, 6900);
				}
				

			}

		}


	}
	else if (setting->boxIsSat())
	{
		int max_chans = 2;

		int start_frq[20];	// see: tune
		int start_sym[20];
		int start_pol[20];
		int start_fe[20];
		int start_dis[20];	// start diseq 0..3, -1 = auto
		FILE *fp;
		int dis, dis_start, dis_end;



		fp = fopen(CONFIGDIR "/lcars/scanlist.dat", "r");
		int co = 0;
		while(!feof(fp))
		{
			char text[256];

			printf("Starting at co: %d\n", co);
	
			fgets(text, 255, fp);
			if (!isdigit(*text)) continue;
			sscanf (text,"%i,%i,%i,%i,%i %s\n",
				&start_frq[co], &start_sym[co],
				&start_pol[co], &start_fe[co],
				&start_dis[co]);
			printf ("Scandat: Freq:%d, SymR:%d, Pol:%d, FEC:%d DiSeqc:%d\n",
				start_frq[co], start_sym[co],
				start_pol[co], start_fe[co],
				start_dis[co]);
			co++;
			if (co >= (sizeof(start_fe)/sizeof(start_fe[0])) )
				break;
		}
		max_chans = co;


		printf("Diseqc: %d\n", dis);		
		int i = 0;

		int old = channels.numberTransponders();
		while (i < max_chans)
		{
			printf("StartDef: %d\n", i);
			printf("numtrans: %d - old: %d\n", channels.numberTransponders(), old);

			if (start_dis[i]  < 0) {
				dis_start = 0;
				dis_end   = 3;	
			} else {
				dis_start = dis_end = start_dis[i];
			}

			for (int dis = dis_start; dis <= dis_end; dis++) 
			{
				char message[255];
				sprintf(message, "Searching on %d - %d - %d - %d - %d",
					start_frq[i], start_sym[i],
					start_pol[i], start_fe[i], dis);
				osd_obj->setPerspectiveName(message);
				osd_obj->addCommand("SHOW perspective");

				printf ("Start tuning\n");
	
				tuner_obj->tune(start_frq[i], start_sym[i], start_pol[i], start_fe[i], dis);
				
				printf("FInished tuning\n");	

				printf ("Start NIT\n");
				number = nit_obj->getTransportStreams(&channels, dis);
				printf ("End NIT\n");
			}

			i++;
		}
		
		if (channels.numberTransponders() < 1)
		{
			osd_obj->setPerspectiveName("Sorry, no NIT found! Check cables!!!");
			osd_obj->addCommand("SHOW perspective");
			exit(-1);
		}
	}

	printf("Transponders found: %d\n", channels.numberTransponders());	
	sleep(5);
	int count = 0;
	int numberTS = channels.numberTransponders();
	int numberChannels = 0;
	
	channels.setBeginTS();
	
	char message[100];
	sprintf(message, "Scanning Channels");
	osd_obj->setPerspectiveName(message);
	osd_obj->addCommand("SHOW perspective");

	do
	{
		channels.tuneCurrentTS(tuner_obj);	

		printf("getChannels - Start\n");
		sdt_obj->getChannels(&channels);
			
		if (full)
		{
			pat_obj->readPAT();
			for (int i = numberChannels; i < channels.numberChannels(); i++)
			{
				channels.setCurrentChannel(i);
				channels.setCurrentPMT(pat_obj->getPMT(channels.getCurrentSID()));

				pmt_data pmt_entry;
				if (channels.getCurrentPMT() != 0)
				{
					pmt_entry = pmt_obj->readPMT(channels.getCurrentPMT());
					
					channels.setCurrentPCR(pmt_entry.PCR);

					channels.deleteCurrentAPIDs();
					for (int j = 0; j < pmt_entry.pid_counter; j++)
					{
						if (pmt_entry.type[j] == 0x02)
						{
							channels.setCurrentVPID(pmt_entry.PID[j]);
						}
						else if (pmt_entry.type[j] == 0x04 || pmt_entry.type[j] == 0x03)
						{
							channels.addCurrentAPID(pmt_entry.PID[j]);
						}
					}
		
					for (int j = 0; j < pmt_entry.ecm_counter; j++)
					{
						if (setting->getCAID() == pmt_entry.CAID[j])
							channels.addCurrentCA(pmt_entry.CAID[j], pmt_entry.ECM[j]);
					}
				}
				else
				{
					channels.deleteCurrentAPIDs();
					channels.setCurrentVPID(0x1fff);
					channels.addCurrentAPID(0x1fff);
				}


				numberChannels = channels.numberChannels();
		
			}
		}

		osd_obj->setScanChannelNumber(channels.numberChannels());
		printf("getChannels - Finish\n");
		count++;
		osd_obj->setScanProgress((int)(((float)count / numberTS) * 100));
	} while(channels.setNextTS());

	osd_obj->addCommand("HIDE scan");
	osd_obj->addCommand("HIDE perspective");
	osd_obj->hidePerspective();

	printf("Found channels: %d\n", channels.numberChannels());
	//channels.saveDVBChannels();
	
	return channels;
}

void scan::updateChannels(channels *chan)
{
	channels newchannels = scanChannels();

	int newsort[(*chan).numberChannels() + newchannels.numberChannels()];
	int notfound[newchannels.numberChannels()];
	int numbernotfoundchannels = 0;

	printf("Starting update-Compare\n");
	printf("Old: %d - New: %d\n", (*chan).numberChannels(), newchannels.numberChannels());
	for (int i = 0; i < (*chan).numberChannels(); i++)
		newsort[i] = -1;
	
	printf("----> 1\n");
	for (int i = 0; i < newchannels.numberChannels(); i++)
	{
		printf("%d\n", i);
		channel tmp_chan = newchannels.getChannelByNumber(i);
		
		int chan_num = (*chan).getChannelNumber(tmp_chan.TS, tmp_chan.ONID, tmp_chan.SID);

		printf("Channelnumber: %d\n", chan_num);
		if (chan_num != -1)
			newsort[chan_num] = i;
		else
		{
			notfound[numbernotfoundchannels++] = i;
		}
	}
	printf("----> 2\n");
	

	for (int i = 0; i < numbernotfoundchannels; i++)
	{
		newsort[(*chan).numberChannels() + i] = notfound[i];
	}

	printf("----> 3\n");
	channel empty_chan;
	memset (&empty_chan, 0, sizeof(struct channel));
	strcpy(empty_chan.serviceName, "--> Deleted");
	
	printf("Complete: %d\n", (*chan).numberChannels() + numbernotfoundchannels);
	printf("----> 4\n");
	int numbercomplete = (*chan).numberChannels() + numbernotfoundchannels;
	
	(*chan).clearChannels();
	
	for (int i = 0; i < numbercomplete; i++)
	{
		channel tmp_chan;

		if (newsort[i] != -1)
			tmp_chan = newchannels.getChannelByNumber(newsort[i]);
		else
			tmp_chan = empty_chan;

		(*chan).addChannel(tmp_chan);
	}

	newchannels.setBeginTS();
	for (int i = 0; i < newchannels.numberTransponders(); i++)
	{
		printf("Adding TS %x ONID %x Frequ %d\n", newchannels.getCurrentSelectedTS(), newchannels.getCurrentSelectedONID(), newchannels.getCurrentFrequency());
		(*chan).addTS(newchannels.getCurrentSelectedTS(), newchannels.getCurrentSelectedONID(), newchannels.getCurrentFrequency(), newchannels.getCurrentSymbolrate(), newchannels.getCurrentPolarization(), newchannels.getCurrentFEC());
		newchannels.setNextTS();
	}

}
