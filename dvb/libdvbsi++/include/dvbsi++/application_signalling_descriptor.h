/*
 * $Id: application_signalling_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __application_signalling_descriptor_h__
#define __application_signalling_descriptor_h__

#include "descriptor.h"

class ApplicationSignalling
{
	protected:
		unsigned applicationType			: 16;
		unsigned aitVersionNumber			: 5;

	public:
		ApplicationSignalling(const uint8_t * const buffer);

		uint16_t getApplicationType(void) const;
		uint8_t getAitVersionNumber(void) const;
};

typedef std::list<ApplicationSignalling *> ApplicationSignallingList;
typedef ApplicationSignallingList::iterator ApplicationSignallingIterator;
typedef ApplicationSignallingList::const_iterator ApplicationSignallingConstIterator;

class ApplicationSignallingDescriptor : public Descriptor
{
	protected:
		ApplicationSignallingList applicationSignallings;

	public:
		ApplicationSignallingDescriptor(const uint8_t * const buffer);
		~ApplicationSignallingDescriptor(void);

		const ApplicationSignallingList *getApplicationSignallings(void) const;
};

#endif /* __application_signalling_descriptor_h__ */
