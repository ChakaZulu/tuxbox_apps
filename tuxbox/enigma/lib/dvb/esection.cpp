#define DEMUX "/dev/ost/demux0"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <ost/dmx.h>
#include "esection.h"

QList<eSection> eSection::active;

eSection::eSection(int pid, int tableid, int tableidext, int version, int flags, int tableidmask): pid(pid), tableid(tableid), tableidext(tableidext), tableidmask(tableidmask), flags(flags), version(version)
{
	handle=-1;
	notifier=0;
	section=0;
	lockcount=0;
	timer=new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(timeout()));
	if (!(flags&SECREAD_NOABORT))
		active.append(this);
}

eSection::~eSection()
{
	if (!(flags&SECREAD_NOABORT))
		active.remove(this);
	closeFilter();
	if (lockcount)
		qFatal("deleted still locked table");
}

int eSection::start()
{
	if ((version==-1) && !(flags&SECREAD_NOTIMEOUT))
		timer->start((pid==0x14)?60000:10000, true);
	return setFilter(pid, tableid, tableidext, version);
}

void eSection::closeFilter()
{
	if (handle>=0)
	{
		delete notifier;
		notifier=0;
		timer->stop();
//		qDebug(" -----------------------------> -%d", handle);
		close(handle);
		handle=-1;
	}
}

int crc32(void *, int len)
{
	return 0;
}

void eSection::data(int socket)
{
	if (lockcount)
		qFatal("eSection::data on locked section!");
	qDebug("data");
	timer->start(10000, true);
	read(handle, buf, 3);
	int seclen=0;
	seclen |= ((buf[1] & 0x0F) << 8);
	seclen |= (buf[2] & 0xFF);
	read(handle, buf+3, seclen);
	seclen+=3;
	if (crc32((char*)buf, seclen))
	{
		printf("CRC check failed!\n");
		return;
	}
	maxsec=buf[7];
	
//	printf("%d/%d, we want %d  | service_id %04x | version %04x\n", buf[6], maxsec, section, (buf[3]<<8)|buf[4], buf[5]);

	version=buf[5];

	if ((!(flags&SECREAD_INORDER)) || (section==buf[6]))
	{
		int err;
		if ((err=sectionRead(buf))) // || !(flags&SECREAD_INORDER))
		{
			if (err>0)
				err=0;
			closeFilter();
			sectionFinish(err);
			return;
		}
		section=buf[6]+1;
	}

	if ((section>maxsec) && (flags&SECREAD_INORDER))		// last section?
	{
		closeFilter();										// stop feeding
		sectionFinish(0);
	}
}

void eSection::timeout()
{
	closeFilter();
	sectionFinish(-ETIMEDOUT);
}

int eSection::setFilter(int pid, int tableid, int tableidext, int version)
{
	__u8 data[4], mask[4];
	data[0]=tableid; mask[0]=tableidmask;
	data[1]=0; mask[1]=0;									// das hier basiert auf einem bug im treibe. normalerweise sind die offsets anders!!
	data[2]=0; mask[2]=0;
	data[3]=0; mask[3]=0;
	if (tableidext!=-1)
	{
		data[1]=tableidext>>8; mask[1]=0xFF;
		data[2]=tableidext&0xFF; mask[2]=0xFF;
	} 
	if (version!=-1)
	{
//		qDebug("FILTERING version %x", version);
		data[3]=version; mask[3]=0xFF;
	}
	return setFilter(pid, data, mask, 4);
}

int eSection::setFilter(int pid, __u8 *data, __u8 *mask, int len)
{
	dmxSctFilterParams secFilterParams;
	closeFilter();
	handle=open(DEMUX, O_RDWR);
//	qDebug(" -----------------------------> +%d", handle);
	if (handle<0)
	{
		perror(DEMUX);
		return -errno;
	}
	notifier=new QSocketNotifier(handle, QSocketNotifier::Read);
	connect(notifier, SIGNAL(activated(int)), SLOT(data(int)));

	secFilterParams.pid=pid;
	memset(&secFilterParams.filter.filter, 0, DMX_FILTER_SIZE);
	memset(&secFilterParams.filter.mask, 0, DMX_FILTER_SIZE);
	secFilterParams.timeout=0;
	secFilterParams.flags=DMX_IMMEDIATE_START;

	if (flags&SECREAD_CRC)
		secFilterParams.flags|=DMX_CHECK_CRC;

	for (int i = 0; i < DMX_FILTER_SIZE; i++)
	{
		secFilterParams.filter.filter[i]=i<len?data[i]:0;
		secFilterParams.filter.mask[i]=i<len?mask[i]:0;
	}
	if (ioctl(handle, DMX_SET_FILTER, &secFilterParams) < 0)
	{
		close(handle);
		return -1;
	}
	return 0;
}

int eSection::abort()
{
	if (handle!=-1)
	{
		closeFilter();
		sectionFinish(-ECANCELED);
		return 0;
	} else
		return -ENOENT;
}

int eSection::abortAll()
{
	for (QListIterator<eSection> i(active); i.current(); ++i)
		i.current()->abort();
	return 0;
}

int eSection::sectionRead(__u8 *data)
{
	return -1;
}

void eSection::sectionFinish(int)
{
}

int eSection::lock()
{
//	qDebug("lock on %p: now: %d", this, lockcount+1);
	return ++lockcount;
}

int eSection::unlock()
{
//	qDebug("unlock on %p: now: %d", this, lockcount-1);
	if (lockcount)
		return lockcount--;
	else
		qFatal("unlocking while not locked");
	return 0;
}

void eTable::sectionFinish(int err)
{
	if (err)
	{
		qDebug("eTable: error %d", err);
		error=err;
	}
	ready=1;
	emit tableReady(error);
}

eTable::eTable(int pid, int tableid, int tableidext, int version): eSection(pid, tableid, tableidext, version, (pid==0x14)?0:(SECREAD_INORDER|SECREAD_CRC))
{
	error=0;
	ready=0;
}

eTable *eTable::createNext()
{
	return 0;
}

void eAUGTable::slotTableReady(int error)
{
	getNext(error);
}
