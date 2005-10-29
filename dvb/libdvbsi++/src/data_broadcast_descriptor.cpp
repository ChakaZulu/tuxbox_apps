/*
 * $Id: data_broadcast_descriptor.cpp,v 1.4 2005/10/29 00:10:16 obi Exp $
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
#include <dvbsi++/data_broadcast_descriptor.h>

DataBroadcastDescriptor::DataBroadcastDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	dataBroadcastId = UINT16(&buffer[2]);
	componentTag = buffer[3];
	selectorLength = buffer[4];

	for (size_t i = 0; i < selectorLength; ++i)
		selectorBytes.push_back(buffer[i + 5]);

	iso639LanguageCode.assign((char *)&buffer[selectorLength + 5], 3);
	textLength = buffer[selectorLength + 8];
	text.assign((char *)&buffer[selectorLength + 9], textLength);
}

uint16_t DataBroadcastDescriptor::getDataBroadcastId(void) const
{
	return dataBroadcastId;
}

uint8_t DataBroadcastDescriptor::getComponentTag(void) const
{
	return componentTag;
}

const selectorByteList *DataBroadcastDescriptor::getSelectorBytes(void) const
{
	return &selectorBytes;
}

const std::string &DataBroadcastDescriptor::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

const std::string &DataBroadcastDescriptor::getText(void) const
{
	return text;
}

