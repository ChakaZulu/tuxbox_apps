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
$Log: sdt.cpp,v $
Revision 1.5  2002/06/12 23:30:03  TheDOC
basic NVOD should work again

Revision 1.4  2002/06/02 12:18:47  TheDOC
source reformatted, linkage-pids correct, xmlrpc removed, all debug-messages removed - 110k smaller lcars with -Os :)

Revision 1.3  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <stdio.h>

#include <ost/dmx.h>

#include "sdt.h"
#include "channels.h"

#define BSIZE 10000

int sdt::getChannels(channels *channels)
{
	int fd, r;
	struct dmxSctFilterParams flt;
	unsigned char buffer[BSIZE];

	// Lies den SDT
	fd=open("/dev/ost/demux0", O_RDONLY);

	memset (&flt.filter, 0, sizeof (struct dmxFilter));
	r = BSIZE;
	flt.pid            = 0x11;
	flt.filter.filter[0] = 0x42;
	flt.filter.mask[0] = 0xFF;
	flt.timeout        = 10000;
	flt.flags          = DMX_IMMEDIATE_START | DMX_ONESHOT;

	ioctl(fd, DMX_SET_FILTER, &flt);
	r=read(fd, buffer, r);
	ioctl(fd,DMX_STOP,0);

	int transport_stream_id = (buffer[3] << 8) | buffer[4];

	int start = 11;
	while (start < r - 5)
	{
		int type = -1;
		char providerName[200];
		char serviceName[200];
		int emmpids = 0;
		int caid[10];
		int nvods = 0;
		int nvod_SID[10];
		int nvod_TS[10];

		int length = ((buffer[start + 3] & 0xf) << 8) | buffer[start + 4], counter = 0;;
		while (length > counter)
		{
			if (buffer[start + 5 + counter] == 0x48) // Service Descripto
			{
				type = buffer[start + 5 + counter + 2];

				int prov_name_length = buffer[start + 5 + counter + 3];
				if (prov_name_length > 100)
					prov_name_length = 99;
				for (int i = 0; i < buffer[start + 5 + counter + 3]; i++)
					providerName[i] = buffer[start + 5 + counter + 4 + i];
				providerName[buffer[start + 5 + counter + 3]] = '\0';
				if (prov_name_length < 1)
					strcpy(providerName, "N/A");

				int serv_name_length = buffer[start + 5 + counter + 4 + prov_name_length];
				if (serv_name_length > 100)
					serv_name_length = 99;
				for (int i = 0; i < serv_name_length; i++)
					serviceName[i] = buffer[start + 10 + counter + prov_name_length + i];
				serviceName[serv_name_length] = '\0';
				if (serv_name_length < 1)
					strcpy(serviceName, "N/A");

			}
			else if (buffer[start + 5 + counter] == 0x53) // CA
			{
				emmpids = buffer[start + 5 + counter + 1] / 2;
				for (int i = 0; i < emmpids; i++)
				{
					caid[i] = (buffer[start + 5 + counter + 2 + i * 2] << 8) | buffer[start + 5 + counter + 2 + i * 2 + 1];
				}
			}
			else if (buffer[start + 5 + counter] == 0x4b) // nvod-service
			{
				nvods = buffer[start + 5 + counter + 1] / 6;
				if (nvods > 8)
					nvods = 8;
				for (int i = 0; i < nvods; i++)
				{
					nvod_TS[i] = (buffer[start + 5 + counter + 2 + i * 6] << 8) | buffer[start + 5 + counter + 2 + i * 6 + 1];
					nvod_SID[i] = (buffer[start + 5 + counter + 2 + i * 6 + 4] << 8) | buffer[start + 5 + counter + 2 + i * 6 + 5];
				}
			}

			counter += buffer[start + 5 + counter + 1] + 2;
		}
		std::string  sname(serviceName);
		std::string  pname(providerName);

		if (type != -1) // no nvod-services
		{
			(*channels).addChannel();
			(*channels).setCurrentTS(transport_stream_id);
			(*channels).setCurrentSID((buffer[start] << 8) | buffer[start + 1]);
			(*channels).setCurrentType(type);
			(*channels).setCurrentServiceName(sname);
			(*channels).setCurrentProviderName(pname);
			(*channels).setCurrentONID((buffer[8] << 8) | buffer[9]);
			(*channels).setCurrentNVODCount(nvods);
			(*channels).setCurrentVPID(0x1FFF);
			(*channels).addCurrentAPID(0x1FFF);
			for (int i = 0; i < emmpids; i++)
				(*channels).addCurrentCA(caid[i], 0);
			for (int i = 0; i < nvods; i++)
				(*channels).addCurrentNVOD(nvod_TS[i], nvod_SID[i], i);

		}

		start = start + 5 + counter;
	}
	close(fd);
	return 0;
}

void sdt::getNVODs(channels *channels)
{
	int fd, r;
	struct dmxSctFilterParams flt;
	unsigned char buffer[BSIZE];

	// Lies den SDT
	//printf("Reading SDT\n");
	//printf("looking for SID %x\n", (*channels).getCurrentSID());
	fd=open("/dev/ost/demux0", O_RDONLY);

	memset (&flt.filter, 0, sizeof (struct dmxFilter));
	r = BSIZE;
	flt.pid            = 0x11;
	flt.filter.filter[0] = 0x42;
	flt.filter.mask[0] = 0xFF;
	flt.timeout        = 10000;
	flt.flags          = DMX_IMMEDIATE_START | DMX_ONESHOT;

	ioctl(fd, DMX_SET_FILTER, &flt);
	r=read(fd, buffer, r);
	ioctl(fd,DMX_STOP,0);

	int start = 11;
	while (start < r - 5)
	{
		//printf("SID: %x\n", (buffer[start] << 8) | buffer[start + 1]);
		int type = -1;
		char providerName[200];
		char serviceName[200];
		int emmpids = 0;
		int caid[10];
		int nvods = 0;
		int nvod_SID[10];
		int nvod_TS[10];
		int nvod_ONID[10];

		int length = ((buffer[start + 3] & 0xf) << 8) | buffer[start + 4], counter = 0;;
		while (length > counter)
		{
			if (buffer[start + 5 + counter] == 0x48) // Service Descriptor
			{
				type = buffer[start + 5 + counter + 2];

				int prov_name_length = buffer[start + 5 + counter + 3];
				if (prov_name_length > 100)
					prov_name_length = 99;
				for (int i = 0; i < buffer[start + 5 + counter + 3]; i++)
					providerName[i] = buffer[start + 5 + counter + 4 + i];
				providerName[buffer[start + 5 + counter + 3]] = '\0';
				if (prov_name_length < 1)
					strcpy(providerName, "N/A");

				int serv_name_length = buffer[start + 5 + counter + 4 + prov_name_length];
				if (serv_name_length > 100)
					serv_name_length = 99;
				for (int i = 0; i < serv_name_length; i++)
					serviceName[i] = buffer[start + 10 + counter + prov_name_length + i];
				serviceName[serv_name_length] = '\0';
				if (serv_name_length < 1)
					strcpy(serviceName, "N/A");

			}
			else if (buffer[start + 5 + counter] == 0x53) // CA
			{
				emmpids = buffer[start + 5 + counter + 1] / 2;
				for (int i = 0; i < emmpids; i++)
				{
					caid[i] = (buffer[start + 5 + counter + 2 + i * 2] << 8) | buffer[start + 5 + counter + 2 + i * 2 + 1];
				}
			}
			else if (buffer[start + 5 + counter] == 0x4b) // nvod-service
			{
				nvods = buffer[start + 5 + counter + 1] / 6;
				if (nvods > 8)
					nvods = 8;
				for (int i = 0; i < nvods; i++)
				{
					nvod_TS[i] = (buffer[start + 5 + counter + 2 + i * 6] << 8) | buffer[start + 5 + counter + 2 + i * 6 + 1];
					nvod_SID[i] = (buffer[start + 5 + counter + 2 + i * 6 + 4] << 8) | buffer[start + 5 + counter + 2 + i * 6 + 5];
					nvod_ONID[i] = (buffer[8] << 8) | buffer[9];
				}
			}

			counter += buffer[start + 5 + counter + 1] + 2;
		}
		std::string  sname(serviceName);
		std::string  pname(providerName);

		if (type != -1 && (*channels).getCurrentSID() == ((buffer[start] << 8) | buffer[start + 1]))
		{
			//printf("Name: %s\n", serviceName);
			(*channels).clearCurrentNVODs();
			(*channels).setCurrentNVODCount(nvods);
			for (int i = 0; i < nvods; i++)
				(*channels).addCurrentNVOD(nvod_TS[i], nvod_ONID[i], nvod_SID[i], i);
			break;

		}


		start = start + 5 + counter;
	}
	close(fd);
}
