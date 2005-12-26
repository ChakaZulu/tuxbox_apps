/*
 *  $Id: enhanced_ac3_descriptor.h,v 1.1 2005/12/26 20:48:57 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __enhanced_ac3_descriptor_h__
#define __enhanced_ac3_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> AdditionalInfoVector;
typedef AdditionalInfoVector::iterator AdditionalInfoIterator;
typedef AdditionalInfoVector::const_iterator AdditionalInfoConstIterator;

class EnhancedAC3Descriptor : public Descriptor
{
	protected:
		unsigned componentTypeFlag		: 1;
		unsigned bsidFlag			: 1;
		unsigned mainidFlag			: 1;
		unsigned asvcFlag			: 1;
		unsigned mixInfoExistsFlag		: 1;
		unsigned substream1Flag			: 1;
		unsigned substream2Flag			: 1;
		unsigned substream3Flag			: 1;

		uint8_t componentType;
		uint8_t bsid;
		uint8_t mainid;
		uint8_t avsc;
		uint8_t substream1;
		uint8_t substream2;
		uint8_t substream3;

		AdditionalInfoVector additionalInfo;

	public:
		EnhancedAC3Descriptor(const uint8_t* const buffer);
		virtual ~EnhancedAC3Descriptor();

		uint8_t getComponentTypeFlag() const;
		uint8_t getBsidFlag() const;
		uint8_t getMainidFlag() const;
		uint8_t getAsvcFlag() const;
		uint8_t getMixInfoExistsFlag() const;
		uint8_t getSubstream1Flag() const;
		uint8_t getSubstream2Flag() const;
		uint8_t getSubstream3Flag() const;

		uint8_t getComponentType() const;
		uint8_t getBsid() const;
		uint8_t getMainid() const;
		uint8_t getAvsc() const;
		uint8_t getSubstream1() const;
		uint8_t getSubstream2() const;
		uint8_t getSubstream3() const;

		const AdditionalInfoVector* getAdditionalInfo() const;
};

#endif /* __enhanced_ac3_descriptor_h__*/
