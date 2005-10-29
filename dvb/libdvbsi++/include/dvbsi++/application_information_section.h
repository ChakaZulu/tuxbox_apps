/*
 * $Id: application_information_section.h,v 1.4 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __application_information_section_h__
#define __application_information_section_h__

#include "descriptor_container.h"
#include "long_crc_section.h"
#include "application_identifier.h"

class ApplicationInformation : public DescriptorContainer
{
	protected:
		ApplicationIdentifier *applicationIdentifier;
		unsigned applicationControlCode			: 8;
		unsigned applicationDescriptorsLoopLength	: 12;

	public:
		ApplicationInformation(const uint8_t * const buffer);
		~ApplicationInformation(void);

		const ApplicationIdentifier *getApplicationIdentifier(void) const;
		uint8_t getApplicationControlCode(void) const;

	friend class ApplicationInformationSection;
};

typedef std::list<ApplicationInformation *> ApplicationInformationList;
typedef ApplicationInformationList::iterator ApplicationInformationIterator;
typedef ApplicationInformationList::const_iterator ApplicationInformationConstIterator;

class ApplicationInformationSection : public LongCrcSection, public DescriptorContainer
{
	protected:
		unsigned commonDescriptorsLength		: 12;
		unsigned applicationLoopLength			: 12;
		ApplicationInformationList applicationInformation;

	public:
		ApplicationInformationSection(const uint8_t * const buffer);
		~ApplicationInformationSection(void);

		static const enum TableId TID = TID_AIT;
		static const uint32_t TIMEOUT = 12000;

		const ApplicationInformationList *getApplicationInformation(void) const;
};

typedef std::list<ApplicationInformationSection *> ApplicationInformationSectionList;
typedef ApplicationInformationSectionList::iterator ApplicationInformationSectionIterator;
typedef ApplicationInformationSectionList::const_iterator ApplicationInformationSectionConstIterator;

#endif /* __application_information_section_h__ */
