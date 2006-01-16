/*
$Id: genpsi.c,v 1.2 2006/01/16 12:45:54 sat_man Exp $

 Copyright (c) 2004 gmo18t, Germany. All rights reserved.

 aktuelle Versionen gibt es hier:
 $Source: /cvs/tuxbox/apps/tuxbox/neutrino/src/driver/genpsi.c,v $

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 675 Mass Ave, Cambridge MA 02139, USA.

 Mit diesem Programm koennen Neutrino TS Streams für das Abspielen unter Enigma gepatched werden 
 */
#include <transform.h>
#include <driver/genpsi.h>

#define SIZE_TS_PKT			188
#define OFS_HDR_2			5
#define OFS_PMT_DATA		13
#define OFS_STREAM_TAB		17
#define SIZE_STREAM_TAB_ROW	5 
#define OFS_ENIGMA_TAB		31
#define SIZE_ENIGMA_TAB_ROW   4  

#define ES_TYPE_MPEG12		0x02
#define ES_TYPE_MPA			0x03
#define ES_TYPE_AC3			0x81

#define EN_TYPE_VIDEO		0x00
#define EN_TYPE_AUDIO		0x01
#define EN_TYPE_TELTEX		0x02
#define EN_TYPE_PCR			0x03
typedef struct 
{
	 short	nba;
	 uint16_t	vpid;
	 uint16_t	apid[10];
	 short	isAC3[10];
} T_AV_PIDS;

	 T_AV_PIDS avPids;
void transfer_pids(uint16_t pid,uint16_t pidart,short isAC3)
{
	switch(pidart)
	{
		case EN_TYPE_VIDEO:
			avPids.vpid=pid;
			break;
		case EN_TYPE_AUDIO:
			avPids.apid[avPids.nba]=pid;
			avPids.isAC3[avPids.nba]=isAC3;
			avPids.nba++;
			break;
		case EN_TYPE_TELTEX:
			break;

		default:
			break;
	}
}
//-- special enigma stream description packet for  --
//-- at least 1 video, 1 audo and 1 PCR-Pid stream --
//------------------------------------------------------------------------------------
static uint8_t pkt_enigma[] =
{
	0x47, 0x40, 0x1F, 0x10, 0x00,
	0x7F, 0x80, 0x24,
	0x00, 0x00, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x6D, 0x66, 0x30, 0x19, 
	0x80, 0x13, 'N','E','U','T','R','I','N','O','N','G',	// tag(8), len(8), text(10) -> NG hihi ;)
	0x00, 0x02, 0x00, 0x6e,				// cVPID(8), len(8), PID(16)
	0x01, 0x03, 0x00, 0x78, 0x00,			// cAPID(8), len(8), PID(16), ac3flag(8)
// 0x02, 0x02, 0x00, 0x82,// cTPID(8), len(8), ...
	0x03, 0x02, 0x00, 0x6e				// cPCRPID(8), ...
};  
//-- PAT packet for at least 1 PMT --
//----------------------------------------------------------
static uint8_t pkt_pat[] =
{
	0x47, 0x40, 0x00, 0x10, 0x00,			// HEADER-1
	0x00, 0xB0, 0x0D,					// HEADER-2
	0x04, 0x37, 0xE9, 0x00, 0x00,			// HEADER-3 sid
	0x6D, 0x66, 0xEF, 0xFF,				// PAT-DATA - PMT (PID=0xFFF) entry
};

//-- PMT packet for at least 1 video and 1 audio stream --          
//--------------------------------------------------------
static uint8_t pkt_pmt[] =
{
	0x47, 0x4F, 0xFF, 0x10, 0x00,		// HEADER-1
	0x02, 0xB0, 0x17,				// HEADER-2
	0x6D, 0x66, 0xE9, 0x00, 0x00,		// HEADER-3
	0xE0, 0x00, 0xF0, 0x00,			// PMT-DATA  
	0x02, 0xE0, 0x00, 0xF0, 0x00,		//   (video stream 1)
	0x03, 0xE0, 0x00, 0xF0, 0x00		//   (audio stream 1)
};  


//== setup a new TS packet with format ==
//== predefined with a template        ==
//=======================================
#define COPY_TEMPLATE(dst, src) copy_template(dst, src, sizeof(src))

static int copy_template(uint8_t *dst, uint8_t *src, int len)
{
//-- reset buffer --
	memset(dst, 0xFF, SIZE_TS_PKT);
//-- copy PMT template --
	memcpy(dst, src, len);
	
	return len;
}
int genpsi(int fd2)
{
	int  bytes = 0;
	uint8_t   pkt[SIZE_TS_PKT];
	int       i, data_len, patch_len, ofs;

//-- copy "Enigma"-template --
	data_len = COPY_TEMPLATE(pkt, pkt_enigma);

//-- adjust len dependent to number of audio streams --
	data_len += ((SIZE_ENIGMA_TAB_ROW+1) * (avPids.nba-1));

	patch_len = data_len - OFS_HDR_2 + 1;
	pkt[OFS_HDR_2+1] |= (patch_len>>8);
	pkt[OFS_HDR_2+2]  = (patch_len & 0xFF); 
//-- write row with desc. for video stream --  
	ofs = OFS_ENIGMA_TAB;
	pkt[ofs]   = EN_TYPE_VIDEO;
	pkt[ofs+1] = 0x02;
	pkt[ofs+2] = (avPids.vpid>>8);
	pkt[ofs+3] = (avPids.vpid & 0xFF);
//-- for each audio stream, write row with desc. --
	ofs += SIZE_ENIGMA_TAB_ROW;  
	for (i=0; i<avPids.nba; i++)
	{
		pkt[ofs]   = EN_TYPE_AUDIO;
		pkt[ofs+1] = 0x03;
		pkt[ofs+2] = (avPids.apid[i]>>8);
		pkt[ofs+3] = (avPids.apid[i] & 0xFF);
		pkt[ofs+4] = (avPids.isAC3[i]==1)? 0x01 : 0x00;

		ofs += (SIZE_ENIGMA_TAB_ROW + 1);
	}
//-- write row with desc. for pcr stream (eq. video) -- 
	pkt[ofs]   = EN_TYPE_PCR;
	pkt[ofs+1] = 0x02;
	pkt[ofs+2] = (avPids.vpid>>8);
	pkt[ofs+3] = (avPids.vpid & 0xFF);
 
//-- calculate CRC --
	calc_crc32psi(&pkt[data_len], &pkt[OFS_HDR_2], data_len-OFS_HDR_2 );
//-- write TS packet --
	bytes += write(fd2, pkt, SIZE_TS_PKT);
//-- (II) build PAT --
	data_len = COPY_TEMPLATE(pkt, pkt_pat);
//-- calculate CRC --
	calc_crc32psi(&pkt[data_len], &pkt[OFS_HDR_2], data_len-OFS_HDR_2 );
//-- write TS packet --
	bytes += write(fd2, pkt, SIZE_TS_PKT);

//-- (III) build PMT --
	data_len = COPY_TEMPLATE(pkt, pkt_pmt);
//-- adjust len dependent to count of audio streams --
	data_len += (SIZE_STREAM_TAB_ROW * (avPids.nba-1));
	patch_len = data_len - OFS_HDR_2 + 1;
	pkt[OFS_HDR_2+1] |= (patch_len>>8);
	pkt[OFS_HDR_2+2]  = (patch_len & 0xFF); 
//-- patch pcr PID --
	ofs = OFS_PMT_DATA;
	pkt[ofs]  |= (avPids.vpid>>8);
	pkt[ofs+1] = (avPids.vpid & 0xFF);
//-- write row with desc. for ES video stream --
	ofs = OFS_STREAM_TAB;
	pkt[ofs]   = ES_TYPE_MPEG12;
	pkt[ofs+1] = 0xE0 | (avPids.vpid>>8);
	pkt[ofs+2] = (avPids.vpid & 0xFF);
	pkt[ofs+3] = 0xF0;
	pkt[ofs+4] = 0x00;

//-- for each ES audio stream, write row with desc. --
	for (i=0; i<avPids.nba; i++)
	{
		ofs += SIZE_STREAM_TAB_ROW;
		pkt[ofs]   = (avPids.isAC3[i]==1)? ES_TYPE_AC3 : ES_TYPE_MPA;
		pkt[ofs+1] = 0xE0 | (avPids.apid[i]>>8);
		pkt[ofs+2] = (avPids.apid[i] & 0xFF);
		pkt[ofs+3] = 0xF0;
		pkt[ofs+4] = 0x00;
	}
//-- calculate CRC --
	calc_crc32psi(&pkt[data_len], &pkt[OFS_HDR_2], data_len-OFS_HDR_2 );
//-- write TS packet --
	bytes += write(fd2, pkt, SIZE_TS_PKT);
//-- finish --
	avPids.vpid=0;
	avPids.nba=0;
	return 1;
}
