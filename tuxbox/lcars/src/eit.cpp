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
$Log: eit.cpp,v $
Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <stdio.h>

#include <ost/dmx.h>

#include "eit.h"
#include "help.h"
#include "osd.h"
#include "descriptors.h"

#define BSIZE 10000

int eit::start_thread()
{
	
	int status;
  	
	pthread_mutex_init(&mutex, NULL);
	status = pthread_create( &eitThread,
                           NULL,
                           start_eitqueue,
                           (void *)this );
	return status;

}

void* eit::start_eitqueue( void * this_ptr )
{
	eit *e = (eit *) this_ptr;
	time_t next_time = time(0) + 10000;

	while(1)
	{
		while(e->isEmpty() && time(0) < next_time)
		{	
			usleep(150);
			//printf("time: %d - nexttime: %d\n", time(0), next_time);
		}
		if (time(0) >= next_time)
			e->addCommand("RECEIVE last");
		e->executeQueue();
		next_time = time(0) + 60;
	}
}

eit::eit(settings *s, osd *o)
{
	setting = s;
	number_perspectives = 0;
	osd_obj = o;
}

void eit::addCommand(std::string command)
{
	while(!command_queue.empty())
		command_queue.pop();
	command_queue.push(command);
	printf("EIT-Command: %s\n", command.c_str());
}

void eit::executeQueue()
{
	while(!isEmpty())
	{
		executeCommand();
	}
}

void eit::executeCommand()
{
	std::string command = command_queue.front();
	command_queue.pop();

	std::istringstream iss(command);
	std::getline(iss, command, ' ');

	if(command == "RECEIVE")
	{
		std::string SID_str;
		std::getline(iss, SID_str, ' ');

		if (SID_str != "last")
			lastSID = atoi(SID_str.c_str());
		receiveNow(lastSID);
		
	}
}


void eit::receiveNow(int SID)
{
	long fd, r;
	struct dmxSctFilterParams flt;
	//unsigned char sec_buffer[10][BSIZE];
	unsigned char buffer[BSIZE];

	number_perspectives = 0;
	curr_linkage = 0;
	// Lies den EIT
	
	printf("Checking mutex\n");
	pthread_mutex_lock( &mutex );
	printf("Mutex passed\n");
	fd=open("/dev/ost/demux0", O_RDONLY);
	
	memset (&flt.filter, 0, sizeof (struct dmxFilter));
	
	eventlist.clear();
	printf("Anfangs-Events: %i\n", eventlist.size());
	flt.pid            = 0x12;
	flt.filter.filter[0] = 0x4e;
	flt.filter.mask[0] = 0xFF;
	flt.timeout        = 10000;
	//flt.timeout        = 0;
	flt.flags          = DMX_IMMEDIATE_START | DMX_CHECK_CRC; //| DMX_ONESHOT;
	
	ioctl(fd, DMX_SET_FILTER, &flt);
	
	//int sec_counter = 0;
	//int last_section = -1;
	//int counter = 0;
	//bool first = false;

	int start_sid, start_section_number;
	r = BSIZE;	
	memset (&buffer, 0, BSIZE);
	r = read(fd, buffer, 3);
	int section_length = ((buffer[1] & 0xf) << 8) | buffer[2];
	r = read(fd, buffer + 3, section_length);
	start_sid = (buffer[3] << 8) | buffer[4];
	start_section_number = buffer[6];
	
	
	memset (&now, 0, sizeof (struct event));
	memset (&next, 0, sizeof (struct event));
	bool quit = false;
	int sec_count = 0;
	do
	{
		sec_count++;
		r = BSIZE;	
		memset (&buffer, 0, BSIZE);
		
		r = read(fd, buffer, 3);
		int section_length = ((buffer[1] & 0xf) << 8) | buffer[2];
		r = read(fd, buffer + 3, section_length);

		int count = 13;

		//printf("------------\nSID_EIT: %x\n", ((buffer[3] << 8) | buffer[4]));
		//printf("TS_EIT: %x\n", ((buffer[8] << 8) | buffer[9]));
		//printf("ONID_EIT: %x\n------------\n", ((buffer[10] << 8) | buffer[11]));

		if (SID == ((buffer[3] << 8) | buffer[4]))
		{
			event tmp_event;
			memset (&tmp_event, 0, sizeof (struct event));

			int time = (buffer[count + 5] << 16) | (buffer[count + 6] << 8) | buffer[count + 7];
			int mjd = (buffer[count + 3] << 8) | buffer[count + 4];
			time_t starttime = dvbtimeToLinuxTime(mjd, time) + ((*setting).getTimeOffset() * 60L);
			printf("Offset: %d\n", (*setting).getTimeOffset());

			tmp_event.duration = (((buffer[count + 8] & 0xf0) >> 4) * 10 + (buffer[count + 8] & 0xf)) * 3600 + (((buffer[count + 9] & 0xf0) >> 4) * 10 + (buffer[count + 9] & 0xf)) * 60 + (((buffer[count + 10] & 0xf0) >> 4) * 10 + (buffer[count + 10] & 0xf));
			printf("Duration: %02x %02x %02x\n", buffer[count + 8], buffer[count + 9], buffer[count + 10]);
			printf("Duration: %d\n", tmp_event.duration);
			tmp_event.starttime = starttime;

			tmp_event.eventid = (buffer[count + 1] << 8) | buffer[count + 2];
			tmp_event.running_status = ((buffer[count + 11] & 0xe0) >> 5);
	
			int start = 26;
			int descriptors_length = ((buffer[13 + 11] & 0xf) << 8) | buffer[13 + 12];
			int text_length = 0;
			printf("Descriptoren-Länge: %x\n", ((buffer[13 + 11] & 0xf) << 8) | buffer[13 + 12]);
			while (start < 20 + descriptors_length)
			{
				printf("Type: %x\n", buffer[start]);
				if (buffer[start] == 0x4d) // short_event_descriptor
				{	
					std::string tmp_string;
					int event_name_length = buffer[start + 5];
					for (int i = 0; i < event_name_length; i++)
						tmp_event.event_name[i] = buffer[start + 6 + i];
					tmp_event.event_name[event_name_length] = '\0';
					printf("eit: %s\n", tmp_event.event_name);
					

					int text_length = buffer[start + 6 + event_name_length];
					for (int i = 0; i < text_length; i++)
						tmp_event.event_short_text[i] = buffer[start + 7 + event_name_length + i];
					tmp_event.event_short_text[text_length] = '\0';
					

				}
				else if (buffer[start] == 0x4a/* && ((buffer[count + 11] & 0xe0) >> 5) == 0x04*/) // linkage
				{
					printf("---> linkage_descriptor: <---\n");
					//memset (&linkage_descr[number_perspectives], 0, sizeof (struct linkage));
					tmp_event.linkage_descr[tmp_event.number_perspectives].TS = (buffer[start + 2] << 8) | buffer[start + 3];
					tmp_event.linkage_descr[tmp_event.number_perspectives].ONID = (buffer[start + 4] << 8) | buffer[start + 5];
					tmp_event.linkage_descr[tmp_event.number_perspectives].SID = (buffer[start + 6] << 8) | buffer[start + 7];
					
					char name[100];
					if (buffer[start + 8] != 0xb0)
					{
						for (int i = 9; i <= buffer[start + 1]; i++)
						{
							printf ("%02x ", buffer[start + i]);
						}
						printf("\n");
					}
					else // Formel 1 Perspektiven - Linkage type 0xb0
					{
						printf("Linkage\n");
						for (int i = 9; i <= buffer[start + 1] + 1; i++)
						{
							name[i - 9] = buffer[start + i]; // Namen der Perspektiven
						}
						name[buffer[start + 1] - 7] = '\0';
						strcpy(tmp_event.linkage_descr[tmp_event.number_perspectives].name, name);
						//printf("%s\n", pname.c_str());
					}
					tmp_event.number_perspectives++;
				}
				else if (buffer[start] == 0x4e) // extended_event_descriptor
				{
					int pos = start + 6 + buffer[start + 6];
					
					int i;
					for (i = 0; i < buffer[pos + 1]; i++)
						tmp_event.event_extended_text[i + text_length] = buffer[pos + 2 + i];
				
					text_length += buffer[pos + 1];
				}
				else if (buffer[start] == 0x50) // component_descriptor - audio-names
				{
					/*printf("---> component_descriptor: <---\n");
					printf ("reserved_future_use: %01x\n", (buffer[start + 2] & 0xf0) >> 4);
					printf ("stream_content: %01x\n", buffer[start + 2] & 0xf);
					printf ("component_type: %02x\n", buffer[start + 3]);
					printf ("component_tag: %02x\n", buffer[start + 4]);
					printf ("language_code: %c%c%c\n", buffer[start + 5], buffer[start + 6], buffer[start + 7]);
					printf ("text: ");
					*/
					tmp_event.component_tag[tmp_event.number_components] = buffer[start + 4];
					tmp_event.stream_content[tmp_event.number_components] = buffer[start + 2] & 0xf;
					tmp_event.component_type[tmp_event.number_components] = buffer[start + 3];
					printf("Found component TAG: %x\n", buffer[start + 4]);

					for (int i = 7; i <= buffer[start + 1]; i++)
					{
						tmp_event.audio_description[tmp_event.number_components][i-7] = buffer[start + 1 + i];
					}
					tmp_event.audio_description[tmp_event.number_components][buffer[start + 1] - 6] = '\0';
					printf("%s\n", tmp_event.audio_description[tmp_event.number_components]);
					tmp_event.number_components++;
	
				}
				else if (buffer[start] == 0x55)
				{
					printf("---> parental_rating_descriptor: <---\n");
					for (int i = 0; i < buffer[start + 1] / 4; i++)
					{
						if (buffer[start + 2 + i * 4] == 'D' && buffer[start + 3 + i * 4] == 'E' && buffer[start + 4 + i * 4] == 'U')
							tmp_event.par_rating = buffer[start + 5 + i * 4];
					}
				}

				start += buffer[start + 1] + 2;

				
			}
			printf("Number of perspectives: %d\n", number_perspectives);
			tmp_event.event_extended_text[text_length] = '\0';
			if (eventlist.count((int)tmp_event.starttime) == 0)
				eventlist.insert(std::pair<int, struct event>((int) tmp_event.starttime, tmp_event));
			printf("Gefunden: %x\n", ((buffer[3] << 8) | buffer[4]));
			
		}

	} while((!((start_sid == ((buffer[3] << 8) | buffer[4])) && (start_section_number == buffer[6]))) && !quit && (sec_count < 100));
	ioctl(fd,DMX_STOP,0);
	
	close(fd);
	pthread_mutex_unlock( &mutex );

	printf("eitrun\n");
	std::multimap<int, struct event>::iterator it = eventlist.begin();
	printf("Gefundene Events: %i\n", eventlist.size());
	for (int i = 0; i < (int)eventlist.size(); i++)
	{
		if (i == 0)
			now = (*it).second;
		if (i == 1)
			next = (*it).second;
		struct tm *t;
		t = localtime(&(*it).second.starttime);
		char acttime[10];
		strftime(acttime, sizeof acttime, "%H:%M %d.%m", t);
		printf("%s - %d - %s\n", acttime, (*it).second.eventid, (*it).second.event_name);
		it++;
	}
	(*osd_obj).setNowTime(now.starttime);
	(*osd_obj).setNowDescription(now.event_name);
	(*osd_obj).setNextTime(next.starttime);
	(*osd_obj).setNextDescription(next.event_name);
	(*osd_obj).setParentalRating(now.par_rating);
	bool found = false;
	for (int i = 0; i < now.number_components; i++)
	{
		if (now.component_tag[i] == audio_comp)
		{
			(*osd_obj).setLanguage(now.audio_description[i]);
			found = true;
		}
	}

	if (!found)
		(*osd_obj).setLanguage("");
	

	printf("endeit\n");
}

void eit::setAudioComponent(int comp)
{
	audio_comp = comp;
	
	for (int i = 0; i < now.number_components; i++)
	{
		if (now.component_tag[i] == audio_comp)
		{
			(*osd_obj).setLanguage(now.audio_description[i]);
		}
	}
}

bool eit::isMultiPerspective()
{
	return (now.number_perspectives > 1); //(now.running_status == 0x4 && now.number_perspectives > 1);
}

void eit::beginLinkage()
{
	curr_linkage = 0;
}

linkage eit::nextLinkage()
{
	return (now.linkage_descr[curr_linkage++]);
}

void eit::readSchedule(int SID, osd *osd)
{
	long fd, r;
	struct dmxSctFilterParams flt;
	//unsigned char sec_buffer[10][BSIZE];
	unsigned char buffer[BSIZE];

	(*osd).createPerspective();
	(*osd).setPerspectiveName("Reading Scheduling Information...");
	(*osd).showPerspective();

	fd=open("/dev/ost/demux0", O_RDONLY);
	
	memset (&flt.filter, 0, sizeof (struct dmxFilter));
	
	eventlist.clear();

	flt.pid            = 0x12;
	flt.filter.filter[0] = 0x50;
	flt.filter.mask[0] = 0xF0;
	flt.timeout        = 0;
	//flt.timeout        = 0;
	flt.flags          = DMX_IMMEDIATE_START | DMX_CHECK_CRC; //| DMX_ONESHOT;
	
	ioctl(fd, DMX_SET_FILTER, &flt);
	
	memset (&buffer, 0, BSIZE);
	r = read(fd, buffer, 3);
	int section_length = ((buffer[1] & 0xf) << 8) | buffer[2];
	r = read(fd, buffer + 3, section_length);

	int number_tables = (buffer[13] & 0xf) + 1;
	bool finished[number_tables];
	int starting_element[number_tables];
	bool quit;

	for (int i = 0; i < number_tables ; i++)
	{
		starting_element[i] = -1;
		finished[i] = false;
	}

	do
	{
		memset (&buffer, 0, BSIZE);
		//printf("\n");
		r = read(fd, buffer, 3);
		int section_length = ((buffer[1] & 0xf) << 8) | buffer[2];
		r = read(fd, buffer + 3, section_length);
	
		if (SID == ((buffer[3] << 8) | buffer[4]))
		{
		/*	for (int i = 0; i < r; i++)
			{
				if (buffer[i] < 91 && buffer[i] > 64)
					printf("%d - %c\n", i, buffer[i]);
				else if (buffer[i] < 123 && buffer[i] > 96)
					printf("%d - %c\n", i, buffer[i]);
				else
					printf("%d - %x\n", i, buffer[i]);


			}
			printf("\n");
		*/	
			//printf("SID: %04x - table_id: %02x - section_number: %02x - last_section_number: %02x - segment_last_section: %02x - last_table: %02x\n", (buffer[3] << 8) | buffer[4], buffer[0], buffer[6], buffer[7], buffer[12], buffer[13]);
			//printf("%d\n", buffer[0] & 0xf);

			if (!(finished[buffer[0] & 0xf]))
			{
				if (starting_element[buffer[0] & 0xf] == buffer[6])
				{
					finished[buffer[0] & 0xf] = true;
				}
				else
				{
					if (starting_element[buffer[0] & 0xf] == -1)
					{
						starting_element[buffer[0] & 0xf] = buffer[6];
					}
					
					int count = 13;
					do
					{
					event tmp_event;
					memset (&tmp_event, 0, sizeof (struct event));

					int time = (buffer[count + 5] << 16) | (buffer[count + 6] << 8) | buffer[count + 7];
					int mjd = (buffer[count + 3] << 8) | buffer[count + 4];
					time_t starttime = dvbtimeToLinuxTime(mjd, time) + ((*setting).getTimeOffset() * 60L);
	
					tmp_event.duration = (((buffer[count + 8] & 0xf0) >> 4) * 10 + (buffer[count + 8] & 0xf)) * 3600 + (((buffer[count + 9] & 0xf0) >> 4) * 10 + (buffer[count + 9] & 0xf)) * 60 + (((buffer[count + 10] & 0xf0) >> 4) * 10 + (buffer[count + 10] & 0xf));
					//printf("Duration: %02x %02x %02x\n", buffer[count + 8], buffer[count + 9], buffer[count + 10]);
					//printf("Duration: %d\n", tmp_event.duration);
					tmp_event.starttime = starttime;

					tmp_event.eventid = (buffer[count + 1] << 8) | buffer[count + 2];
					tmp_event.running_status = ((buffer[count + 11] & 0xe0) >> 5);
	
					int start = count + 13;
					int descriptors_length = ((buffer[count + 11] & 0xf) << 8) | buffer[count + 12];
					int text_length = 0;
					while (start < count /*+ 14 */+ descriptors_length)
					{
						if (buffer[start] == 0x4d) // short_event_descriptor
						{	
							std::string tmp_string;
							int event_name_length = buffer[start + 5];
							for (int i = 0; i < event_name_length; i++)
								tmp_event.event_name[i] = buffer[start + 6 + i];
							tmp_event.event_name[event_name_length] = '\0';
							//printf("eit: %s\n", tmp_event.event_name);
					
			
							int text_length = buffer[start + 6 + event_name_length];
							for (int i = 0; i < text_length; i++)
								tmp_event.event_short_text[i] = buffer[start + 7 + event_name_length + i];
							tmp_event.event_short_text[text_length] = '\0';
					
	
						}
						else if (buffer[start] == 0x4a && ((buffer[count + 11] & 0xe0) >> 5) == 0x04) // linkage
						{
							//printf("---> linkage_descriptor: <---\n");
							//memset (&linkage_descr[number_perspectives], 0, sizeof (struct linkage));
							linkage_descr[number_perspectives].TS = (buffer[start + 2] << 8) | buffer[start + 3];
							linkage_descr[number_perspectives].ONID = (buffer[start + 4] << 8) | buffer[start + 5];
							linkage_descr[number_perspectives].SID = (buffer[start + 6] << 8) | buffer[start + 7];
							
							char name[100];
							if (buffer[start + 8] != 0xb0)
							{
								for (int i = 9; i <= buffer[start + 1]; i++)
								{
									//printf ("%02x ", buffer[start + i]);
								}
								//printf("\n");
							}
							else // Formel 1 Perspektiven - Linkage type 0xb0
							{
								for (int i = 9; i <= buffer[start + 1] + 1; i++)
								{
									name[i - 9] = buffer[start + i]; // Namen der Perspektiven
								}
								name[buffer[start + 1] - 7] = '\0';
								strcpy(linkage_descr[number_perspectives].name, name);
								//printf("%s\n", pname.c_str());
							}
							number_perspectives++;
						}
						else if (buffer[start] == 0x4e) // extended_event_descriptor
						{
							int pos = start + 6 + buffer[start + 6];
							
							int i;
							for (i = 0; i < buffer[pos + 1]; i++)
								tmp_event.event_extended_text[i + text_length] = buffer[pos + 2 + i];
						
							text_length += buffer[pos + 1];
						}
						else if (buffer[start] == 0x50) // component_descriptor - audio-names
						{
							/*printf("---> component_descriptor: <---\n");
							printf ("reserved_future_use: %01x\n", (buffer[start + 2] & 0xf0) >> 4);
							printf ("stream_content: %01x\n", buffer[start + 2] & 0xf);
							printf ("component_type: %02x\n", buffer[start + 3]);
							printf ("component_tag: %02x\n", buffer[start + 4]);
							printf ("language_code: %c%c%c\n", buffer[start + 5], buffer[start + 6], buffer[start + 7]);
							printf ("text: ");
							*/
							tmp_event.component_tag[tmp_event.number_components] = buffer[start + 4];
							tmp_event.stream_content[tmp_event.number_components] = buffer[start + 2] & 0xf;
							tmp_event.component_type[tmp_event.number_components] = buffer[start + 3];
							//printf("Found component TAG: %x\n", buffer[start + 4]);
		
							for (int i = 7; i <= buffer[start + 1]; i++)
							{
								tmp_event.audio_description[tmp_event.number_components][i-7] = buffer[start + 1 + i];
							}
							tmp_event.audio_description[tmp_event.number_components][buffer[start + 1] - 6] = '\0';
							//printf("%s\n", tmp_event.audio_description[tmp_event.number_components]);
							tmp_event.number_components++;
			
						}

						start += buffer[start + 1] + 2;
		
						
					}
			
					//printf("Number of perspectives: %d\n", number_perspectives);
					tmp_event.event_extended_text[text_length] = '\0';
					if (eventlist.count((int)tmp_event.starttime) == 0)
						eventlist.insert(std::pair<int, struct event>((int) tmp_event.starttime, tmp_event));
					//printf("Gefunden: %x\n", ((buffer[3] << 8) | buffer[4]));
					count += count + descriptors_length - 1;
					//printf("count: %d - r: %d\n", count, r);
					
					} while (count < r - 2);
					//sleep(10);
				}
				//printf("Taken\n");
			}
		}
		quit = true;
		for (int i = 0; i < number_tables; i++)
		{
			if (!finished[i])
				quit = false;
		}
	} while(!quit);
	(*osd).hidePerspective();
}

void eit::dumpSchedule(int TS, int ONID, int SID, osd *osd)
{
	/*for (std::multimap<int, struct event>::iterator it = eventlist.begin(); it != eventlist.end(); ++it)
	{
		struct tm *t;
		t = localtime(&(*it).second.starttime);
		char acttime[10];
		strftime(acttime, sizeof acttime, "%H:%M %d.%m", t);
		//printf("%s\n", acttime);
		if (((*it).second.starttime + (*it).second.duration) >= time(0))
		{
			printf("Adding\n");
			(*osd).addScheduleInformation((*it).second.starttime, (*it).second.event_name);
		}
		printf("%d, %d, %d - %s\n", time(0), (int)(*it).second.starttime, (int)(*it).second.duration, (*it).second.event_name);
		//printf("%s - %d - %d - %s\n", ctime(&(*it).second.starttime), (*it).second.section,(*it).second.eventid, (*it).second.event_name);
	}*/
	printf("Finished EIT dump\n");

	struct sid new_sid;
	new_sid.SID = SID;
	new_sid.TS = TS;
	new_sid.ONID = ONID;
	
	std::multimap<struct sid, std::multimap<time_t, int>, ltstr>::iterator it = sid_eventid.find(new_sid);
	//for (std::multimap<struct sid, std::multimap<time_t, int>, ltstr>::iterator it = sid_eventid.begin(); it != sid_eventid.end(); ++it)
	{
		for (std::multimap<time_t, int>::iterator it2 = (*it).second.begin(); it2 != (*it).second.end(); ++it2)
		{
			event tmp_event = (*eventid_event.find((*it2).second)).second;
			//printf("%x %s %s\n", (*it2).second, tmp_event.event_name, ctime(&tmp_event.starttime));
			if ((tmp_event.starttime + tmp_event.duration) >= time(0))
			{
				printf("Adding\n");
				(*osd).addScheduleInformation(tmp_event.starttime, tmp_event.event_name, tmp_event.eventid);
			}
		}
	}
}


void eit::dumpSchedule(int SID, osd *osd)
{
	long fd, r;
	struct dmxSctFilterParams flt;
	//unsigned char sec_buffer[10][BSIZE];
	unsigned char buffer[BSIZE];

	osd->createPerspective();
	osd->setPerspectiveName("Reading Scheduling Information...");
	osd->addCommand("SHOW perspective");

	pthread_mutex_lock( &mutex );
	fd=open("/dev/ost/demux0", O_RDONLY);
	
	memset (&flt.filter, 0, sizeof (struct dmxFilter));
	
	flt.pid            = 0x12;
	flt.filter.filter[0] = 0x50;
	flt.filter.mask[0] = 0xF0;
	flt.timeout        = 10000;
	//flt.timeout        = 0;
	flt.flags          = DMX_IMMEDIATE_START | DMX_CHECK_CRC; //| DMX_ONESHOT;
	
	ioctl(fd, DMX_SET_FILTER, &flt);
	
	memset (&buffer, 0, BSIZE);
	r = read(fd, buffer, 3);
	int section_length = ((buffer[1] & 0xf) << 8) | buffer[2];
	r = read(fd, buffer + 3, section_length);

	bool quit = false;
	int number_tables = (buffer[13] & 0xf) + 1;
	bool finished[number_tables];
	int starting_element[number_tables];

	printf("Number of tables: %d\n", number_tables);

	for (int i = 0; i < number_tables ; i++)
	{
		starting_element[i] = -1;
		finished[i] = false;
	}
	
	int timeout = time(0) + 20;
	printf("Start-Dump\n");
	do
	{
		memset (&buffer, 0, BSIZE);

		r = read(fd, buffer, 3);
		int section_length = ((buffer[1] & 0xf) << 8) | buffer[2];
		r = read(fd, buffer + 3, section_length);
		
		if (r == 0)
			continue;
		
		
		eit_header tmp_header;
		memcpy (&tmp_header, &buffer, sizeof(struct eit_header));
		

		if (!(finished[buffer[0] & 0xf]))
		{
			if (starting_element[buffer[0] & 0xf] == tmp_header.section_number && tmp_header.service_id == SID)
			{
				finished[buffer[0] & 0xf] = true;
			}
			else
			{
				if (starting_element[buffer[0] & 0xf] == -1 && tmp_header.service_id == SID)
				{
					starting_element[buffer[0] & 0xf] = tmp_header.section_number;
				}
				
				// Start Event
				event tmp_event;
				memset (&tmp_event, 0, sizeof (struct event));

				tmp_event.TS = tmp_header.transport_stream_id;
				tmp_event.ONID = tmp_header.original_network_id;
				tmp_event.SID = tmp_header.service_id;
				
				int end_counter = 0;
				int sec_counter = 0;

				while(sizeof(struct eit_header) + end_counter < (unsigned int) (r - 1))
				{
					event_header tmp_event_header;
					memcpy (&tmp_event_header, &(buffer[sizeof(struct eit_header) + end_counter]), sizeof(struct event_header));
					
					time_t starttime = dvbtimeToLinuxTime(tmp_event_header.start_time_mjd, tmp_event_header.start_time_time) + ((*setting).getTimeOffset() * 60L);
					tmp_event.starttime = starttime;
					tmp_event.eventid = tmp_event_header.event_id;
					tmp_event.duration = tmp_event_header.duration;
					tmp_event.running_status = tmp_event_header.running_status;

					int ext_event_length = 0;

					int desc_counter = sizeof(struct eit_header) + sizeof(struct event_header);
					int end_desc = sizeof(struct eit_header) + sizeof(struct event_header) + tmp_event_header.descriptors_loop_length;
					while (desc_counter < end_desc)
					{
						if (buffer[desc_counter + end_counter] == 0x4d)
						{
							short_event_descriptor_header short_event_descriptor;
							memcpy(&short_event_descriptor, &buffer[desc_counter + end_counter], sizeof(struct short_event_descriptor_header));

							int text_position = desc_counter + sizeof(struct short_event_descriptor_header);
							for (int i = 0; i < short_event_descriptor.event_name_length; i++)
							{
								tmp_event.event_name[i] = buffer[text_position + end_counter + i];
							}
							tmp_event.event_name[short_event_descriptor.event_name_length] = '\0';
							

							text_position +=  short_event_descriptor.event_name_length + 1;
							int text_length = buffer[text_position + end_counter - 1];
							
							for (int i = 0; i < text_length; i++)
							{
								tmp_event.event_short_text[i] = buffer[text_position + end_counter + i];
							}
							tmp_event.event_short_text[text_length] = '\0';
							
						}
						else if (buffer[desc_counter + end_counter] == 0x4e)
						{
							extended_event_descriptor_header extended_event_descriptor;
							memcpy(&extended_event_descriptor, &buffer[desc_counter + end_counter], sizeof(struct extended_event_descriptor_header));
							
							int text_position = desc_counter + sizeof(struct extended_event_descriptor_header) + extended_event_descriptor.length_of_items + 1;
							int text_length = buffer[text_position + end_counter - 1];
														
							for (int i = 0; i < text_length; i++)
							{
								tmp_event.event_extended_text[ext_event_length++] = buffer[text_position + end_counter + i];
							}
							
						}
								
							
						desc_counter += buffer[desc_counter + 1 + end_counter] + 2;
					}
					tmp_event.event_extended_text[ext_event_length] = '\0';
					
					if (eventid_event.count(tmp_event.eventid) == 0)
					{
						struct sid new_sid;
						new_sid.SID = tmp_event.SID;
						new_sid.TS = tmp_event.TS;
						new_sid.ONID = tmp_event.ONID;

						if (sid_eventid.count(new_sid) == 0)
						{
							std::multimap<time_t, int> new_timelist;
							new_timelist.insert(std::pair<time_t, int>(tmp_event.starttime, tmp_event.eventid));
							sid_eventid.insert(std::pair<struct sid, std::multimap<time_t, int> >(new_sid, new_timelist));
						}
						else
						{
							(*sid_eventid.find(new_sid)).second.insert(std::pair<time_t, int>(tmp_event.starttime, tmp_event.eventid));
						}
						eventid_event.insert(std::pair<int, struct event>(tmp_event.eventid, tmp_event));
					}

					end_counter += tmp_event_header.descriptors_loop_length + sizeof(struct event_header);
					sec_counter++;
				}
			}
		}
		quit = true;
		for (int i = 0; i < number_tables; i++)
		{
			if (!finished[i])
			{
				quit = false;
			}
		}
	} while(!quit && (time(0) < timeout));
	close(fd);
	pthread_mutex_unlock( &mutex );

	osd->addCommand("HIDE perspective");
}

event eit::getEvent(int eventid)
{
	return (*eventid_event.find(eventid)).second;
}
