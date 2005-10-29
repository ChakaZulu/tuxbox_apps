/*
 * $Id: byte_stream.h,v 1.4 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __byte_stream_h__
#define __byte_stream_h__

#include "compat.h"

#if __BYTE_ORDER == __BIG_ENDIAN
#define r16(p)		(*(const uint16_t * const)(p))
#define r32(p)		(*(const uint32_t * const)(p))
#define r64(p)		(*(const uint64_t * const)(p))
#define w16(p,v)	do { *(uint16_t * const)(p) = ((const uint16_t)v) } while (0)
#define w32(p,v)	do { *(uint32_t * const)(p) = ((const uint32_t)v) } while (0)
#define w64(p,v)	do { *(uint64_t * const)(p) = ((const uint64_t)v) } while (0)
#else
#define r16(p)		bswap_16(*(const uint16_t * const)p)
#define r32(p)		bswap_32(*(const uint32_t * const)p)
#define r64(p)		bswap_64(*(const uint64_t * const)p)
#define w16(p,v)	do { *(uint16_t * const)(p) = bswap_16((const uint16_t)v) } while (0)
#define w32(p,v)	do { *(uint32_t * const)(p) = bswap_32((const uint32_t)v) } while (0)
#define w64(p,v)	do { *(uint64_t * const)(p) = bswap_64((const uint64_t)v) } while (0)
#endif

#define DVB_LENGTH(p)	(r16(p) & 0x0fff)
#define DVB_PID(p)	(r16(p) & 0x1fff)

// deprecated
#define UINT16(p)	r16(p)
#define UINT32(p)	r32(p)

#endif /* __byte_stream_h__ */
