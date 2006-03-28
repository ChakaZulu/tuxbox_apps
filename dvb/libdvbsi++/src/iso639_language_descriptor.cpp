/*
 * $Id: iso639_language_descriptor.cpp,v 1.4 2006/03/28 17:22:00 ghostrider Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/iso639_language_descriptor.h>

Iso639Language::Iso639Language(const uint8_t * const buffer)
{
	iso639LanguageCode.assign((char *)&buffer[0], 3);
	audioType = buffer[3];
}

const std::string &Iso639Language::getIso639LanguageCode(void) const
{
	return iso639LanguageCode;
}

uint8_t Iso639Language::getAudioType(void) const
{
	return audioType;
}

Iso639LanguageDescriptor::Iso639LanguageDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; i += 4) {
		ASSERT_MIN_DLEN(i + 4);
		iso639Languages.push_back(new Iso639Language(&buffer[i + 2]));
	}
}

Iso639LanguageDescriptor::~Iso639LanguageDescriptor(void)
{
	for (Iso639LanguageIterator i = iso639Languages.begin(); i != iso639Languages.end(); ++i)
		delete *i;
}

const Iso639LanguageList *Iso639LanguageDescriptor::getIso639Languages(void) const
{
	return &iso639Languages;
}

