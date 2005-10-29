/*
 * $Id: dvb_html_application_descriptor.h,v 1.3 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __dvb_html_application_descriptor_h__
#define __dvb_html_application_descriptor_h__

#include "descriptor.h"

typedef std::list<uint16_t> ApplicationIdList;
typedef ApplicationIdList::iterator ApplicationIdIterator;
typedef ApplicationIdList::const_iterator ApplicationIdConstIterator;

class DvbHtmlApplicationDescriptor : public Descriptor
{
	protected:
		unsigned appidSetLength				: 8;
		ApplicationIdList applicationIds;
		std::string parameter;

	public:
		DvbHtmlApplicationDescriptor(const uint8_t * const buffer);

		const ApplicationIdList *getApplicationIds(void) const;
		const std::string &getParameter(void) const;
};

#endif /* __dvb_html_application_descriptor_h__ */
