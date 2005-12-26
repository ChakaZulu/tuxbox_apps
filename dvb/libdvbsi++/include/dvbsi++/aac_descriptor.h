/*
 *  $Id: aac_descriptor.h,v 1.1 2005/12/26 20:48:57 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __aac_descriptor_h__
#define __aac_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> AdditionalInfoByteVector;
typedef AdditionalInfoByteVector::iterator AdditionalByteIterator;
typedef AdditionalInfoByteVector::const_iterator AdditionalByteConstIterator;

class AACDescriptor : public Descriptor
{
	protected:
		unsigned profileLevel		: 8;
		unsigned aacTypeFlag		: 1;

		uint8_t aacType;

		AdditionalInfoByteVector additionalInfoBytes;

	public:
		AACDescriptor(const uint8_t* const buffer);
		virtual ~AACDescriptor();

		uint8_t getProfileLevel() const;
		uint8_t getAACTypeFlag() const;
		uint8_t getAACType() const;

		const AdditionalInfoByteVector* getAdditionalInfoBytes() const;
};

#endif /* __aac_descriptor_h__*/
