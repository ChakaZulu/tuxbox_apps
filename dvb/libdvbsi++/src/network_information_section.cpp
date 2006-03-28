/*
 * $Id: network_information_section.cpp,v 1.6 2006/03/28 17:22:00 ghostrider Exp $
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
	networkDescriptorsLength = sectionLength > 9 ? DVB_LENGTH(&buffer[8]) : 0;
	
	uint16_t pos = 10;
	uint16_t bytesLeft = sectionLength > 11 ? sectionLength - 11 : 0;
	uint16_t loopLength = 0;
	uint16_t bytesLeft2 = networkDescriptorsLength;

	while ( bytesLeft >= bytesLeft2 && bytesLeft2 > 1 && bytesLeft2 >= (loopLength = 2 + buffer[pos+1])) {
		descriptor(&buffer[pos], SCOPE_SI);
		pos += loopLength;
		bytesLeft -= loopLength;
		bytesLeft2 -= loopLength;
	}

	if (!bytesLeft2 && bytesLeft > 1) {
		bytesLeft2 = transportStreamLoopLength = DVB_LENGTH(&buffer[pos]);
		bytesLeft -= 2;
		pos += 2;
		while (bytesLeft >= bytesLeft2 && bytesLeft2 > 4 && bytesLeft2 >= (loopLength = 6 + DVB_LENGTH(&buffer[pos+4]))) {
			tsInfo.push_back(new TransportStreamInfo(&buffer[pos]));
			bytesLeft -= loopLength;
			bytesLeft2 -= loopLength;
			pos += loopLength;
		}
	}
}

NetworkInformationSection::~NetworkInformationSection(void)
{
	for (TransportStreamInfoIterator i = tsInfo.begin(); i != tsInfo.end(); ++i)
		delete *i;
}

const TransportStreamInfoList *NetworkInformationSection::getTsInfo(void) const
{
	return &tsInfo;
}

