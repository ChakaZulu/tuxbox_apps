#ifdef DBOX
#define DEMUX "/dev/ost/demux0"
#else
#define DEMUX "/dev/demuxapi0"
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#ifdef DBOX
#include <ost/dmx.h>
#else
#include <xp/xp_osd_user.h>
#endif
#include "esection.h"

QList<eSection> eSection::active;

eSectionReader::eSectionReader()
{
	handle=-1;
}

int eSectionReader::getHandle()
{
	return handle;
}

void eSectionReader::close()
{
	if (handle != -1)
		::close(handle);
	handle=-1;
}

int eSectionReader::open(int pid, __u8 *data, __u8 *mask, int len, int _flags)
{
	flags=flags;
#ifdef DBOX
	dmxSctFilterParams secFilterParams;
#else
	demux_filter_para secFilterParams;
#endif

	close();

	handle=::open(DEMUX, O_RDWR|O_NONBLOCK);
	printf("opened handle %d\n", handle);
	if (handle<0)
	{
		perror(DEMUX);
		qFatal("DEMUX OPEN FAILED");
		return -errno;
	}

	secFilterParams.pid=pid;
#ifdef DBOX
	const int maxsize=DMX_FILTER_SIZE;
	memset(secFilterParams.filter.filter, 0, maxsize);
	memset(secFilterParams.filter.mask, 0, maxsize);

	secFilterParams.timeout=0;
	secFilterParams.flags=DMX_IMMEDIATE_START;
	if (flags&SECREAD_CRC)
		secFilterParams.flags|=DMX_CHECK_CRC;
	for (int i = 0; i < DMX_FILTER_SIZE; i++)
	{
		secFilterParams.filter.filter[i]=i<len?data[i]:0;
		secFilterParams.filter.mask[i]=i<len?mask[i]:0;
	}

	printf("%02x: ", pid);
	for (int i=0; i<DMX_FILTER_SIZE; i++)
		printf("%02x ", secFilterParams.filter.filter[i]);
	printf(" (napi)\n    ");
	for (int i=0; i<DMX_FILTER_SIZE; i++)
		printf("%02x ", secFilterParams.filter.mask[i]);
	printf("\n");

	if (ioctl(handle, DMX_SET_FILTER, &secFilterParams) < 0)
	{
		::close(handle);
		return -1;
	}
#else
	if (len>FILTER_LENGTH)
		len=FILTER_LENGTH;
	memset(secFilterParams.filter, 0, 6);
	memset(secFilterParams.mask, 0, 6);
	memcpy(secFilterParams.filter, data, len);
	memcpy(secFilterParams.mask, mask, len);
	len=6;
	secFilterParams.filter_length=len;
	printf("%02x: ", pid);
	for (int i=0; i<len; i++)
		printf("%02x ", secFilterParams.filter[i]);
	printf("\n    ");
	for (int i=0; i<len; i++)
		printf("%02x ", secFilterParams.mask[i]);
	printf("\n");
	if (ioctl(handle, DEMUX_FILTER_SET, &secFilterParams))
	{
		close(handle);
		return -1;
	}
	if (ioctl(handle, DEMUX_START, 0))
	{
		close(handle);
		return -1;
	}
#endif

	return 0;
}

int eSectionReader::read(__u8 *buf)
{
#ifdef DBOX
	if (::read(handle, buf, 3)<0)
	{
		if (errno==EAGAIN)
			return errno;
		perror("read section");
		return errno;
	}
	int seclen=0;
	seclen |= ((buf[1] & 0x0F) << 8);
	seclen |= (buf[2] & 0xFF);
	::read(handle, buf+3, seclen);
	seclen+=3;
#else
	if (::read(handle, buf, 16384)<0)
	{
		if (errno==EAGAIN)
			break;
		perror("read section");
		break;
	}
#endif
	return 0;
}

eSection::eSection(int pid, int tableid, int tableidext, int version, int flags, int tableidmask): pid(pid), tableid(tableid), tableidext(tableidext), tableidmask(tableidmask), flags(flags), version(version)
{
	notifier=0;
	section=0;
	lockcount=0;
	timer=new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(timeout()));
	if (!(flags&SECREAD_NOABORT))
		active.append(this);
}

eSection::eSection()
{
	timer=0;
	notifier=0;
	section=0;
	lockcount=0;
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

int eSection::setFilter(int pid, int tableid, int tableidext, int version)
{
	reader.close();
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
		data[3]=version; mask[3]=0xFF;
	}

	reader.open(pid, data, mask, 4, flags);
	
	if (notifier)
		delete notifier;
	notifier=new QSocketNotifier(reader.getHandle(), QSocketNotifier::Read);
	connect(notifier, SIGNAL(activated(int)), SLOT(data(int)));
}

void eSection::closeFilter()
{
	if (reader.getHandle()>0)
	{
		delete notifier;
		notifier=0;
		timer->stop();
		reader.close();
	}
}

void eSection::data(int socket)
{
	while (1)
	{
		if (lockcount)
			qFatal("eSection::data on locked section!");
		timer->start(10000, true);
		if (reader.read(buf))
			break;
		maxsec=buf[7];
	
//		printf("%d/%d, we want %d  | service_id %04x | version %04x\n", buf[6], maxsec, section, (buf[3]<<8)|buf[4], buf[5]);

		if (flags&SECREAD_INORDER)
			version=buf[5];

		if ((!(flags&SECREAD_INORDER)) || (section==buf[6]))
		{
			int err;
			if ((err=sectionRead(buf)))
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
			break;
		}
	}
}

void eSection::timeout()
{
	closeFilter();
	sectionFinish(-ETIMEDOUT);
}

int eSection::abort()
{
	if (reader.getHandle()>0)
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
	return ++lockcount;
}

int eSection::unlock()
{
	if (lockcount)
		return lockcount--;
	else
		qFatal("unlocking while not locked");
	return 0;
}

void eTable::sectionFinish(int err)
{
	if (err)
		error=err;
	ready=1;
	emit tableReady(error);
}

eTable::eTable(int pid, int tableid, int tableidext, int version): eSection(pid, tableid, tableidext, version, (pid==0x14)?0:(SECREAD_INORDER|SECREAD_CRC))
{
	error=0;
	ready=0;
}

eTable::eTable(): eSection()
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
