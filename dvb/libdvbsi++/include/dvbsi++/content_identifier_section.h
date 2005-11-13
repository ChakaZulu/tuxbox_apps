/*
 *  $Id: content_identifier_section.h,v 1.2 2005/11/13 17:56:20 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __content_identifier_section_h__
#define __content_identifier_section_h__

#include "long_crc_section.h"

typedef std::vector<uint8_t> ContentIdentifierByteVector;
typedef ContentIdentifierByteVector::iterator ContentIdentifierByteByteIterator;
typedef ContentIdentifierByteVector::const_iterator ContentIdentifierByteByteConstIterator;

class CridLabel
{
	protected:
		unsigned cridRef			:16;
		unsigned prependStringIndex		: 8;
		unsigned uniqueStringLength		: 8;

		ContentIdentifierByteVector uniqueStringBytes;

	public:
		CridLabel(const uint8_t* const buffer);
		virtual ~CridLabel();

		uint16_t getCridRef() const;
		uint8_t getPrependStringIndex() const;
		uint8_t getUniqueStringLength() const;

		const ContentIdentifierByteVector* getUniqueStringBytes() const;
};

typedef std::list<CridLabel*> CridLabelList;
typedef CridLabelList::iterator CridLabelIterator;
typedef CridLabelList::const_iterator CridLabelConstIterator;


class ContentIdentifierSection : public LongCrcSection
{
	protected:
		unsigned transportStreamId		:16;
		unsigned originalNetworkId		:16;
		unsigned prependStringLength		: 8;

		ContentIdentifierByteVector	prependStringsBytes;

		CridLabelList cridLabels;

	public:
		ContentIdentifierSection(const uint8_t* const buffer);
		virtual ~ContentIdentifierSection();

		static const uint16_t LENGTH = 4096;
		static const enum PacketId PID = PID_EIT;
		static const enum TableId TID = TID_CIT;
		static const uint32_t TIMEOUT = 3000;

		uint16_t getTransportStreamId() const;
		uint16_t getOriginalNetworkId() const;
		uint8_t getPrependStringLength() const;

		const ContentIdentifierByteVector* getPrependStringBytes() const;
		const CridLabelList* getCridLabels() const;
};

typedef std::list<ContentIdentifierSection*> ContentIdentifierSectionList;
typedef ContentIdentifierSectionList::iterator ContentIdentifierSectionIterator;
typedef ContentIdentifierSectionList::const_iterator ContentIdentifierSectionConstIterator;

#endif /* __content_identifier_section_h__*/
