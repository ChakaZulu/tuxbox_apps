#include "record.h"
#include <fcntl.h>
#include <unistd.h>
#include <ost/dmx.h>
#include <sys/ioctl.h>

void eDVBRecorder::dataAvailable(int what)
{
	const int BSIZE=16*1024;
	char buffer[BSIZE];
	int r=::read(dvrfd, buffer, BSIZE);
	if (r<=0)
	{
		eDebug("reading failed..");
		return;
	}
	::write(outfd, buffer, r);
}

void eDVBRecorder::thread()
{
	eDebug("enter thread");	
	messagepump.start();
	enter_loop();
	eDebug("leave recording thread");
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
	default:
		eDebug("received unknown message!");
	}
}

void eDVBRecorder::s_open(const char *filename)
{
	eDebug("eDVBRecorder::s_open(%s)", filename);
	pids.clear();
	unlink(filename);
	outfd=::creat(filename, 0555);
	if (outfd < 0)
	{
		eDebug("failed to open DVR file: %s (%m)", filename);
		delete[] filename;
		return;
	}
	delete[] filename;

	dvrfd=::open("/dev/dvb/card0/dvr1", O_RDONLY|O_NONBLOCK);
	if (dvrfd < 0)
	{
		eDebug("failed to open /dev/dvb/card0/dvr1 (%m)");
		::close(outfd);
		outfd=-1;
		return;
	}
	::ioctl(dvrfd, DMX_SET_BUFFER_SIZE, 1024*1024);

	eDebug("eDVBRecorder::s_start();");	
	sn=new eSocketNotifier(this, dvrfd, eSocketNotifier::Read);
	CONNECT(sn->activated, eDVBRecorder::dataAvailable);
	sn->start();
}

void eDVBRecorder::s_addPID(int pid)
{
	pid_t p;
	p.pid=pid;
	p.fd=::open("/dev/dvb/card0/demux1", O_RDWR);
	if (p.fd < 0)
	{
		eDebug("failed to open demux1");
		return;
	}
	dmxPesFilterParams flt;
	flt.pid=p.pid;
	flt.input=DMX_IN_FRONTEND;
	flt.output=DMX_OUT_TS_TAP;
	flt.pesType=DMX_PES_OTHER;
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

eDVBRecorder::eDVBRecorder(): messagepump(this)
{
	CONNECT(messagepump.recv_msg, eDVBRecorder::gotMessage);
	run();
}

eDVBRecorder::~eDVBRecorder()
{
	messagepump.send(eDVBRecorderMessage(eDVBRecorderMessage::mExit));
}
