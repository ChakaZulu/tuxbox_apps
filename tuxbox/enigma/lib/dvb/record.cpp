#include "record.h"

void eDVBRecorder::thread()
{
	printf("enter thread\n");
	enter_loop();
	printf("leave recording thread\n");
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
		printf("received unknown message!\n");
	}
}

void eDVBRecorder::s_open(const char *filename)
{
	printf("eDVBRecorder::s_open(%s)\n", filename);
	delete[] filename;
}

void eDVBRecorder::s_addPID(int pid)
{
	printf("eDVBRecorder::s_addPID(0x%x)\n", pid);
}

void eDVBRecorder::s_removePID(int pid)
{
	printf("eDVBRecorder::s_removePID(0x%x)\n", pid);
}

void eDVBRecorder::s_start()
{
	printf("eDVBRecorder::s_start();\n");
}

void eDVBRecorder::s_stop()
{
	printf("eDVBRecorder::s_start();\n");
}

void eDVBRecorder::s_close()
{
	printf("eDVBRecorder::s_close\n");
}

void eDVBRecorder::s_exit()
{
	printf("eDVBRecorder::s_exit()\n");
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
