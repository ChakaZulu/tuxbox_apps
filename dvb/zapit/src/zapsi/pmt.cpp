/*
 * $Id: pmt.cpp,v 1.17 2002/05/13 14:56:51 obi Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
 * (C) 2002 by Frank Bormann <happydude@berlios.de>
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
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <zapost/dmx.h>

#include "descriptors.h"
#include "pmt.h"

#define DEMUX_DEV "/dev/ost/demux0"
#define PMT_SIZE  1024

/*
 * Stream types
 * ------------
 * 0x01 ISO/IEC 11172 Video
 * 0x02 ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
 * 0x03 ISO/IEC 11172 Audio
 * 0x04 ISO/IEC 13818-3 Audio
 * 0x05 ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private_sections, e.g. MHP Application signalling stream
 * 0x06 ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data, e.g. teletext or ac3
 * 0x0b ISO/IEC 13818-6 type B
 * 0x81 User Private (MTV)
 * 0x90 User Private (Premiere Mail, BD_DVB)
 * 0xc0 User Private (Canal+)
 * 0xc1 User Private (Canal+)
 * 0xc6 User Private (Canal+)
 */

unsigned short parse_ES_info (unsigned char * buffer, CZapitChannel * channel, CCaPmt * caPmt)
{
	uint16_t ES_info_length;
	uint16_t pos;
	uint8_t descriptor_tag;
	uint8_t descriptor_length;
	uint8_t i;

	bool isAc3 = false;
	std::string description = "";
	unsigned char componentTag = 0xFF;

	/* elementary stream info for ca pmt */
	CEsInfo * esInfo = new CEsInfo();

	esInfo->stream_type = buffer[0];
	esInfo->reserved1 = buffer[1] >> 5;
	esInfo->elementary_PID = ((buffer[1] & 0x1F) << 8) | buffer[2];
	esInfo->reserved2 = buffer[3] >> 4;
	esInfo->ES_info_length = 0;

	ES_info_length = ((buffer[3] & 0x0F) << 8) | buffer[4];

	for (pos = 5; pos < ES_info_length + 5; pos += descriptor_length + 2)
	{
		descriptor_tag = buffer[pos];
		descriptor_length = buffer[pos + 1];

		switch (descriptor_tag)
		{
			case 0x02:
				video_stream_descriptor(buffer + pos);
				break;

			case 0x03:
				audio_stream_descriptor(buffer + pos);
				break;

			case 0x09:
				esInfo->ES_info_length += descriptor_length + 2;
				esInfo->addCaDescriptor(buffer + pos);
				break;

			case 0x0A: /* ISO_639_language_descriptor */
				for (i = 0; i < 3; i++)
				{
					description += buffer[pos + i + 2];
				}
				break;

			case 0x13: /* Defined in ISO/IEC 13818-6 */
				break;

			case 0x0E:
				Maximum_bitrate_descriptor(buffer + pos);
				break;

			case 0x0F:
				Private_data_indicator_descriptor(buffer + pos);
				break;

			case 0x11:
				STD_descriptor(buffer + pos);
				break;

			case 0x45:
				VBI_data_descriptor(buffer + pos);
				break;

			case 0x52: /* stream_identifier_descriptor */
				componentTag = buffer[pos + 2];
				break;

			case 0x56: /* teletext descriptor */
				channel->setTeletextPid(esInfo->elementary_PID);
				break;

			case 0x59:
				subtitling_descriptor(buffer + pos);
				break;

			case 0x5F:
				private_data_specifier_descriptor(buffer + pos);
				break;

			case 0x66:
				data_broadcast_id_descriptor(buffer + pos);
				break;

			case 0x6A: /* AC3 descriptor */
				isAc3 = true;
				break;

			case 0x6F: /* unknown, Astra 19.2E */
				break;

			case 0x90: /* unknown, Astra 19.2E */
				break;

			case 0xB1: /* unknown, Astra 19.2E */
				break;

			case 0xC0: /* unknown, Astra 19.2E */
				break;

			case 0xC1: /* unknown, Astra 19.2E */
				break;

			case 0xC2: /* User Private descriptor - Canal+ */
				printf("[pmt.cpp] 0xC2 dump:\n");
				for (i = 0; i < descriptor_length; i++)
				{
					printf("%c", buffer[pos + 2 + i]);

					if (((i+1) % 8) == 0)
					{
						printf("\n");
					}
				}
				break;

			case 0xC5: /* User Private descriptor - Canal+ Radio */
				for (i = 0; i < 24; i++)
				{
					description += buffer[pos + i + 3];
				}
				break;

			case 0xC6: /* unknown, Astra 19.2E */
				break;

			case 0xFD: /* unknown, Astra 19.2E */
				break;

			case 0xFE: /* unknown, Astra 19.2E */
				break;

			default:
				printf("[pmt.cpp] descriptor_tag (b): %02x\n", descriptor_tag);
				break;
		}
	}

	switch (esInfo->stream_type)
	{
	case 0x01:
	case 0x02:
		channel->setVideoPid(esInfo->elementary_PID);
		break;

	case 0x03:
	case 0x04:
		if (description == "")
		{
			description = esInfo->elementary_PID;
		}
		channel->addAudioChannel(esInfo->elementary_PID, false, description, componentTag);
		break;

	case 0x05:
		break;

	case 0x06:
		if (isAc3)
		{
			if (description == "")
			{
				description = esInfo->elementary_PID;
				description += " (AC3)";
			}
			channel->addAudioChannel(esInfo->elementary_PID, true, description, componentTag);
		}
		break;

	case 0x0B:
		break;

	case 0x90:
		break;

	case 0x93:
		break;

	default:
		printf("[pmt.cpp] stream_type: %02x\n", esInfo->stream_type);
		break;
	}

	if (esInfo->ES_info_length != 0)
	{
		esInfo->ca_pmt_cmd_id = 0x01; // ok_descrambling
		esInfo->ES_info_length += 1;
	}

	caPmt->length_field += esInfo->ES_info_length + 5;
	caPmt->es_info.insert(caPmt->es_info.end(), esInfo);

	return ES_info_length;
}

int parse_pmt (int demux_fd, CZapitChannel * channel)
{
	uint8_t buffer[PMT_SIZE];

	/* current position in buffer */
	uint16_t i;

	/* length of elementary stream description */
	uint16_t ES_info_length;

	/* TS_program_map_section elements */
	uint16_t section_length;
	uint16_t program_info_length;

	if (channel->getPmtPid() == 0)
	{
		return -1;
	}

	if (setDmxSctFilter(demux_fd, channel->getPmtPid(), 0x02, channel->getServiceId() >> 8, channel->getServiceId()) < 0)
	{
		return -1;
	}

	if (read(demux_fd, buffer, PMT_SIZE) < 0)
	{
		perror("[pmt.cpp] read");
		return -1;
	}

	CCaPmt * caPmt = new CCaPmt();

	/* ca pmt */
	caPmt->ca_pmt_tag = 0x9F8032;
	caPmt->length_field = 0x06;
	caPmt->ca_pmt_list_management = 0x04; // add
	caPmt->program_number = (buffer[3] << 8) + buffer[4];
	caPmt->reserved1 = buffer[5] >> 6;
	caPmt->version_number = (buffer[5] >> 1) & 0x1F;
	caPmt->current_next_indicator = buffer[5] & 0x01;
	caPmt->reserved2 = buffer[10] >> 4;
	caPmt->program_info_length = 0;

	/* pmt */
	section_length = ((buffer[1] & 0x0F) << 8) + buffer[2];
	channel->setPcrPid(((buffer[8] & 0x1F) << 8) + buffer[9]);
	program_info_length = ((buffer[10] & 0x0F) << 8) | buffer[11];

	if (program_info_length != 0)
	{
		for (i = 12; i < 12 + program_info_length; i += buffer[i + 1] + 2)
		{
			switch (buffer[i])
			{
			case 0x09:
				caPmt->program_info_length += buffer[i + 1] + 2;
				caPmt->addCaDescriptor(buffer + i);
				break;

			default:
				printf("[pmt.cpp] decriptor_tag (a): %02x\n", buffer[i]);
				break;
			}
		}
	}

	/* ca pmt */
	if (caPmt->program_info_length != 0)
	{
		caPmt->ca_pmt_cmd_id = 0x01; // ok_descrambling
		caPmt->program_info_length += 1;
		caPmt->length_field += caPmt->program_info_length;
	}

	/* pmt */
	for (i = 12 + program_info_length; i < section_length - 1; i += ES_info_length + 5)
	{
		ES_info_length = parse_ES_info(buffer + i, channel, caPmt);
	}

	channel->setCaPmt(caPmt);
	channel->setPidsFlag();

	return 0;
}

