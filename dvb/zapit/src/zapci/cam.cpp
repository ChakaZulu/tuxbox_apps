/*
 * $Id: cam.cpp,v 1.10 2002/04/28 05:38:51 obi Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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
#include <ost/dmx.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "cam.h"

#define CA_DEV  "/dev/ost/ca0"

#ifdef USE_EXTERNAL_CAMD
#define CAMD_UDS_NAME "/tmp/camd.socket"
#endif

CCam::CCam ()
{
#ifndef USE_EXTERNAL_CAMD
	if ((caSystemId = readCaSystemId()) == 0)
	{
		initialized = false;
	}
#else
	if ((caSystemId = 0x1702) == 0)
	{
		initialized = false;
	}
#endif
	else
	{
		initialized = true;
	}
}

CCam::~CCam ()
{
}

#ifdef USE_EXTERNAL_CAMD
bool CCam::camdConnect ()
{
	struct sockaddr_un servaddr;
	int clilen;

	std::string filename = CAMD_UDS_NAME;

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, filename.c_str());
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);

	if ((camdSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("[CCam::camdConnect] socket");
		return false;
	}

	if (connect(camdSocket, (struct sockaddr*) &servaddr, clilen) < 0)
	{
		perror("[CCam::camdConnect] connect");
		return false;
	}

	return true;
}

void CCam::camdDisconnect ()
{
	if (camdSocket != -1)
	{
		close(camdSocket);
		camdSocket = -1;
	}
}
#endif

int CCam::reset ()
{
	uint8_t buffer[1] = { 0x09 };
	return sendMessage(buffer, 1);
}

uint16_t CCam::readCaSystemId ()
{
	ca_msg_t ca_msg;

	uint8_t buffer[1] = { 0x03 };
	sendMessage(buffer, 1);

	do
	{
		ca_msg = getMessage(9);
	}
	while (ca_msg.length == 0);

	return (ca_msg.msg[6] << 8) | ca_msg.msg[7];
}

ca_msg_t CCam::getMessage (uint16_t length)
{
	int ca_fd = -1;

	ca_msg_t ca_msg;
	ca_msg.length = length;

	if ((ca_fd = open(CA_DEV, O_RDWR)) < 0)
	{
		perror(CA_DEV);
		ca_msg.length = 0;
	}
	else if (ioctl(ca_fd, CA_GET_MSG, &ca_msg) < 0)
	{
		perror("[cam.cpp] CA_GET_MSG");
		ca_msg.length = 0;
	}

	if (ca_fd != -1)
	{
		close(ca_fd);
	}

	return ca_msg;
}

int CCam::sendMessage (uint8_t *data, uint16_t length)
{
#ifdef USE_EXTERNAL_CAMD
	camdBuffer[0] = 0x50;
	camdBuffer[1] = length;
	memcpy(camdBuffer + 2, data, length);

	if (camdConnect() == false)
	{
		return -1;
	}
	else if (write(camdSocket, camdBuffer, length + 2) < 0)
	{
		perror("[CCam::sendMessage] write");
		camdDisconnect();
		return -1;
	}
	else
	{
		camdDisconnect();
		return 0;
	}
#else
	uint8_t i;
	int ca_fd;
	ca_msg_t *ca_msg = new ca_msg_t();

	ca_msg->index = 0;
	ca_msg->type = 0;
	ca_msg->length = length + 4;

	ca_msg->msg[0] = 0x50;
	ca_msg->msg[1] = ca_msg->length - 3;
	ca_msg->msg[2] = 0x23;

	memcpy(ca_msg->msg + 3, data, length);

	ca_msg->msg[ca_msg->length - 1] = 0x6E;

	for (i = 0; i < ca_msg->length - 1; i++)
	{
		ca_msg->msg[ca_msg->length - 1] ^= ca_msg->msg[i];
	}

	if ((ca_fd = open(CA_DEV, O_RDWR)) < 0)
	{
		perror(CA_DEV);
		delete ca_msg;
		return -1;
	}
	else if (ioctl(ca_fd, CA_SEND_MSG, ca_msg) < 0)
	{
		perror("[cam.cpp] CA_SEND_MSG");
		close(ca_fd);
		delete ca_msg;
		return -1;
	}

	close(ca_fd);
	delete ca_msg;
	return 0;
#endif
}

int CCam::setEcm (CZapitChannel *channel)
{
#ifdef USE_EXTERNAL_CAMD
	uint8_t buffer[15];

	buffer[0] = 0x0D;
	buffer[1] = channel->getOriginalNetworkId() >> 8;
	buffer[2] = channel->getOriginalNetworkId();
	buffer[3] = channel->getServiceId() >> 8;
	buffer[4] = channel->getServiceId();
	buffer[5] = caSystemId >> 8;
	buffer[6] = caSystemId;
	buffer[7] = channel->getEcmPid() >> 8;
	buffer[8] = channel->getEcmPid();
	buffer[9] = channel->getVideoPid() >> 8;
	buffer[10] = channel->getVideoPid();
	buffer[11] = channel->getAudioPid() >> 8;
	buffer[12] = channel->getAudioPid();
	buffer[13] = channel->getTeletextPid() >> 8;
	buffer[14] = channel->getTeletextPid();

	return sendMessage(buffer, 15);
#else
	uint8_t i;
	uint8_t buffer[12 + (4 * (channel->getPids()->count_vpids + channel->getPids()->count_apids))];
	uint8_t pos = 12;

	buffer[0] = 0x0D;
	buffer[1] = channel->getTsidOnid() >> 8;
	buffer[2] = channel->getTsidOnid() & 0xFF;
	buffer[3] = channel->getTsidOnid() >> 24;
	buffer[4] = (channel->getTsidOnid() >> 16) & 0xFF;
	buffer[5] = 0x01;
	buffer[6] = 0x04;
	buffer[7] = caSystemId >> 8;
	buffer[8] = caSystemId & 0xFF;
	buffer[9] = channel->getEcmPid() >> 8;
	buffer[10] = channel->getEcmPid() & 0xFF;
	buffer[11] = channel->getPids()->count_vpids + channel->getPids()->count_apids;

	for (i = 0; i < channel->getPids()->count_vpids; i++)
  	{
		buffer[pos++] = channel->getVideoPid() >> 8;
		buffer[pos++] = channel->getVideoPid() & 0xFF;
		buffer[pos++] = 0x80;
		buffer[pos++] = 0x00;
	}

	for (i = 0; i < channel->getPids()->count_apids; i++)
	{
		buffer[pos++] = channel->getPids()->apids[i].pid >> 8;
		buffer[pos++] = channel->getPids()->apids[i].pid & 0xFF;
		buffer[pos++] = 0x80;
		buffer[pos++] = 0x00;
	}

	return sendMessage(buffer, pos);
#endif
}

int CCam::setEmm (CZapitChannel *channel)
{
#ifdef USE_EXTERNAL_CAMD
	uint8_t buffer[5];

	buffer[0] = 0x84;
	buffer[1] = caSystemId >> 8;
	buffer[2] = caSystemId;
	buffer[3] = channel->getEmmPid() >> 8;
	buffer[4] = channel->getEmmPid();

	return sendMessage(buffer, 5);
#else
	uint8_t buffer[7];

	buffer[0] = 0x84;
	buffer[1] = 0x01;
	buffer[2] = 0x04;
	buffer[3] = caSystemId >> 8;
	buffer[4] = caSystemId & 0xFF;
	buffer[5] = channel->getEmmPid() >> 8;
	buffer[6] = channel->getEmmPid() & 0xFF;

	return sendMessage(buffer, 7);
#endif
}

