/*
 * $Id: network_name_descriptor.cpp,v 1.2 2005/10/29 00:10:17 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/network_name_descriptor.h>

NetworkNameDescriptor::NetworkNameDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	networkName.assign((char *)&buffer[2], descriptorLength);
}

const std::string &NetworkNameDescriptor::getNetworkName(void) const
{
	return networkName;
}

