#ifndef __LIB_BASE_CONSOLE_H__
#define __LIB_BASE_CONSOLE_H__

#include <lib/base/ebase.h>

class eString;

class eConsoleAppContainer: public Object
{
	int fd[2];
	int pid;
	int killstate;
	char *outbuf;
	eSocketNotifier *in, *out;
	void readyRead(int what);
	void readyWrite(int what);
	void closePipes();
public:
	eConsoleAppContainer( const eString &str );
	~eConsoleAppContainer();
	void kill();
	void write( const eString &s );
	bool running() { return fd[0] && fd[1]; }
	Signal1<void, eString> dataAvail;
	Signal1<void,int> dataSent;
	Signal1<void,int> appClosed;
};

#endif // __LIB_BASE_CONSOLE_H__
