/*
 * $Id: registration_descriptor.h,v 1.2 2008/11/17 17:01:30 ghostrider Exp $
 *
 * Copyright (C) 2008 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __registration_descriptor_h__
#define __registration_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> AdditionalIdentificationInfoVector;
typedef AdditionalIdentificationInfoVector::iterator AdditionalIdentificationInfoIterator;
typedef AdditionalIdentificationInfoVector::const_iterator AdditionalIdentificationInfoConstIterator;

class RegistrationDescriptor : public Descriptor
{
	protected:
		uint32_t formatIdentifier;
		AdditionalIdentificationInfoVector additionalIdentificationInfo;

	public:
		RegistrationDescriptor(const uint8_t * const buffer);

		uint32_t getFormatIdentifier(void) const;
		const AdditionalIdentificationInfoVector *getAdditionalIdentificationInfo(void) const;
};

#endif /* __registration_descriptor_h__ */
