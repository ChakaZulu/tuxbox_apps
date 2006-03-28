/*
 * $Id: announcement_support_descriptor.cpp,v 1.5 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/announcement_support_descriptor.h>
#include <dvbsi++/byte_stream.h>

Announcement::Announcement(const uint8_t * const buffer)
{
	announcementType = (buffer[0] >> 4) & 0x0f;
	referenceType = buffer[0] & 0x07;

	if ((referenceType >= 0x01) && (referenceType <= 0x03)) {
		originalNetworkId = UINT16(&buffer[1]);
		transportStreamId = UINT16(&buffer[3]);
		serviceId = UINT16(&buffer[5]);
		componentTag = buffer[7];
	}
}

uint8_t Announcement::getAnnouncementType(void) const
{
	return announcementType;
}

uint8_t Announcement::getReferenceType(void) const
{
	return referenceType;
}

uint16_t Announcement::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

uint16_t Announcement::getTransportStreamId(void) const
{
	return transportStreamId;
}

uint16_t Announcement::getServiceId(void) const
{
	return serviceId;
}

uint8_t Announcement::getComponentTag(void) const
{
	return componentTag;
}

AnnouncementSupportDescriptor::AnnouncementSupportDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	Announcement *a;
	size_t headerLength = 2;
	ASSERT_MIN_DLEN(headerLength);

	announcementSupportIndicator = UINT16(&buffer[2]);

	for (size_t i = 0; i < descriptorLength - 2; ++i) {
		headerLength++;
		ASSERT_MIN_DLEN(headerLength);

		a = new Announcement(&buffer[i + 4]);
		announcements.push_back(a);
		switch (a->getReferenceType()) {
		case 0x01:
		case 0x02:
		case 0x03:
			// FIXME: might already have parsed beyond end
			// of memory in Announcement()
			headerLength += 7;
			ASSERT_MIN_DLEN(headerLength);

			i += 7;
			break;
		default:
			break;
		}
	}
}

AnnouncementSupportDescriptor::~AnnouncementSupportDescriptor(void)
{
	for (AnnouncementIterator i = announcements.begin(); i != announcements.end(); ++i)
		delete *i;
}

uint16_t AnnouncementSupportDescriptor::getAnnouncementSupportIndicator(void) const
{
	return announcementSupportIndicator;
}

const AnnouncementList *AnnouncementSupportDescriptor::getAnnouncements(void) const
{
	return &announcements;
}

