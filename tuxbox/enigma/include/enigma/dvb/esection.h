#ifndef __esection_h
#define __esection_h

#include <ebase.h>
#include <asm/types.h>
#include <libsig_comp.h>

#define SECREAD_INORDER	1			// read them in order (read full tables)
#define SECREAD_CRC			2			// check CRCs
#define SECREAD_NOTIMEOUT	4		// never timeout
#define SECREAD_NOABORT	8			// do not abort on transponderchange

class eSectionReader
{
	int handle;
	int flags;
public:
	eSectionReader();
	int getHandle();		// for SocketNotifiers
	void close();
	int open(int pid, __u8 *data, __u8 *mask, int len, int flags);
	int read(__u8 *data);
};

class eSection: public Object
{
	eSectionReader reader;
	static ePtrList<eSection> active;
	eSocketNotifier *notifier;
	virtual int sectionRead(__u8 *data);
	virtual void sectionFinish(int error);
	
	int pid, tableid, tableidext, tableidmask;
	int maxsec, section, flags;
	void closeFilter();
	eTimer *timer;
	__u8 buf[65536];
	int lockcount;
	int setFilter(int pid, int tableid, int tableidext, int version);
public:
	void data(int socket);
	void timeout();
public:
	eSection(int pid, int tableid, int tableidext, int version, int flags, int tableidmask=0xFF);
	eSection();
	virtual ~eSection();

	int start();
	int abort();
	static int abortAll();
	int getLockCount() { return lockcount; }
	int lock();
	int unlock();
	
	int version;
};

class eTable: public eSection
{
	virtual int sectionRead(__u8 *d) { int err=data(d); if (err<0) error=err; return err; }
	virtual void sectionFinish(int err);
protected:
	virtual int data(__u8 *data)=0;
public:
	Signal1<void, int> tableReady;
	eTable(int pid, int tableid, int tableidext=-1, int version=-1);
	eTable();
	virtual eTable *createNext();
	int incrementVersion(int version) { return (version&0xC1)|((version+2)&0x3E); }
	int error;
	int ready;
};


class eAUGTable: public Object
{
protected:
	void slotTableReady(int);
public:
	Signal1<void, int> tableReady;
	virtual void getNext(int err)=0;
};

template <class Table>
class eAUTable: public eAUGTable
{
	Table *current, *next;		// current is READY AND ERRORFREE, next is not yet ready
	int first;
public:
	eAUTable()
	{
		current=next=0;
	}

	~eAUTable()
	{
		delete current;
		delete next;
	}
	
	int start(Table *cur)
	{
		first=1;
		if (current)
			delete current;
		current=0;
		if (next)
			delete next;
		next=cur;
		CONNECT(next->tableReady, eAUTable::slotTableReady);
		return next->start();
	}
	
	int get()
	{
		if (current)
		{
			/*emit*/ tableReady(0);
			return 0;
		} else if (!next)
		{
			/*emit*/ tableReady(-1);
			return 0;
		} else
			return 1;
	}
	
	Table *getCurrent()
	{
		if (!current)
			eFatal("getCurrent - and nothing ready!");
		current->lock();
		return current;
	}
	
	void abort()
	{
		eDebug("eAUTable: aborted!");
		if (next)
			next->abort();
		delete next;
		next=0;
	}
	
	int ready()
	{
		return !!current;
	}
	
	void inject(Table *t)
	{
		delete next;
		next=t;
		getNext(0);
	}

	void getNext(int error)
	{
//		qDebug("eAUTable: got table (ready %d, error %d)", next->ready, next->error);
		if (current && current->getLockCount())
		{
			delete next;
		} else
		{
			if (error)
			{
				delete next;
				next=0;
				if (first)
					/*emit*/ tableReady(error);
				first=0;
				return;
			} else
			{
				if (current)
					delete current;
				current=next;
			}
		}
		next=0;
		first=0;
		
		if (!current->ready)
			eFatal("was soll das denn? not ready? ICH GLAUBE ES HACKT!");
			
		/*emit*/ tableReady(0);

		next=(Table*)current->createNext();
		if (next)
		{
			CONNECT(next->tableReady, eAUTable::slotTableReady);
			
			next->start();
		}
	}
};

#endif
