/*
 * $Id: announcement_support_descriptor.h,v 1.1 2004/02/13 15:27:37 obi Exp $
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

#ifndef __announcement_support_descriptor_h__
#define __announcement_support_descriptor_h__

#include "descriptor.h"

class Announcement
{
	protected:
		unsigned announcementType			: 4;
		unsigned referenceType				: 3;
		unsigned originalNetworkId			: 16;
		unsigned transportStreamId			: 16;
		unsigned serviceId				: 16;
		unsigned componentTag				: 8;

	public:
		Announcement(const uint8_t * const buffer);

		uint8_t getAnnouncementType(void) const;
		uint8_t getReferenceType(void) const;
		uint16_t getOriginalNetworkId(void) const;
		uint16_t getTransportStreamId(void) const;
		uint16_t getServiceId(void) const;
		uint8_t getComponentTag(void) const;
};

typedef std::vector<Announcement *> AnnouncementVector;
typedef AnnouncementVector::iterator AnnouncementIterator;
typedef AnnouncementVector::const_iterator AnnouncementConstIterator;

class AnnouncementSupportDescriptor : public Descriptor
{
	protected:
		unsigned announcementSupportIndicator		: 16;
		AnnouncementVector announcements;

	public:
		AnnouncementSupportDescriptor(const uint8_t * const buffer);
		~AnnouncementSupportDescriptor(void);

		uint16_t getAnnouncementSupportIndicator(void) const;
		const AnnouncementVector *getAnnouncements(void) const;
};

#endif /* __announcement_support_descriptor_h__ */
