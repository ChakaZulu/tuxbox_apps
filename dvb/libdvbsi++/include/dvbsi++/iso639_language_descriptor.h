/*
 * $Id: iso639_language_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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
