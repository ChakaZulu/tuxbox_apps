#include "record.h"

void eDVBRecorder::thread()
{
	eDebug("enter thread");	
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
	delete[] filename;
	pids.clear();
}

void eDVBRecorder::s_addPID(int pid)
{
	pids.insert(pid);
	eDebug("eDVBRecorder::s_addPID(0x%x)", pid);
}

void eDVBRecorder::s_removePID(int pid)
{
	pids.erase(pid);
	eDebug("eDVBRecorder::s_removePID(0x%x)", pid);
}

void eDVBRecorder::s_removeAllPIDs()
{
	pids.clear();
	eDebug("eDVBRecorder::s_removeAllPIDs()");
}

void eDVBRecorder::s_start()
{
	for (std::set<int>::iterator i(pids.begin()); i != pids.end(); ++i)
		printf("opening pidfilter for pid %d\n", *i);
	eDebug("eDVBRecorder::s_start();");
}

void eDVBRecorder::s_stop()
{
	eDebug("eDVBRecorder::s_start();");
}

void eDVBRecorder::s_close()
{
	eDebug("eDVBRecorder::s_close");
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
