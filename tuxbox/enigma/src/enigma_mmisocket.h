#ifndef __ENIGMA_MMI_H_
#define __ENIGMA_MMI_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <map>
#include <lib/dvb/dvbservice.h>
#include <enigma_ci.h>
#include <enigma_main.h>
#include <enigma_setup.h>

class eSocketMMIHandler: public Object
{
	int listenfd, clilen;
	struct sockaddr_un servaddr;
	eSocketNotifier *sn;
	void dataAvail(int what);
	void initiateMMI();
	void setupOpened( eSetupWindow *setup, int *entrynum );
	const char *sockname;
	const char *name;
public:
	const char *getName() const { return name; }
	Signal2<void, const char*, int> mmi_progress;
	int send_to_mmisock( void *, size_t );
	eSocketMMIHandler( const char *sockname, const char *name );
	~eSocketMMIHandler();
};

class eSocketMMI : public enigmaMMI
{
	eSocketMMIHandler *handler;
	void beginExec();
	void sendAnswer( AnswerType ans, int param, unsigned char *data );
	static std::map<eSocketMMIHandler*,eSocketMMI*> exist;
public:
	static eSocketMMI *getInstance( eSocketMMIHandler *handler );
	eSocketMMI(eSocketMMIHandler *handler);
};

class eDreamcryptMMI : public eSocketMMIHandler
{
public:
	eDreamcryptMMI()
		:eSocketMMIHandler("/tmp/dc.mmi.socket", "Dreamcrypt" )
	{
	}
};

#endif // __ENIGMA_MMI_H_
