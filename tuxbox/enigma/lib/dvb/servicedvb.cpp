#include "servicedvb.h"
#include <core/dvb/edvb.h>
#include <core/dvb/dvbservice.h>
#include <core/system/init.h>
#include <core/driver/streamwd.h>
#include <core/dvb/servicefile.h>
#include <core/dvb/servicestructure.h>
#include <core/dvb/dvb.h>
#include <core/dvb/decoder.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

eDVRPlayerThread::eDVRPlayerThread(const char *filename, eServiceHandlerDVB *handler): handler(handler), buffer(64*1024), messages(this)
{
	state=stateInit;

	sourcefd=::open(filename, O_RDONLY);
	if (sourcefd<0)
	{
		eDebug("error opening %s", filename);
		state=stateError;
	}
	
	dvrfd=::open("/dev/pvr", O_WRONLY|O_NONBLOCK);		// TODO: change to /dev/dvb/dvr0 (but only when drivers support this!)
	if (dvrfd<0)
	{
		eDebug("couldn't open /dev/pvr - buy the new $$$ box and load pvr.o! (%m)");
		state=stateError;
	}
	
	outputsn=new eSocketNotifier(this, dvrfd, eSocketNotifier::Write, 0);
	CONNECT(outputsn->activated, eDVRPlayerThread::outputReady);
	inputsn=new eSocketNotifier(this, sourcefd, eSocketNotifier::Read, 0);
	CONNECT(inputsn->activated, eDVRPlayerThread::readMore);
	
	CONNECT(messages.recv_msg, eDVRPlayerThread::gotMessage);
	
	maxBufferSize=256*1024;
	
	speed=1;

	if (state != stateError)
		run();
}

void eDVRPlayerThread::thread()
{
	messages.start();
	exec();
}

void eDVRPlayerThread::outputReady(int what)
{
	buffer.tofile(dvrfd, 65536);
	if ((state == stateBufferFull) && (buffer.size()<maxBufferSize))
	{
		state=statePlaying;
		eDebug("starting play..");
		inputsn->start();
	}
	if (buffer.empty())
	{
		outputsn->stop();
		if (state!=stateFileEnd)
			state=stateBuffering;
		else
		{
			eDebug("ok, everything played..");
			handler->messages.send(eServiceHandlerDVB::eDVRPlayerThreadMessage(eServiceHandlerDVB::eDVRPlayerThreadMessage::done));
		}
	}
}

void eDVRPlayerThread::dvrFlush()
{
	eDebug("dvrflush");
	eDebug("%d", ::ioctl(dvrfd, 0)); // PVR_FLUSH_BUFFER
	Decoder::flushBuffer();
}

void eDVRPlayerThread::readMore(int what)
{
	if ((state != statePlaying) && (state != stateBuffering))
	{
		eDebug("wrong state (%d)", state);
		return;
	}
	
	int flushbuffer=0;
	
	if (buffer.size() < maxBufferSize)
	{
		if (buffer.fromfile(sourcefd, maxBufferSize) < maxBufferSize)
			flushbuffer=1;
	}
	
	if ((state == stateBuffering) && (buffer.size()>16384))
	{
		state=statePlaying;
		eDebug("entering playing state");
		outputsn->start();
	}
	
	if (flushbuffer)
	{
		eDebug("end of file...");
		state=stateFileEnd;
		inputsn->stop();
	}
	
	if ((state == statePlaying) && (buffer.size() >= maxBufferSize))
	{
		state=stateBufferFull;
		inputsn->stop();
		eDebug("stopping inputsn");
	}
}

eDVRPlayerThread::~eDVRPlayerThread()
{
	kill(); // wait for thread exit.

	delete inputsn;
	delete outputsn;
	if (dvrfd >= 0)
		::close(dvrfd);
	if (sourcefd >= 0)
		::close(sourcefd);
}

void eDVRPlayerThread::gotMessage(const eDVRPlayerThreadMessage &message)
{
	switch (message.type)
	{
	case eDVRPlayerThreadMessage::start:
		if (state == stateInit)
		{
			state=stateBuffering;
			eDebug("init message..");
			inputsn->start();
		}
		break;
	case eDVRPlayerThreadMessage::exit:
		eDebug("got quit message..");
		quit();
		break;
	case eDVRPlayerThreadMessage::setSpeed:
		speed=message.parm;
		if (message.parm == 0)
		{
			if ((state==stateBuffering) || (state==stateBufferFull) || (statePlaying))
			{
				inputsn->stop();
				outputsn->stop();
				state=statePause;
				dvrFlush();
			}
		} else if (state == statePause)
		{
			eDebug("resume");
			inputsn->start();
			outputsn->start();
			speed=message.parm;
			state=stateBuffering;
		} else
		{
			buffer.clear();
			dvrFlush();
		}
		break;
	case eDVRPlayerThreadMessage::seek:
	case eDVRPlayerThreadMessage::skip:
	case eDVRPlayerThreadMessage::seekreal:
	{
		int offset=0;
		if (message.type == eDVRPlayerThreadMessage::skip)
		{
			int br=3000000; // assuming 3MBit bitrate...
			br/=8;
		
			br*=message.parm;
			offset=buffer.size();
			buffer.clear();
			offset+=br/1000;
			eDebug("skipping %d bytes (br: %d)..", offset, br);
			offset+=lseek(sourcefd, 0, SEEK_CUR);
			if (offset<0)
				offset=0;
		} else if (message.type == eDVRPlayerThreadMessage::seek)
			offset=(lseek(sourcefd, 0, SEEK_END) >> 8) * (message.parm>>8);
		else
			offset=message.parm;

		eDebug("seeking to %d", offset);
		lseek(sourcefd, offset, SEEK_SET);
		dvrFlush();
		buffer.clear();
		
		if (state == statePlaying)
		{
			eDebug("skip");
			inputsn->start();
			state=stateBuffering;
		}
		
		break;
	}
	}
}

void eServiceHandlerDVB::addFile(void *node, const eString &filename)
{
	if (filename.right(3).upper()==".TS")
	{
		unsigned int pos;
		eString part=filename.left(filename.rfind("."));
		if ((pos=part.rfind(".")) == eString::npos)
			return;
		eString service_type=part.mid(pos+1);
		part=part.left(pos);
		if ((pos=part.rfind(".")) == eString::npos)
			return;
		eString sid=part.mid(pos+1);
		part=part.left(pos);
		if ((pos=part.rfind("/")) == eString::npos)
			if ((pos=part.rfind(".")) == eString::npos)
				return;
		eString onid=part.mid(pos+1);

		int original_network_id, service_id;
		sscanf(onid.c_str(), "%04x", &original_network_id);
		sscanf(sid.c_str(), "%04x", &service_id);

		eServiceReferenceDVB ref(
				eTransportStreamID(-1),
				eOriginalNetworkID(original_network_id), 
				eServiceID(service_id), 
				atoi(service_type.c_str())
			);
		ref.path=filename;
		
		eServiceFileHandler::getInstance()->addReference(node, ref);
	}
}

int eServiceHandlerDVB::getID() const
{
	return eServiceReference::idDVB;
}

void eServiceHandlerDVB::scrambledStatusChanged(bool scrambled)
{
	int oldflags=flags;

	if (scrambled)
		flags |= flagIsScrambled;
	else
		flags &= ~flagIsScrambled;

	if (oldflags != flags)
		serviceEvent(eServiceEvent(eServiceEvent::evtFlagsChanged) );
}

void eServiceHandlerDVB::switchedService(const eServiceReferenceDVB &, int err)
{
	int oldstate=state;
	error = err;
	if (error)
		state=stateError;
	else
		state=statePlaying;
	if (state != oldstate)
		serviceEvent(eServiceEvent(eServiceEvent::evtStateChanged));

	serviceEvent(eServiceEvent(eServiceEvent::evtStart));
}

void eServiceHandlerDVB::gotEIT(EIT *, int)
{
	serviceEvent(eServiceEvent(eServiceEvent::evtGotEIT));
}

void eServiceHandlerDVB::gotSDT(SDT *)
{
	serviceEvent(eServiceEvent(eServiceEvent::evtGotSDT));
}

void eServiceHandlerDVB::gotPMT(PMT *)
{
	serviceEvent(eServiceEvent(eServiceEvent::evtGotPMT));
}

void eServiceHandlerDVB::leaveService(const eServiceReferenceDVB &)
{
	serviceEvent(eServiceEvent(eServiceEvent::evtStop));
}

void eServiceHandlerDVB::aspectRatioChanged(int isanamorph)
{
	aspect=isanamorph;
	serviceEvent(eServiceEvent(eServiceEvent::evtAspectChanged));
}

eServiceHandlerDVB::eServiceHandlerDVB(): eServiceHandler(eServiceReference::idDVB), messages(eApp), cache(*this)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);

	CONNECT(eServiceFileHandler::getInstance()->fileHandlers, eServiceHandlerDVB::addFile);

	CONNECT(eDVB::getInstance()->scrambled, eServiceHandlerDVB::scrambledStatusChanged);
	CONNECT(eDVB::getInstance()->switchedService, eServiceHandlerDVB::switchedService);
	CONNECT(eDVB::getInstance()->gotEIT, eServiceHandlerDVB::gotEIT);
	CONNECT(eDVB::getInstance()->gotSDT, eServiceHandlerDVB::gotSDT);
	CONNECT(eDVB::getInstance()->gotPMT, eServiceHandlerDVB::gotPMT);
	CONNECT(eDVB::getInstance()->leaveService, eServiceHandlerDVB::leaveService);
	CONNECT(eStreamWatchdog::getInstance()->AspectRatioChanged, eServiceHandlerDVB::aspectRatioChanged);

	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 0xFFFFFFFF),
			new eService(eServiceReference::idDVB, "DVB - bouquets")
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1) ),
			new eService(eServiceReference::idDVB, "DVB - bouquets (TV)")
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1<<2 ),
			new eService(eServiceReference::idDVB, "DVB - bouquets (Radio)")
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 0xFFFFFFFF), 
			new eService(eServiceReference::idDVB, "DVB - all services")
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, (1<<4)|(1<<1) ), // TV and NVOD
			new eService(eServiceReference::idDVB, "DVB - TV services")
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 1<<2 ), 	// radio
			new eService(eServiceReference::idDVB, "DVB - Radio services")
		);
}

eServiceHandlerDVB::~eServiceHandlerDVB()
{
	if (eServiceInterface::getInstance()->unregisterHandler(id)<0)
		eFatal("couldn't unregister serviceHandler %d", id);
}


int eServiceHandlerDVB::play(const eServiceReference &service)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (service.type != eServiceReference::idDVB)
		return -1;
	int oldflags=flags;
	if (service.path.length())
	{
		decoder=new eDVRPlayerThread(service.path.c_str(), this);
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::start));
		flags|=flagIsSeekable;
	} else
	{
		decoder=0;
		flags &= ~flagIsSeekable;
	}
	if (oldflags != flags)
		serviceEvent(eServiceEvent(eServiceEvent::evtFlagsChanged) );
	if (sapi)
		return sapi->switchService((const eServiceReferenceDVB&)service);
	return -1;
}

int eServiceHandlerDVB::serviceCommand(const eServiceCommand &cmd)
{
	switch (cmd.type)
	{
	case eServiceCommand::cmdRecordOpen:
	{
		eString servicename="/mnt/movie/";
		const eServiceReference &service=eServiceInterface::getInstance()->service;
		eService *s=addRef(service);
		if (s)
		{
			servicename+=s->service_name;
			removeRef(service);
		} else
			servicename+="unnamed";
		servicename += eString().sprintf(".%d.", time(0)+eDVB::getInstance()->time_difference);
		servicename += eString().sprintf("%04x.", ((eServiceReferenceDVB&)service).getOriginalNetworkID().get());
		servicename += eString().sprintf("%04x.", ((eServiceReferenceDVB&)service).getServiceID().get());
		servicename += eString().sprintf("%d.ts", ((eServiceReferenceDVB&)service).getServiceType());
		eDebug("begin recording to %s", servicename.c_str());
		eDVB::getInstance()->recBegin(servicename.c_str());
		break;
	}
	case eServiceCommand::cmdRecordStart:
		eDVB::getInstance()->recResume();
		break;
	case eServiceCommand::cmdRecordStop:
		eDVB::getInstance()->recPause();
		break;
	case eServiceCommand::cmdRecordClose:
		eDVB::getInstance()->recEnd();
		break;
	case eServiceCommand::cmdSetSpeed:
		if ((state == statePlaying) || (state == statePause) || (state == stateSkipping))
		{
			if (cmd.parm < 0)
				return -1;
			decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::setSpeed, cmd.parm));
			if (cmd.parm == 0)
				state=statePause;
			else if (cmd.parm == 1)
				state=statePlaying;
			else
				state=stateSkipping;
		} else
			return -2;
		break;
	case eServiceCommand::cmdSkip:
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::skip, cmd.parm));
		break;
	case eServiceCommand::cmdSeekAbsolute:
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::seek, cmd.parm));
		break;
	case eServiceCommand::cmdSeekReal:
		eDebug("seekreal");
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::seekreal, cmd.parm));
		break;
	default:
		return -1;
	}
	return 0;
}

PMT *eServiceHandlerDVB::getPMT()
{
	return eDVB::getInstance()->getPMT();
}

void eServiceHandlerDVB::setPID(const PMTEntry *e)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		sapi->setPID(e);
		sapi->setDecoder();
	}
}

SDT *eServiceHandlerDVB::getSDT()
{
	return eDVB::getInstance()->getSDT();
}

EIT *eServiceHandlerDVB::getEIT()
{
	return eDVB::getInstance()->getEIT();
}

int eServiceHandlerDVB::getFlags()
{
	return flags;
}

int eServiceHandlerDVB::getAspectRatio()
{
	return aspect;
}

int eServiceHandlerDVB::getState()
{
	return state;
}

int eServiceHandlerDVB::getErrorInfo()
{
	return error;
}

int eServiceHandlerDVB::stop()
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

	if (sapi)
		sapi->switchService(eServiceReferenceDVB());
		
	if (decoder)
	{
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::exit));
		delete decoder;
	}

	return 0;
}

struct eServiceHandlerDVB_addService
{
	Signal1<void,const eServiceReference&> &callback;
	int mask;
	eServiceHandlerDVB_addService(Signal1<void,const eServiceReference&> &callback, int mask): callback(callback), mask(mask)
	{
	}
	void operator()(const eServiceReference &service)
	{
		int t = ((eServiceReferenceDVB&)service).getServiceType();
		if (t < 0)
			t=0;
		if (t >= 31)
			t=31;
		if (mask & (1<<t))
			callback(service);
	}
};

void eServiceHandlerDVB::enterDirectory(const eServiceReference &ref, Signal1<void,const eServiceReference&> &callback)
{
	switch (ref.type)
	{
	case eServiceReference::idDVB:
		switch (ref.data[0])
		{
		case -2:  // all TV or all Radio Services
			eDVB::getInstance()->settings->getTransponders()->forEachServiceReference(eServiceHandlerDVB_addService(callback, ref.data[1]));
			break;
		case -3:	// normal dvb bouquet
		{
			eBouquet *b=eDVB::getInstance()->settings->getBouquet(ref.data[2]);
			if (!b)
				break;
			for (std::list<eServiceReferenceDVB>::iterator i(b->list.begin());  i != b->list.end(); ++i)
			{
				int t = i->getServiceType();
				if (t < 0)
					t=0;
				if (t >= 31)
					t=31;
				if (ref.data[1] & (1<<t))
					callback(*i);
			}
			break;
		}
		default:
			break;
		}
	default:
		break;
	}
	cache.enterDirectory(ref, callback);
}

eService *eServiceHandlerDVB::createService(const eServiceReference &node)
{
	if (node.data[0]>=0)
	{
		eString path=node.path.mid(node.path.rfind('/')+1);
		path=path.left(path.find('.'));
		if (!path)
			path="movie.";

		return new eService(eServiceID(0), path.c_str());
	}
	switch (node.data[0])
	{
	case -3:
	{
		eBouquet *b=eDVB::getInstance()->settings->getBouquet(node.data[2]);
		if (!b)
			return 0;
		return new eService(eServiceID(0), b->bouquet_name.c_str());
	}
	}
	return 0;
}

void eServiceHandlerDVB::loadNode(eServiceCache<eServiceHandlerDVB>::eNode &node, const eServiceReference &ref)
{
	switch (ref.type)
	{
	case eServiceReference::idStructure:
		switch (ref.data[0])
		{
		case eServiceStructureHandler::modeRoot:
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 0xFFFFFFFF));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, (1<<4)|(1<<1) ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 1<<2 ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 0xFFFFFFFF));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1) ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1<<2 ));
			break;
		case eServiceStructureHandler::modeTV:
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, (1<<4)|(1<<1) ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1) ));
			break;
		case eServiceStructureHandler::modeRadio:
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 1<<2 ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1<<2 ));
			break;
		}
		break;
	case eServiceReference::idDVB:
		switch (ref.data[0])
		{
		case -1:  // handle bouquets
		{
			ePtrList<eBouquet> &list=*eDVB::getInstance()->settings->getBouquets();
			for (ePtrList<eBouquet>::iterator i(list.begin()); i != list.end(); ++i)
			{
				int flags=eServiceReference::mustDescent|eServiceReference::canDescent|eServiceReference::isDirectory;

				if (ref.data[1] >= 0) 		// sort only automatic generated services
					flags|=eServiceReference::shouldSort;

				int found = 0;
				for ( std::list<eServiceReferenceDVB>::iterator s(i->list.begin()); s != i->list.end(); ++s)
				{
					int t = s->getServiceType();
					if (t < 0)
						t=0;
					if (t >= 31)
						t=31;
					if (ref.data[1] & (1<<t))
					{
						found++;
						break;
					}
				}
				if (found)
					cache.addToNode(node, eServiceReference(eServiceReference::idDVB, flags, -3, ref.data[1], i->bouquet_id));
			}
			break;
		}
		}
		break;
	}
}

void eServiceHandlerDVB::leaveDirectory(const eServiceReference &dir)
{
	cache.leaveDirectory(dir);
}

eService *eServiceHandlerDVB::addRef(const eServiceReference &service)
{
	if (service.data[0] < 0)
	{
		eService *s=cache.addRef(service);
		if (s)
			return s;
		else
			return 0;
	} else if (!service.path.length())
	{
		eTransponderList *tl=eTransponderList::getInstance();
		if (!tl)
			return 0;
		return tl->searchService(service);
	} else
		return eServiceFileHandler::getInstance()->addRef(service);
}

void eServiceHandlerDVB::removeRef(const eServiceReference &service)
{
	if (service.path.length())
		return eServiceFileHandler::getInstance()->removeRef(service);
	else if (service.data[0] < 0)
		cache.removeRef(service);
}

eAutoInitP0<eServiceHandlerDVB> i_eServiceHandlerDVB(7, "eServiceHandlerDVB");
