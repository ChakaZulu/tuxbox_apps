#ifndef __E_ERROR__
#define __E_ERROR__

//#define MEMLEAK_CHECK
#undef MEMLEAK_CHECK

#ifdef MEMLEAK_CHECK
#include <list>
#include <lib/system/elock.h>
#endif

#include <new>
#include <libsig_comp.h>
#include <config.h>
#include <lib/base/estring.h>

class eString;

void eFatal(const char* fmt, ...);

enum { lvlDebug=1, lvlWarning=2, lvlFatal=4 };

extern Signal2<void, int, const eString&> logOutput;
extern int logOutputConsole;

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef DEBUG
void eDebug(const char* fmt, ...);
void eDebugNoNewLine(const char* fmt, ...);
void eWarning(const char* fmt, ...);
#define ASSERT(x) { if (!(x)) eFatal("%s:%d ASSERTION %s FAILED!", __FILE__, __LINE__, #x); }

#ifdef MEMLEAK_CHECK
typedef struct
{
	unsigned int address;
	unsigned int size;
	char file[64];
	unsigned int line;
	unsigned int type;
} ALLOC_INFO;

typedef std::list<ALLOC_INFO*> AllocList;

extern AllocList *allocList;
extern pthread_mutex_t memLock;

static inline void AddTrack(unsigned int addr,  unsigned int asize,  const char *fname, unsigned int lnum, unsigned int type)
{
	ALLOC_INFO *info;

	if(!allocList)
	{
		allocList = new(AllocList);
	}

	info = new(ALLOC_INFO);
	info->address = addr;
	strncpy(info->file, fname, 63);
	info->line = lnum;
	info->size = asize;
	info->type = type;
	singleLock s(memLock);
	allocList->insert(allocList->begin(), info);
};

static inline void RemoveTrack(unsigned int addr, unsigned int type)
{
	singleLock s(memLock);
	AllocList::iterator i;

	if(!allocList)
		return;
	for(i = allocList->begin(); i != allocList->end(); i++)
	{
		if ( (*i)->type == 3 )
			continue;
		if((*i)->address == addr)
		{
			if ( (*i)->type != type )
			{
				(*i)->type=3;
				break;
			}
			allocList->remove((*i));
			break;
		}
	}
};

inline void * operator new(unsigned int size,
																			 const char *file, int line)
{
	void *ptr = (void *)malloc(size);
	AddTrack((unsigned int)ptr, size, file, line, 1);
	return(ptr);
};

inline void operator delete(void *p)
{
	RemoveTrack((unsigned int)p,1);
	free(p);
};

inline void * operator new[](unsigned int size,
																			 const char *file, int line)
{
	void *ptr = (void *)malloc(size);
	AddTrack((unsigned int)ptr, size, file, line, 2);
	return(ptr);
};

inline void operator delete[](void *p)
{
	RemoveTrack((unsigned int)p, 2);
	free(p);
};

inline void DumpUnfreed()
{
	AllocList::iterator i;
	unsigned int totalSize = 0;

	if(!allocList)
		return;

	for(i = allocList->begin(); i != allocList->end(); i++)
	{
		eDebug("%s\tLINE %d\tADDRESS %p\t%d unfreed\ttype %d\n",
			(*i)->file, (*i)->line, (*i)->address, (*i)->size, (*i)->type);
		totalSize += (*i)->size;
	}

	eDebug("-----------------------------------------------------------\n");
	eDebug("Total Unfreed: %d bytes\n", totalSize);
};
#define new new(__FILE__, __LINE__)

#endif // MEMLEAK_CHECK

#else
#define eDebug(fmt, args...)
#define eDebugNoNewLine(fmt, args...)
#define eWarning(fmt, args...)
#define ASSERT(x)
#endif //DEBUG

#endif // __E_ERROR__
