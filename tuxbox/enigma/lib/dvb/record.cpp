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

void eDVBRecorder::dataAvailable(int what)
{
	const int BSIZE=16*1024;
	char buffer[BSIZE];
	int res;
	int r=::read(dvrfd, buffer, BSIZE);
	(void)what;
	if (r<=0)
	{
		eDebug("reading failed..(err %d)", -r);
		return;
	}
	res=::write(outfd, buffer, r);
	if (res <= 0)
	{
		eDebug("recording write error, maybe disk full");
//		s_close();
		rmessagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::rWriteError));
		return;
	}
	size+=res;
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
	::ioctl(dvrfd, DMX_SET_BUFFER_SIZE, 1024*1024);

	eDebug("eDVBRecorder::s_start();");	
	if (outfd >= 0)
	{
		sn=new eSocketNotifier(this, dvrfd, eSocketNotifier::Read);
		CONNECT(sn->activated, eDVBRecorder::dataAvailable);
		sn->start();
	} else
		rmessagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::rWriteError));
}

void eDVBRecorder::s_addPID(int pid)
{
	pid_t p;
	p.pid=pid;
	p.fd=::open(DEMUX1_DEV, O_RDWR);
	if (p.fd < 0)
	{
		eDebug("failed to open demux1");
		return;
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
		return;
	}

	pids.insert(p);
	eDebug("eDVBRecorder::s_addPID(0x%x)", pid);
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
	for (std::set<pid_t>::iterator i(pids.begin()); i != pids.end(); ++i)
	{
		printf("startin pidfilter for pid %d\n", i->pid);

		if (::ioctl(i->fd, DMX_START, 0)<0)
		{
			eDebug("DMX_START failed");
			::close(i->fd);
			continue;
		}
	}
}

void eDVBRecorder::s_stop()
{
	for (std::set<pid_t>::iterator i(pids.begin()); i != pids.end(); ++i)
		if (i->fd >= 0)
			::ioctl(i->fd, DMX_STOP, 0);

	eDebug("eDVBRecorder::s_stop();");
}

void eDVBRecorder::s_close()
{
	if (outfd < 0)
		return;
	delete sn;
	for (std::set<pid_t>::iterator i(pids.begin()); i != pids.end(); ++i)
		if (i->fd >= 0)
			::close(i->fd);
	::close(dvrfd);
	eDebug("eDVBRecorder::s_close");
	::close(outfd);
}

void eDVBRecorder::s_exit()
{
	eDebug("eDVBRecorder::s_exit()");
	exit_loop(); 
}


eDVBRecorder::eDVBRecorder(): messagepump(this, 1), rmessagepump(this, 1)
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
