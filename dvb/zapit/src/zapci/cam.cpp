/*
 * $Id: cam.cpp,v 1.8 2002/04/17 09:45:54 obi Exp $
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

#include "cam.h"

CCam::CCam ()
{
	if ((ca_fd = open(CA_DEV, O_RDWR)) < 0)
	{
		perror(CA_DEV);
		initialized = false;
	}
#ifndef DVBS
	else if ((caSystemId = readCaSystemId()) == 0)
	{
		initialized = false;
	}
	else
	{
		initialized = true;
	}
#else
	else
	{
		caSystemId = 0x1702;
		initialized = true;
	}
#endif
}

CCam::~CCam ()
{
	close(ca_fd);
}

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
	ca_msg_t ca_msg;
	ca_msg.length = length;

	if (ioctl(ca_fd, CA_GET_MSG, &ca_msg) < 0)
	{
		perror("[cam.cpp] CA_GET_MSG");
		ca_msg.length = 0;
		return ca_msg;
	}

	return ca_msg;
}

int CCam::sendMessage (uint8_t *data, uint16_t length)
{
	uint8_t i;
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

	if (ioctl(ca_fd, CA_SEND_MSG, ca_msg) < 0)
	{
		perror("[cam.cpp] CA_SEND_MSG");
		delete ca_msg;
		return -1;
	}

	delete ca_msg;
	return 0;
}

int CCam::setEcm (CZapitChannel *channel)
{
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
}

int CCam::setEmm (CZapitChannel *channel)
{
	uint8_t buffer[7];

	buffer[0] = 0x84;
	buffer[1] = 0x01;
	buffer[2] = 0x04;
	buffer[3] = caSystemId >> 8;
	buffer[4] = caSystemId & 0xFF;
	buffer[5] = channel->getEmmPid() >> 8;
	buffer[6] = channel->getEmmPid() & 0xFF;

	return sendMessage(buffer, 7);
}

