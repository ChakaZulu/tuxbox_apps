/*
 * $Id: service.h,v 1.2 2004/02/13 16:18:23 obi Exp $
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

#ifndef __dvb_channel_service_h__
#define __dvb_channel_service_h__

#include <string>
#include <map>
#include <dvbsi++/service_descriptor.h>

class Service
{
	protected:
		uint16_t serviceId;
		uint8_t serviceType;
		std::string serviceProviderName;
		std::string serviceName;

	public:
		Service(uint16_t serviceId, uint8_t serviceType, std::string serviceProviderName, std::string serviceName);
		Service(uint16_t serviceId, ServiceDescriptor *sd);

		uint16_t getId(void) const;
		uint8_t getType(void) const;
		std::string getProviderName(void) const;
		std::string getName(void) const;
};

typedef std::map<const uint16_t, Service *> ServiceMap;
typedef ServiceMap::iterator ServiceMapIterator;
typedef ServiceMap::const_iterator ServiceMapConstIterator;

#endif /* __dvb_channel_service_h__ */
