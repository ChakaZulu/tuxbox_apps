#ifdef ENABLE_DYN_FLASH
#ifndef __enigma_dyn_flash_h
#define __enigma_dyn_flash_h

#include <lib/base/thread.h>
#include <lib/base/message.h>

class eHTTPDynPathResolver;
void ezapFlashInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb);
eString getConfigFlashMgr(void);

class eFlashOperationsHandler: public eMainloop, private eThread, public Object
{
	eString progressMessage1, progressMessage2, progressComplete;
	struct Message
	{
		int type;
		const char *mtd;
		const char *filename;
		enum
		{
			read,
			write,
			quit
		};
		Message(int type = 0, const char *mtd = 0, const char *filename = 0)
			:type(type), mtd(mtd), filename(filename)
		{}
	};
	eFixedMessagePump<Message> messages;
	static eFlashOperationsHandler *instance;
	void gotMessage(const Message &message);
	void thread();
	int writeFlash(eString mtd, eString fileName);
	int readFlash(eString mtd, eString fileName);
public:
	eFlashOperationsHandler();
	~eFlashOperationsHandler();
	void readPartition(const char * mtd, const char * filename);
	void writePartition(const char * mtd, const char * filename);
	eString getProgressMessage1() { return progressMessage1; }
	eString getProgressMessage2() { return progressMessage2; }
	eString getProgressComplete() { return progressComplete; }
	static eFlashOperationsHandler *getInstance() { return (instance) ? instance : new eFlashOperationsHandler(); }
};

class eFlashMgr
{
	eString h1, h2, h3, h4;
	typedef struct
	{
		eString dev, name, size, erasesize;
	} t_mtd;
	std::list<t_mtd> mtds;
	
public:
	eFlashMgr();
	~eFlashMgr();
	eString htmlList();
};

#endif /* __enigma_dyn_flash_h */
#endif

