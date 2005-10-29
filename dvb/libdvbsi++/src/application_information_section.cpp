/*
 * $Id: application_information_section.cpp,v 1.5 2005/10/29 00:10:16 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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

