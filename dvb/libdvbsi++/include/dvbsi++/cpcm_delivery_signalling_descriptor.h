/*
 *  $Id: cpcm_delivery_signalling_descriptor.h,v 1.1 2009/06/30 12:03:02 mws Exp $
 *
 *  Copyright (C) 2009 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __cpcm_delivery_signalling_descriptor_h__
#define __cpcm_delivery_signalling_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> SelectorByteVector;
typedef SelectorByteVector::iterator SelectorByteByteIterator;
typedef SelectorByteVector::const_iterator SelectorByteConstIterator;

class CpcmDeliverySignallingDescriptor : public Descriptor
{
	protected:
		unsigned extensionTag		: 8;
		unsigned cpcmVersion		: 8;

		SelectorByteVector selectorBytes;

	public:
		CpcmDeliverySignallingDescriptor(const uint8_t* const buffer);
		virtual ~CpcmDeliverySignallingDescriptor();

		uint8_t getExtensionTag() const;
		uint8_t getCpcmVersion() const;
		const SelectorByteVector* getSelectorBytes() const;
};

#endif /* __cpcm_delivery_signalling_descriptor_h__*/
