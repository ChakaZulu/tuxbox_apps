/*
 * $Id: network_information_section.cpp,v 1.3 2004/05/31 21:21:23 obi Exp $
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
#include <dvbsi++/network_information_section.h>

TransportStreamInfo::TransportStreamInfo(const uint8_t * const buffer)
{
	transportStreamId = UINT16(&buffer[0]);
	originalNetworkId = UINT16(&buffer[2]);
	transportDescriptorsLength = DVB_LENGTH(&buffer[4]);

	for (size_t i = 6; i < transportDescriptorsLength + 6; i += buffer[i + 1] + 2)
		descriptor(&buffer[i], SCOPE_SI);
}

uint16_t TransportStreamInfo::getTransportStreamId(void) const
{
	return transportStreamId;
}

uint16_t TransportStreamInfo::getOriginalNetworkId(void) const
{
	return originalNetworkId;
}

NetworkInformationSection::NetworkInformationSection(const uint8_t * const buffer) : LongCrcSection(buffer)
{
	networkDescriptorsLength = DVB_LENGTH(&buffer[8]);

	for (size_t i = 10; i < networkDescriptorsLength + 10; i += buffer[i + 1] + 2)
		descriptor(&buffer[i], SCOPE_SI);

	transportStreamLoopLength = DVB_LENGTH(&buffer[networkDescriptorsLength + 10]);

	for (size_t i = networkDescriptorsLength + 12; i < sectionLength + 3 - 4; i += DVB_LENGTH(&buffer[i + 4]) + 6)
		tsInfo.push_back(new TransportStreamInfo(&buffer[i]));
}

NetworkInformationSection::~NetworkInformationSection(void)
{
	for (TransportStreamInfoIterator i = tsInfo.begin(); i != tsInfo.end(); ++i)
		delete *i;
}

const TransportStreamInfoVector *NetworkInformationSection::getTsInfo(void) const
{
	return &tsInfo;
}

