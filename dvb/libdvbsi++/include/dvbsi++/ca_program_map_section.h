/*
 * $Id: ca_program_map_section.h,v 1.5 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2002-2005 Andreas Oberritter <obi@saftware.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __ca_program_map_section_h__
#define __ca_program_map_section_h__

#include "ca_descriptor.h"
#include "program_map_section.h"
#include "descriptor_container.h"

class CaLengthField
{
	protected:
		unsigned sizeIndicator				: 1;
		unsigned lengthValue				: 7;
		unsigned lengthFieldSize			: 7;
		std::list<uint8_t> lengthValueByte;

	public:
		CaLengthField(const uint32_t length);

		size_t writeToBuffer(uint8_t * const buffer) const;
};

class CaElementaryStreamInfo
{
	protected:
		unsigned streamType				: 8;
		unsigned elementaryPid				: 13;
		unsigned esInfoLength				: 12;
		unsigned caPmtCmdId				: 8;
		CaDescriptorList descriptors;

	public:
		CaElementaryStreamInfo(const ElementaryStreamInfo * const info, const uint8_t cmdId);
		~CaElementaryStreamInfo(void);

		uint16_t getLength(void) const;

		size_t writeToBuffer(uint8_t * const buffer) const;
};

typedef std::list<CaElementaryStreamInfo *> CaElementaryStreamInfoList;
typedef CaElementaryStreamInfoList::iterator CaElementaryStreamInfoIterator;
typedef CaElementaryStreamInfoList::const_iterator CaElementaryStreamInfoConstIterator;

class CaProgramMapSection : public DescriptorContainer
{
	protected:
		uint32_t length;
		unsigned caPmtTag				: 24;
		unsigned caPmtListManagement			: 8;
		unsigned programNumber				: 16;
		unsigned versionNumber				: 5;
		unsigned currentNextIndicator			: 1;
		unsigned programInfoLength			: 12;
		unsigned caPmtCmdId				: 8;
		CaElementaryStreamInfoList esInfo;

	public:
		CaProgramMapSection(const ProgramMapSection * const pmt, const uint8_t listManagement, const uint8_t cmdId);
		~CaProgramMapSection(void);

		bool append(const ProgramMapSection * const pmt);
		void injectDescriptor(const uint8_t *descriptor, bool back=true);
		size_t writeToBuffer(uint8_t * const buffer) const;
		ssize_t writeToFile(int fd) const;
};

typedef std::list<CaProgramMapSection *> CaProgramMapSectionList;
typedef CaProgramMapSectionList::iterator CaProgramMapSectionIterator;
typedef CaProgramMapSectionList::const_iterator CaProgramMapSectionConstIterator;

#endif /* __ca_program_map_section_h__ */
