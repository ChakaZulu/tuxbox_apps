/*
 * $Id: camd.c,v 1.1 2002/05/05 01:37:37 obi Exp $
 *
 * (C) 2001, 2002 by gillem, Hunz, kwon, tmbinc, TripleDES, obi
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

#include <fcntl.h>
#include <ost/ca.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "cat.h"

#define CAMD_UDS_NAME	"/tmp/camd.socket"
#define MAX_PIDS	4
#define MAX_SERVICES	8

int camfd;
pthread_t camlisten;

//service-descramble
typedef struct descrambleservice_t
{
	unsigned char valid;
	unsigned char started;
	unsigned short status;
	unsigned short onID;
	unsigned short sID;
	unsigned short Unkwn;
	unsigned short caID;
	unsigned short ecmPID;
	unsigned char numpids;
	unsigned short pid[MAX_PIDS];

} descrambleservice_s;

descrambleservice_s descrambleservice[MAX_SERVICES];
//end service-descramble

//card-status
unsigned short caid[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned char caid_count = 0;

unsigned char slot1 = 0;
unsigned char slot2 = 0;

unsigned char card_country[3];
unsigned char card_number[10];
unsigned char card_version[2];
//end cam-status

unsigned short current_onid = 0;

int _writecam (unsigned char command, unsigned char * data, unsigned short length)
{
	unsigned short i;
	ca_msg_t ca_msg;

	ca_msg.index = 0;
	ca_msg.type = 0;
	ca_msg.length = length + 4;

	/* ca message */
	ca_msg.msg[0] = 0x50;
	ca_msg.msg[1] = (ca_msg.length - 3) | ((command != 0x23) ? 0x80 : 0x00);
	ca_msg.msg[2] = command;
	memcpy(ca_msg.msg + 3, data, length);

	/* message checksum */
	ca_msg.msg[ca_msg.length - 1] = 0x6E;

	for (i = 0; i < ca_msg.length - 1; i++)
	{
		ca_msg.msg[ca_msg.length - 1] ^= ca_msg.msg[i];
	}

#if 0
	printf("[camd] ca_msg (%d):", ca_msg.length);
	for (i = 0; i < ca_msg.length; i++) printf(" %02x", ca_msg.msg[i]);
	printf("\n");
#endif

	if (ioctl(camfd, CA_SEND_MSG, &ca_msg) < 0)
	{
		perror("[camd] CA_SEND_MSG");
		return -1;
	}

	return 0;
}

int writecam (unsigned char * data, unsigned short len)
{
	return _writecam(0x23, data, len);
}

int descramble (unsigned short onID, unsigned short serviceID, unsigned short unknown, unsigned short caID, unsigned short ecmpid, unsigned char numpids, unsigned short * pid)
{
	unsigned char i;
	unsigned char buffer[12 + (numpids << 2)];

	buffer[0] = 0x0F;  //0x0D
	buffer[1] = onID >> 8;
	buffer[2] = onID;
	buffer[3] = serviceID >> 8;
	buffer[4] = serviceID;
	buffer[5] = unknown >> 8;
	buffer[6] = unknown;
	buffer[7] = caID >> 8;
	buffer[8] = caID;
	buffer[9] = ecmpid >> 8;
	buffer[10] = ecmpid;
	buffer[11] = numpids;

	for (i = 0; i < numpids; i++)
	{
		buffer[12 + (i << 2)] = pid[i] >> 8;
		buffer[13 + (i << 2)] = pid[i];
		buffer[14 + (i << 2)] = 0x80;
		buffer[15 + (i << 2)] = 0x00;
	}

	return writecam(buffer, sizeof(buffer));
}

int status (void)
{
	unsigned char buffer[1];

	buffer[0] = 0x03;

	return writecam(buffer, 1);
}

int reset (void)
{
	unsigned char i;
	unsigned char buffer[1];

	for (i = 0; i < MAX_SERVICES; i++)
	{
		descrambleservice[i].valid = 0;
		descrambleservice[i].onID = 0;
		descrambleservice[i].sID = 0;
	}

	buffer[0] = 0x09;

	return writecam(buffer, 1);
}

int init (void)
{
	unsigned char buffer[1];

	buffer[0] = 0x39;

	return writecam(buffer, 1);
}

int init2 (void)
{
	unsigned char buffer[1];

	buffer[0] = 0x29;

	return writecam(buffer, 1);
}

int start (unsigned short service_id)
{
	unsigned char buffer[3];

	buffer[0] = 0x3D;
	buffer[1] = service_id >> 8;
	buffer[2] = service_id;

	return writecam(buffer, 3);
}

int setemm (unsigned short unknown, unsigned short ca_system_id, unsigned short pid)
{
	unsigned char buffer[7];

	buffer[0] = 0x84;
	buffer[1] = unknown >> 8;
	buffer[2] = unknown;
	buffer[3] = ca_system_id >> 8;
	buffer[4] = ca_system_id;
	buffer[5] = pid >> 8;
	buffer[6] = pid;

	return writecam(buffer, 7);
}

void startdescramble (void)
{
	unsigned char i;

	for (i = 0; i < MAX_SERVICES; i++)
	{
		if ((descrambleservice[i].valid == 1) && (descrambleservice[i].started == 0))
		{
			printf("[camd] starting onid %04x sid %04x\n", descrambleservice[i].onID, descrambleservice[i].sID);

			descramble
			(
				descrambleservice[i].onID,
				descrambleservice[i].sID,
				0x104,
				descrambleservice[i].caID,
				descrambleservice[i].ecmPID,
				descrambleservice[i].numpids,
				descrambleservice[i].pid
			);

			descrambleservice[i].started = 1;
		}
	}
}

int adddescrambleservice (unsigned char numpids, unsigned short onid, unsigned short sid, unsigned short unkwn, unsigned short caid, unsigned short ecmpid, unsigned short * pid)
{
	unsigned char i;
	unsigned char j;

	for (i = 0; i < MAX_SERVICES; i++)
	{
		if ((descrambleservice[i].onID == onid) && (descrambleservice[i].sID == sid))
		{
			printf("[camd] refusing duplicate service\n");
			return -1;
		}

		if (descrambleservice[i].valid == 0)
		{
			break;
		}
	}

	if (i == MAX_SERVICES)
	{
		printf("[camd] no free service, reset needed\n");
		reset();
		i = 0;
	}

	descrambleservice[i].valid = 1;
	descrambleservice[i].started = 0;
	descrambleservice[i].status = 0;
	descrambleservice[i].onID = onid;
	descrambleservice[i].sID = sid;
	descrambleservice[i].Unkwn = unkwn;
	descrambleservice[i].caID = caid;
	descrambleservice[i].ecmPID = ecmpid;
	descrambleservice[i].numpids = numpids;

	for (j = 0; j < numpids; j++)
	{
		descrambleservice[i].pid[j] = pid[j];
	}

	startdescramble();

	return 0;
}

int adddescrambleservicestruct (descrambleservice_s * service)
{
	return adddescrambleservice (service->numpids, service->onID, service->sID, service->Unkwn, service->caID, service->ecmPID, service->pid);
}

void class_23 (unsigned char * buffer, unsigned int len)
{
	int i;

	switch ((buffer[4] & 0x3C) >> 2)
	{
	case 0x00:
		caid_count = buffer[4];

		for (i = 0; i < buffer[5]; i++)
		{
			caid[i] = (buffer[6 + (i << 1)] << 8) | buffer[7 + (i << 1)];
			printf("[camd] ca system id: %04x\n", caid[i]);
		}
		break;

	case 0x02:
		for (i = 0; i < caid_count; i++)
		{
			unsigned short emmpid = parse_cat(caid[i]);

			if (emmpid != 0x0000)
			{
				setemm(0x104, caid[i], emmpid);
			}
		}
		break;

	case 0x03:
		printf("descramble onid: %02x%02x sid: %02x%02x status: %02x%02x\n", buffer[5], buffer[6], buffer[7], buffer[8], buffer[12], buffer[13]);
		for (i = 0; i < MAX_SERVICES; i++)
		{
			if ((descrambleservice[i].onID == ((buffer[5] << 8) | buffer[6])) && (descrambleservice[i].sID == ((buffer[7] << 8) | buffer[8])))
			{
				descrambleservice[i].status = (buffer[12] << 8) | buffer[13];
			}
		}
		break;

	case 0x07:
		if ((buffer[5] & 0x01) == 0)
		{
			printf("[camd] a card is in slot #2\n");
			slot1 = 1;
		}
		else
		{
			printf("[camd] no card is in slot #2\n");
			slot1 = 0;
		}

		if ((buffer[6] & 0x01) == 0)
		{
			printf("[camd] a card is in slot #1\n");
			slot2 = 1;
		}
		else
		{
			printf("[camd] no card is in slot #1\n");
			slot2 = 0;
		}
		break;

	case 0x08:
		memcpy(card_country, buffer + 5, 3);
		printf("[camd] card_country: %s\n", card_country);
		memcpy(card_number, buffer + 8, 10);
		printf("[camd] card_number: %s\n", card_number);
		memcpy(card_version, buffer + 18, 2);
		printf("[camd] card_version: %02x%02x\n", card_version[0], card_version[1]);
		break;

	default:
		printf("[camd] Unknown 2.CMD-Class: %02x (%02x)\n", buffer[4], (buffer[4] & 0x3C) >> 2);
		break;
	}
}

void class_E0 (unsigned char * buffer, unsigned int len)
{
	printf("[camd] 1.CMD-Class 0xE0\n");
}

void class_E1 (unsigned char * buffer, unsigned int len)
{
	printf("[camd] 1.CMD-Class 0xE1\n");
}

void * camlistenthread (void * thread_arg)
{
	ca_msg_t ca_msg;

	while (1)
	{
		unsigned char buffer[128];
		int len;
		int csum = 0;
		int i;

		memset(buffer, 0, sizeof(buffer));
		memset(&ca_msg, 0, sizeof(ca_msg));
		ca_msg.length = 4;

		if (ioctl(camfd, CA_GET_MSG, &ca_msg) < 0)
		{
			perror("[camd] CA_GET_MSG");
			break;
		}

		len = ca_msg.length;

		if (len <= 0)
		{
			usleep(500);
			continue;
		}
		else
		{
			memcpy(buffer, ca_msg.msg, ca_msg.length);
		}

		if ((buffer[0] != 0x6F) || (buffer[1] != 0x50)) //not good with i2c-addr inside
		{
			printf("[camd] out of sync! %02x %02x %02x %02x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
			break;
		}

		len = buffer[2] & 0x7F;
		ca_msg.length = len;

		if (ioctl(camfd, CA_GET_MSG, &ca_msg) < 0)
		{
			perror("[camd] CA_GET_MSG");
			break;
		}

		if (ca_msg.length != len)
		{
			printf("[camd] incorrect length");
			break;
		}

		memcpy(buffer + 4, ca_msg.msg, ca_msg.length);

		for (i = 0; i < len + 4; i++)
		{
			csum ^= buffer[i];
		}

		if (csum != 0)
		{
			printf("[camd] checksum failed. packet was:");
			for (i = 0; i < len + 4; i++) printf(" %02x", buffer[i]);
			printf("\n");
			continue;
		}

#if 0
		printf("[camd] read buffer:");
		for (i = 0; i < len + 4; i++) printf(" %02x", buffer[i]);
		printf("\n");
#endif

		switch (buffer[3])
		{
			case 0x23:
				class_23(buffer, len + 4);
				break;

			case 0xE0:
				class_E0(buffer, len + 4);
				break;

			case 0xE1:
				class_E1(buffer, len + 4);
				break;

			default:
				printf("[camd] Unknown 1.CMD-Class %02x\n", buffer[3]);
				break;
		}
	}

	return 0;
}

int parse_ca_pmt (unsigned char *buffer)
{
	unsigned short pos;
	unsigned short pos2;

	//unsigned char i;
	unsigned char j;

	/* ca pmt elements */
	unsigned char lengthField = buffer[3];
	//unsigned char caPmtListManagement = buffer[4];
	unsigned short programNumber = (buffer[5] << 8) | buffer[6];
	//unsigned char versionNumber = (buffer[7] >> 1) & 0x1F;
	//unsigned char currentNextIndicator = buffer[7] & 0x01;
	unsigned short programInfoLength = ((buffer[8] & 0x0F) << 8) | buffer[9];

	//unsigned char caPmtCmdId;
	//unsigned char descriptorTag;
	unsigned char descriptorLength;
	unsigned short caSystemId;
	unsigned short caPid;
	//unsigned char privateDataByte;

	unsigned char streamType;
	unsigned short elementaryPid;
	unsigned short esInfoLength;

	descrambleservice_s service;

	service.valid = 0;
	service.started = 0;
	service.status = 0;
	service.onID = current_onid;
	service.sID = programNumber;
	service.Unkwn = 0x0104;
	service.caID = 0;
	service.ecmPID = 0;
	service.numpids = 0;

	if (programInfoLength != 0)
	{
		//caPmtCmdId = buffer[10];

		for (pos = 11; pos < programInfoLength + 10; pos += descriptorLength + 2)
		{
			//descriptorTag = buffer[pos];
			descriptorLength = buffer[pos + 1];
			caSystemId = (buffer[pos + 2] << 8) | buffer[pos + 3];
			caPid = ((buffer[pos + 4] & 0x1F) << 8) | buffer[pos + 5];

			//for (i = 0; i < descriptorLength - 4; i++)
			//{
			//	privateDataByte = buffer[pos + 6 + i];
			//}

			for (j = 0; j < caid_count; j++)
			{
				if (caid[j] == caSystemId)
				{
					service.caID = caSystemId;
					service.ecmPID = caPid;
				}
			}
		}
	}

	for (pos = 10 + programInfoLength; pos < lengthField + 4; pos += esInfoLength + 5)
	{
		streamType = buffer[pos];
		elementaryPid = ((buffer[pos + 1] & 0x1F) << 8) | buffer[pos + 2];
		esInfoLength = ((buffer[pos + 3] & 0x0F) << 8) | buffer[pos + 4];

		if (service.numpids < MAX_PIDS)
		{
			service.pid[service.numpids++] = elementaryPid;
		}

		if (esInfoLength != 0)
		{
			//caPmtCmdId = buffer[pos + 5];

			for (pos2 = pos + 6; pos2 < pos + esInfoLength + 5; pos2 += descriptorLength + 2)
			{
				//descriptorTag = buffer[pos2];
				descriptorLength = buffer[pos2 + 1];
				caSystemId = (buffer[pos2 + 2] << 8) | buffer[pos2 + 3];
				caPid = ((buffer[pos2 + 4] & 0x1F) << 8) | buffer[pos2 + 5];

				//for (i = 0; i < descriptorLength - 4; i++)
				//{
				//	privateDataByte = buffer[pos2 + 6 + i];
				//}

				for (j = 0; j < caid_count; j++)
				{
					if (caid[j] == caSystemId)
					{
						service.caID = caSystemId;
						service.ecmPID = caPid;
					}
				}
			}
		}
	}

	if ((service.numpids != 0) && (service.caID != 0))
	{
		adddescrambleservicestruct(&service);
		return 0;
	}
	else
	{
		return -1;
	}
}

void handlesockmsg (unsigned char * buffer, int len, int connfd)
{
	int i;
#if 0
	printf("handlesockmsg (%02x):", len);
	for (i = 0; i < len; i++) printf(" %02x", buffer[i]);
	printf("\n");
#endif
	switch (buffer[0])
	{
	case 0x03: //0x50 0x01 0x03
		if (len >= 1)
		{
			unsigned char sendbuf[9];

			// quick'n'dirty
			sendbuf[0] = 0x6F;
			sendbuf[1] = 0x50;
			sendbuf[2] = 0x05;
			sendbuf[3] = 0x23;
			sendbuf[4] = 0x83;
			sendbuf[5] = caid_count;
			for (i = 0; i < caid_count; i++)
			{
				sendbuf[6 + (i << 2)] = caid[i] >> 8;
				sendbuf[7 + (i << 2)] = caid[i];
			}
			sendbuf[8 + (i << 2)] = 0x8e; // TODO: calc

			if (write(connfd, sendbuf, 9) < 0)
			{
				perror("[camd] write");
			}
		}
		break;

	case 0x0D: //0x50 0x11 0x0D 0x00 0x85 0x00 0x09 0x17 0x02 0x10 0x0a 0x00 0xff 0x01 0x00 0x01 0x01 0x01 0x02
		if (len >= 11)
		{
			unsigned short pid[MAX_PIDS];
			unsigned char numpids = (len - 9) >> 1;

			for (i = 0; i < numpids; i++)
			{
				if (i == MAX_PIDS)
				{
					printf("[camd] too many pids received\n");
					break;
				}

				pid[i] = (buffer[9 + (i << 1)] << 8) | buffer[10 + (i << 1)];
			}

			/* add service */
			adddescrambleservice(numpids,(buffer[1]<<8)|buffer[2],(buffer[3]<<8)|buffer[4],0x104,(buffer[5]<<8)|buffer[6],(buffer[7]<<8)|buffer[8],pid);
		}
		break;

	case 0x09: //0x50 0x03 0x09 0x00 0x85
		if (len >= 3)
		{
			current_onid = (buffer[1] << 8) | buffer[2];
		}
		reset();
		break;

	case 0x84: //0x50 0x07 0x84 0x17 0x02 0x10 0x00
		if (len >= 5)
		{
			setemm(0x104, (buffer[1] << 8) | buffer[2], (buffer[3] << 8) | buffer[4]);
		}
		break;

	case 0xCA: //0x50 0x33
		if (len >= 15)
		{
			parse_ca_pmt(buffer + 1);
		}
		break;

	default:
		printf("[camd] unknown socket command: %02x\n", buffer[0]);
		break;
	}
}

int main (int argc, char **argv)
{
	int listenfd;
	int connfd;
	struct sockaddr_un servaddr;
	int clilen;
	char msgbuffer[2 + 255];

	switch (fork())
	{
	case -1:
		perror("[camd] fork");
		return -1;

	case 0:
		break;

	default:
		return 0;
	}

	if (setsid() < 0)
	{
		perror("[camd] setsid");
		return -1;
	}

	if ((camfd = open("/dev/ost/ca0", O_RDWR)) < 0)
	{
		perror("[camd] /dev/ost/ca0");
		return -1;
	}

	pthread_create(&camlisten, 0, camlistenthread, 0);

	/* software reset */
	reset();

	/* read caid */
	status();

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, CAMD_UDS_NAME);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	unlink(CAMD_UDS_NAME);

	if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("[camd] socket");
		return -1;
	}

	if (bind(listenfd, (struct sockaddr *) &servaddr, clilen) < 0)
	{
		perror("[camd] bind");
		return -1;
	}

	if (listen(listenfd, 5) < 0)
	{
		perror("[camd] listen");
		return -1;
	}

	while (1)
	{
		connfd = accept(listenfd, (struct sockaddr *) &servaddr, (socklen_t *) &clilen);

		switch (read(connfd, msgbuffer, 2))
		{
		case -1:
			perror("[camd] read");
			break;

		case 0 ... 1:
			printf("[camd] too short message received\n");
			break;

		case 2:
			if ((msgbuffer[0] == 0x50) && (msgbuffer[1] > 0))
			{
				read(connfd, msgbuffer + 2, msgbuffer[1]);
				handlesockmsg(msgbuffer + 2, msgbuffer[1], connfd);
			}
			else
			{
				printf("[camd] unknown message received\n");
			}
			break;

		default:
			break;
		}

		close(connfd);
	}
}

