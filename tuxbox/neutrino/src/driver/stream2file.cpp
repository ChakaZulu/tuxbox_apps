/*
 * $Id: stream2file.cpp,v 1.13 2004/05/31 14:12:25 thegoodguy Exp $
 * 
 * streaming to file/disc
 * 
 * Copyright (C) 2004 Axel Buehning <diemade@tuxbox.org>,
 *                    thegoodguy <thegoodguy@berlios.de>
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

#include <stream2file.h>

#include <eventserver.h>
#include <neutrinoMessages.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <linux/dvb/dmx.h>

//#include <transform.h>
#define TS_SIZE 188

#include <pthread.h>
#include <signal.h>

extern "C" {
#include <gui/ringbuffer.h>
}           	

/* conversion buffer sizes */
// TS_SIZE is 188
#define IN_SIZE		(TS_SIZE * 362)

#define RINGBUFFERSIZE IN_SIZE * 20

/* demux buffer size */
#define DMX_BUFFER_SIZE (256 * 1024)

/* maximum number of pes pids */
#define MAXPIDS		64

/* devices */
#define DMXDEV	"/dev/dvb/adapter0/demux0"
#define DVRDEV	"/dev/dvb/adapter0/dvr0"

#define FILENAMEBUFFERSIZE 512

static int demuxfd[MAXPIDS];
static int dvrfd;

static unsigned char demuxfd_count = 0;

static stream2file_status_t exit_flag = STREAM2FILE_STATUS_IDLE;
static unsigned char busy_count = 0;

static pthread_t demux_thread[MAXPIDS];
static unsigned long long limit;

static char myfilename[512];

typedef struct filenames_t
{
	const char * extension;
	ringbuffer_t * ringbuffer;
};

static int sync_byte_offset(const unsigned char * buf, const unsigned int len) {

	unsigned int i;

	for (i = 0; i < len; i++)
		if (buf[i] == 0x47)
			return i;

	return -1;
}


static int setPesFilter(const unsigned short pid, const dmx_output_t dmx_output)
{
	int fd;
	struct dmx_pes_filter_params flt; 

	if ((fd = open(DMXDEV, O_RDWR)) < 0)
		return -1;

	if (ioctl(fd, DMX_SET_BUFFER_SIZE, DMX_BUFFER_SIZE) < 0)
		return -1;

	flt.pid = pid;
	flt.input = DMX_IN_FRONTEND;
	flt.output = dmx_output;
	flt.pes_type = DMX_PES_OTHER;
	flt.flags = DMX_IMMEDIATE_START;

	if (ioctl(fd, DMX_SET_PES_FILTER, &flt) < 0)
		return -1;

	return fd;
}


static void unsetPesFilter(const int fd)
{
	ioctl(fd, DMX_STOP);
	close(fd);
}


void * FileThread(void * v_arg)
{
	ringbuffer_data_t vec[2];
	size_t readsize;
	unsigned int filecount = 0;
	const unsigned long long splitsize = (limit / TS_SIZE) * TS_SIZE;
	unsigned long long remfile=0;
	int fd2 = -1;
	ringbuffer_t * ringbuf = ((struct filenames_t *)v_arg)->ringbuffer;

	while (1)
	{
		ringbuffer_get_read_vector(ringbuf, &(vec[0]));
		readsize = vec[0].len + vec[1].len;
		if (readsize)
		{
			// Do Splitting if necessary
			if (remfile == 0)
			{
				char filename[FILENAMEBUFFERSIZE];

				sprintf(filename, "%s.%3.3d.%s", myfilename, ++filecount, ((struct filenames_t *)v_arg)->extension);
				if (fd2 != -1)
					close(fd2);
				if ((fd2 = open(filename, O_WRONLY | O_CREAT | O_SYNC | O_TRUNC | O_LARGEFILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
				{
					perror("[stream2file]: error opening outfile");
					exit_flag = STREAM2FILE_STATUS_WRITE_OPEN_FAILURE;
					pthread_exit(NULL);
				}
				remfile = splitsize;
			}

			/* make sure file contains complete TS-packets and is <= splitsize */
			if ((unsigned long long)readsize > remfile)
			{ 
				readsize = remfile;

				if (vec[0].len > readsize)
					vec[0].len = readsize;

				vec[1].len = readsize - vec[0].len;
			}

			ssize_t written;

			while (1)
			{
				if ((written = write(fd2, vec[0].buf, vec[0].len)) < 0)
				{
					if (errno != EAGAIN)
					{
						exit_flag = STREAM2FILE_STATUS_WRITE_FAILURE;
						perror("[stream2file]: error in write");
						goto terminate_thread;
					}
				}
				else
				{
					ringbuffer_read_advance(ringbuf, written);
					
					if (vec[0].len == (size_t)written)
					{
						if (vec[1].len == 0)
						{
							goto all_bytes_written;
						}
						
						vec[0] = vec[1];
						vec[1].len = 0;
					}
					else
					{
						vec[0].len -= written;
						vec[0].buf += written;
					}
				}
			}

		all_bytes_written:
			fdatasync(fd2);
			
			remfile -= (unsigned long long)readsize;
		}
		else
		{
			if (exit_flag != STREAM2FILE_STATUS_RUNNING)
				goto terminate_thread;
			usleep(1000);
		}
	}
 terminate_thread:
	if (fd2 != -1)
		close (fd2);

	pthread_exit(NULL);
}

void * DMXThread(void * v_arg)
{
	pthread_t file_thread;
	struct filenames_t filename_data;
	char filename_extension[3];
	ringbuffer_data_t vec[2];
	ssize_t written;
	ssize_t todo;
	ssize_t todo2;
	unsigned char buf[TS_SIZE];
	int     offset = 0;
	ssize_t r = 0;

	ringbuffer_t * ringbuf = ringbuffer_create(RINGBUFFERSIZE);

	filename_data.ringbuffer = ringbuf;

	if (v_arg == &dvrfd)
	{
		filename_data.extension = "ts";
	}
	else
	{
		for (int i = 0; i < MAXPIDS; i++)
			if (v_arg == (&(demuxfd[i])))
				sprintf(filename_extension, "%u", i);
		filename_data.extension = filename_extension;
	}

	pthread_create(&file_thread, 0, FileThread, &filename_data);

	if (v_arg == &dvrfd)
		while (exit_flag == STREAM2FILE_STATUS_RUNNING)
		{
			r = read(*(int *)v_arg, &(buf[0]), TS_SIZE);
			if (r > 0)
			{
				offset = sync_byte_offset(&(buf[0]), r);
				if (offset != -1)
					break;
			}
		}
	else
		offset = 0;

	written = ringbuffer_write(ringbuf, (char *)&(buf[offset]), r - offset);
	// TODO: Retry
	if (written != r - offset) {
		printf("PANIC: wrote less than requested to ringbuffer, written %d, requested %d\n", written, r - offset);
		exit_flag = STREAM2FILE_STATUS_BUFFER_OVERFLOW;
	}
	todo = IN_SIZE - (r - offset);

	/* IN_SIZE > TS_SIZE => todo > 0 */

	while (exit_flag == STREAM2FILE_STATUS_RUNNING)
	{
		ringbuffer_get_write_vector(ringbuf, &(vec[0]));
		todo2 = todo - vec[0].len;
		if (todo2 < 0)
		{
			todo2 = 0;
		}
		else
		{
			if (((size_t)todo2) > vec[1].len)
			{
				printf("PANIC: not enough space in ringbuffer, available %d, needed %d\n", vec[0].len + vec[1].len, todo + todo2);
				exit_flag = STREAM2FILE_STATUS_BUFFER_OVERFLOW;
			}
			todo = vec[0].len;
		}

		while (exit_flag == STREAM2FILE_STATUS_RUNNING)
		{
			r = read(*(int *)v_arg, vec[0].buf, todo);
			
			if (r > 0)
			{
				ringbuffer_write_advance(ringbuf, r);

				if (todo == r)
				{
					if (todo2 == 0)
						goto next;

					todo = todo2;
					todo2 = 0;
					vec[0].buf = vec[1].buf;
				}
				else
				{
					vec[0].buf += r;
					todo -= r;
				}
			}
		}
	next:
		todo = IN_SIZE;
	}
	//sleep(1); // give FileThread some time to write remaining content of ringbuffer to file
	//	pthread_kill(rcst, SIGKILL);

	if (v_arg == &dvrfd)
		close(*(int *)v_arg);
	else
		unsetPesFilter(*(int *)v_arg);

	pthread_join(file_thread, NULL);

	ringbuffer_free(ringbuf);

	if (v_arg == &dvrfd)
		while (demuxfd_count > 0)
			unsetPesFilter(demuxfd[--demuxfd_count]);

	busy_count--;

	if ((v_arg == &dvrfd) || (v_arg == (&(demuxfd[0]))))
	{
		CEventServer eventServer;
		eventServer.registerEvent2(NeutrinoMessages::EVT_RECORDING_ENDED, CEventServer::INITID_NEUTRINO, "/tmp/neutrino.sock");
		eventServer.sendEvent(NeutrinoMessages::EVT_RECORDING_ENDED, CEventServer::INITID_NEUTRINO, &exit_flag, sizeof(exit_flag));
		printf("[stream2file] pthreads exit code: %u\n", exit_flag);
	}

	pthread_exit(NULL);
}


stream2file_error_msg_t start_recording(const char * const filename,
					const char * const info,
					const unsigned long long splitsize,
					const unsigned int numpids,
					const unsigned short * const pids,
					const bool write_ts)
{
	int fd;
	char buf[FILENAMEBUFFERSIZE];

	if (busy_count != 0)
	{
		if (exit_flag == STREAM2FILE_STATUS_RUNNING)
			return STREAM2FILE_BUSY;

		/* give threads a second to exit */
		sleep(1);

		puts("[stream2file] recording attempt 2");

		if (busy_count != 0)
			return STREAM2FILE_BUSY;
	}

	busy_count++;

	strcpy(myfilename, filename);

	// write stream information (should wakeup the disk from standby, too)
	sprintf(buf, "%s.xml", filename);
	if ((fd = open(buf, O_SYNC | O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) >= 0)
	{
		write(fd, info, strlen(info));
		fdatasync(fd);
		close(fd);
	}
	else
	{
		busy_count--;
		return STREAM2FILE_INVALID_DIRECTORY;
	}

	if (splitsize < TS_SIZE)
	{
		limit = 1099511627776ULL; // 1024GB, virtually no splitting
	}
	else
		limit = splitsize;

	for (unsigned int i = 0; i < numpids; i++)
	{
		if (pids[i] > 0x1fff)
		{
			busy_count--;
			return STREAM2FILE_INVALID_PID;
		}
		
		if ((demuxfd[i] = setPesFilter(pids[i], write_ts ? DMX_OUT_TS_TAP : DMX_OUT_TAP)) < 0)
		{
			for (unsigned int j = 0; j < i; j++)
				unsetPesFilter(demuxfd[j]);

			busy_count--;
			return STREAM2FILE_PES_FILTER_FAILURE;
		}
	}
	
	demuxfd_count = numpids;

	if (write_ts)
	{
		if ((dvrfd = open(DVRDEV, O_RDONLY)) < 0)
		{
			while (demuxfd_count > 0)
				unsetPesFilter(demuxfd[--demuxfd_count]);

			busy_count--;
			return STREAM2FILE_DVR_OPEN_FAILURE;
		}
		exit_flag = STREAM2FILE_STATUS_RUNNING;
		pthread_create(&demux_thread[0], 0, DMXThread, &dvrfd);
	}
	else
	{
		exit_flag = STREAM2FILE_STATUS_RUNNING;
		for (unsigned int i = 0; i < numpids; i++)
		{
			busy_count++;
			pthread_create(&demux_thread[i], 0, DMXThread, &demuxfd[i]);
		}
		busy_count--;
	}

	return STREAM2FILE_OK;
}

stream2file_error_msg_t stop_recording(void)
{
	if (exit_flag == STREAM2FILE_STATUS_RUNNING)
	{
		exit_flag = STREAM2FILE_STATUS_IDLE;
		return STREAM2FILE_OK;
	}
	else
		return STREAM2FILE_RECORDING_THREADS_FAILED;
}
