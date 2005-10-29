/*
 * $Id: application_storage_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
 
#ifndef __application_storage_descriptor_h__
#define __application_storage_descriptor_h__

#include "descriptor.h"

class ApplicationStorageDescriptor : public Descriptor
{
	protected:
		unsigned storageProperty			: 8;
		unsigned notLaunchableFromBroadcast		: 1;
		unsigned version				: 32;
		unsigned priority				: 8;

	public:
		ApplicationStorageDescriptor(const uint8_t * const buffer);

		uint8_t getStorageProperty(void) const;
		uint8_t getNotLaunchableFromBroadcast(void) const;
		uint32_t getVersion(void) const;
		uint8_t getPriority(void) const;
};

#endif /* __application_storage_descriptor_h__ */
