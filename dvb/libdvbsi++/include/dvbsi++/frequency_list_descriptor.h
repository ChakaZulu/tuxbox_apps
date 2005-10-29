/*
 * $Id: frequency_list_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __frequency_list_descriptor_h__
#define __frequency_list_descriptor_h__

#include "descriptor.h"

typedef std::list<uint32_t> CentreFrequencyList;
typedef CentreFrequencyList::iterator CentreFrequencyIterator;
typedef CentreFrequencyList::const_iterator CentreFrequencyConstIterator;

class FrequencyListDescriptor : public Descriptor
{
	protected:
		unsigned codingType				: 2;
		CentreFrequencyList centreFrequencies;

	public:
		FrequencyListDescriptor(const uint8_t * const buffer);

		uint8_t getCodingType(void) const;
		const CentreFrequencyList *getCentreFrequencies(void) const;
};

#endif /* __frequency_list_descriptor_h__ */
