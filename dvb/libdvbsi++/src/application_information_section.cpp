/*
 * $Id: application_information_section.cpp,v 1.4 2005/09/29 23:49:44 ghostrider Exp $
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

#include <dvbsi++/application_information_section.h>
#include <dvbsi++/byte_stream.h>

ApplicationInformation::ApplicationInformation(const uint8_t * const buffer)
{
	applicationIdentifier = new ApplicationIdentifier(&buffer[0]);
	applicationControlCode = buffer[6];
	applicationDescriptorsLoopLength = DVB_LENGTH(&buffer[7]);

	for (size_t i = 0; i < applicationDescriptorsLoopLength; i += buffer[i + 10] + 2)
		descriptor(&buffer[i + 9], SCOPE_MHP);
}

ApplicationInformation::~ApplicationInformation(void)
{
	delete applicationIdentifier;
}

const ApplicationIdentifier *ApplicationInformation::getApplicationIdentifier(void) const
{
	return applicationIdentifier;
}

uint8_t ApplicationInformation::getApplicationControlCode(void) const
{
	return applicationControlCode;
}

ApplicationInformationSection::ApplicationInformationSection(const uint8_t * const buffer) : LongCrcSection(buffer)
{
	commonDescriptorsLength = DVB_LENGTH(&buffer[8]);

	for (size_t i = 0; i < commonDescriptorsLength; i += buffer[i + 11] + 2)
		descriptor(&buffer[i + 10], SCOPE_MHP);

	applicationLoopLength = DVB_LENGTH(&buffer[commonDescriptorsLength + 10]);

	for (size_t i = 0; i < applicationLoopLength; i += 9) {
		ApplicationInformation *a = new ApplicationInformation(&buffer[commonDescriptorsLength + 12]);
		applicationInformation.push_back(a);
		i += a->applicationDescriptorsLoopLength;
	}
}

ApplicationInformationSection::~ApplicationInformationSection(void)
{
	for (ApplicationInformationIterator i = applicationInformation.begin(); i != applicationInformation.end(); ++i)
		delete *i;
}

const ApplicationInformationList *ApplicationInformationSection::getApplicationInformation(void) const
{
	return &applicationInformation;
}

