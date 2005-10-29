/*
 * $Id: application_name_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __application_name_descriptor_h__
#define __application_name_descriptor_h__

#include "descriptor.h"

class ApplicationName
{
	protected:
		std::string iso639LanguageCode;
		unsigned applicationNameLength			: 8;
		std::string applicationName;

	public:
		ApplicationName(const uint8_t * const buffer);

		const std::string &getIso639LanguageCode(void) const;
		const std::string &getApplicationName(void) const;
};

typedef std::list<ApplicationName *> ApplicationNameList;
typedef ApplicationNameList::iterator ApplicationNameIterator;
typedef ApplicationNameList::const_iterator ApplicationNameConstIterator;

class ApplicationNameDescriptor : public Descriptor
{
	protected:
		ApplicationNameList applicationNames;

	public:
		ApplicationNameDescriptor(const uint8_t * const buffer);
		~ApplicationNameDescriptor(void);

		const ApplicationNameList *getApplicationNames(void) const;
};

#endif /* __application_name_descriptor_h__ */
