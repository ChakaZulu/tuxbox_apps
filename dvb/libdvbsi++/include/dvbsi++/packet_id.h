/*
 * $Id: packet_id.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __packet_id_h__
#define __packet_id_h__

enum PacketId {
	/* ETSI EN 300 468 V1.5.1 (2003-05) */
	PID_PAT		= 0x0000,
	PID_CAT		= 0x0001,
	PID_TSDT	= 0x0002,
	PID_NIT		= 0x0010,
	PID_BAT		= 0x0011,
	PID_SDT		= 0x0011,
	PID_EIT		= 0x0012,
	PID_RST		= 0x0013,
	PID_TDT		= 0x0014,
	PID_TOT		= 0x0014,
	PID_NS		= 0x0015,	/* network synchronization */
	PID_IS		= 0x001C,	/* inband signaling (SIS-12) */
	PID_M		= 0x001D,	/* measurement (SIS-10) */
	PID_DIT		= 0x001E,
	PID_SIT		= 0x001F,
	PID_RESERVED	= 0x1FFF
};

#endif /* __packet_id_h__ */
