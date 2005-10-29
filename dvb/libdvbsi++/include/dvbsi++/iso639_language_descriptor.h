/*
 * $Id: iso639_language_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __iso639_language_descriptor_h__
#define __iso639_language_descriptor_h__

#include "descriptor.h"

class Iso639Language
{
	protected:
		std::string iso639LanguageCode;
		unsigned audioType				: 8;

	public:
		Iso639Language(const uint8_t * const buffer);

		const std::string &getIso639LanguageCode(void) const;
		uint8_t getAudioType(void) const;
};

typedef std::list<Iso639Language *> Iso639LanguageList;
typedef Iso639LanguageList::iterator Iso639LanguageIterator;
typedef Iso639LanguageList::const_iterator Iso639LanguageConstIterator;

class Iso639LanguageDescriptor : public Descriptor
{
	protected:
		Iso639LanguageList iso639Languages;

	public:
		Iso639LanguageDescriptor(const uint8_t * const buffer);
		~Iso639LanguageDescriptor(void);

		const Iso639LanguageList *getIso639Languages(void) const;
};

#endif /* __iso639_language_descriptor_h__ */
