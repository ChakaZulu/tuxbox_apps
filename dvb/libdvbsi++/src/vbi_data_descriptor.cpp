/*
 * $Id: vbi_data_descriptor.cpp,v 1.4 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/vbi_data_descriptor.h>

VbiDataLine::VbiDataLine(const uint8_t * const buffer)
{
	fieldParity = (buffer[0] >> 5) & 0x01;
	lineOffset = buffer[0] & 0x1F;
}

uint8_t VbiDataLine::getFieldParity(void) const
{
	return fieldParity;
}

uint8_t VbiDataLine::getLineOffset(void) const
{
	return lineOffset;
}

VbiDataService::VbiDataService(const uint8_t * const buffer)
{
	uint16_t i;

	dataServiceId = buffer[0];
	dataServiceDescriptorLength = buffer[1];

	switch (dataServiceId) {
	case 0x01:
	case 0x02:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		for (i = 0; i < dataServiceDescriptorLength; ++i)
			vbiDataLines.push_back(new VbiDataLine(&buffer[i + 2]));
		break;

	default:
		break;
	}
}

VbiDataService::~VbiDataService(void)
{
	for (VbiDataLineIterator i = vbiDataLines.begin(); i != vbiDataLines.end(); ++i)
		delete *i;
}

uint8_t VbiDataService::getDataServiceId(void) const
{
	return dataServiceId;
}

const VbiDataLineList *VbiDataService::getVbiDataLines(void) const
{
	return &vbiDataLines;
}

VbiDataDescriptor::VbiDataDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += buffer[i + 3] + 2)
		vbiDataServices.push_back(new VbiDataService(&buffer[i + 2]));
}

VbiDataDescriptor::~VbiDataDescriptor(void)
{
	for (VbiDataServiceIterator i = vbiDataServices.begin(); i != vbiDataServices.end(); ++i)
		delete *i;
}

const VbiDataServiceList *VbiDataDescriptor::getVbiDataServices(void) const
{
	return &vbiDataServices;
}

