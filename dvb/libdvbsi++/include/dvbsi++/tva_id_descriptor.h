/*
 *  $Id: tva_id_descriptor.h,v 1.1 2005/11/10 23:55:32 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __tva_id_descriptor_h__
#define __tva_id_descriptor_h__

#include "descriptor.h"

class TVAIdentifier
{
	protected:
		unsigned id				:16;
		unsigned runningStatus			: 3;

	public:
		TVAIdentifier(const uint8_t* const buffer);
		~TVAIdentifier();

		uint16_t getId() const;
		uint8_t getRunningStatus() const;
};

typedef std::list<TVAIdentifier*> TVAIdentifierList;
typedef TVAIdentifierList::iterator TVAIdentifierIterator;
typedef TVAIdentifierList::const_iterator TVAIdentifierConstIterator;

class TVAIdDescriptor : public Descriptor
{
	protected:
		TVAIdentifierList identifier;

	public:
		TVAIdDescriptor(const uint8_t* const buffer);
		virtual ~TVAIdDescriptor();

		const TVAIdentifierList* getIdentifier() const;
};

#endif /* __tva_id_descriptor_h__*/
