#ifndef DISABLE_FILE

#include <lib/dvb/record.h>
#include <config.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#if HAVE_DVB_API_VERSION < 3
#include <ost/dmx.h>
#define DVR_DEV "/dev/dvb/card0/dvr1"
#define DEMUX1_DEV "/dev/dvb/card0/demux1"
#else
#include <linux/dvb/dmx.h>
#define DVR_DEV "/dev/dvb/adapter0/dvr1"
#define DEMUX1_DEV "/dev/dvb/adapter0/demux1"
#endif

void eDVBRecorder::dataAvailable(int)
{
	int res = 0;

	int r = buffer.fromfile(dvrfd,32768*8);
	if (r<=0)
	{
		eDebug("reading failed..(err %d)", -r);
		return;
	}

	while ( buffer.size() > 32767 )
	{
		res=buffer.tofile(outfd, 32768);

		if (res <= 0 && buffer.size() > 32767 )
		{
			eDebug("recording write error, maybe disk full");
//		s_close();
			rmessagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::rWriteError));
			return;
		}
		size+=res;
	}
	if (size > splitsize)
		openFile(++splits);
}

void eDVBRecorder::thread()
{
	messagepump.start();
	enter_loop();
	lock.unlock();
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

void eDVBRecorder::gotMessage(const eDVBRecorderMessage &msg)
{
	switch (msg.code)
	{
	case eDVBRecorderMessage::mOpen:
		s_open(msg.filename);
		break;
	case eDVBRecorderMessage::mAddPID:
		s_addPID(msg.pid);
		break;
	case eDVBRecorderMessage::mRemovePID:
		s_removePID(msg.pid);
		break;
	case eDVBRecorderMessage::mRemoveAllPIDs:
		s_removeAllPIDs();
		break;
	case eDVBRecorderMessage::mClose:
		s_close();
		break;
	case eDVBRecorderMessage::mStart:
		s_start();
		break;
	case eDVBRecorderMessage::mStop:
		s_stop();
		break;
	case eDVBRecorderMessage::mExit:
		s_exit();
		break;
	case eDVBRecorderMessage::mWrite:
		::write(outfd, msg.write.data, msg.write.len);
		::free(msg.write.data);
		break;
	case eDVBRecorderMessage::mAddNewPID:
	{
		pid_t p;
		p.pid = msg.pid;
		newpids.insert(p);
		break;
	}
	case eDVBRecorderMessage::mValidatePIDs:
		s_validatePIDs();
		break;
	default:
		eDebug("received unknown message!");
	}
}

void eDVBRecorder::openFile(int suffix)
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
		eDebug("failed to open DVR file: %s (%m)", tfilename.c_str());	
}

void eDVBRecorder::s_open(const char *_filename)
{
	eDebug("eDVBRecorder::s_open(%s)", _filename);
	pids.clear();

	filename=eString(_filename);
	delete[] _filename;
	
	splitsize=1024*1024*1024; // 1G
	outfd=-1;
	openFile(splits=0);

	dvrfd=::open(DVR_DEV, O_RDONLY|O_NONBLOCK);
	if (dvrfd < 0)
	{
		eDebug("failed to open "DVR_DEV" (%m)");
		::close(outfd);
		outfd=-1;
		return;
	}
	if (outfd >= 0)
	{
		sn=new eSocketNotifier(this, dvrfd, eSocketNotifier::Read);
		CONNECT(sn->activated, eDVBRecorder::dataAvailable);
		sn->start();
	} else
		rmessagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::rWriteError));
}

std::pair<std::set<eDVBRecorder::pid_t>::iterator,bool> eDVBRecorder::s_addPID(int pid)
{
	eDebug("eDVBRecorder::s_addPID(0x%x)", pid );
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

void eDVBRecorder::s_validatePIDs()
{
	eDebug("s_validatePIDs");
	for (std::set<pid_t>::iterator it(pids.begin()); it != pids.end(); ++it )
	{
		std::set<pid_t>::iterator i = newpids.find(*it);
		if ( i == newpids.end() )  // no more existing pid...
		{
			s_removePID(it->pid);
			it = pids.begin();
		}
	}
	for (std::set<pid_t>::iterator it(newpids.begin()); it != newpids.end(); ++it )
	{
		std::pair<std::set<pid_t>::iterator,bool> newpid = s_addPID(it->pid);
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

void eDVBRecorder::s_removePID(int pid)
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
	eDebug("eDVBRecorder::s_removePID(0x%x)", pid);
}

void eDVBRecorder::s_removeAllPIDs()
{
	pids.clear();
	eDebug("eDVBRecorder::s_removeAllPIDs()");
}

void eDVBRecorder::s_start()
{
	eDebug("eDVBRecorder::s_start();");

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
	state = stateRunning;
}

void eDVBRecorder::s_stop()
{
	eDebug("eDVBRecorder::s_stop();");
	for (std::set<pid_t>::iterator i(pids.begin()); i != pids.end(); ++i)
		if (i->fd >= 0)
			::ioctl(i->fd, DMX_STOP, 0);

	state = stateStopped;

	buffer.tofile(outfd,buffer.size());
}

void eDVBRecorder::s_close()
{
	eDebug("eDVBRecorder::s_close");
	if (outfd < 0)
		return;
	delete sn;
	for (std::set<pid_t>::iterator i(pids.begin()); i != pids.end(); ++i)
		if (i->fd >= 0)
			::close(i->fd);
	::close(dvrfd);
	::close(outfd);
}

void eDVBRecorder::s_exit()
{
	eDebug("eDVBRecorder::s_exit()");
	exit_loop();
}


eDVBRecorder::eDVBRecorder()
:messagepump(this, 1), rmessagepump(this, 1), buffer(32768)
{
	CONNECT(messagepump.recv_msg, eDVBRecorder::gotMessage);
	CONNECT(rmessagepump.recv_msg, eDVBRecorder::gotBackMessage);
	lock.lock();
	run();
}

eDVBRecorder::~eDVBRecorder()
{
	messagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::mExit));
	lock.lock();
}

void eDVBRecorder::writeSection(void *data, int pid)
{
	__u8 *table=(__u8*)data;
	int len=(table[1]<<8)&0x1F;
	len|=table[2];
	
	eDebug("len: %d", len);
	
	len+=3;
	
	int first=1;
	int cc=0;
	
	while (len)
	{
		// generate header:
		__u8 *packet=(__u8*)malloc(188); // yes, malloc
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
		messagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::mWrite, packet, 188));
		first=0;
	}
}

#endif //DISABLE_FILE
