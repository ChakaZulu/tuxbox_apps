/*
 * $Id: byte_stream.h,v 1.1 2003/08/20 22:47:17 obi Exp $
 *
 * Copyright (C) 2002, 2003 Andreas Oberritter <obi@saftware.de>
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

#ifndef __dvb_byte_stream_h__
#define __dvb_byte_stream_h__

#include <inttypes.h>

#if __BYTE_ORDER == __BIG_ENDIAN
inline uint16_t UINT16(const void * const ptr)
{
	return *(const uint16_t * const)ptr;
}

inline uint32_t UINT32(const void * const ptr)
{
	return *(const uint32_t * const)ptr;
}
#else
#include <byteswap.h>
static inline uint16_t UINT16(const void * const ptr)
{
	return bswap_16(*(const uint16_t * const)ptr);
}

static inline uint32_t UINT32(const void * const ptr)
{
	return bswap_32(*(const uint32_t * const)ptr);
}
#endif

inline uint16_t DVB_LENGTH(const void * const ptr)
{
	return UINT16(ptr) & 0x0fff;
}

inline uint16_t DVB_PID(const void * const ptr)
{
	return UINT16(ptr) & 0x1fff;
}

#endif /* __dvb_byte_stream_h__ */
