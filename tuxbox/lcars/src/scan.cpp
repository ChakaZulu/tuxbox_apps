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
Revision 1.3  2001/12/07 14:10:33  rasc
Fixes for SAT tuning and Diseqc. Diseqc doesn't work properly for me (diseqc 2.0 switch).
Someone should check this please..

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <unistd.h>
#include <stdio.h>
#include "scan.h"

scan::scan(settings &s, pat &p1, pmt &p2, nit &n, sdt &s1, osd *o, tuner &t) : setting(s), pat_obj(p1), pmt_obj(p2), nit_obj(n), sdt_obj(s1), osd_obj(o), tuner_obj(t)
{

}

channels scan::scanChannels(bool full = false, int start_frequency = -1, int start_symbol = -1, int start_polarization = -1, int start_fec = -1)
{
	int number;
	channels channels(setting, pat_obj, pmt_obj);
	bool badcable = false;

	//settings settings;
	(*osd_obj).createScan();
	(*osd_obj).addCommand("SHOW scan");
	sleep(1);

	(*osd_obj).setScanProgress(0);
	(*osd_obj).setScanChannelNumber(0);


	if (setting.boxIsCable())
	{
		start_frequency = 3460;
		start_symbol = 6900;
		(*osd_obj).createPerspective();
		

		while(channels.numberTransponders() < 1)
		{
			char message[100];
			sprintf(message, "Searching NIT on %d - %d", start_frequency, start_symbol);
			(*osd_obj).setPerspectiveName(message);
			(*osd_obj).addCommand("SHOW perspective");
			
			tuner_obj.tune(start_frequency, start_symbol);

			number = nit_obj.getTransportStreams(&channels);
			channels.dumpTS();
			start_frequency += 80;
			if (start_frequency > 4000)
				break;
		}

		if (channels.numberTransponders() < 1)
		{
			(*osd_obj).setPerspectiveName("Sorry, no NIT found! Check cables!!!");
			(*osd_obj).addCommand("SHOW perspective");
			
			exit(-1);
		}

		channels.setBeginTS();
		int test_frequ = channels.getCurrentFrequency();

		if (test_frequ > 10000)
		{
			(*osd_obj).setPerspectiveName("Your cable-company sucks! Manually searching...");
			badcable = true;
			(*osd_obj).addCommand("SHOW perspective");

			channels.clearTS();

			sleep(5);

			for (int i = 3300; i < 4600; i += 80)
			{
				char message[100];
				sprintf(message, "Checking %d - %d", i, 6900);
				(*osd_obj).setPerspectiveName(message);
				(*osd_obj).addCommand("SHOW perspective");

				tuner_obj.tune(i, 6900);

				if (pat_obj.readPAT())
				{
					channels.addTS(pat_obj.getTS(), pat_obj.getONID(), i, 6900);
				}
				

			}

		}


	}
	else if (setting.boxIsSat())
	{
		int max_chans = 2;

		int start_frq[10];
		int start_sym[10];
		int start_pol[10];
		int start_fe[10];

		FILE *fp;

		fp = fopen("/var/lcars/scanlist.dat", "r");
		int co = 0;
		while(!feof(fp))
		{
			char text[100];

			printf("Starting at co: %d\n", co);
			
			fgets(text, 255, fp);
			start_frq[co] = atoi(text);
			printf("%s - %d\n", text, start_frq[co]);
			fgets(text, 255, fp);
			start_sym[co] = atoi(text);
			fgets(text, 255, fp);
			start_pol[co] = atoi(text);
			fgets(text, 255, fp);
			start_fe[co++] = atoi(text);
		}
		max_chans = co;

		for (int dis = 0; dis < 3; dis++)
		{
			printf("Diseqc: %d\n", dis);		
			int i = 0;

			int old = channels.numberTransponders();
			while (i < max_chans)
			{
				printf("i: %d\n", i);
				printf("numtrans: %d - old: %d\n", channels.numberTransponders(), old);
				char message[100];
				sprintf(message, "Searching NIT on %d - %d - %d - %d", start_frq[i], start_sym[i], start_pol[i], start_fe[i]);
				(*osd_obj).setPerspectiveName(message);
				(*osd_obj).addCommand("SHOW perspective");

				printf ("Start tuning\n");
	
				tuner_obj.tune(start_frq[i], start_sym[i], start_pol[i], start_fe[i], dis);
				
				printf("FInished tuning\n");	

				printf ("Start NIT\n");
				number = nit_obj.getTransportStreams(&channels, dis);
				printf ("End NIT\n");
				i++;
	
			
			}
		}
		
		if (channels.numberTransponders() < 1)
		{
			(*osd_obj).setPerspectiveName("Sorry, no NIT found! Check cables!!!");
			(*osd_obj).addCommand("SHOW perspective");
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
	(*osd_obj).setPerspectiveName(message);
	(*osd_obj).addCommand("SHOW perspective");

	do
	{
		channels.tuneCurrentTS(&tuner_obj);	

		printf("getChannels - Start\n");
		sdt_obj.getChannels(&channels);
			
		if (full)
		{
			pat_obj.readPAT();
			for (int i = numberChannels; i < channels.numberChannels(); i++)
			{
				channels.setCurrentChannel(i);
				channels.setCurrentPMT(pat_obj.getPMT(channels.getCurrentSID()));

				pmt_data pmt_entry;
				if (channels.getCurrentPMT() != 0)
				{
					pmt_entry = pmt_obj.readPMT(channels.getCurrentPMT());
					
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
						if (setting.getCAID() == pmt_entry.CAID[j])
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

		(*osd_obj).setScanChannelNumber(channels.numberChannels());
		printf("getChannels - Finish\n");
		count++;
		(*osd_obj).setScanProgress((int)(((float)count / numberTS) * 100));
	} while(channels.setNextTS());

	(*osd_obj).addCommand("HIDE scan");
	(*osd_obj).addCommand("HIDE perspective");
	(*osd_obj).hidePerspective();

	printf("Found channels: %d\n", channels.numberChannels());
	channels.saveDVBChannels();
	
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
