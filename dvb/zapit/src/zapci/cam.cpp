/*
 * $Id: cam.cpp,v 1.14 2002/05/13 17:17:04 obi Exp $
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
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string>
#include <unistd.h>

#include "cam.h"

#define CAMD_UDS_NAME "/tmp/camd.socket"

CCam::CCam ()
{
}

CCam::~CCam ()
{
}

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

ca_msg_t CCam::getMessage (unsigned short length)
{
	ca_msg_t ca_msg;

	ca_msg.index = 0;
	ca_msg.type = 0;

	if (camdSocket == -1)
	{
		ca_msg.length = 0;
	}
	else if ((ca_msg.length = read(camdSocket, ca_msg.msg, length)) < 0)
	{
		perror("[CCam::getMessage] read");
		ca_msg.length = 0;
	}

	return ca_msg;
}

int CCam::sendMessage (unsigned char * data, unsigned short length)
{
	camdBuffer[0] = 0x50;
	camdBuffer[1] = length;
	memcpy(camdBuffer + 2, data, length);

	camdDisconnect();

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
		return 0;
	}
}

int CCam::reset (unsigned short originalNetworkId)
{
	unsigned char buffer[3];

	buffer[0] = 0x09;
	buffer[1] = originalNetworkId >> 8;
	buffer[2] = originalNetworkId;

	return sendMessage(buffer, 3);
}

int CCam::setCaPmt (CCaPmt * caPmt)
{
	unsigned char buffer[caPmt->length_field + 5];
	unsigned short pos;
	unsigned short pos2;
	unsigned short i;
	unsigned short j;
	unsigned short k;

	buffer[0] = 0xCA;
	buffer[1] = caPmt->ca_pmt_tag >> 16;
	buffer[2] = caPmt->ca_pmt_tag >> 8;
	buffer[3] = caPmt->ca_pmt_tag;
	buffer[4] = caPmt->length_field;
	buffer[5] = caPmt->ca_pmt_list_management;
	buffer[6] = caPmt->program_number >> 8;
	buffer[7] = caPmt->program_number;
	buffer[8] = (caPmt->reserved1 << 6) | (caPmt->version_number << 1) | caPmt->current_next_indicator;
	buffer[9] = (caPmt->reserved2 << 4) | (caPmt->program_info_length >> 8);
	buffer[10] = caPmt->program_info_length;

	if (caPmt->program_info_length != 0)
	{
		buffer[11] = caPmt->ca_pmt_cmd_id;

		for (pos = 12, i = 0; pos < caPmt->program_info_length + 11; pos += caPmt->ca_descriptor[i]->descriptor_length + 2, i++)
		{
			buffer[pos] = caPmt->ca_descriptor[i]->descriptor_tag;
			buffer[pos + 1] = caPmt->ca_descriptor[i]->descriptor_length;
			buffer[pos + 2] = caPmt->ca_descriptor[i]->CA_system_ID >> 8;
			buffer[pos + 3] = caPmt->ca_descriptor[i]->CA_system_ID;
			buffer[pos + 4] = (caPmt->ca_descriptor[i]->reserved1 << 5) | (caPmt->ca_descriptor[i]->CA_PID >> 8);
			buffer[pos + 5] = caPmt->ca_descriptor[i]->CA_PID;

			for (j = 0; j < caPmt->ca_descriptor[i]->descriptor_length - 4; j++)
			{
				buffer[pos + 6 + j] = caPmt->ca_descriptor[i]->private_data_byte[j];
			}
		}
	}

	for (pos = caPmt->program_info_length + 11, i = 0; pos < caPmt->length_field + 5; pos += caPmt->es_info[i]->ES_info_length + 5, i++)
	{
		buffer[pos] = caPmt->es_info[i]->stream_type;
		buffer[pos + 1] = (caPmt->es_info[i]->reserved1 << 5) | (caPmt->es_info[i]->elementary_PID >> 8);
		buffer[pos + 2] = caPmt->es_info[i]->elementary_PID;
		buffer[pos + 3] = (caPmt->es_info[i]->reserved2 << 4) | (caPmt->es_info[i]->ES_info_length >> 8);
		buffer[pos + 4] = caPmt->es_info[i]->ES_info_length;

		if (caPmt->es_info[i]->ES_info_length != 0)
		{
			buffer[pos + 5] = caPmt->es_info[i]->ca_pmt_cmd_id;

			for (pos2 = pos + 6, j = 0; pos2 < pos + caPmt->es_info[i]->ES_info_length + 5; pos2 += caPmt->es_info[i]->ca_descriptor[j]->descriptor_length + 2, j++)
			{
				buffer[pos2] = caPmt->es_info[i]->ca_descriptor[j]->descriptor_tag;
				buffer[pos2 + 1] = caPmt->es_info[i]->ca_descriptor[j]->descriptor_length;
				buffer[pos2 + 2] = caPmt->es_info[i]->ca_descriptor[j]->CA_system_ID >> 8;
				buffer[pos2 + 3] = caPmt->es_info[i]->ca_descriptor[j]->CA_system_ID;
				buffer[pos2 + 4] = (caPmt->es_info[i]->ca_descriptor[j]->reserved1 << 5) | (caPmt->es_info[i]->ca_descriptor[j]->CA_PID >> 8);
				buffer[pos2 + 5] = caPmt->es_info[i]->ca_descriptor[j]->CA_PID;

				for (k = 0; k < caPmt->es_info[i]->ca_descriptor[j]->descriptor_length - 4; k++)
				{
					buffer[pos2 + 6 + k] = caPmt->es_info[i]->ca_descriptor[j]->private_data_byte[k];
				}
			}
		}
	}

	return sendMessage(buffer, pos);
}

