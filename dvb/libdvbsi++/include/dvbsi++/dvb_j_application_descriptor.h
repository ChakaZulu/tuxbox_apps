/*
 * $Id: dvb_j_application_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __dvb_j_application_descriptor_h__
#define __dvb_j_application_descriptor_h__

#include "descriptor.h"

class DvbJApplication
{
	protected:
		unsigned parameterLength			: 8;
		std::string parameter;

	public:
		DvbJApplication(const uint8_t * const buffer);

		uint8_t getParameterLength(void) const;
		const std::string &getParameter(void) const;
};

typedef std::list<DvbJApplication *> DvbJApplicationList;
typedef DvbJApplicationList::iterator DvbJApplicationIterator;
typedef DvbJApplicationList::const_iterator DvbJApplicationConstIterator;

class DvbJApplicationDescriptor : public Descriptor
{
	protected:
		DvbJApplicationList dvbJApplications;

	public:
		DvbJApplicationDescriptor(const uint8_t * const buffer);
		~DvbJApplicationDescriptor(void);

		const DvbJApplicationList *getDvbJApplications(void) const;
};

#endif /* __dvb_j_application_descriptor_h__ */
