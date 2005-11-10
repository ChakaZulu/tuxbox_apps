/*
 *  $Id: ecm_repetition_rate_descriptor.h,v 1.1 2005/11/10 23:55:32 mws Exp $
 *
 *  Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __ecm_repetition_rate_descriptor_h__
#define __ecm_repetition_rate_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> ECMRepetitionPrivateByteVector;
typedef ECMRepetitionPrivateByteVector::iterator ECMRepetitionPrivateByteIterator;
typedef ECMRepetitionPrivateByteVector::const_iterator ECMRepetitionPrivateByteConstIterator;

class ECMRepetitionRateDecriptor : public Descriptor
{
	protected:
		unsigned caSystemId			:16;
		unsigned repetitionRate			:16;

		ECMRepetitionPrivateByteVector privateDataBytes;

	public:
		ECMRepetitionRateDecriptor(const uint8_t* const buffer);
		virtual ~ECMRepetitionRateDecriptor();

		uint16_t getCaSystemId(void) const;
		uint16_t getRepetitionRate(void) const;

		const ECMRepetitionPrivateByteVector* getPrivateDataBytes() const;
};

#endif /* __ecm_repetition_rate_descriptor_h__*/
