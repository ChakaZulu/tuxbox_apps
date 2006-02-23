/*
 *  $Id: transport_stream_descriptor.h,v 1.1 2006/02/23 19:12:39 mws Exp $
 *
 *  Copyright (C) 2006 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __transport_stream_descriptor_h__
#define __transport_stream_descriptor_h__

#include "descriptor.h"

typedef std::vector<uint8_t> PrivateDataByteVector;
typedef PrivateDataByteVector::iterator PrivateDataByteIterator;
typedef PrivateDataByteVector::const_iterator PrivateDataByteConstIterator;

class TransportStreamDescriptor : public Descriptor
{
	protected:
		PrivateDataByteVector privateDataBytes;

	public:
		TransportStreamDescriptor(const uint8_t* const buffer);
		virtual ~TransportStreamDescriptor();

		const PrivateDataByteVector* getPrivateDataBytes(void) const;
};

#endif /* __transport_stream_descriptor_h__ */
