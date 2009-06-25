/*
 *  $Id: service_type.h,v 1.6 2009/06/25 16:29:01 obi Exp $
 * 
 *  Copyright (C) 2002-2006 Andreas Oberritter <obi@saftware.de>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __service_type_h__
#define __service_type_h__

enum ServiceType {
	/* 0x00 - 0x1B: ETSI EN 300 468 V1.9.1 (2009-03)*/
	ST_RESERVED				= 0x00,
	ST_DIGITAL_TELEVISION_SERVICE		= 0x01,
	ST_DIGITAL_RADIO_SOUND_SERVICE		= 0x02,
	ST_TELETEXT_SERVICE			= 0x03,
	ST_NVOD_REFERENCE_SERVICE		= 0x04,
	ST_NVOD_TIME_SHIFTED_SERVICE		= 0x05,
	ST_MOSAIC_SERVICE			= 0x06,
	/* 0x07 - 0x09 are reserved for future use */
	/* old values kept as comment */
	/* ST_PAL_CODED_SIGNAL			= 0x07, */
	/* ST_SECAM_CODED_SIGNAL		= 0x08, */
	/* ST_D_D2_MAC				= 0x09, */
	
	/* 0x0a - 0x0b redefined */
	/* old values kept as comment */
	/* ST_FM_RADIO				= 0x0A, */
	/* ST_NTSC_CODED_SIGNAL			= 0x0B, */
	ST_AVC_DIGITAL_RADIO_SOUND_SERVICE	= 0x0A,
	ST_AVC_MOSAIC_SERVICE			= 0x0B,

	ST_DATA_BROADCAST_SERVICE		= 0x0C,
	ST_COMMON_INTERFACE_RESERVED		= 0x0D,
	ST_RCS_MAP				= 0x0E,
	ST_RCS_FLS				= 0x0F,
	ST_DVB_MHP_SERVICE			= 0x10,
	ST_MPEG2_HD_DIGITAL_TV_SERVICE 		= 0x11,
	/* 0x12 to 0x15: reserved for future use */
	ST_AVC_SD_DIGITAL_TV_SERVICE 		= 0x16,
	ST_AVC_SD_NVOD_TIME_SHIFTED_SERVICE 	= 0x17,
	ST_AVC_SD_NVOD_REFERENCE_SERVICE 	= 0x18,
	ST_AVC_HD_DIGITAL_TV_SERVICE 		= 0x19,
	ST_AVC_HD_NVOD_TIME_SHIFTED_SERVICE 	= 0x1A,
	ST_AVC_HD_NVOD_REFERENCE_SERVICE 	= 0x1B,
	/* 0x1c - 0x7F: reserved for future use */
	ST_MULTIFEED				= 0x69
	/* 0x80 - 0xFE: user defined*/
	/* 0xFF: reserved for future use*/
};

#endif /* __service_type_h__*/
