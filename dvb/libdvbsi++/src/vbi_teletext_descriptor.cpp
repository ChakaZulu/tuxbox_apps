/*
 * $Id: vbi_teletext_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/vbi_teletext_descriptor.h>

VbiTeletext::VbiTeletext(const uint8_t * const buffer)
{
	iso639LanguageCode.assign((char *)&buffer[0], 3);
	teletextType = (buffer[3] >> 3) & 0x1F;
	teletextMagazineNumber = buffer[3] & 0x07;
	teletextPageNumber = buffer[4];
}

const std::string &VbiTeletext::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

uint8_t VbiTeletext::getTeletextType(void) const
{
	return teletextType;
}

uint8_t VbiTeletext::getTeletextMagazineNumber(void) const
{
	return teletextMagazineNumber;
}

uint8_t VbiTeletext::getTeletextPageNumber(void) const
{
	return teletextPageNumber;
}

VbiTeletextDescriptor::VbiTeletextDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 5) {
		ASSERT_MIN_DLEN(i + 5);
		vbiTeletexts.push_back(new VbiTeletext(&buffer[i + 2]));
	}
}

VbiTeletextDescriptor::~VbiTeletextDescriptor(void)
{
	for (VbiTeletextIterator i = vbiTeletexts.begin(); i != vbiTeletexts.end(); ++i)
		delete *i;
}

const VbiTeletextList *VbiTeletextDescriptor::getVbiTeletexts(void) const
{
	return &vbiTeletexts;
}

