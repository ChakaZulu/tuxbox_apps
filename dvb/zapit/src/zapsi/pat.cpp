/*
 * $Id: pat.cpp,v 1.17 2002/04/10 18:36:21 obi Exp $
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <string>

#include <ost/dmx.h>
#include <ost/frontend.h>

#include "pat.h"
#include "sdt.h"
#include "scan.h"

#define DEMUX_DEV	"/dev/ost/demux0"
#define PAT_LENGTH	1024

std::map <uint, CZapitChannel> nvodchannels;
std::map <uint, CZapitChannel>::iterator cI;

extern int found_transponders;

int parse_pat (uint16_t original_network_id, std::map<uint, CZapitChannel> *cmap)
{
	struct dmxSctFilterParams flt;
	int demux_fd;
	uint8_t buffer[PAT_LENGTH];
	uint8_t section = 0;

	/* current positon in buffer */
	uint16_t pos;

	/* program_association_section elements */
	uint16_t section_length;
	uint16_t transport_stream_id;
	uint16_t program_number;
	uint16_t program_map_PID;

	memset (&flt.filter, 0, sizeof(struct dmxFilter));

	flt.pid = 0x00;
	flt.filter.mask[0]  = 0xFF;
	flt.timeout = 1000;
	flt.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;

	if ((demux_fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
		perror("[pat.cpp] " DEMUX_DEV);
		return -1;
	}

	if (ioctl(demux_fd, DMX_SET_FILTER, &flt) < 0)
	{
		perror("[pat.cpp] DMX_SET_FILTER");
		close(demux_fd);
		return -1;
	}

	do
	{
		if (read(demux_fd, buffer, PAT_LENGTH) < 0)
		{
			perror("[pat.cpp] read PAT");
			close(demux_fd);
    			return -1;
    		}

		section_length = ((buffer[1] & 0xF) << 8) + buffer[2];
		transport_stream_id = (buffer[3] << 8) + buffer[4];
#ifdef DEBUG
		printf("[pat.cpp] section_length: %04x\n", section_length);
		printf("[pat.cpp] transport_stream_id: %04x\n", transport_stream_id);
		printf("[pat.cpp] section_number: %02x\n", buffer[6]);
		printf("[pat.cpp] last_section_number: %02x\n", buffer[7]);
#endif
		for (pos = 8; pos < section_length -2; pos += 4)
		{
			program_number = (buffer[pos] << 8) + buffer[pos + 1];
			program_map_PID = ((buffer[pos + 2] & 0x1f) << 8) | buffer[pos + 3];
#ifdef DEBUG
			printf("[pat.cpp] program_number: %04x, program_map_PID: %04x\n", program_number, program_map_PID);
#endif
			if ((*cmap).count((original_network_id << 16) + program_number) > 0)
			{
				cI = (*cmap).find((original_network_id << 16) + program_number);
				cI->second.setPmtPid(program_map_PID);
			}
		}
	}
	while (section++ != buffer[7]);

	close(demux_fd);
	return 1;
}

