/*
 *  $Id: extension_descriptor.h,v 1.1 2005/12/26 20:48:57 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __extension_descriptor_h__
#define __extension_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> SelectorByteVector;
typedef SelectorByteVector::iterator SelectorByteByteIterator;
typedef SelectorByteVector::const_iterator SelectorByteConstIterator;

class ExtensionDescriptor : public Descriptor
{
	protected:
		unsigned extensionTag		: 8;

		SelectorByteVector selectorBytes;

	public:
		ExtensionDescriptor(const uint8_t* const buffer);
		virtual ~ExtensionDescriptor();

		uint8_t getExtensionTag() const;
		const SelectorByteVector* getSelectorBytes() const;
};

#endif /* __extension_descriptor_h__*/
