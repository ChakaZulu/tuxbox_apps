/*
 * $Id: capmt.cpp,v 1.2 2003/08/20 22:47:35 obi Exp $
 *
 * Copyright (C) 2002, 2003 Andreas Oberritter <obi@saftware.de>
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

#include <dvb/id/descriptor_tag.h>
#include <dvb/table/capmt.h>

CaLengthField::CaLengthField(const uint32_t length)
{
	if (length < 0x80) {
		sizeIndicator = 0;
		lengthValue = length;
	}
	else {
		uint32_t mask = 0xff;

		sizeIndicator = 1;
		lengthFieldSize = 1;

		while ((length & mask) != length) {
			lengthFieldSize++;
			mask = (mask << 8) | 0xff;
		}

		for (uint8_t i = lengthFieldSize; i > 0; i--)
			lengthValueByte.push_back((length >> ((i - 1) << 3)) & 0xff);
	}
}

CaElementaryStreamInfo::CaElementaryStreamInfo(const ElementaryStreamInfo * const info, const uint8_t cmdId)
{
	streamType = info->streamType;
	elementaryPid = info->elementaryPid;
	esInfoLength = 0;

	for (DescriptorConstIterator i = info->getDescriptors()->begin(); i != info->getDescriptors()->end(); ++i)
		if ((*i)->getTag() == CA_DESCRIPTOR) {
			descriptors.push_back(new CaDescriptor(*(CaDescriptor *)*i));
			esInfoLength += (*i)->getLength() + 2;
		}

	if (esInfoLength) {
		caPmtCmdId = cmdId;
		esInfoLength++;
	}
}

CaElementaryStreamInfo::~CaElementaryStreamInfo(void)
{
	for (CaDescriptorIterator i = descriptors.begin(); i != descriptors.end(); ++i)
		delete *i;
}

uint16_t CaElementaryStreamInfo::getLength(void) const
{
	return esInfoLength + 5;
}

CaProgramMapTable::CaProgramMapTable(const ProgramMapTable * const pmt, const uint8_t listManagement, const uint8_t cmdId)
{
	uint32_t length = 6;

	caPmtTag = 0x9f80c3;
	caPmtListManagement = listManagement;

	programNumber = pmt->tableIdExtension;
	versionNumber = pmt->versionNumber;
	currentNextIndicator = pmt->currentNextIndicator;
	programInfoLength = 0;

	for (DescriptorConstIterator i = pmt->getDescriptors()->begin(); i != pmt->getDescriptors()->end(); ++i)
		if ((*i)->getTag() == CA_DESCRIPTOR) {
			descriptors.push_back(new CaDescriptor(*(CaDescriptor *)*i));
			programInfoLength += (*i)->getLength() + 2;
		}

	if (programInfoLength) {
		caPmtCmdId = cmdId;
		programInfoLength++;
		length += programInfoLength;
	}

	for (ElementaryStreamInfoConstIterator i = pmt->esInfo.begin(); i != pmt->esInfo.end(); ++i) {
		CaElementaryStreamInfo *info = new CaElementaryStreamInfo(*i, cmdId);
		esInfo.push_back(info);
		length += info->getLength();
	}

	lengthField = new CaLengthField(length);
}

CaProgramMapTable::~CaProgramMapTable(void)
{
	for (CaDescriptorIterator i = descriptors.begin(); i != descriptors.end(); ++i)
		delete *i;

	for (CaElementaryStreamInfoIterator i = esInfo.begin(); i != esInfo.end(); ++i)
		delete *i;

	delete lengthField;
}

