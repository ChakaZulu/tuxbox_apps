/*
 * $Id: service_type.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
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
	/* 0x00 - 0x10: ETSI EN 300 468 V1.5.1 (2003-05) */
	ST_RESERVED			= 0x00,
	ST_DIGITAL_TELEVISION_SERVICE	= 0x01,
	ST_DIGITAL_RADIO_SOUND_SERVICE	= 0x02,
	ST_TELETEXT_SERVICE		= 0x03,
	ST_NVOD_REFERENCE_SERVICE	= 0x04,
	ST_NVOD_TIME_SHIFTED_SERVICE	= 0x05,
	ST_MOSAIC_SERVICE		= 0x06,
	ST_PAL_CODED_SIGNAL		= 0x07,
	ST_SECAM_CODED_SIGNAL		= 0x08,
	ST_D_D2_MAC			= 0x09,
	ST_FM_RADIO			= 0x0A,
	ST_NTSC_CODED_SIGNAL		= 0x0B,
	ST_DATA_BROADCAST_SERVICE	= 0x0C,
	ST_COMMON_INTERFACE_RESERVED	= 0x0D,
	ST_RCS_MAP			= 0x0E,
	ST_RCS_FLS			= 0x0F,
	ST_DVB_MHP_SERVICE		= 0x10,
	/* 0x11 - 0x7F: reserved for future use */
	ST_MULTIFEED			= 0x69
	/* 0x80 - 0xFE: user defined */
	/* 0xFF: reserved for future use */
};

#endif /* __service_type_h__ */
