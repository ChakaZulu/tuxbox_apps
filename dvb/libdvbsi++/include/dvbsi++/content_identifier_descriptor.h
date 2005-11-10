/*
 *  $Id: content_identifier_descriptor.h,v 1.1 2005/11/10 23:55:32 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __content_identifier_descriptor_h__
#define __content_identifier_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> ContentReferenceIdentifierByteVector;
typedef ContentReferenceIdentifierByteVector::iterator ContentReferenceIdentifierByteIterator;
typedef ContentReferenceIdentifierByteVector::const_iterator ContentReferenceIdentifierByteConstIterator;

class ContentReferenceIdentifier
{
	protected:
		unsigned type				: 6;
		unsigned location			: 2;

		// case location == 0x00
		unsigned length				: 8;
		ContentReferenceIdentifierByteVector cridBytes;

		// case location == 0x01
		unsigned reference			:16;

	public:
		ContentReferenceIdentifier(const uint8_t* const buffer);
		~ContentReferenceIdentifier();

		uint8_t getType() const;
		uint8_t getLocation() const;
		uint8_t getLength() const;
		const ContentReferenceIdentifierByteVector* getBytes() const;
		uint16_t getReference() const;

		friend class ContentIdentifierDescriptor;
};

typedef std::list<ContentReferenceIdentifier*> ContentReferenceIdentifierList;
typedef ContentReferenceIdentifierList::iterator ContentReferenceIdentifierIterator;
typedef ContentReferenceIdentifierList::const_iterator ContentReferenceIdentifierConstIterator;

class ContentIdentifierDescriptor : public Descriptor
{
	protected:
		ContentReferenceIdentifierList identifier;

	public:
		ContentIdentifierDescriptor(const uint8_t* const buffer);
		virtual ~ContentIdentifierDescriptor();

		const ContentReferenceIdentifierList* getIdentifier() const;
};

#endif /* __content_identifier_descriptor_h__*/
