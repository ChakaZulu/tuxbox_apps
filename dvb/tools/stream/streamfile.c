/*
 * $Id: streamfile.c,v 1.10 2004/04/29 18:32:32 thegoodguy Exp $
 * 
 * streaming ts to file/disc
 * 
 * Copyright (C) 2004 Axel Buehning <diemade@tuxbox.org>
 *
 * based on code which is
 * Copyright (C) 2001 TripleDES
 * Copyright (C) 2000, 2001 Marcus Metzler <marcus@convergence.de>
 * Copyright (C) 2002 Andreas Oberritter <obi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <linux/dvb/dmx.h>
#include <transform.h>
#include <pthread.h>
#include <signal.h>
#include <poll.h>

#include "ringbuffer.h"

/* conversion buffer sizes */
// TS_SIZE is 188
#define IN_SIZE		(TS_SIZE * 362)

#define RINGBUFFERSIZE IN_SIZE * 20
#define MAXREADSIZE IN_SIZE
#define MINREADSIZE IN_SIZE

/* demux buffer size */
#define DMX_BUFFER_SIZE (256 * 1024)

/* maximum number of pes pids */
#define MAXPIDS		64

/* devices */
#define DMXDEV	"/dev/dvb/adapter0/demux0"
#define DVRDEV	"/dev/dvb/adapter0/dvr0"

unsigned char * buf;

static int dvrfd;
static int demuxfd[MAXPIDS];

static unsigned char demuxfd_count = 0;
static unsigned char exit_flag = 0;

static ringbuffer_t *ringbuf;
static int fd2 = -1;

static int silent = 0;
#define dprintf(fmt, args...) {if(!silent) printf( "[streamfile] " fmt, ## args);}

unsigned int limit=2;

void clean_exit(int signal);

static int sync_byte_offset (const unsigned char * buf, const unsigned int len) {

	unsigned int i;

	for (i = 0; i < len; i++)
		if (buf[i] == 0x47)
			return i;

	return -1;
}


static int setPesFilter (const unsigned short pid)
{
	int fd;
	struct dmx_pes_filter_params flt; 

	if ((fd = open(DMXDEV, O_RDWR)) < 0)
		return -1;

	if (ioctl(fd, DMX_SET_BUFFER_SIZE, DMX_BUFFER_SIZE) < 0)
		return -1;

	flt.pid = pid;
	flt.input = DMX_IN_FRONTEND;
	flt.output = DMX_OUT_TS_TAP;
	flt.pes_type = DMX_PES_OTHER;
	flt.flags = 0;

	if (ioctl(fd, DMX_SET_PES_FILTER, &flt) < 0)
		return -1;

	if (ioctl(fd, DMX_START, 0) < 0)
		return -1;

	return fd;
}


static void unsetPesFilter (int fd) {
	ioctl(fd, DMX_STOP);
	close(fd);
}


void *FileThread (void *v_arg)
{
	ringbuffer_data_t vec[2];
	size_t readsize, maxreadsize=0;
	unsigned long long filesize = 0;
	unsigned int filecount = 0;
	unsigned long long splitsize=1024*1024*1024; // 1GB
	splitsize = limit * splitsize; // (lmit)GB
	char filename[512];
	time_t timer1 = 0;
	unsigned long long filesize2 = 0;
	unsigned int bitrate = 0;

	struct pollfd pfd[1];
	
	while (!exit_flag)
	{
		ringbuffer_get_read_vector(ringbuf, &(vec[0]));
		readsize = vec[0].len + vec[1].len;
		if ( readsize ) {
			if (readsize > maxreadsize) {
				maxreadsize = readsize;
			}
			readsize = (readsize / TS_SIZE) * TS_SIZE;

			// Do Splitting if necessary
			if ((filesize + RINGBUFFERSIZE >= splitsize) || (fd2 == -1)) {
				sprintf(filename, "%s.%3.3d.ts", (char *)v_arg, ++filecount);
				if (fd2 != -1 )
					close(fd2);
				if ((fd2 = open(filename, O_WRONLY | O_CREAT | O_NONBLOCK | O_TRUNC | O_LARGEFILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
					perror("[streamfile]: opening outfile");
					pthread_exit (NULL);
				}
				pfd[0].fd = fd2;
				pfd[0].events = POLLOUT;

				filesize = 0;
				timer1 = time(NULL);
			}

			if (poll(pfd, 1, 5000)>0) {
				if (pfd[0].revents & POLLOUT) {
					ssize_t written;
					const char * buf = vec[0].buf;
					ssize_t todo = vec[0].len;
					
					/* readsize has been reduced to a multiple of TS_SIZE, hence might be smaller than vec[0].len */
					if (todo > readsize)
						todo = readsize;
					ssize_t todo2 = readsize - todo;

					do {
						if (((written = write(fd2, buf, todo)) < 0) && 
						    (errno != EAGAIN))
							perror("[streamfile]: write");	// CIFS returns EINVAL all the time :S
						else
						{
							if (todo == written)
							{
								todo = todo2;
								buf = vec[1].buf;
								todo2 = 0;
							}
							else
							{
								todo -= written;
								buf += written;
							}
							ringbuffer_read_advance(ringbuf, written);
						}
					} while (todo>0 && !exit_flag);
					fdatasync(fd2);
					filesize += (unsigned long long)readsize;
					filesize2 += (unsigned long long)readsize;
				}
			} else {
				perror ("[streamfile]: poll");	
			}

			if ((time(NULL) - timer1) > 10) {
				bitrate = (filesize2 / (time(NULL) - timer1) * 8);
				dprintf("Datarate %d bits/sec, %d Kbits/sec, max. rb used %d bytes\n"
					, (int)bitrate, (int)bitrate/1024, maxreadsize);
				filesize2 = 0;
				timer1 = time(NULL);
			}
		}
		else
			usleep(1000);
	}
	if (fd2!=-1) close (fd2);
	pthread_exit (NULL);
}


int
main (int argc, char ** argv) {

	int pid;
	int pids[MAXPIDS];
	char *fname;
	ssize_t written;
	int i;
	pthread_t rcst;
	int fd;

	if (argc < 4 ) {
		dprintf("Usage: streamfile file vpid apid [ pid3 pid4 ... ] (HEX-values without leading 0x!)\n");
		dprintf("file: filename without trailing '.ts'\n");
		return EXIT_FAILURE;
	}

	// set signal handler for clean termination
	signal(SIGTERM, clean_exit);

	buf = (unsigned char *) malloc(IN_SIZE);
	memset(buf, 0x00, IN_SIZE);

	if (buf == NULL) {
		perror("malloc buf");
		return EXIT_FAILURE;
	}

	i = 1;
	while (argv[i][0] == '-') {
		if (!strcmp(argv[i], "-s"))
			silent = 1;
		if (!strcmp(argv[i], "-l"))
			sscanf(argv[++i], "%d", &limit);
		i++;
	}
	fname = argv[i++];
	for (; i < argc; i++) {
		sscanf(argv[i], "%x", &pid);

		if (pid>0x1fff){
			printf ("invalid pid 0x%04x specified\n",pid);
			return EXIT_FAILURE;
		}
		
		pids[demuxfd_count] = pid;

		if ((demuxfd[demuxfd_count] = setPesFilter(pid)) < 0)
			break;

		dprintf("set filter for pid 0x%x\n", pid);

		demuxfd_count++;
	}

	// create and delete temp-file to wakeup the disk from standby
	sprintf(buf, "%s.tmp", fname);
	fd = open(buf, O_SYNC | O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, S_IRUSR | S_IWUSR);
	write(fd, buf, IN_SIZE);
	fdatasync(fd);
	close(fd);
	unlink(buf);

	if ((dvrfd = open(DVRDEV, O_RDONLY)) < 0) {
		free(buf);
		perror ("[streamfile]: open dvr");
		return EXIT_FAILURE;
	}

	ringbuf = ringbuffer_create (RINGBUFFERSIZE);
	pthread_create (&rcst, 0, FileThread, fname);
	
	/* write raw transport stream */
	int offset;

	ringbuffer_data_t vec[2];
	ssize_t r;
	ssize_t todo;
	ssize_t todo2;

	while (!exit_flag)
	{
		r = read(dvrfd, buf, IN_SIZE);
		if (r > 0)
		{
			offset = sync_byte_offset(buf, r);
			if (offset != -1)
				break;
		}
	}

	written = ringbuffer_write(ringbuf, buf + offset, r - offset);
	// TODO: Retry
	if (written != todo) {
		dprintf("PANIC: wrote less than requested to ringbuffer, written %d, requested %d\n", written, r - offset);
		exit_flag = 1;
	}
	todo = IN_SIZE - (r - offset);

	while (!exit_flag)
	{
		ringbuffer_get_write_vector(ringbuf, &(vec[0]));
		todo2 = todo - vec[0].len;
		if (todo2 < 0)
		{
			todo2 = 0;
		}
		else
		{
			if (todo2 > vec[1].len)
			{
				dprintf("PANIC: not enough space in ringbuffer, available %d, needed %d\n", vec[0].len + vec[1].len, todo + todo2);
				exit_flag = 1;
			}
			todo = vec[0].len;
		}

		while ((!exit_flag) && (todo))
		{
			r = read(dvrfd, vec[0].buf, todo);
			
			if (r > 0)
			{
				if (todo == r)
				{
					todo = todo2;
					todo2 = 0;
					vec[0].buf = vec[1].buf;
				}
				else
				{
					vec[0].buf += r;
					todo -= r;
				}

				ringbuffer_write_advance(ringbuf, r);
			}
		}

		todo = IN_SIZE;
	}
	exit_flag = 1;
	//sleep(1); // give FileThread some time to write remaining content of ringbuffer to file
	//	pthread_kill(rcst, SIGKILL);

	while (demuxfd_count > 0)
		unsetPesFilter(demuxfd[--demuxfd_count]);

	close(dvrfd);

	pthread_join(rcst,NULL);
	
	free(buf);

	ringbuffer_free(ringbuf);

	dprintf("End of main(). All filters are unset now.\n");
	return EXIT_SUCCESS;
}

void clean_exit(int signal)
{
	dprintf("Received signal. Terminating.\n");
	exit_flag = 1;
}
