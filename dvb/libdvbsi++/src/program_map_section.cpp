/*
 * $Id: program_map_section.cpp,v 1.7 2005/10/29 00:10:17 obi Exp $
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
	pcrPid = DVB_PID(&buffer[8]);
	programInfoLength = DVB_LENGTH(&buffer[10]);

	for (size_t i = 12; i < programInfoLength + 12; i += buffer[i + 1] + 2)
		descriptor(&buffer[i], SCOPE_SI);

	for (size_t i = programInfoLength + 12; i < sectionLength - 1; i += DVB_LENGTH(&buffer[i + 3]) + 5)
		esInfo.push_back(new ElementaryStreamInfo(&buffer[i]));
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

