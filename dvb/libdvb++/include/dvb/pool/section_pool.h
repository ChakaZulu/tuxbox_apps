/*
 * $Id: section_pool.h,v 1.1 2003/07/17 01:07:35 obi Exp $
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

#ifndef __dvb_pool_section_pool_h__
#define __dvb_pool_section_pool_h__

#include <map>

#include <inttypes.h>

#include "section_handler.h"

/*
 * SectionPool
 *
 * uses select() to call h->section(buffer)
 * for incoming sections;
 */

typedef std::map<int, SectionHandler *> SectionPoolMap;

class SectionPool
{
protected:
	SectionPool(void);
	~SectionPool(void);

	static SectionPool *instance;
	static SectionPoolMap pool;
	static pthread_mutex_t pool_locked_mutex;
	static pthread_mutex_t pool_changed_mutex;
	static pthread_cond_t pool_changed_cond;

	// select stuff
	bool thread_running;
	pthread_t select_thread;

	// must be called whenever pool is changed
	void updateSelect(void);

	static void *select(void *);

public:
	static SectionPool *getInstance(void) {
		if (!instance)
			instance = new SectionPool();
		return instance;
	}

	static void deleteInstance(void) {
		delete instance;
		instance = NULL;
	}

	bool addFilter(const int fd, SectionHandler *h);
	bool removeFilter(const int fd);
	void clear(void);

	int openDemux(const uint8_t adapter = 0, const uint8_t demux = 0);
	int closeDemux(const int fd);
};

#endif /* __dvb_pool_section_pool_h__ */
