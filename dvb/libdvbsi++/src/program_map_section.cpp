/*
 * $Id: program_map_section.cpp,v 1.2 2004/02/13 17:51:08 obi Exp $
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

#include <dvbsi++/byte_stream.h>
#include <dvbsi++/program_map_section.h>

ElementaryStreamInfo::ElementaryStreamInfo(const uint8_t * const buffer)
{
	streamType = buffer[0];
	elementaryPid = DVB_PID(&buffer[1]);
	esInfoLength = DVB_LENGTH(&buffer[3]);

	for (size_t i = 5; i < esInfoLength + 5; i += buffer[i + 1] + 2)
		descriptor(&buffer[i]);
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
		descriptor(&buffer[i]);

	for (size_t i = programInfoLength + 12; i < sectionLength - 1; i += DVB_LENGTH(&buffer[i + 3]) + 5)
		esInfo.push_back(new ElementaryStreamInfo(&buffer[i]));
}

uint16_t ProgramMapSection::getPcrPid(void) const
{
	return pcrPid;
}

const ElementaryStreamInfoVector *ProgramMapSection::getEsInfo(void) const
{
	return &esInfo;
}

ProgramMapSection::~ProgramMapSection(void)
{
	for (ElementaryStreamInfoIterator i = esInfo.begin(); i != esInfo.end(); ++i)
		delete *i;
}

