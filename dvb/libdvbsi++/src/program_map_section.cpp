/*
 * $Id: program_map_section.cpp,v 1.8 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/program_map_section.h>

ElementaryStreamInfo::ElementaryStreamInfo(const uint8_t * const buffer)
{
	streamType = buffer[0];
	elementaryPid = DVB_PID(&buffer[1]);
	esInfoLength = DVB_LENGTH(&buffer[3]);

	for (size_t i = 5; i < esInfoLength + 5; i += buffer[i + 1] + 2)
		descriptor(&buffer[i], SCOPE_SI);
}

uint8_t ElementaryStreamInfo::getType(void) const
{
	return streamType;
}

uint16_t ElementaryStreamInfo::getPid(void) const
{
	return elementaryPid;
}

ProgramMapSection::ProgramMapSection(const uint8_t * const buffer) : LongCrcSection(buffer)
{
	pcrPid = sectionLength > 10 ? DVB_PID(&buffer[8]) : 0;
	programInfoLength = sectionLength > 12 ? DVB_LENGTH(&buffer[10]) : 0;

	uint16_t pos = 12;
	uint16_t bytesLeft = sectionLength > 13 ? sectionLength - 13 : 0;
	uint16_t loopLength = 0;
	uint16_t bytesLeft2 = programInfoLength;

	while (bytesLeft >= bytesLeft2 && bytesLeft2 > 1 && bytesLeft2 >= (loopLength = 2 + buffer[pos+1])) {
		descriptor(&buffer[pos], SCOPE_SI);
		pos += loopLength;
		bytesLeft -= loopLength;
		bytesLeft2 -= loopLength;
	}

	if (!bytesLeft2) {
		while (bytesLeft > 4 && bytesLeft >= (loopLength = 5 + DVB_LENGTH(&buffer[pos+3]))) {
			esInfo.push_back(new ElementaryStreamInfo(&buffer[pos]));
			bytesLeft -= loopLength;
			pos += loopLength;
		}
	}
}

uint16_t ProgramMapSection::getPcrPid(void) const
{
	return pcrPid;
}

uint16_t ProgramMapSection::getProgramNumber(void) const
{
	return getTableIdExtension();
}

const ElementaryStreamInfoList *ProgramMapSection::getEsInfo(void) const
{
	return &esInfo;
}

ProgramMapSection::~ProgramMapSection(void)
{
	for (ElementaryStreamInfoIterator i = esInfo.begin(); i != esInfo.end(); ++i)
		delete *i;
}

