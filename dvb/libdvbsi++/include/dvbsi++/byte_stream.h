/*
 * $Id: byte_stream.h,v 1.2 2004/02/14 21:57:43 obi Exp $
 *
 * Copyright (C) 2002-2004 Andreas Oberritter <obi@saftware.de>
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

#ifndef __byte_stream_h__
#define __byte_stream_h__

#include "compat.h"

#if __BYTE_ORDER == __BIG_ENDIAN
#define r16(p)		*(const uint16_t * const)(p)
#define r32(p)		*(const uint32_t * const)(p)
#define r64(p)		*(const uint64_t * const)(p)
#define w16(p,v)	*(uint16_t * const)(p) = ((const uint16_t)v)
#define w32(p,v)	*(uint32_t * const)(p) = ((const uint32_t)v)
#define w64(p,v)	*(uint64_t * const)(p) = ((const uint64_t)v)
#else
#define r16(p)		bswap_16(*(const uint16_t * const)p)
#define r32(p)		bswap_32(*(const uint32_t * const)p)
#define r64(p)		bswap_64(*(const uint64_t * const)p)
#define w16(p,v)	*(uint16_t * const)(p) = bswap_16((const uint16_t)v)
#define w32(p,v)	*(uint32_t * const)(p) = bswap_32((const uint32_t)v)
#define w64(p,v)	*(uint64_t * const)(p) = bswap_64((const uint64_t)v)
#endif

#define DVB_LENGTH(p)	r16(p) & 0xffff
#define DVB_PID(p)	r16(p) & 0x1fff

// deprecated
#define UINT16(p)	r16(p)
#define UINT32(p)	r32(p)

#endif /* __byte_stream_h__ */
