/*
 * $Id: section_pool.cpp,v 1.3 2005/11/03 08:12:17 mws Exp $
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

#include <cerrno>
#include <vector>

#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>

#include <dvb/byte_stream.h>
#include <dvb/debug/debug.h>
#include <dvb/pool/section_pool.h>

SectionPool *SectionPool::instance = NULL;
SectionPoolMap SectionPool::pool;
pthread_mutex_t SectionPool::pool_locked_mutex;
pthread_mutex_t SectionPool::pool_changed_mutex;
pthread_cond_t SectionPool::pool_changed_cond;

/*
 * SectionPool
 *
 * uses select() to call h->section(buffer)
 * for incoming sections;
 */

SectionPool::SectionPool(void)
{
	pthread_mutex_init(&pool_locked_mutex, NULL);
	pthread_mutex_init(&pool_changed_mutex, NULL);
	pthread_cond_init(&pool_changed_cond, NULL);
	pthread_create(&select_thread, NULL, select, NULL);
	pthread_detach(select_thread);
}

SectionPool::~SectionPool(void)
{
	pthread_cancel(select_thread);
	pthread_mutex_destroy(&pool_locked_mutex);
	pthread_mutex_destroy(&pool_changed_mutex);
	pthread_cond_destroy(&pool_changed_cond);
}

void *SectionPool::select(void *)
{
	fd_set rfds;
	//fd_set efds;
	int highfd;
	unsigned char buffer[4096];
	int ret;

	struct timeval tv = {
		tv_sec: 0,
		tv_usec: 0
	};

	while (1) {
		FD_ZERO(&rfds);

		highfd = -1;

		// lock while iterating
		pthread_mutex_lock(&pool_locked_mutex);

		for (SectionPoolMap::const_iterator i = pool.begin(); i != pool.end(); ++i) {
			FD_SET(i->first, &rfds);
			if (i->first > highfd)
				highfd = i->first;
		}

		pthread_mutex_unlock(&pool_locked_mutex);

		if (highfd == -1) {
			pthread_mutex_unlock(&pool_locked_mutex);
			pthread_cond_wait(&pool_changed_cond, &pool_changed_mutex);
			continue;
		}

		ret = ::select(highfd + 1, &rfds, NULL, NULL, &tv);

		if (ret == -1) {
			if (errno == EINTR) {
				DVB_INFO("received EINTR - shutting down sections thread");
				break;
			}
			DVB_ERROR("select");
			continue;
		}

		if (ret > 0) {
			// lock again while iterating.
			// note that riscs caused by not locking while
			// select() is waiting should be thought of.
			pthread_mutex_lock(&pool_locked_mutex);
			for (SectionPoolMap::iterator i = pool.begin(); i != pool.end(); /* inc done within loop */ ) {
				int fd = i->first;
				if (FD_ISSET(fd, &rfds)) {
					ssize_t size;
					if ((size = DVB_FOP(read, buffer, sizeof(buffer))) == -1) {
						++i;
						continue;
					}
					else if ((size < 8) || (size != DVB_LENGTH(&buffer[1]) + 3)) {
						DVB_INFO("incomplete section (size == %d, section_length + 3 == %d)",
							size, (size < 3) ? -1 : DVB_LENGTH(&buffer[1]) + 3);
					}
					else if (i->second->section(buffer, size) == true) {
							i->second->release();
							pool.erase(i++); // remove finished table fd
							continue;
					}
				}
				++i;
			}
			pthread_mutex_unlock(&pool_locked_mutex);
		}
	}

	pthread_exit(NULL);
}

bool SectionPool::addFilter(const int fd, SectionHandler *h)
{
	if ((fd < 0) || (h == NULL))
		return false;

	pthread_mutex_lock(&pool_locked_mutex);
	pool[fd] = h;
	pthread_mutex_unlock(&pool_locked_mutex);
	pthread_cond_signal(&pool_changed_cond);

	return true;
}

bool SectionPool::removeFilter(const int fd)
{
	if (fd < 0)
		return false;

	pthread_mutex_lock(&pool_locked_mutex);
	size_t ret = pool.erase(fd);
	pthread_mutex_unlock(&pool_locked_mutex);

	return (ret != 0);
}

void SectionPool::clear(void)
{
	DVB_FATAL("not implemented");
}

int SectionPool::openDemux(const uint8_t adapter, const uint8_t demux)
{
	char filename[32];
	snprintf(filename, 32, "/dev/dvb/adapter%hu/demux%hu", adapter, demux);
	int dmx = open(filename, O_RDWR);
	if (dmx == -1)
		DVB_ERROR(filename);
	return dmx;
}

int SectionPool::closeDemux(const int fd)
{
	if (fd < 0)
		return fd;
	if (removeFilter(fd))
		DVB_INFO("removed stale file descriptor from pool");
	return close(fd);
}

