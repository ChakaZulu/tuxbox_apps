/*
 * $Id: pmt.cpp,v 1.9 2002/04/20 12:35:37 obi Exp $
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
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

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

uint16_t parse_ES_info(uint8_t *buffer, pids *ret_pids, uint16_t ca_system_id)
{
	uint8_t stream_type;
	dvb_pid_t elementary_PID;
	uint16_t ES_info_length;
	uint16_t descr_pos;
	uint8_t descriptor_tag;
	uint8_t descriptor_length;
	uint8_t ap_count = ret_pids->count_apids;
	uint8_t vp_count = ret_pids->count_vpids;
	dvb_pid_t ecm_pid = ret_pids->ecmpid;
	int destination_apid_list_entry = -1;
	bool apid_previously_found = false;
	int apid_list_entry;

	stream_type = buffer[0];
	elementary_PID = ((buffer[1] & 0x1f) << 8) | buffer[2];
	ES_info_length = ((buffer[3] & 0x0f) << 8) | buffer[4];

	if ((stream_type == 0x03 || stream_type == 0x04 || stream_type == 0x06) && ap_count < max_num_apids)
	{
		ret_pids->apids[ap_count].component_tag = 0xFF;	// indikator fuer "kein component_tag"
		ret_pids->apids[ap_count].is_ac3 = false;
		ret_pids->apids[ap_count].desc[0] = 0;
	}

	for (descr_pos = 5; descr_pos < ES_info_length + 5; descr_pos += descriptor_length + 2)
	{
		descriptor_tag = buffer[descr_pos];
		descriptor_length = buffer[descr_pos + 1];
		destination_apid_list_entry = -1;

		switch (descriptor_tag)
		{
			case 0x02:
				video_stream_descriptor(buffer + descr_pos);
				break;

			case 0x03:
				audio_stream_descriptor(buffer + descr_pos);
				break;

			case 0x09:
				if ((ecm_pid == NONE) || (ecm_pid == INVALID))
				{
					CA_descriptor(buffer + descr_pos, ca_system_id, &ecm_pid);
				}
				break;

			case 0x0A: /* ISO_639_language_descriptor */
				if (ap_count < max_num_apids && (stream_type == 0x03 || stream_type == 0x04 || stream_type == 0x06))
				{
					if (ret_pids->apids[ap_count].desc[0] == 0)
					{
						memcpy(ret_pids->apids[ap_count].desc, &(buffer[descr_pos + 2]), descriptor_length);
						ret_pids->apids[ap_count].desc[3] = 0;
					}
				}
				break;

			case 0x0E:
				Maximum_bitrate_descriptor(buffer + descr_pos);
				break;

			case 0x0F:
				Private_data_indicator_descriptor(buffer + descr_pos);
				break;

			case 0x11:
				STD_descriptor(buffer + descr_pos);
				break;

			case 0x45:
				VBI_data_descriptor(buffer + descr_pos);
				break;

			case 0x52: /* stream_identifier_descriptor */
				if (ap_count < max_num_apids && (stream_type == 0x03 || stream_type == 0x04 || stream_type == 0x06))
				{
					ret_pids->apids[ap_count].component_tag = buffer[descr_pos + 2];
				}
				break;

			case 0x56: /* teletext descriptor */
				ret_pids->vtxtpid = elementary_PID;
				break;

			case 0x59:
				subtitling_descriptor(buffer + descr_pos);
				break;

			case 0x66:
				data_broadcast_id_descriptor(buffer + descr_pos);
				break;

			case 0x6a: /* AC3 descriptor */
				if (ap_count < max_num_apids)
				{
					ret_pids->apids[ap_count].is_ac3 = true;
				}
				break;

			case 0xc5: /* User Private descriptor - Canal+ Radio                                     */
				   /* Double apid entries are ignored or overwritten (depending on the name tag) */
				if (stream_type == 0x03 || stream_type == 0x04 || stream_type == 0x06)
				{
					apid_list_entry = 0;

					while (destination_apid_list_entry == -1 && apid_list_entry < ap_count)
					{
						if (elementary_PID == ret_pids->apids[apid_list_entry].pid)
						{
							apid_previously_found = true;
							if (strcmp(ret_pids->apids[apid_list_entry].desc, "LIBRE") == 0 ||
							    ret_pids->apids[apid_list_entry].desc[0] == 0)
							{
								destination_apid_list_entry = apid_list_entry;
								ret_pids->apids[apid_list_entry].desc[0] = 0;
							}
						}
						apid_list_entry++;
					}

					if (destination_apid_list_entry == -1 && ap_count < max_num_apids)
					{
						destination_apid_list_entry = ap_count;
					}

					if (destination_apid_list_entry != -1)
					{
						if (ret_pids->apids[destination_apid_list_entry].desc[0] == 0)
						{
							memcpy(ret_pids->apids[destination_apid_list_entry].desc, &(buffer[descr_pos + 3]), 0x18);
							ret_pids->apids[destination_apid_list_entry].desc[24] = 0;
						}
					}
				}
				break;

			default:
				printf("[pmt.cpp] descriptor_tag (b): %02x\n", descriptor_tag);
				break;
		}
	}

	switch (stream_type)
	{
		case 0x01:
		case 0x02:
			ret_pids->vpid = elementary_PID;
			vp_count++;
			break;

		case 0x03:
		case 0x04:
		case 0x06:
			if (stream_type == 0x03 || stream_type == 0x04 || ret_pids->apids[ap_count].is_ac3)
			{
				if (destination_apid_list_entry == -1 && ap_count < max_num_apids)
				{
					destination_apid_list_entry = ap_count;
				}

				if (destination_apid_list_entry != -1)
				{
					ret_pids->apids[destination_apid_list_entry].pid = elementary_PID;
					if ((!apid_previously_found) || (destination_apid_list_entry == ap_count &&
					    ret_pids->apids[destination_apid_list_entry].desc[0] != 0 &&
					    strncmp(ret_pids->apids[destination_apid_list_entry].desc, "LIBRE", 5) != 0))
					{
						ap_count++;
					}
				}
			}
			break;

                default:
			printf("[pmt.cpp] stream_type: %02x\n", stream_type);
                        break;
	}

	ret_pids->count_apids = ap_count;
	ret_pids->count_vpids = vp_count;
	ret_pids->ecmpid = ecm_pid;

	return ES_info_length + 5;
}

pids parse_pmt (dvb_pid_t pmt_pid, uint16_t ca_system_id, uint16_t program_number)
{
	uint8_t buffer[PMT_SIZE];
	int fd;
	struct dmxSctFilterParams flt;
	pids ret_pids;
	uint8_t apid_list_entry;

	/* current position in buffer */
	uint16_t pos;

	/* length of elementary stream description */
	uint16_t ES_info_length;

	/* TS_program_map_section elements */
	uint16_t section_length;
	uint16_t program_info_length;

	if (pmt_pid == 0)
	{
	        ret_pids.count_apids = 0;
	        ret_pids.count_vpids = 0;
		return ret_pids;
	}

	memset(&ret_pids, 0, sizeof(ret_pids));

	ret_pids.pmtpid = pmt_pid;

	if ((fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
		perror("[pmt.cpp] open");
		return ret_pids;
	}

	memset(&flt.filter.filter, 0, DMX_FILTER_SIZE);
	memset(&flt.filter.mask, 0, DMX_FILTER_SIZE);

	flt.pid = pmt_pid;
	flt.filter.filter[0] = 0x02;
	flt.filter.filter[1] = (program_number >> 8) & 0xFF;
	flt.filter.filter[2] = program_number & 0xFF;
	flt.filter.mask[0]  = 0xFF;
	flt.filter.mask[1]  = 0xFF;
	flt.filter.mask[2]  = 0xFF;
	flt.timeout = 1000;
	flt.flags= DMX_CHECK_CRC | DMX_ONESHOT | DMX_IMMEDIATE_START;

	if (ioctl(fd, DMX_SET_FILTER, &flt) < 0)
	{
		perror("[pmt.cpp] DMX_SET_FILTER");
		close(fd);
		return ret_pids;
	}

	if (read(fd, buffer, sizeof(buffer)) < 0)
	{
		perror("[pmt.cpp] read");
		close(fd);
		return ret_pids;
	}

	close(fd);

	section_length = ((buffer[1] & 0xF) << 8) + buffer[2];
	ret_pids.pcrpid = ((buffer[8] & 0x1f) << 8) + buffer[9];
	program_info_length = ((buffer[10] & 0xF) << 8) | buffer[11];
	ret_pids.ecmpid = NONE;

	if (program_info_length > 0)
	{
		for (pos = 12; pos < 12 + program_info_length; pos += buffer[pos + 1] + 2)
		{
			switch (buffer[pos])
			{
			case 0x09:
				if ((ret_pids.ecmpid == NONE) || (ret_pids.ecmpid == INVALID))
				{
					CA_descriptor(buffer + pos, ca_system_id, &ret_pids.ecmpid);
				}
				break;

			default:
				printf("[pmt.cpp] decriptor_tag (a): %02x\n", buffer[pos]);
				break;
			}
		}
	}

	for (pos = 12 + program_info_length; pos < section_length - 1; pos += ES_info_length)
	{
		ES_info_length = parse_ES_info(buffer+pos, &ret_pids, ca_system_id);
	}

	for (apid_list_entry = 0; apid_list_entry < ret_pids.count_apids; apid_list_entry++)
	{
		if (ret_pids.apids[apid_list_entry].desc[0] == 0)
		{
			sprintf(ret_pids.apids[apid_list_entry].desc, "%02d", apid_list_entry + 1);
		}
	}

	return ret_pids;
}

