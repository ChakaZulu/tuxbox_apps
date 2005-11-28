/*
 * $Id: carousel_identifier_descriptor.h,v 1.5 2005/11/28 16:25:09 ghostrider Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __carousel_identifier_descriptor_h__
#define __carousel_identifier_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> PrivateDataByteVector;
typedef PrivateDataByteVector::iterator PrivateDataByteIterator;
typedef PrivateDataByteVector::const_iterator PrivateDataByteConstIterator;

class CarouselIdentifierDescriptor : public Descriptor
{
	protected:
		unsigned carouselId				: 32;
		unsigned formatId				: 8;
		unsigned moduleVersion				: 8;
		unsigned moduleId				: 16;
		unsigned blockSize				: 16;
		unsigned moduleSize				: 32;
		unsigned compressionMethod			: 8;
		unsigned originalSize				: 32;
		unsigned timeout				: 8;
		unsigned objectKeyLength			: 8;
		std::string objectKey;
		PrivateDataByteVector privateDataBytes;

	public:
		CarouselIdentifierDescriptor(const uint8_t * const buffer);

		uint32_t getCarouselId(void) const;
		uint8_t getFormatId(void) const;
		uint8_t getModuleVersion(void) const;
		uint16_t getModuleId(void) const;
		uint16_t getBlockSize(void) const;
		uint32_t getModuleSize(void) const;
		uint8_t getCompressionMethod(void) const;
		uint32_t getOriginalSize(void) const;
		uint8_t getTimeout(void) const;
		const std::string &getObjectKey(void) const;
		const PrivateDataByteVector *getPrivateDataBytes(void) const;
};

#endif /* __carousel_identifier_descriptor_h__ */
