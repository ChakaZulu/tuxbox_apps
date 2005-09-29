/*
 * $Id: multilingual_component_descriptor.h,v 1.2 2005/09/29 23:49:41 ghostrider Exp $
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

#ifndef __multilingual_component_descriptor_h__
#define __multilingual_component_descriptor_h__

#include "descriptor.h"

class MultilingualComponent
{
	protected:
		std::string iso639LanguageCode;
		unsigned textDescriptionLength			: 8;
		std::string text;

	public:
		MultilingualComponent(const uint8_t * const buffer);

		const std::string &getIso639LanguageCode(void) const;
		const std::string &getText(void) const;
};

typedef std::list<MultilingualComponent *> MultilingualComponentList;
typedef MultilingualComponentList::iterator MultilingualComponentIterator;
typedef MultilingualComponentList::const_iterator MultilingualComponentConstIterator;

class MultilingualComponentDescriptor : public Descriptor
{
	protected:
		unsigned componentTag				: 8;
		MultilingualComponentList multilingualComponents;

	public:
		MultilingualComponentDescriptor(const uint8_t * const buffer);
		~MultilingualComponentDescriptor(void);

		uint8_t getComponentTag(void) const;
		const MultilingualComponentList *getMultilingualComponents(void) const;
};

#endif /* __multilingual_component_descriptor_h__ */
