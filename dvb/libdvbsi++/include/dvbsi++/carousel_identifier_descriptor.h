/*
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
