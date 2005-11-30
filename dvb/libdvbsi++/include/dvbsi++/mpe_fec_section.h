/*
 * MPE_FEC Section ETSI EN 301 192 V1.4.1
 *
 * $Id: mpe_fec_section.h,v 1.1 2005/11/30 16:43:25 mws Exp $
 *
 * Copyright (C) 2005 Marcel Siegert <mws@twisted-brains.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */
#ifndef __mpe_fec_section_h__
#define __mpe_fec_section_h__

#include "dvbsi++/long_crc_section.h"

class MpeFecSection;

class RealTimeParameters
{
	protected:
		unsigned delta_t	:12;
		unsigned tableBoundary	: 1;
		unsigned frameBoundary	: 1;
		unsigned address	:18;
	public:
		RealTimeParameters(const uint8_t* const buffer);
		~RealTimeParameters();

		uint16_t getDeltaT() const;
		uint8_t getTableBoundary() const;
		uint8_t getFrameBoundary() const;
		uint32_t getAddress() const;
};

typedef std::vector<uint8_t> RSDataByteVector;
typedef RSDataByteVector::iterator RSDataByteIterator;
typedef RSDataByteVector::const_iterator RSDataByteConstIterator;

class MpeFecSection : public LongCrcSection
{
	protected:
		RealTimeParameters rtParam;

		RSDataByteVector rsDataBytes;

	public:
		MpeFecSection(const uint8_t* const buffer);
		virtual ~MpeFecSection();

		static const uint16_t LENGTH = 4096;
		static const enum PacketId PID = PID_RESERVED;
		static const enum TableId TID = TID_MPE_FEC;
		static const uint32_t TIMEOUT = 3000;

		const RealTimeParameters* getRealTimeParameters() const;
		const RSDataByteVector* getRSDataBytes() const;
};

typedef std::list<MpeFecSection*> MpeFecSectionList;
typedef MpeFecSectionList::iterator MpeFecSectionIterator;
typedef MpeFecSectionList::const_iterator MpeFecSectionConstIterator;

#endif //__mpe_fec_section_h__
