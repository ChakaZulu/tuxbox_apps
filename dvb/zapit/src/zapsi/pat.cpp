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

using namespace std;

map<uint,channel> nvodchannels;
map<uint,channel>::iterator cI;
extern int found_transponders;

/* cable only */
int fake_pat(std::map<int,transpondermap> *tmap, FrontendParameters feparams)
{
	struct dmxSctFilterParams flt;
	int demux_fd;
	uint8_t buffer[PAT_LENGTH];
  	uint16_t transport_stream_id;

  	if ((demux_fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
    		perror("[pat.cpp] " DEMUX_DEV);
    		return -1;
  	}

  	memset (&flt.filter, 0, sizeof (struct dmxFilter));
  
  	flt.pid = 0x00;
    	flt.filter.mask[0]  = 0xFF;
  	flt.timeout = 1000;
  	flt.flags = DMX_CHECK_CRC | DMX_ONESHOT | DMX_IMMEDIATE_START;
 
  	if (ioctl(demux_fd, DMX_SET_FILTER, &flt) < 0)
	{
    		perror("[pat.cpp] DMX_SET_FILTER");
		close(demux_fd);
		return -1;
  	}

	if (read(demux_fd, buffer, PAT_LENGTH) < 0)
	{
   		perror("[pat.cpp] read");
    		close(demux_fd);
    		return -1;
    	}
 
	close(demux_fd);

	transport_stream_id = (buffer[3] << 8) | buffer[4];

	(*tmap).insert(std::pair<int, transpondermap>(transport_stream_id, transpondermap(transport_stream_id, feparams)));
	found_transponders++;

	return 23;
}

int pat(uint oonid, std::map<uint, channel> *cmap)
{
	struct dmxSctFilterParams flt;
	int demux_fd;
	uint8_t buffer[PAT_LENGTH];

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
			if ((*cmap).count((oonid << 16) + program_number) > 0)
			{
				cI = (*cmap).find((oonid << 16) + program_number);
				cI->second.pmt = program_map_PID;
			}
		}
	}
	while (buffer[6] != buffer[7]);

	if (ioctl(demux_fd, DMX_STOP, 0) < 0)
	{
		perror("[pat.cpp] DMX_STOP");
	}

	close(demux_fd);
	return 1;
}

