/*
 * $Id: component_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __component_descriptor_h__
#define __component_descriptor_h__

#include "descriptor.h"

class ComponentDescriptor : public Descriptor
{
	protected:
		unsigned streamContent				: 4;
		unsigned componentType				: 8;
		unsigned componentTag				: 8;
		std::string iso639LanguageCode;
		std::string text;

	public:
		ComponentDescriptor(const uint8_t * const buffer);

		uint8_t getStreamContent(void) const;
		uint8_t getComponentType(void) const;
		uint8_t getComponentTag(void) const;
		const std::string &getIso639LanguageCode(void) const;
		const std::string &getText(void) const;
};

#endif /* __component_descriptor_h__ */
