/*
 * $Id: section_filter.h,v 1.6 2005/11/03 12:27:37 mws Exp $
 *
 * Copyright (C) 2002, 2003 Andreas Oberritter <obi@saftware.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __dvb_hardware_section_filter_h__
#define __dvb_hardware_section_filter_h__

#include <cstdio>
#include <ctime>
#include <vector>

#include <linux/dvb/dmx.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <dvb/debug/debug.h>
#include <dvb/pool/section_pool.h>

/**
 * SectionFilter template class.
 * Inherits release() and section() from SectionHandler class.
 * Uses SectionPool class to get section data.
 */

template <class T>
class SectionFilter : public SectionHandler
{
	protected:
		/**
		 * The demux file descriptor.
		 */
		int fd;

		/**
		 * The dvb api section filter parameters struct.
		 */
		struct dmx_sct_filter_params dsfp;

		/**
		 * Storage for received section data.
		 * Typically one SI table consists of one or more sections.
		 * Can also be used to store different tables, if clear() is
		 * not called.
		 */
		std::vector<T*> sections;

		/**
		 * Used internally to start demultiplexing.
		 */
		bool start(void);

		/**
		 * Used internally to stop demultiplexing.
		 * Only needed for non-oneshot filters.
		 */
		bool stop(void);

		/**
		 * Pointer to the global SectionPool.
		 * Shared between multiple SectionFilter instances.
		 */
		SectionPool *pool;

		/**
		 * Indicates that all sections which belong to the current
		 * table have been delivered or that a timeout has occured.
		 */
		bool done;

		/**
		 * Indicates that section filtering was stopped
		 * due to a timeout.
		 */
		bool timedOut;

		/**
		 * The time at which filtering started or restarted.
		 */
		time_t startTime;

		/**
		 * Used to mark which section_number has already been received.
		 * DVB SI sections are delivered sequentially. However, a filter
		 * could start in the middle of all sections and it might also
		 * miss some.
		 */
		std::map<uint8_t, uint8_t> sectionsCounter;

		/**
		 * Selected timeout value. Default is given by table definition
		 */
		time_t timeout;

	public:
		SectionFilter<T>(void);
		virtual ~SectionFilter<T>(void);

		/**
		 * The following five methods provide filtering mechanisms common to all DVB SI tables.
		 * The last four of them are only valid for tables with section_syntax_indicator = '1'.
		 */
		void setTableId(const uint8_t tableId, const uint8_t tableIdMask = 0xff, const bool positive = true);
		void setTableIdExtension(const uint16_t tableIdExtension, const bool positive = true);
		void setVersionNumber(const uint8_t versionNumber, const bool positive = true);
		void setCurrentNextIndicator(const uint8_t currentNextIndicator, const bool positive = true);
		void setSectionNumber(const uint8_t sectionNumber, const bool positive = true);

		/**
		 * Two methods to provide custom filtering ability, 8 or 16 bit wide.
		 */
		void setFilter8(const uint8_t index, const uint8_t filter, const uint8_t mask = 0xff, const bool positive = true);
		void setFilter16(const uint8_t index, const uint16_t filter, const uint16_t mask = 0xffff, const bool positive = true);

		/**
		 * Specify a custom timeout instead of the table default.
		 * @param timeout in milliseconds (zero for no timeout).
		 */
		void setTimeout(const time_t timeout);

		/**
		 * Start filtering.
		 * @param pid The packet_id for which the filter applies.
		 *            Defaults to a standard pid if available, else to an invalid pid.
		 * @return true if filtering started successfully.
		 */
		bool filter(const uint16_t pid = T::PID);

		/**
		 * Clear filter parameters and received sections.
		 * @param doRelease Close Demux file handle and remove it from SectionPool.
		 * @param doSetup Set up default filter parameters (table id, current next indicator).
		 * @param doDelete Delete stored section data.
		 */
		void clear(bool doRelease = true, bool doDelete = true, bool doSetup = true);

		/**
		 * Provide a pointer to received sections.
		 */
		const std::vector<T*> *getSections(void) const;

		/**
		 * Shall be called by programs waiting for a table to complete.
		 * @return true if section filtering has stopped.
		 */
		bool isDone(void);

		/**
		 * Shall be called by programs after waiting is complete, to
		 * verify presence of section data.
		 * @return true if section filtering timed out.
		 */
		bool isTimedOut(void);

		/**
		 * Inherited from SectionHandler.
		 * @see SectionHandler.
		 */
		void release(void);
		virtual bool section(const uint8_t * const data, const uint16_t size);
};

template <class T>
SectionFilter<T>::SectionFilter<T>(void)
{
	fd = -1;
	pool = SectionPool::getInstance();

	clear(false, false, true);
}

template <class T>
SectionFilter<T>::~SectionFilter<T>(void)
{
	clear(true, true, false);
}

template <class T>
void SectionFilter<T>::clear(bool doRelease, bool doDelete, bool doSetup)
{
	if (doRelease)
		release();

	if (doDelete) {
		for (unsigned int i = 0; i < sections.size(); i++)
			delete sections[i];

		sections.clear();
		sectionsCounter.clear();
	}

	if (doSetup) {
		done = false;
		timedOut = false;
		startTime = 0;
		memset(&dsfp.filter, 0, sizeof(struct dmx_filter));

		setTableId(T::TID);
		if (T::SYNTAX == 1)
			setCurrentNextIndicator(1);
		setTimeout(T::TIMEOUT);
	}
}

template <class T>
void SectionFilter<T>::setTableId(const uint8_t tableId, const uint8_t tableIdMask, const bool positive)
{
	dsfp.filter.filter[0] = tableId;
	dsfp.filter.mask[0] = tableIdMask;
	dsfp.filter.mode[0] = positive ? 0x00 : tableIdMask;
}

template <class T>
void SectionFilter<T>::setTableIdExtension(const uint16_t tableIdExtension, const bool positive)
{
	if (T::SYNTAX == 1) {
		dsfp.filter.filter[1] = (tableIdExtension >> 8) & 0xff;
		dsfp.filter.filter[2] = tableIdExtension & 0xff;
		dsfp.filter.mask[1] = 0xff;
		dsfp.filter.mask[2] = 0xff;
		dsfp.filter.mode[1] = positive ? 0x00 : 0xff;
		dsfp.filter.mode[2] = positive ? 0x00 : 0xff;
	}
}

template <class T>
void SectionFilter<T>::setVersionNumber(const uint8_t versionNumber, const bool positive)
{
	if (T::SYNTAX == 1) {
		dsfp.filter.filter[3] |= (versionNumber & 0x1f) << 1;
		dsfp.filter.mask[3] |= 0x3e;
		dsfp.filter.mode[3] |= positive ? 0x00 : 0x3e;
	}
}

template <class T>
void SectionFilter<T>::setCurrentNextIndicator(const uint8_t currentNextIndicator, const bool positive)
{
	if (T::SYNTAX == 1) {
		dsfp.filter.filter[3] |= currentNextIndicator & 0x01;
		dsfp.filter.mask[3] |= 0x01;
		dsfp.filter.mode[3] |= positive ? 0x00 : 0x01;
	}
}

template <class T>
void SectionFilter<T>::setSectionNumber(const uint8_t sectionNumber, const bool positive)
{
	if (T::SYNTAX == 1) {
		dsfp.filter.filter[4] = sectionNumber;
		dsfp.filter.mask[4] = 0xff;
		dsfp.filter.mode[4] = positive ? 0x00 : 0xff;
	}
}

template <class T>
void SectionFilter<T>::setFilter8(const uint8_t index, const uint8_t filter, const uint8_t mask, const bool positive)
{
	if (index >= DMX_FILTER_SIZE)
		DVB_FATAL("index >= DMX_FILTER_SIZE");

	dsfp.filter.filter[index] = filter;
	dsfp.filter.mask[index] = mask;
	dsfp.filter.mode[index] = positive ? 0x00 : mask;
}

template <class T>
void SectionFilter<T>::setFilter16(const uint8_t index, const uint16_t filter, const uint16_t mask, const bool positive)
{
	setFilter8(index, (filter >> 8) & 0xff, (mask >> 8) & 0xff, positive);
	setFilter8(index + 1, filter & 0xff, mask & 0xff, positive);
}

template <class T>
void SectionFilter<T>::setTimeout(const time_t timeout)
{
	this->timeout = timeout;
}

template <class T>
bool SectionFilter<T>::start(void)
{
	if (DVB_FOP(ioctl, DMX_SET_FILTER, &dsfp) < 0)
		return false;

	startTime = time(NULL);

	return true;
}

template <class T>
bool SectionFilter<T>::stop(void)
{
	if (DVB_FOP(ioctl, DMX_STOP) < 0)
		return false;

	return true;
}

template <class T>
void SectionFilter<T>::release(void)
{
	if (fd != -1) {
		if (!done) {
			stop();
		}
		pool->closeDemux(fd);
		fd = -1;
	}
}

template <class T>
bool SectionFilter<T>::section(const uint8_t * const data, const uint16_t size)
{
	if ((data[0] & dsfp.filter.mask[0]) != (dsfp.filter.filter[0] & dsfp.filter.mask[0]))
		DVB_FATAL("received unwanted table_id %02x (f: %02x, m: %02x)",
			data[0], dsfp.filter.filter[0], dsfp.filter.mask[0]);

	if (T::LENGTH < size)
		DVB_FATAL("T::LENGTH %hu < size %hu!", T::LENGTH, size);

	if (T::SYNTAX == 0) {
		sections.push_back(new T(data));
		done = true;
	}
	else {
		//DVB_INFO("section number %02x", data[6]);
		if (sectionsCounter[data[6]] == 0) {
			sectionsCounter[data[6]] = 1;
			sections.push_back(new T(data));
			//DVB_INFO("%d/%d sections received", sectionsCounter.size(), data[7] + 1);
			if (sectionsCounter.size() == (unsigned)(data[7] + 1)) {
				done = true;
			}
			else if (sectionsCounter.size() > (unsigned)(data[7] + 1)) {
				DVB_FATAL("sectionsCounter.size() > (data[7] + 1)");
			}
		}
		startTime = time(NULL);
	}

	return done;
}

template <class T>
bool SectionFilter<T>::filter(const uint16_t pid)
{
	done = false;
	timedOut = false;

	if (fd == -1) {
		fd = pool->openDemux();
		if (fd == -1)
			return false;
	}

	dsfp.pid = pid;
	dsfp.timeout = 0;

	if (T::SYNTAX == 0)
		dsfp.flags = DMX_ONESHOT | DMX_IMMEDIATE_START;
	else
		dsfp.flags = DMX_IMMEDIATE_START;

	if (T::CRC32)
		dsfp.flags |= DMX_CHECK_CRC;

	if (!pool->addFilter(fd, this))
		return false;

	return start();
}

template <class T>
const std::vector<T*> *SectionFilter<T>::getSections(void) const
{
	return &sections;
}

template <class T>
bool SectionFilter<T>::isDone(void)
{
	if (!done)
		isTimedOut();
	return done;
}

template <class T>
bool SectionFilter<T>::isTimedOut(void)
{
	time_t diff;

	if ((!startTime) || (!timeout))
		return false;

	if (done)
		return timedOut;

	diff = (time(NULL) - startTime - 1) * 1000;

	if (diff >= timeout) {
		DVB_INFO("diff: %ld, timeout: %ld", diff, timeout);
		timedOut = done = true;
	}

	return timedOut;
}

#endif /* __dvb_hardware_section_filter_h__ */
