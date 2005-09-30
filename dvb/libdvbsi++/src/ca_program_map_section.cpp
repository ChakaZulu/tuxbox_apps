/*
 * $Id: ca_program_map_section.cpp,v 1.6 2005/09/30 16:23:08 ghostrider Exp $
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

#include <dvbsi++/ca_program_map_section.h>
#include <dvbsi++/compat.h>
#include <dvbsi++/descriptor_tag.h>

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

		for (size_t i = lengthFieldSize; i > 0; --i)
			lengthValueByte.push_back((length >> ((i - 1) << 3)) & 0xff);
	}
}

size_t CaLengthField::writeToBuffer(uint8_t * const buffer) const
{
	size_t total = 0;

	if (sizeIndicator == 0) {
		buffer[total++] = lengthValue;
	}
	else {
		buffer[total++] = (sizeIndicator << 7) | lengthFieldSize;
		for (std::list<uint8_t>::const_iterator i = lengthValueByte.begin(); i != lengthValueByte.end(); i++)
			buffer[total++] = *i;
	}

	return total;
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

size_t CaElementaryStreamInfo::writeToBuffer(uint8_t * const buffer) const
{
	size_t total = 0;

	buffer[total++] = streamType;
	buffer[total++] = (elementaryPid >> 8) & 0xff;
	buffer[total++] = (elementaryPid >> 0) & 0xff;
	buffer[total++] = (esInfoLength >> 8) & 0xff;
	buffer[total++] = (esInfoLength >> 0) & 0xff;

	if (esInfoLength != 0) {
		buffer[total++] = caPmtCmdId;
		for (CaDescriptorConstIterator i = descriptors.begin(); i != descriptors.end(); ++i)
			total += (*i)->writeToBuffer(&buffer[total]);
	}

	return total;
}

bool CaProgramMapSection::append(const ProgramMapSection * const pmt)
{
	if ( pmt->tableIdExtension != programNumber || pmt->versionNumber != versionNumber || currentNextIndicator != pmt->currentNextIndicator )
		return false;

	for (DescriptorConstIterator i = pmt->getDescriptors()->begin(); i != pmt->getDescriptors()->end(); ++i)
		if ((*i)->getTag() == CA_DESCRIPTOR) {
			descriptorList.push_back(new CaDescriptor(*(CaDescriptor *)*i));
			programInfoLength += (*i)->getLength() + 2;
			length += (*i)->getLength() + 2;
		}

	for (ElementaryStreamInfoConstIterator i = pmt->esInfo.begin(); i != pmt->esInfo.end(); ++i) {
		CaElementaryStreamInfo *info = new CaElementaryStreamInfo(*i, caPmtCmdId);
		esInfo.push_back(info);
		length += info->getLength();
	}
	
	return true;
}

CaProgramMapSection::CaProgramMapSection(const ProgramMapSection * const pmt, const uint8_t listManagement, const uint8_t cmdId)
	:programInfoLength(0)
{
	length = 6;

	caPmtTag = 0x9f8032;
	caPmtListManagement = listManagement;
	caPmtCmdId = cmdId;

	programNumber = pmt->tableIdExtension;
	versionNumber = pmt->versionNumber;
	currentNextIndicator = pmt->currentNextIndicator;

	append(pmt);
}

CaProgramMapSection::~CaProgramMapSection(void)
{
	for (CaElementaryStreamInfoIterator i = esInfo.begin(); i != esInfo.end(); ++i)
		delete *i;
}

void CaProgramMapSection::injectDescriptor(const uint8_t *descr, bool back)
{
	descriptorSi(descr, back);
	int len = (back ? descriptorList.back()->getLength() : descriptorList.front()->getLength()) + 2;
	programInfoLength += len + 2;
	length += descriptorList.back()->getLength() + 2;
}

size_t CaProgramMapSection::writeToBuffer(uint8_t * const buffer) const
{
	unsigned programInfoLength = this->programInfoLength;
	uint32_t length = this->length;

	size_t total = 0;

	if (programInfoLength) {
			programInfoLength++;
			length++;
	}

	CaLengthField lengthField(length);

	buffer[total++] = (caPmtTag >> 16) & 0xff;
	buffer[total++] = (caPmtTag >>  8) & 0xff;
	buffer[total++] = (caPmtTag >>  0) & 0xff;

	total += lengthField.writeToBuffer(&buffer[total]);

	buffer[total++] = caPmtListManagement;
	buffer[total++] = (programNumber >> 8) & 0xff;
	buffer[total++] = (programNumber >> 0) & 0xff;
	buffer[total++] = (versionNumber << 1) | currentNextIndicator;
	buffer[total++] = (programInfoLength >> 8) & 0xff;
	buffer[total++] = (programInfoLength >> 0) & 0xff;

	if (programInfoLength) {
		buffer[total++] = caPmtCmdId;
		for (DescriptorConstIterator i = descriptorList.begin(); i != descriptorList.end(); ++i)
			total += (*i)->writeToBuffer(&buffer[total]);
	}

	for (CaElementaryStreamInfoConstIterator i = esInfo.begin(); i != esInfo.end(); ++i)
		total += (*i)->writeToBuffer(&buffer[total]);

	return total;
}

ssize_t CaProgramMapSection::writeToFile(const int fd) const
{
	unsigned char buffer[4096];
	size_t length;

	length = writeToBuffer(buffer);

	return write(fd, buffer, length);
}

