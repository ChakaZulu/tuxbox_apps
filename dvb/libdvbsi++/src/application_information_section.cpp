/*
 * $Id: application_information_section.cpp,v 1.6 2006/03/28 17:22:00 ghostrider Exp $
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
	commonDescriptorsLength = sectionLength > 10 ? DVB_LENGTH(&buffer[8]) : 0;

	uint16_t pos = 10;
	uint16_t bytesLeft = sectionLength > 11 ? sectionLength - 11 : 0;
	uint16_t loopLength = 0;
	uint16_t bytesLeft2 = commonDescriptorsLength;

	while (bytesLeft >= bytesLeft && bytesLeft2 > 1 && bytesLeft2 >= (loopLength = 2 + buffer[pos+1])) {
		descriptor(&buffer[pos], SCOPE_MHP);
		pos += loopLength;
		bytesLeft -= loopLength;
		bytesLeft2 -= loopLength;
	}

	if (!bytesLeft2 && bytesLeft > 1) {
		bytesLeft2 = applicationLoopLength = DVB_LENGTH(&buffer[pos]);
		pos+=2;
		bytesLeft-=2;
		while (bytesLeft >= bytesLeft2 && bytesLeft2 > 8 && bytesLeft2 >= (loopLength = 8 + DVB_LENGTH(&buffer[pos+7]))) {
			applicationInformation.push_back(new ApplicationInformation(&buffer[pos]));
			pos += loopLength;
			bytesLeft -= loopLength;
			bytesLeft2 -= loopLength;
		}
	}
	else
		applicationLoopLength = 0;
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

