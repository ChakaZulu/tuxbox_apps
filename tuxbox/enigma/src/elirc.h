#ifndef DISABLE_LIRC

#ifndef __elirc_h
#define __elirc_h

#include <lib/gui/listbox.h>
#include <lib/gui/ewindow.h>
#include <lib/socket/socket.h>

class ELirc: public Object
{
	eTimer timeout;
	eSocket *sd;
	int fd;
	std::string device;

	void sendcommand(std::string cmd);
	void sendcommandlist(std::string filename);

	ELirc();
	~ELirc();
public:
	static ELirc *getNew() { return new ELirc(); }
	void sendstart();
	void sendstop();
};

#endif

#endif // DISABLE_LIRC
