#include <lib/dvb/servicedvb.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvbservice.h>
#include <lib/system/init.h>
#include <lib/driver/streamwd.h>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/servicefile.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/decoder.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

eDVRPlayerThread::eDVRPlayerThread(const char *_filename, eServiceHandlerDVB *handler): handler(handler), buffer(64*1024), lock(), messages(this, 1)
{
	state=stateInit;

	seekbusy=0;
	seeking=0;
	dvrfd=::open("/dev/pvr", O_WRONLY|O_NONBLOCK);		// TODO: change to /dev/dvb/dvr0 (but only when drivers support this!)
	if (dvrfd<0)
	{
		eDebug("couldn't open /dev/pvr - buy the new $$$ box and load pvr.o! (%m)");
		state=stateError;
	}
	
	outputsn=new eSocketNotifier(this, dvrfd, eSocketNotifier::Write, 0);
	CONNECT(outputsn->activated, eDVRPlayerThread::outputReady);
	
	filename=_filename;
	
	sourcefd=-1;
	inputsn=0;
	
	slice=0;
	struct stat s;
	filelength=0;
	while (!stat((filename + (slice ? eString().sprintf(".%03d", slice) : eString(""))).c_str(), &s))
	{
		filelength+=s.st_size/1880;
		slice++;
	}
		
	if (openFile(slice=0))
	{
		state=stateError;
		eDebug("error opening %s (%m)", filename.c_str());
	}

	CONNECT(messages.recv_msg, eDVRPlayerThread::gotMessage);
	
	maxBufferSize=256*1024;
	
	speed=1;

	run();
}

int eDVRPlayerThread::openFile(int slice)
{
	eString tfilename=filename;
	if (slice)
		tfilename += eString().sprintf(".%03d", slice);

	if (inputsn)
	{
		delete inputsn;
		inputsn=0;
	}
	if (sourcefd >= 0)
		::close(sourcefd);

	sourcefd=::open(tfilename.c_str(), O_RDONLY|O_LARGEFILE);
	if (!slice)
	{
		slicesize=lseek64(sourcefd, 0, SEEK_END);
		if (slicesize <= 0)
			slicesize=1024*1024;
		lseek64(sourcefd, 0, SEEK_SET);
	}
	if (sourcefd >= 0)
	{
		inputsn=new eSocketNotifier(this, sourcefd, eSocketNotifier::Read, 0);
		inputsn->start();
		CONNECT(inputsn->activated, eDVRPlayerThread::readMore);
		return 0;
	}
	return -1;
}

void eDVRPlayerThread::thread()
{
	lock.lock();
	messages.start();
	exec();
	lock.unlock();
}

void eDVRPlayerThread::outputReady(int what)
{
	seekbusy-=buffer.tofile(dvrfd, 65536);
	if (seekbusy < 0)
		seekbusy=0;
	
	if ((state == stateBufferFull) && (buffer.size()<maxBufferSize))
	{
		state=statePlaying;
		if (inputsn)
			inputsn->start();
	}
	if (buffer.empty())
	{
		eDebug("buffer empty, state %d", state);
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
	::ioctl(dvrfd, 0); // PVR_FLUSH_BUFFER
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
		{
			if (openFile(++slice)) // if no next part found, else continue there...
				flushbuffer=1;
		}
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
	}
	
	if ((state == statePlaying) && (buffer.size() >= maxBufferSize))
	{
		state=stateBufferFull;
		if (inputsn)
			inputsn->stop();
	}
}

eDVRPlayerThread::~eDVRPlayerThread()
{
	lock.lock();		// wait for message loop exit
	kill(); // join the thread

	if (inputsn)
		delete inputsn;
	delete outputsn;
	if (dvrfd >= 0)
		::close(dvrfd);
	if (sourcefd >= 0)
		::close(sourcefd);
}

int eDVRPlayerThread::getPosition(int real)
{
	eLocker l(poslock);
	if (real)
		return ((::lseek(sourcefd, 0, SEEK_CUR)-buffer.size()) / 1880) + slice*(slicesize/1880);
	return (((::lseek(sourcefd, 0, SEEK_CUR)-buffer.size()) / 1880) + slice*(slicesize/1880)) / 250;
}

int eDVRPlayerThread::getLength(int real)
{
	eLocker l(poslock);
	if (real)
		return filelength;
	return filelength/250;
}

void eDVRPlayerThread::gotMessage(const eDVRPlayerThreadMessage &message)
{
	switch (message.type)
	{
	case eDVRPlayerThreadMessage::start:
		if (!(inputsn && outputsn))
			break;
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
		if (!(inputsn && outputsn))
			break;
		speed=message.parm;
		if (message.parm == 0)
		{
			if ((state==stateBuffering) || (state==stateBufferFull) || (statePlaying))
			{
				inputsn->stop();
				outputsn->stop();
				state=statePause;
				Decoder::Pause();
			}
		} else if (state == statePause)
		{
			eDebug("resume");
			inputsn->start();
			outputsn->start();
			speed=message.parm;
			state=stateBuffering;
			Decoder::Resume();
		} else
		{
			buffer.clear();
			dvrFlush();
		}
		break;
	case eDVRPlayerThreadMessage::seekmode:
		if (!(inputsn && outputsn))
			break;
		switch (message.parm)
		{
		case 0:
			if (seeking)
				Decoder::stopTrickmode();
			seeking=0;
			break;
		case 1:
			if (!seeking)
				Decoder::startTrickmode();
			seeking=1;
			break;
		}
		break;
	case eDVRPlayerThreadMessage::seek:
	case eDVRPlayerThreadMessage::skip:
	case eDVRPlayerThreadMessage::seekreal:
	{
		if (!(inputsn && outputsn))
			break;
		if (seekbusy)
			break;
		seekbusy=256*1024; // next seek only after 128k (video) data
		off64_t offset=0;
		if (message.type != eDVRPlayerThreadMessage::seekreal)
		{
			int br=10000; // assuming 3MBit bitrate...
			br/=8;
			
			br*=message.parm;
			offset=-(buffer.size()+1000*1000); // account for pvr buffer
			buffer.clear();
			offset+=br;
			if (message.type == eDVRPlayerThreadMessage::skip)
				offset+=lseek64(sourcefd, 0, SEEK_CUR)+slice*slicesize;
			if (offset<0)
				offset=0;
		} else
		{
			buffer.clear();
			offset=((off64_t)message.parm)*1880;
		}
		
		if ((offset / slicesize) != slice)
		{
			slice=offset/slicesize;
			if (openFile(slice))
				state=stateError;
		}
		
		if (state != stateError)
		{
			::lseek64(sourcefd, offset%slicesize, SEEK_SET);
			dvrFlush();
			buffer.clear();
		}
		
		if (state == statePlaying)
		{
			if (inputsn)
				inputsn->start();
			state=stateBuffering;
		}
		
		break;
	}
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

void eServiceHandlerDVB::handleDVBEvent( const eDVBEvent & e )
{
	switch ( e.type )
	{
		case eDVBEvent::eventRecordWriteError:
			serviceEvent(eServiceEvent(eServiceEvent::evtRecordFailed));
		break;
	}
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

eServiceHandlerDVB::eServiceHandlerDVB()
	:eServiceHandler(eServiceReference::idDVB), messages(eApp, 0), decoder(0), flags(0), cache(*this)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);

	CONNECT(eDVB::getInstance()->scrambled, eServiceHandlerDVB::scrambledStatusChanged);
	CONNECT(eDVB::getInstance()->switchedService, eServiceHandlerDVB::switchedService);
	CONNECT(eDVB::getInstance()->gotEIT, eServiceHandlerDVB::gotEIT);
	CONNECT(eDVB::getInstance()->gotSDT, eServiceHandlerDVB::gotSDT);
	CONNECT(eDVB::getInstance()->gotPMT, eServiceHandlerDVB::gotPMT);
	CONNECT(eDVB::getInstance()->leaveService, eServiceHandlerDVB::leaveService);
	CONNECT(eDVB::getInstance()->eventOccured, eServiceHandlerDVB::handleDVBEvent);
	CONNECT(eStreamWatchdog::getInstance()->AspectRatioChanged, eServiceHandlerDVB::aspectRatioChanged);

	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 0xFFFFFFFF),
			new eService("DVB - bouquets")
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1) ),
			new eService("DVB - bouquets (TV)")
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1<<2 ),
			new eService("DVB - bouquets (Radio)")
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 0xFFFFFFFF), 
			new eService("DVB - all services")
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, (1<<4)|(1<<1) ), // TV and NVOD
			new eService("DVB - TV services")
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 1<<2 ), 	// radio
			new eService("DVB - Radio services")
		);
		
	recording=0;
	CONNECT(messages.recv_msg, eServiceHandlerDVB::gotMessage);
	messages.start();
}

eServiceHandlerDVB::~eServiceHandlerDVB()
{
	if (recording)
		eDVB::getInstance()->recEnd();
	if (eServiceInterface::getInstance()->unregisterHandler(id)<0)
		eFatal("couldn't unregister serviceHandler %d", id);
}


int eServiceHandlerDVB::play(const eServiceReference &service)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (service.type != eServiceReference::idDVB)
		return -1;
//	int oldflags=flags;
	if (service.path.length())
	{
		decoder=new eDVRPlayerThread(service.path.c_str(), this);
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::start));
		flags|=flagIsSeekable|flagSupportPosition;
	} else
	{
		decoder=0;
		flags &= ~(flagIsSeekable|flagSupportPosition);
	}
//	if (oldflags != flags)
		serviceEvent(eServiceEvent(eServiceEvent::evtFlagsChanged) );
	if (sapi)
	{
		eDebug("play -> switchService");
		return sapi->switchService((const eServiceReferenceDVB&)service);
	}
	return -1;
}

int eServiceHandlerDVB::serviceCommand(const eServiceCommand &cmd)
{
	switch (cmd.type)
	{
	case eServiceCommand::cmdRecordOpen:
	{
		if (!recording)
		{
			char *filename=reinterpret_cast<char*>(cmd.parm);
			eDVB::getInstance()->recBegin(filename);
			delete[] (filename);
			recording=1;
		} else
			return -1;
		break;
	}
	case eServiceCommand::cmdRecordStart:
		if (recording == 1)
		{
			eDVB::getInstance()->recResume();
			recording=2;
		} else	
			return -1;
		break;
	case eServiceCommand::cmdRecordStop:
		if (recording == 2)
		{
			eDVB::getInstance()->recPause();
			recording=1;
		} else
			return -1;
		break;
	case eServiceCommand::cmdRecordClose:
		if (recording)
		{
			recording=0;
			eDVB::getInstance()->recEnd();
		} else
			return -1;
		break;
	case eServiceCommand::cmdSetSpeed:
		eDebug("eServiceCommand::cmdSetSpeed");
		if (!decoder)
			return -1;
		eDebug("decoder exist");
		if ((state == statePlaying) || (state == statePause) || (state == stateSkipping))
		{
			eDebug("state...");
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
			{
			eDebug("return -2");
			return -2;
			}
		break;
	case eServiceCommand::cmdSkip:
		if (!decoder)
			return -1;
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::skip, cmd.parm));
		break;
	case eServiceCommand::cmdSeekAbsolute:
		if (!decoder)
			return -1;
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::seek, cmd.parm));
		break;
	case eServiceCommand::cmdSeekReal:
		if (!decoder)
			return -1;
		eDebug("seekreal");
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::seekreal, cmd.parm));
		break;
	case eServiceCommand::cmdSeekBegin:
		if (!decoder)
			return -1;
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::seekmode, 1));
		break;
	case eServiceCommand::cmdSeekEnd:
		if (!decoder)
			return -1;
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::seekmode, 0));
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

	eDebug("eServiceHandlerDVB::stop()");

	if (sapi)
		sapi->switchService(eServiceReferenceDVB());
		
	if (decoder)
	{
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::exit));
		delete decoder;
		decoder=0;
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
		path=path.left(path.rfind('.'));
		if (node.descr)
			path=node.descr;
		if (!path)
			path="movie";

		return new eService(path.c_str());
	}
	switch (node.data[0])
	{
	case -3:
	{
		eBouquet *b=eDVB::getInstance()->settings->getBouquet(node.data[2]);
		if (!b)
			return 0;
		return new eService(b->bouquet_name.c_str());
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
    case eServiceStructureHandler::modeTvRadio:
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1) ));
      cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1<<2 ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, (1<<4)|(1<<1) ));
      cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 1<<2 ));
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

				if (i->bouquet_id >= 0) 		// sort only automatic generated services
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
	if ((service.data[0] < 0) || (service.path.length()))
	{
		eService *s=cache.addRef(service);
		if (s)
			return s;
		else
			return 0;
	} else
	{
		eTransponderList *tl=eTransponderList::getInstance();
		if (!tl)
			return 0;
		return tl->searchService(service);
	}
}

void eServiceHandlerDVB::removeRef(const eServiceReference &service)
{
	if ((service.data[0] < 0) || (service.path.length()))
	{
		cache.removeRef(service);
	}
}

void eServiceHandlerDVB::gotMessage(const eDVRPlayerThreadMessage &message)
{
	if (message.type == eDVRPlayerThreadMessage::done)
	{
		state=stateStopped;
		serviceEvent(eServiceEvent(eServiceEvent::evtEnd));
	}
}

int eServiceHandlerDVB::getPosition(int what)
{
	if (!decoder)
		return -1;
	switch (what)
	{
	case posQueryLength:
		return decoder->getLength(0);
	case posQueryCurrent:
		return decoder->getPosition(0);
	case posQueryRealLength:
		return decoder->getLength(1);
	case posQueryRealCurrent:
		return decoder->getPosition(1);
	default:
		return -1;
	}
}

eAutoInitP0<eServiceHandlerDVB> i_eServiceHandlerDVB(7, "eServiceHandlerDVB");
