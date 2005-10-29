/*
 * $Id: vbi_teletext_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __vbi_teletext_descriptor_h__
#define __vbi_teletext_descriptor_h__

#include "descriptor.h"

class VbiTeletext
{
	protected:
		std::string iso639LanguageCode;
		unsigned teletextType				: 5;
		unsigned teletextMagazineNumber			: 3;
		unsigned teletextPageNumber			: 8;

	public:
		VbiTeletext(const uint8_t * const buffer);

		const std::string &getIso639LanguageCode(void) const;
		uint8_t getTeletextType(void) const;
		uint8_t getTeletextMagazineNumber(void) const;
		uint8_t getTeletextPageNumber(void) const;
};

typedef std::list<VbiTeletext *> VbiTeletextList;
typedef VbiTeletextList::iterator VbiTeletextIterator;
typedef VbiTeletextList::const_iterator VbiTeletextConstIterator;

class VbiTeletextDescriptor : public Descriptor
{
	protected:
		VbiTeletextList vbiTeletexts;

	public:
		VbiTeletextDescriptor(const uint8_t * const buffer);
		~VbiTeletextDescriptor(void);

		const VbiTeletextList *getVbiTeletexts(void) const;
};

#endif /* __vbi_teletext_descriptor_h__ */
