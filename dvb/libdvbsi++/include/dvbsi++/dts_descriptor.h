/*
 *  $Id: dts_descriptor.h,v 1.1 2005/12/26 20:48:57 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __dts_descriptor_h__
#define __dts_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> AdditionalInfoByteVector;
typedef AdditionalInfoByteVector::iterator AdditionalByteIterator;
typedef AdditionalInfoByteVector::const_iterator AdditionalByteConstIterator;

class DTSDescriptor : public Descriptor
{
	protected:
		unsigned sampleRate		: 4;
		unsigned bitRate		: 6;
		unsigned numberOfBlocks		: 7;
		unsigned frameSize		:14;
		unsigned surroundMode		: 6;
		unsigned lfeFlag		: 1;
		unsigned extendedSurroundFlag	: 2;

		AdditionalInfoByteVector additionalInfoBytes;
	public:
		DTSDescriptor(const uint8_t* const buffer);
		virtual ~DTSDescriptor();

		uint8_t getSampleRate() const;
		uint8_t getBitRate() const;
		uint8_t getNumberOfBlocks() const;
		uint16_t getFrameSize() const;
		uint8_t getLfeFlag() const;
		uint8_t getExtendedSurroundFlag() const;

		const AdditionalInfoByteVector* getAdditionalInfoBytes() const;
};

#endif /* __dts_descriptor_h__*/
