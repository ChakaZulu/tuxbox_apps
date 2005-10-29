/*
 * $Id: multilingual_component_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
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
