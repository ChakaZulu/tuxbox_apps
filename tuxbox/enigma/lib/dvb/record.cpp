#ifndef DISABLE_FILE

#include <lib/dvb/record.h>
#include <config.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <lib/dvb/dvbservice.h>

#if HAVE_DVB_API_VERSION < 3
#include <ost/dmx.h>
#define DVR_DEV "/dev/dvb/card0/dvr1"
#define DEMUX1_DEV "/dev/dvb/card0/demux1"
#else
#include <linux/dvb/dmx.h>
#define DVR_DEV "/dev/dvb/adapter0/dvr1"
#define DEMUX1_DEV "/dev/dvb/adapter0/demux1"
#endif

int eDVBRecorder::flushBuffer()
{
	if (!bufptr)
		return 0;
	int wr = ::write(outfd, buf, bufptr);
	if ( wr < bufptr )
	{
		eDebug("recording write error, maybe disk full");
		rmessagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::rWriteError));
		return -1;
	}
	size+=wr;
	bufptr=0;
	return 0;
}

void eDVBRecorder::thread()
{
	int wr=-1;
	while (state == stateRunning)
	{
		wr=0;
//		singleLock s(bufferLock);

		int r = ::read(dvrfd, buf+bufptr, 65536-bufptr );
		if ( r < 0 )
			continue;

		bufptr += r;

		if ( bufptr > 65535 )
			if (flushBuffer())
				state = stateError;

		if (size > splitsize)
			if (openFile(++splits))
			{
				state = stateError;
				rmessagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::rWriteError));
			}
	}
}

void eDVBRecorder::PMTready(int error)
{
	eDebug("eDVBRecorder PMTready");
	if ( !error )
	{
		PMT *pmt=tPMT.ready()?tPMT.getCurrent():0;
		if ( pmt )
		{
			eDVBCaPMTClientHandler::distribute_gotPMT(recRef, pmt);

			eDebug("UpdatePIDs");
			addNewPID(0); // PAT
			addNewPID(pmt->pid);  // PMT
			addNewPID(pmt->PCR_PID);  // PCR

			for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
			{
				int record=0;
				switch (i->stream_type)
				{
					case 1:	// video..
					case 2:
						record=1;
						break;
					case 3:	// audio..
					case 4:
						record=1;
						break;
					case 6:
					for (ePtrList<Descriptor>::iterator it(i->ES_info); it != i->ES_info.end(); ++it)
					{
						if (it->Tag() == DESCR_AC3)
						{
							record=1;
							break;
						}
#ifdef RECORD_TELETEXT
						if (it->Tag() == DESCR_TELETEXT)
						{
							record=1;
							break;
						}
#endif
					}
					break;
				}
				if (record)
					addNewPID(i->elementary_PID);
			}
			validatePIDs();

			pmt->unlock();
		}
	}
}

void eDVBRecorder::gotBackMessage(const eDVBRecorderMessage &msg)
{
	switch (msg.code)
	{
	case eDVBRecorderMessage::rWriteError:
		/* emit */ recMessage(recWriteError);
		break;
	default:
		break;
	}
}

int eDVBRecorder::openFile(int suffix)
{
	eString tfilename=filename;
	if (suffix)
		tfilename+=eString().sprintf(".%03d", suffix);

	size=0;

	if (outfd >= 0)
		::close(outfd);

	::unlink(tfilename.c_str());
	outfd=::open(tfilename.c_str(), O_CREAT|O_WRONLY|O_TRUNC|O_LARGEFILE, 0555);

	if (outfd < 0)
	{
		eDebug("failed to open DVR file: %s (%m)", tfilename.c_str());	
		return -1;
	}
	return 0;
}

void eDVBRecorder::open(const char *_filename)
{
	eDebug("eDVBRecorder::open(%s)", _filename);
	pids.clear();
	newpids.clear();

	filename=_filename;

	splitsize=1024*1024*1024; // 1G

	openFile(splits=0);

	if ( dvrfd >= 0 )
		::close(dvrfd);

	dvrfd=::open(DVR_DEV, O_RDONLY);
	if (dvrfd < 0)
	{
		eDebug("failed to open "DVR_DEV" (%m)");
		::close(outfd);
		outfd=-1;
		return;
	}
	if (outfd < 0)
		rmessagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::rWriteError));
}

std::pair<std::set<eDVBRecorder::pid_t>::iterator,bool> eDVBRecorder::addPID(int pid)
{
	eDebug("eDVBRecorder::addPID(0x%x)", pid );
	pid_t p;
	p.pid=pid;
	if ( pids.find(p) != pids.end() )
	{
		eDebug("we already have this pid... skip!");
		return std::pair<std::set<pid_t>::iterator, bool>(pids.end(),false);
	}
	p.fd=::open(DEMUX1_DEV, O_RDWR);
	if (p.fd < 0)
	{
		eDebug("failed to open demux1");
		return std::pair<std::set<pid_t>::iterator, bool>(pids.end(),false);
	}
#if HAVE_DVB_API_VERSION < 3
	dmxPesFilterParams flt;
	flt.pesType=DMX_PES_OTHER;
#else
	dmx_pes_filter_params flt;
	flt.pes_type=DMX_PES_OTHER;
#endif
	flt.pid=p.pid;
	flt.input=DMX_IN_FRONTEND;
	flt.output=DMX_OUT_TS_TAP;

	flt.flags=0;

	if (::ioctl(p.fd, DMX_SET_PES_FILTER, &flt)<0)
	{
		eDebug("DMX_SET_PES_FILTER failed (for pid %d)", flt.pid);
		::close(p.fd);
		return std::pair<std::set<pid_t>::iterator, bool>(pids.end(),false);
	}

	return pids.insert(p);
}

void eDVBRecorder::addNewPID(int pid)
{
	pid_t p;
	p.pid = pid;
	newpids.insert(p);
}

void eDVBRecorder::validatePIDs()
{
	if (state != stateRunning)
		return;
	eDebug("validatePIDs");
	for (std::set<pid_t>::iterator it(pids.begin()); it != pids.end(); ++it )
	{
		std::set<pid_t>::iterator i = newpids.find(*it);
		if ( i == newpids.end() )  // no more existing pid...
		{
			removePID(it->pid);
			it = pids.begin();
		}
	}
	for (std::set<pid_t>::iterator it(newpids.begin()); it != newpids.end(); ++it )
	{
		std::pair<std::set<pid_t>::iterator,bool> newpid = addPID(it->pid);
		if ( newpid.second )
		{
			if ( state == stateRunning )
			{
				if (::ioctl(newpid.first->fd, DMX_START, 0)<0)
				{
					eDebug("DMX_START failed (%m)");
					::close(newpid.first->fd);
				}
			}
		}
		else
			eDebug("error while add new pid");
	}
	newpids.clear();
}

void eDVBRecorder::removePID(int pid)
{
	pid_t p;
	p.pid=pid;
	std::set<pid_t>::iterator pi=pids.find(p);
	if (pi != pids.end())
	{
		if (pi->fd >= 0)
			::close(pi->fd);
		pids.erase(pi);
	}
	eDebug("eDVBRecorder::removePID(0x%x)", pid);
}

void eDVBRecorder::start()
{
	if ( state == stateRunning )
		return;

	eDebug("eDVBRecorder::start()");

	state = stateRunning;

	if ( !thread_running() )
	{
		eDebug("run thread");
		run();
	}

	for (std::set<pid_t>::iterator i(pids.begin()); i != pids.end(); ++i)
	{
		printf("starting pidfilter for pid %d\n", i->pid );

		if (::ioctl(i->fd, DMX_START, 0)<0)
		{
			eDebug("DMX_START failed");
			::close(i->fd);
			continue;
		}
	}
}

void eDVBRecorder::stop()
{
	if ( state == stateStopped )
		return;

	eDebug("eDVBRecorder::stop()");
	state = stateStopped;

	int timeout=20;
	while ( thread_running() && timeout )
	{
		usleep(100000);  // 2 sec time for thread shutdown
		--timeout;
	}

	for (std::set<pid_t>::iterator i(pids.begin()); i != pids.end(); ++i)
		if (i->fd >= 0)
			::ioctl(i->fd, DMX_STOP, 0);

	flushBuffer();
}

void eDVBRecorder::close()
{
	if (state != stateStopped)
		stop();

	eDebug("eDVBRecorder::close()");

	if (outfd < 0)
		return;

	for (std::set<pid_t>::iterator i(pids.begin()); i != pids.end(); ++i)
		if (i->fd >= 0)
			::close(i->fd);

	::close(dvrfd);
	::close(outfd);

	if ( thread_running() )
		kill(true);
}

eDVBRecorder::eDVBRecorder(PMT *pmt)
:state(stateStopped), rmessagepump(eApp, 1), dvrfd(-1), outfd(-1), bufptr(0)
{
	CONNECT(rmessagepump.recv_msg, eDVBRecorder::gotBackMessage);
	rmessagepump.start();

	if (pmt)
	{
		CONNECT( tPMT.tableReady, eDVBRecorder::PMTready );
		tPMT.start((PMT*)pmt->createNext());
	}
//	pthread_mutex_init(&bufferLock, 0);
}

eDVBRecorder::~eDVBRecorder()
{
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if ( sapi && sapi->service != recRef )
		eDVBCaPMTClientHandler::distribute_leaveService(recRef);
	close();
}

void eDVBRecorder::writeSection(void *data, int pid)
{
	if ( state != stateStopped )
		return;

	__u8 *table=(__u8*)data;
	int len=(table[1]<<8)&0x1F;
	len|=table[2];

	len+=3;

	int first=1;
	int cc=0;
	
	while (len)
	{
		// generate header:
		__u8 packet[188]; // yes, malloc
		int pos=0;
		packet[pos++]=0x47;        // sync_byte
		packet[pos]=pid>>8;        // pid
		if (first)
			packet[pos]|=1<<6;       // PUSI
		pos++;
		packet[pos++]=pid&0xFF;    // pid
		packet[pos++]=cc++|0x10;   // continuity counter, adaption_field_control
		if (first)
			packet[pos++]=0;
		int tc=len;
		if (tc > (188-pos))
			tc=188-pos;
		memcpy(packet+pos, table, tc);
		len-=tc;
		pos+=tc;
		memset(packet+pos, 0xFF, 188-pos);

//		singleLock s(bufferLock);

		memcpy(buf+bufptr, packet, 188);
		bufptr += 188;

		if ( bufptr > 65535 )
			flushBuffer();

		first=0;
	}
}
#endif //DISABLE_FILE
