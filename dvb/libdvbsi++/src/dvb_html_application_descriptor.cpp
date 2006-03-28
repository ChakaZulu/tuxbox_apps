/*
 * $Id: dvb_html_application_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/dvb_html_application_descriptor.h>
#include <dvbsi++/byte_stream.h>

DvbHtmlApplicationDescriptor::DvbHtmlApplicationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	ASSERT_MIN_DLEN(1);

	appidSetLength = buffer[2];

	ASSERT_MIN_DLEN(appidSetLength + 1);

	for (size_t i = 0; i < appidSetLength; i += 2)
		applicationIds.push_back(r16(&buffer[i + 3]));

	parameter.assign((char *)&buffer[appidSetLength + 3], descriptorLength - appidSetLength - 1);
}

const ApplicationIdList *DvbHtmlApplicationDescriptor::getApplicationIds(void) const
{
	return &applicationIds;
}

const std::string &DvbHtmlApplicationDescriptor::getParameter(void) const
{
	return parameter;
}
