/*
 * $Id: sdt.h,v 1.2 2003/08/20 22:47:24 obi Exp $
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

#ifndef __dvb_table_sdt_h__
#define __dvb_table_sdt_h__

#include <dvb/descriptor/container.h>
#include "long_crc_table.h"

class ServiceDescription : public DescriptorContainer
{
	protected:
		unsigned serviceId				: 16;
		unsigned eitScheduleFlag			: 1;
		unsigned eitPresentFollowingFlag		: 1;
		unsigned runningStatus				: 3;
		unsigned freeCaMode				: 1;
		unsigned descriptorsLoopLength			: 12;

	public:
		ServiceDescription(const uint8_t * const buffer);

		uint16_t getServiceId(void) const;
		uint8_t getEitScheduleFlag(void) const;
		uint8_t getEitPresentFollowingFlag(void) const;
		uint8_t getRunningStatus(void) const;
		uint8_t getFreeCaMode(void) const;
};

typedef std::vector<ServiceDescription *> ServiceDescriptionVector;
typedef ServiceDescriptionVector::iterator ServiceDescriptionIterator;
typedef ServiceDescriptionVector::const_iterator ServiceDescriptionConstIterator;

class ServiceDescriptionTable : public LongCrcTable
{
	protected:
		unsigned originalNetworkId			: 16;
		ServiceDescriptionVector description;

	public:
		ServiceDescriptionTable(const uint8_t * const buffer);
		~ServiceDescriptionTable(void);

		static const enum PacketId PID = PID_SDT;
		static const enum TableId TID = TID_SDT_ACTUAL;
		static const uint32_t TIMEOUT = 3000;

		uint16_t getOriginalNetworkId(void) const;
		const ServiceDescriptionVector *getDescriptions(void) const;
};

typedef std::vector<ServiceDescriptionTable *> ServiceDescriptionTableVector;
typedef ServiceDescriptionTableVector::iterator ServiceDescriptionTableIterator;
typedef ServiceDescriptionTableVector::const_iterator ServiceDescriptionTableConstIterator;

#endif /* __dvb_table_sdt_h__ */
