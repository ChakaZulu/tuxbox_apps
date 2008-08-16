/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/daemons/sectionsd/dmx.cpp,v 1.38 2008/08/16 19:23:18 seife Exp $
 *
 * DMX class (sectionsd) - d-box2 linux project
 *
 * (C) 2001 by fnbrd,
 *     2003 by thegoodguy <thegoodguy@berlios.de>
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


#include <dmx.h>
#include <dmxapi.h>
#include <debug.h>

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <string>
#include <map>

/**/
#define PAUSE_EQUALS_STOP 1
/**/
/*
#define DEBUG_MUTEX 1
#define DEBUG_CACHED_SECTIONS 1
*/

typedef std::map<sections_id_t, version_number_t, std::less<sections_id_t> > MyDMXOrderUniqueKey;
static MyDMXOrderUniqueKey myDMXOrderUniqueKey;

extern void showProfiling(std::string text);
extern bool timeset;


DMX::DMX(const unsigned short p, const unsigned short bufferSizeInKB, const bool c)
{
	fd = -1;
	lastChanged = time(NULL);
	filter_index = 0;
	pID = p;
	dmxBufferSizeInKB = bufferSizeInKB;
	pthread_mutex_init(&pauselock, NULL);        // default = fast mutex
#ifdef DEBUG_MUTEX
	pthread_mutexattr_t start_stop_mutex_attr;
	pthread_mutexattr_init(&start_stop_mutex_attr);
	pthread_mutexattr_settype(&start_stop_mutex_attr, PTHREAD_MUTEX_ERRORCHECK_NP);
	pthread_mutex_init(&start_stop_mutex, &start_stop_mutex_attr);
#else
	pthread_mutex_init(&start_stop_mutex, NULL); // default = fast mutex
#endif
	pthread_cond_init (&change_cond, NULL);
	pauseCounter = 0;
	real_pauseCounter = 0;
	current_service = 0;

	first_skipped = 0;
	cache = c;
}

DMX::~DMX()
{
	first_skipped = 0;
	myDMXOrderUniqueKey.clear();
	pthread_mutex_destroy(&pauselock);
	pthread_mutex_destroy(&start_stop_mutex);
	pthread_cond_destroy (&change_cond);
	closefd();
}

ssize_t DMX::read(char * const buf, const size_t buflength, const unsigned timeoutMInSeconds)
{
	return readNbytes(fd, buf, buflength, timeoutMInSeconds);
}

void DMX::closefd(void)
{
	if (isOpen())
	{
		close(fd);
		fd = -1;
	}
}

void DMX::addfilter(const unsigned char filter, const unsigned char mask)
{
	s_filters tmp;
	tmp.filter = filter;
	tmp.mask   = mask;
	filters.push_back(tmp);
}

int DMX::immediate_stop(void)
{
	if (!isOpen())
		return 1;
	
	if (real_pauseCounter == 0)
		closefd();
	
	return 0;
}

int DMX::stop(void)
{
	int rc;
	
	lock();
	
	rc = immediate_stop();
	
	unlock();
	
	return rc;
}

void DMX::lock(void)
{
	//dprintf("DMX::lock, thread %lu\n", pthread_self());
#ifdef DEBUG_MUTEX
	int rc = pthread_mutex_lock(&start_stop_mutex);
	if (rc != 0)
	{
		fprintf(stderr, "[sectionsd] mutex_lock: %d %d %d\n", rc, EINVAL, EDEADLK); fflush(stderr);
		fprintf(stderr, "[sectionsd] pid: %d\n", getpid()); fflush(stderr);
	}
#else
	pthread_mutex_lock(&start_stop_mutex);
#endif
}

void DMX::unlock(void)
{
	//dprintf("DMX::unlock, thread %lu\n", pthread_self());
#ifdef DEBUG_MUTEX
	int rc = pthread_mutex_unlock(&start_stop_mutex);
	if (rc != 0)
	{
		fprintf(stderr, "[sectionsd] mutex_unlock: %d %d %d\n", rc, EINVAL, EPERM); fflush(stderr);
		fprintf(stderr, "[sectionsd] pid: %d\n", getpid()); fflush(stderr);
	}
#else
	pthread_mutex_unlock(&start_stop_mutex);
#endif
	sched_yield();
}

sections_id_t DMX::create_sections_id(const unsigned char table_id, const unsigned short extension_id, const unsigned char section_number, const unsigned short onid, const unsigned short tsid)
{
	return 	(sections_id_t) (	((sections_id_t) table_id 	<< 56) |
					((sections_id_t) extension_id 	<< 40) |
					((sections_id_t) section_number << 32) |
					((sections_id_t) onid		<< 16) |
					((sections_id_t) tsid));
}

bool DMX::check_complete(const unsigned char table_id, const unsigned short extension_id, const unsigned short onid, const unsigned short tsid, const unsigned char last)
{
	int current_section_number = 0;

	if (((table_id == 0x4e) || (table_id == 0x50)) && (current_service == extension_id)) {
		if (last == 0)
			return true;
		MyDMXOrderUniqueKey::iterator di = myDMXOrderUniqueKey.find(create_sections_id(
							table_id,
							extension_id,
							current_section_number,
							onid,
							tsid));
		if (di != myDMXOrderUniqueKey.end()) {
			di++;
		}
		while ((di != myDMXOrderUniqueKey.end()) && ((uint8_t) ((di->first >> 56) & 0xff) == table_id) &&
			((uint16_t) ((di->first >> 40) & 0xffff) == extension_id) &&
			(((uint8_t) ((di->first >> 32) & 0xff) == current_section_number + 1) || 
			((uint8_t) ((di->first >> 32) & 0xff) == current_section_number + 8)) &&
			((uint16_t) ((di->first >> 16) & 0xffff) == onid) &&
			((uint16_t) (di->first & 0xffff) == tsid))
		{
			if ((uint8_t) ((di->first >> 32) & 0xff) == last) {
				return true;
			}
			else {
				current_section_number = (uint8_t) (di->first >> 32) & 0xff;
				di++;
			}
		}
	}
	return false;
}

char * DMX::getSection(const unsigned timeoutInMSeconds, int &timeouts)
{
	struct minimal_section_header {
		unsigned table_id                 : 8;
#if __BYTE_ORDER == __BIG_ENDIAN
		unsigned section_syntax_indicator : 1;
		unsigned reserved_future_use      : 1;
		unsigned reserved1                : 2;
		unsigned section_length_hi        : 4;
#else
		unsigned section_length_hi        : 4;
		unsigned reserved1                : 2;
		unsigned reserved_future_use      : 1;
		unsigned section_syntax_indicator : 1;
#endif
		unsigned section_length_lo        : 8;
	} __attribute__ ((packed));  // 3 bytes total
	
	struct extended_section_header {
		unsigned table_extension_id_hi    : 8;
		unsigned table_extension_id_lo    : 8;
#if __BYTE_ORDER == __BIG_ENDIAN
		unsigned reserved		  : 2;
		unsigned version_number		  : 5;
		unsigned current_next_indicator	  : 1;
#else
		unsigned current_next_indicator	  : 1;
		unsigned version_number		  : 5;
		unsigned reserved		  : 2;
#endif
		unsigned section_number		  : 8;
		unsigned last_section_number	  : 8;
	} __attribute__ ((packed));  // 5 bytes total

	struct eit_extended_section_header {
		unsigned transport_stream_id_hi	  : 8;
		unsigned transport_stream_id_lo	  : 8;
		unsigned original_network_id_hi   : 8;
		unsigned original_network_id_lo   : 8;
		unsigned segment_last_section_number : 8;
		unsigned last_table_id		  : 8;
	} __attribute__ ((packed));  // 6 bytes total

	minimal_section_header initial_header;
	extended_section_header *extended_header;
	eit_extended_section_header *eit_extended_header;
	char * buf;
	int    rc;
	unsigned short section_length;
	unsigned short current_onid = 0;
	unsigned short current_tsid = 0;

	/* filter == 0 && maks == 0 => EIT dummy filter to slow down EIT thread startup */
	if (pID == 0x12 && filters[filter_index].filter == 0 && filters[filter_index].mask == 0)
	{
		//dprintf("dmx: dummy filter, sleeping for %d ms\n", timeoutInMSeconds);
		usleep(timeoutInMSeconds * 1000);
		timeouts++;
		return NULL;
	}
		
	lock();
	
	rc = read((char *) &initial_header, 3, timeoutInMSeconds);
	
	if (rc != 3)
	//	if (rc <= 0)
	{
		unlock();
		if (rc == 0)
		{
			dprintf("dmx.read timeout - filter: %x - timeout# %d\n", filters[filter_index].filter, timeouts);
			timeouts++;
		}
		else
		{
			dprintf("dmx.read rc: %d - filter: %x\n", rc, filters[filter_index].filter);
			// restart DMX
			real_pause();
			real_unpause();
		}
		return NULL;
	}

	section_length = (initial_header.section_length_hi * 256) | initial_header.section_length_lo;
	
	timeouts = 0;
	buf = new char[section_length + 3];
	
	if (!buf)
	{
		unlock();
		closefd();
		fprintf(stderr, "[sectionsd] FATAL: Not enough memory: filter: %x\n", filters[filter_index].filter);
		throw std::bad_alloc();
		return NULL;
	}
	
	if (section_length > 0)
		rc = read(buf + 3, section_length, timeoutInMSeconds);
	
	//	if (rc <= 0)
	if (rc != section_length)
	{
		unlock();
		delete[] buf;
		if (rc == 0)
		{
			dprintf("dmx.read timeout after header - filter: %x\n", filters[filter_index].filter);
		}
		else
		{
			dprintf("dmx.read rc: %d after header - filter: %x\n", rc, filters[filter_index].filter);
		}
		// DMX restart required, since part of the header has been read
		real_pause();
		real_unpause();
		return NULL;
	}
	
	// check if the filter worked correctly
	if (((initial_header.table_id ^ filters[filter_index].filter) & filters[filter_index].mask) != 0)
	{
		printf("[sectionsd] filter 0x%x mask 0x%x -> skip sections for table 0x%x\n", filters[filter_index].filter, filters[filter_index].mask, initial_header.table_id);
		unlock();
		delete[] buf;
		real_pause();
		real_unpause();
		return NULL;
	}

	unlock();	
	// skip sections which are too short
	if ((section_length < 5) || ((initial_header.table_id >= 0x4e) && (initial_header.table_id <= 0x6f) && 
		(section_length < 14)))
	{
		delete[] buf;
		return NULL;
	}

	// check if it's extended syntax, e.g. NIT, BAT, SDT, EIT
	if (initial_header.section_syntax_indicator != 0)
	{
		extended_header = (extended_section_header *)(buf+3); 
		
		// only current sections 
		if (extended_header->current_next_indicator != 0) {
			// if ((initial_header.table_id >= 0x4e) && (initial_header.table_id <= 0x6f))
			if (pID == 0x12) {
				eit_extended_header = (eit_extended_section_header *)(buf+8);
				current_onid = 	eit_extended_header->original_network_id_hi * 256 +
						eit_extended_header->original_network_id_lo;
				current_tsid = 	eit_extended_header->transport_stream_id_hi * 256 +
						eit_extended_header->transport_stream_id_lo;
			}
			else {
				current_onid = 0;
				current_tsid = 0;
			}

			int eh_tbl_extension_id = extended_header->table_extension_id_hi * 256
						  + extended_header->table_extension_id_lo;

			if (initial_header.table_id == 0x4e &&
			    eh_tbl_extension_id == current_service &&
			    extended_header->version_number != eit_version) {
				dprintf("EIT old: %d new version: %d\n",eit_version,extended_header->version_number);
				eit_version = extended_header->version_number;
			}

			/* if we are not caching the already read sections (CN-thread), then get out now */
			if (!cache)
				goto out;

			//find current section in list
			MyDMXOrderUniqueKey::iterator di = myDMXOrderUniqueKey.find(create_sections_id(
							initial_header.table_id,
							eh_tbl_extension_id,
							extended_header->section_number,
							current_onid,
							current_tsid));
			if (di != myDMXOrderUniqueKey.end())
			{
				//the current section was read before
				if (di->second == extended_header->version_number) {
#ifdef DEBUG_CACHED_SECTIONS
					dprintf("[sectionsd] skipped duplicate section for table 0x%02x table_extension 0x%04x section 0x%02x\n",
								initial_header.table_id,
								eh_tbl_extension_id,
								extended_header->section_number);
#endif
					//the version number is still up2date
					if (first_skipped == 0) {
						//the last section was new - this is the 1st dup
						first_skipped = create_sections_id(
								initial_header.table_id,
								eh_tbl_extension_id,
								extended_header->section_number,
								current_onid,
								current_tsid);
					}
					else {
						//this is not the 1st new - check if it's the last
						//or to be more precise only dups occured since
						if (first_skipped == create_sections_id(
								initial_header.table_id,
								eh_tbl_extension_id,
								extended_header->section_number,
								current_onid,
								current_tsid))
							timeouts = -1;
					}
					//since version is still up2date, check if table complete
					if (check_complete(initial_header.table_id,
						eh_tbl_extension_id,
						current_onid,
						current_tsid,
						extended_header->last_section_number))
						timeouts = -2;
					delete[] buf;
					return NULL;
				}
				else {
#ifdef DEBUG_CACHED_SECTIONS
					dprintf("[sectionsd] version update from 0x%02x to 0x%02x for table 0x%02x table_extension 0x%04x section 0x%02x\n",
								di->second,
								extended_header->version_number,
								initial_header.table_id,
								eh_tbl_extension_id,
								extended_header->section_number);
#endif
					//update version number
					di->second = extended_header->version_number;
				}
			}
			else
			{
#ifdef DEBUG_CACHED_SECTIONS
					dprintf("[sectionsd] new section for table 0x%02x table_extension 0x%04x section 0x%02x\n",
								initial_header.table_id,
								eh_tbl_extension_id,
								extended_header->section_number);
#endif
				//section was not read before - insert in list
				myDMXOrderUniqueKey.insert(std::make_pair(create_sections_id(
								initial_header.table_id,
								eh_tbl_extension_id,
								extended_header->section_number,
								current_onid,
								current_tsid),
								extended_header->version_number));
				//check if table is now complete
				if (check_complete(initial_header.table_id,
							eh_tbl_extension_id,
							current_onid,
							current_tsid,
							extended_header->last_section_number))
					timeouts = -2;
			}
			//if control comes to here the sections skipped counter must be restarted
			first_skipped = 0;
		}
	}
	
 out:
	memcpy(buf, &initial_header, 3);
	
	return buf;
}

int DMX::immediate_start(void)
{
	if (isOpen())
		return 1;

	if (real_pauseCounter != 0) {
		dprintf("DMX::immediate_start: realPausecounter !=0 !\n");
		return 0;
	}

	if ((fd = open(DEMUX_DEVICE, O_RDWR|O_NONBLOCK)) == -1)
	{
		perror("[sectionsd] open dmx");
		return 2;
	}

	if (ioctl(fd, DMX_SET_BUFFER_SIZE, (unsigned long)(dmxBufferSizeInKB*1024UL)) == -1)
	{
		closefd();
		perror("[sectionsd] DMX: DMX_SET_BUFFER_SIZE");
		return 3;
	}

	/* setfilter() only if this is no dummy filter... */
	if (filters[filter_index].filter && filters[filter_index].mask &&
	    !setfilter(fd, pID, filters[filter_index].filter, filters[filter_index].mask, DMX_IMMEDIATE_START | DMX_CHECK_CRC))
	{
		closefd();
		return 4;
	}
	return 0;
}

int DMX::start(void)
{
	int rc;
	
	lock();

	rc = immediate_start();

	unlock();

	return rc;
}

int DMX::real_pause(void)
{
	if (!isOpen()) {
		dprintf("DMX::real_pause: (!isOpen())\n");
		return 1;
	}

	lock();

	if (real_pauseCounter == 0)
	{
#ifdef PAUSE_EQUALS_STOP
		immediate_stop();
#else
		if (ioctl(fd, DMX_STOP, 0) == -1)
		{
			closefd();
			perror("[sectionsd] DMX: DMX_STOP");
			unlock();
			return 2;
		}
#endif
	}
	//else
	//	dprintf("real_pause: counter %d\n", real_pauseCounter);

	unlock();

	return 0;
}

int DMX::real_unpause(void)
{
#ifndef PAUSE_EQUALS_STOP
	if (!isOpen())
		return 1;
#endif

	lock();

	if (real_pauseCounter == 0)
	{
#ifdef PAUSE_EQUALS_STOP
		immediate_start();
#else
		if (ioctl(fd, DMX_START, 0) == -1)
		{
			closefd();
			perror("[sectionsd] DMX: DMX_START");
			unlock();
			return 2;
		}
#endif
		//dprintf("real_unpause DONE: %d\n", real_pauseCounter);
	}
	//else
	//	dprintf("real_unpause NOT DONE: %d\n", real_pauseCounter);

	unlock();

	return 0;
}

int DMX::request_pause(void)
{
	real_pause(); // unlocked

	lock();
	//dprintf("request_pause: %d\n", real_pauseCounter);
	
	real_pauseCounter++;
	
	unlock();
	
	return 0;
}


int DMX::request_unpause(void)
{
	lock();

	//dprintf("request_unpause: %d\n", real_pauseCounter);
	--real_pauseCounter;

	unlock();

	real_unpause(); // unlocked

	return 0;
}


int DMX::pause(void)
{
#ifndef PAUSE_EQUALS_STOP
	if (!isOpen())
		return 1;
#endif

	pthread_mutex_lock(&pauselock);

	//dprintf("lock from pc: %d\n", pauseCounter);
	pauseCounter++;

	pthread_mutex_unlock(&pauselock);

	return 0;
}

int DMX::unpause(void)
{
#ifndef PAUSE_EQUALS_STOP
	if (!isOpen())
		return 1;
#endif

	pthread_mutex_lock(&pauselock);

	//dprintf("unlock from pc: %d\n", pauseCounter);
	--pauseCounter;

	pthread_mutex_unlock(&pauselock);

	return 0;
}

const char *dmx_filter_types [] = {
			"dummy filter",
			"other transport stream, now/next",
			"actual transport stream, scheduled",
			"actual transport stream, scheduled2",
//			"other transport stream, scheduled"
			"other transport stream, scheduled 1/2",
			"other transport stream, scheduled 2/2"
};

int DMX::change(const int new_filter_index)
{
	if (debug)
		showProfiling("changeDMX: before pthread_mutex_lock(&start_stop_mutex)");
        lock();

	if (debug)
		showProfiling("changeDMX: after pthread_mutex_lock(&start_stop_mutex)");

	filter_index = new_filter_index;
	first_skipped = 0;

#if 0
/* i have to think about this. This #if 0 now makes .change() automatically unpause the
 * demux.  No idea if there are negative side effects - we will find out :)  -- seife
 */
	if (!isOpen())
	{
#ifdef PAUSE_EQUALS_STOP
		pthread_cond_signal(&change_cond);
#endif
		unlock();
		dprintf("DMX::change(%d): not open!\n",new_filter_index);
		return 1;
	}
#endif

	if (real_pauseCounter > 0)
	{
		printf("changeDMX: for 0x%x not ignored! even though real_pauseCounter> 0\n",
			filters[new_filter_index].filter);
		/* immediate_start() checks for real_pauseCounter again (and
		   does nothing in that case), so we can just continue here. */
	}

	if (debug) { // friendly debug output...
		if(pID==0x12 && filters[0].filter != 0x4e) { // Only EIT
			printdate_ms(stderr);
			fprintf(stderr, "changeDMX [EIT]-> %d (0x%x/0x%x) %s (%ld seconds)\n",
				new_filter_index, filters[new_filter_index].filter,
				filters[new_filter_index].mask, dmx_filter_types[new_filter_index],
				time(NULL)-lastChanged);
		} else {
			printdate_ms(stderr);
			fprintf(stderr, "changeDMX [%x]-> %d (0x%x/0x%x) (%ld seconds)\n", pID,
				new_filter_index, filters[new_filter_index].filter,
				filters[new_filter_index].mask, time(NULL)-lastChanged);
		}
	}

	closefd();

	int rc = immediate_start();

	if (rc != 0)
	{
		unlock();
		return rc;
	}

	if (debug)
		showProfiling("after DMX_SET_FILTER");

        pthread_cond_signal(&change_cond);

	if (timeset)
		lastChanged = time(NULL);

	unlock();

	return 0;
}

// Liest n Bytes aus einem Socket per read
// Liefert 0 bei timeout
// und -1 bei Fehler
// ansonsten die Anzahl gelesener Bytes
/* inline */
ssize_t DMX::readNbytes(int fd, char * buf, const size_t n, unsigned timeoutInMSeconds)
{
	size_t j;

	for (j = 0; j < n;)
	{
		struct pollfd ufds;
		ufds.fd = fd;
		ufds.events = POLLIN;
		ufds.revents = 0;
		int rc = ::poll(&ufds, 1, timeoutInMSeconds);

		if (!rc)
			return 0; // timeout
		else if (rc < 0 && errno == EINTR)
			continue; // interuppted
		else if (rc < 0)
		{
			perror ("[sectionsd] DMX::readNbytes poll");
			//printf("errno: %d\n", errno);
			return -1;
		}
#ifdef PAUSE_EQUALS_STOP
		if ((ufds.revents & POLLERR) != 0) /* POLLERR means buffer error, i.e. buffer overflow */
		{
			printdate_ms(stderr);
			fprintf(stderr, "[sectionsd] DMX::readNbytes received POLLERR, pid 0x%x, filter[%d] "
			       "filter 0x%02x mask 0x%02x\n", pID, filter_index,
			       filters[filter_index].filter, filters[filter_index].mask);
			return -1;
		}
#endif
		if (!(ufds.revents&POLLIN))
		{
			// POLLHUP, beim dmx bedeutet das DMXDEV_STATE_TIMEDOUT
			// kommt wenn ein Timeout im Filter gesetzt wurde
			// dprintf("revents: 0x%hx\n", ufds.revents);

			usleep(100*1000UL); // wir warten 100 Millisekunden bevor wir es nochmal probieren

			if (timeoutInMSeconds <= 200000)
				return 0; // timeout

			timeoutInMSeconds -= 200000;

			continue;
		}

		int r = ::read(fd, buf, n - j);

		if (r > 0)
		{
			j += r;
			buf += r;
		}
		else if (r <= 0 && errno != EINTR)
		{
			perror ("[sectionsd] DMX::readNbytes read");
			return -1;
		}
	}

	return j;
}



int DMX::setPid(const unsigned short new_pid)
{
        lock();

	if (!isOpen())
	{
#ifdef PAUSE_EQUALS_STOP
		pthread_cond_signal(&change_cond);
#endif
		unlock();
		return 1;
	}

	if (real_pauseCounter > 0)
	{
		dprintf("changeDMX: for 0x%x ignored! because of real_pauseCounter> 0\n", new_pid);
		unlock();
		return 0;	// not running (e.g. streaming)
	}
	closefd();

	pID = new_pid;
	int rc = immediate_start();

	if (rc != 0)
	{
		unlock();
		return rc;
	}

        pthread_cond_signal(&change_cond);

	if (timeset)
		lastChanged = time(NULL);

	unlock();

	return 0;
}

int DMX::setCurrentService(int new_current_service)
{
	lock();

	/* there might be occurences of setCurrentService with the devcice stopped */
	current_service = new_current_service;

	if (!isOpen())
	{
#ifdef PAUSE_EQUALS_STOP
		pthread_cond_signal(&change_cond);
#endif
		dprintf("DMX::setCurrentService(0x%x) not open!\n",new_current_service);
		unlock();
		return 1;
	}

	if (real_pauseCounter > 0)
	{
		/*d*/printf("currentDMX: for 0x%x ignored! because of real_pauseCounter> 0\n", new_current_service);
		unlock();
		return 0;	// not running (e.g. streaming)
	}
	closefd();

	int rc = immediate_start();

	if (rc != 0)
	{
		unlock();
		return rc;
	}

        pthread_cond_signal(&change_cond);

	unlock();

	return 0;
}

int DMX::dropCachedSectionIDs()
{
        lock();

/* i think that those checks are wrong for dropCachedSectionIDs(), since
   this is called from the houskeeping thread while sectionsd might be
   idle waiting for the EIT update filter to trigger -- seife
 */
#if 0
	if (!isOpen())
	{
#ifdef PAUSE_EQUALS_STOP
		pthread_cond_signal(&change_cond);
#endif
		unlock();
		return 1;
	}

	if (real_pauseCounter > 0)
	{
		unlock();
		return 0;	// not running (e.g. streaming)
	}
	closefd();
#endif

	myDMXOrderUniqueKey.clear();

#if 0
	int rc = immediate_start();

	if (rc != 0)
	{
		unlock();
		return rc;
	}
#endif

        pthread_cond_signal(&change_cond);

	unlock();

	return 0;
}

void DMX::reset_eit_version()
{
	eit_version = 0xff;
}

unsigned char DMX::get_eit_version()
{
	return eit_version;
}

unsigned int DMX::get_current_service()
{
	return current_service;
}
