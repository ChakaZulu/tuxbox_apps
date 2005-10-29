/*
 * $Id: announcement_support_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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

typedef std::list<Announcement *> AnnouncementList;
typedef AnnouncementList::iterator AnnouncementIterator;
typedef AnnouncementList::const_iterator AnnouncementConstIterator;

class AnnouncementSupportDescriptor : public Descriptor
{
	protected:
		unsigned announcementSupportIndicator		: 16;
		AnnouncementList announcements;

	public:
		AnnouncementSupportDescriptor(const uint8_t * const buffer);
		~AnnouncementSupportDescriptor(void);

		uint16_t getAnnouncementSupportIndicator(void) const;
		const AnnouncementList *getAnnouncements(void) const;
};

#endif /* __announcement_support_descriptor_h__ */
