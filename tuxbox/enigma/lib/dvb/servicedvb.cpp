#define TIMESHIFT
		// intial time to lag behind live
#define TIMESHIFT_GUARD (1024*1024)
#include <lib/base/i18n.h>
#include <lib/dvb/servicedvb.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/frontend.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/info.h>
#include <lib/driver/streamwd.h>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/servicefile.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/decoder.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdio.h>

#if HAVE_DVB_API_VERSION < 3
#include <ost/audio.h>
#define AUDIO_DEV "/dev/dvb/card0/audio0"
#else
#include <linux/dvb/audio.h>
#define AUDIO_DEV "/dev/dvb/adapter0/audio0"
#endif

#ifndef DISABLE_FILE
#include <lib/dvb/record.h>

eDVRPlayerThread::eDVRPlayerThread(const char *_filename, eServiceHandlerDVB *handler, int livemode )
	:handler(handler), buffer(65424), livemode(livemode), liveupdatetimer(this)
	,inputsn(0), outputsn(0), messages(this, 1)
{
	state=stateInit;

	pthread_mutex_init(&poslock, 0);

	int count=0;
	seekbusy=0;
	seeking=0;
#if HAVE_DVB_API_VERSION < 3
	do
	{
		dvrfd=::open("/dev/pvr", O_WRONLY|O_NONBLOCK); // TODO: change to /dev/dvb/dvr0 (but only when drivers support this!)
		if (dvrfd < 0)
		{
			if ( errno == EBUSY )
			{
				eDebug("pvr device busy try %d", count++);
				if ( count < 40 )
				{
					usleep(20000);
					continue;
				}
			}
			eDebug("couldn't open /dev/pvr - buy the new $$$ box and load pvr.o! (%m)");
			state=stateError;
		}
		break;
	}
	while( dvrfd < 0 );
#else
	if ((dvrfd = ::open("/dev/dvb/adapter0/dvr0", O_WRONLY|O_NONBLOCK)) == -1) 
	{
		eDebug("couldn't open /dev/dvb/adapter0/dvr0 (%m)");
		state=stateError;
	}
#endif
	outputsn=new eSocketNotifier(this, dvrfd, eSocketNotifier::Write, 0);
	CONNECT(outputsn->activated, eDVRPlayerThread::outputReady);

	CONNECT(liveupdatetimer.timeout, eDVRPlayerThread::updatePosition);

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

	if (openFile(0))
	{
		state=stateError;
		eDebug("error opening %s (%m)", filename.c_str());
	}

	CONNECT(messages.recv_msg, eDVRPlayerThread::gotMessage);

	if ( eSystemInfo::getInstance()->getHwType() < 3 ) // dbox2
		maxBufferSize=128*1024;
	else
		maxBufferSize=256*1024; 

	speed=1;

	run();

	if (livemode)
	{
		int fileend;
		if (filelength > (TIMESHIFT_GUARD/1880))
			fileend = filelength - (TIMESHIFT_GUARD/1880);
		else
			fileend = 0;
		if ( livemode == 1 )
			messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::seekreal, fileend));
	}
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
	if (sourcefd >= 0)
	{
		int slicesize=this->slicesize;
		eDebug("opened slice %d", slice);
		if (!slice)
		{
			if (!livemode)
			{
				slicesize=lseek64(sourcefd, 0, SEEK_END);
				if (slicesize <= 0)
					slicesize=1024*1024*1024;
				lseek64(sourcefd, 0, SEEK_SET);
			}
			else
				slicesize=1024*1024*1024;
		}

		inputsn=new eSocketNotifier(this, sourcefd, eSocketNotifier::Read, 0);
		inputsn->start();
		CONNECT(inputsn->activated, eDVRPlayerThread::readMore);

		singleLock s(poslock);
		this->slicesize = slicesize;
		this->slice = slice;
		position=0;
		return 0;
	}
	return -1;
}

void eDVRPlayerThread::thread()
{
	exec();
}

void eDVRPlayerThread::outputReady(int what)
{
	(void)what;

#if 0
	// i don't know if this is better.. or not.. please check..
	// on dbox2 we write only blocks of 65424 bytes
	if ( eSystemInfo::getInstance()->getHwType() < 3	// dbox2
		&& buffer.size() < 65424		// no full block in buffer
		&& state != stateFileEnd )	// not eof
		return;
#endif

	seekbusy-=buffer.tofile(dvrfd,65424);

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
		if (state != stateFileEnd)
		{
			if (inputsn)
				inputsn->start();
			state=stateBuffering;
		} else
		{
			if (seeking)
			{
				Decoder::stopTrickmode();
				seeking=0;
			}
			if (!livemode)
			{
				eDebug("ok, everything played..");
				handler->messages.send(eServiceHandlerDVB::eDVRPlayerThreadMessage(eServiceHandlerDVB::eDVRPlayerThreadMessage::done));
			} else
			{
				eDebug("liveeof");
				handler->messages.send(eServiceHandlerDVB::eDVRPlayerThreadMessage(eServiceHandlerDVB::eDVRPlayerThreadMessage::liveeof));
			}
		}
	}
}

void eDVRPlayerThread::dvrFlush()
{
#if HAVE_DVB_API_VERSION < 3
	if ( ::ioctl(dvrfd, 0)< 0 )
		eDebug("PVR_FLUSH_BUFFER failed (%m)");
#endif
	Decoder::flushBuffer();
}

void eDVRPlayerThread::readMore(int what)
{
	(void)what;
	if ((state != statePlaying) && (state != stateBuffering))
	{
		eDebug("wrong state (%d)", state);
		return;
	}

	int flushbuffer=0;

	if (buffer.size() < maxBufferSize)
	{
		int rd = buffer.fromfile(sourcefd, maxBufferSize);

		if ( rd < maxBufferSize)
		{
			int next=1;
			if (livemode)
			{
				struct stat s;
				eString tfilename=filename;
				tfilename += eString().sprintf(".%03d", slice+1);

				if (::stat(tfilename.c_str(), &s) < 0)
				{
					eDebug("no next file, stateFileEnd");
					flushbuffer=1;
					if (inputsn)
						inputsn->stop();
					next=0;
				}
			}
			if (next && openFile(slice+1)) // if no next part found, else continue there...
				flushbuffer=1;
		}
		else
		{
			singleLock s(poslock);
			position+=rd;
		}
	}

	int bla = eSystemInfo::getInstance()->getHwType() < 3 ? 100000 : 16384;

	if ( (state == stateBuffering && buffer.size() > bla) || flushbuffer )
	{
		state=statePlaying;
		outputsn->start();
	}

	if (flushbuffer)
	{
		state=stateFileEnd;
		if (inputsn)
			inputsn->stop();
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
//	messages.send(eDVRPlayerThreadMessage(eDVRPlayerThreadMessage::setSpeed, 0));
	messages.send(eDVRPlayerThreadMessage(eDVRPlayerThreadMessage::exit));
	kill(); 			// join the thread

	int fd = Decoder::getAudioDevice();
	bool wasOpen = fd != -1;
	if (!wasOpen)
		fd = open(AUDIO_DEV, O_RDWR);
	if (ioctl(fd, AUDIO_SET_MUTE, 0) < 0)
		eDebug("AUDIO_SET_MUTE error (%m)");
	if (!wasOpen)
		close(fd);

	delete inputsn;
	delete outputsn;
	if (dvrfd >= 0)
		::close(dvrfd);
	if (sourcefd >= 0)
		::close(sourcefd);
	pthread_mutex_destroy(&poslock);
}

void eDVRPlayerThread::updatePosition()
{
	int slice=0;
	struct stat s;
	int filelength=0;
	while (!stat((filename + (slice ? eString().sprintf(".%03d", slice) : eString(""))).c_str(), &s))
	{
		filelength+=s.st_size/1880;
		slice++;
	}

	if (state == stateFileEnd)
	{
		eDebug("file end reached, retrying..");
		state = stateBuffering;
		if (inputsn)
			inputsn->start();
	}
	singleLock l(poslock);
	this->filelength=filelength;
}

int eDVRPlayerThread::getPosition(int real)
{
	singleLock s(poslock);
	int ret = (position / 1880) + slice * (slicesize/1880);
	if (!real)
		ret /= 250;
	return ret;
}

int eDVRPlayerThread::getLength(int real)
{
	singleLock s(poslock);
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
			inputsn->start();
		}
		livemode=message.parm;
		if (livemode)
			liveupdatetimer.start(1000);
		else
			liveupdatetimer.stop();
		break;
	case eDVRPlayerThreadMessage::exit:
		quit();
		break;
	case eDVRPlayerThreadMessage::setSpeed:
		if (!(inputsn && outputsn))
			break;
		speed=message.parm;
		if (message.parm == 0)
		{
			if ((state==stateBuffering) || (state==stateBufferFull) || (state==statePlaying))
			{
				inputsn->stop();
				outputsn->stop();
				state=statePause;
				Decoder::Pause();
			}
		}
		else if (state == statePause)
		{
			inputsn->start();
			outputsn->start();
			speed=message.parm;
			state=stateBuffering;
			Decoder::Resume();
		}
		else
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
			offset=10000; // assuming 3MBit bitrate...
			offset/=8;
			offset*=message.parm;
			buffer.clear();
			offset-=1000*1000; // account for pvr buffer
			if (message.type == eDVRPlayerThreadMessage::skip)
				offset+=position+slice*slicesize;
			if (offset<0)
				offset=0;
		} else
		{
			buffer.clear();
			offset=((off64_t)message.parm)*1880;
		}

		if ((offset / slicesize) != slice)
		{
			if (openFile(offset/slicesize))
			{
				eDebug("open slice %d failed\n", slice);
				state=stateError;
			}
		}

		if (state != stateError)
		{
			int newpos=::lseek64(sourcefd, offset%slicesize, SEEK_SET);
			dvrFlush();
			singleLock s(poslock);
			position=newpos;
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

void eServiceHandlerDVB::gotMessage(const eDVRPlayerThreadMessage &message)
{
	if (message.type == eDVRPlayerThreadMessage::done)
	{
		state=stateStopped;
		serviceEvent(eServiceEvent(eServiceEvent::evtEnd));
	}
	else if (message.type == eDVRPlayerThreadMessage::liveeof)
		stopPlayback(1);
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

void eServiceHandlerDVB::startPlayback(const eString &filename, int livemode)
{
	stopPlayback();
	decoder=new eDVRPlayerThread(filename.c_str(), this, livemode);
	decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::start, livemode));
	flags=flagIsSeekable|flagSupportPosition;
	state=statePlaying;
	if ( livemode == 2)
		flags|=flagStartTimeshift;
	pcrpid = Decoder::current.pcrpid;
		// stop pcrpid
	Decoder::parms.pcrpid = -1;
	Decoder::Set();
	serviceEvent(eServiceEvent(eServiceEvent::evtFlagsChanged) );
	if ( livemode == 2)
		flags&=~flagStartTimeshift;
}

void eServiceHandlerDVB::stopPlayback( int waslivemode )
{
	if (decoder)
	{
			// reenable pcrpid
		Decoder::parms.pcrpid = pcrpid;
		Decoder::Set();
		flags&=~(flagIsSeekable|flagSupportPosition);
		serviceEvent(eServiceEvent(eServiceEvent::evtFlagsChanged) );
		delete decoder;
		decoder=0;
	}
}

#endif //DISABLE_FILE

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

void eServiceHandlerDVB::switchedService(const eServiceReferenceDVB &s, int err)
{
	if ( !s )
		return;
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

void eServiceHandlerDVB::leaveService(const eServiceReferenceDVB &e)
{
	if ( e )
		serviceEvent(eServiceEvent(eServiceEvent::evtStop));
}

void eServiceHandlerDVB::aspectRatioChanged(int isanamorph)
{
	aspect=isanamorph;
	serviceEvent(eServiceEvent(eServiceEvent::evtAspectChanged));
}

eServiceHandlerDVB::eServiceHandlerDVB()
	:eServiceHandler(eServiceReference::idDVB),
#ifndef DISABLE_FILE
	messages(eApp, 0),
	decoder(0),
#endif
	cache(*this)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);

	CONNECT(eDVB::getInstance()->scrambled, eServiceHandlerDVB::scrambledStatusChanged);
	CONNECT(eDVB::getInstance()->switchedService, eServiceHandlerDVB::switchedService);
	CONNECT(eDVB::getInstance()->gotEIT, eServiceHandlerDVB::gotEIT);
	CONNECT(eDVB::getInstance()->gotSDT, eServiceHandlerDVB::gotSDT);
	CONNECT(eDVB::getInstance()->gotPMT, eServiceHandlerDVB::gotPMT);
	CONNECT(eDVB::getInstance()->leaveService, eServiceHandlerDVB::leaveService);
#ifndef DISABLE_FILE
	CONNECT(eDVB::getInstance()->eventOccured, eServiceHandlerDVB::handleDVBEvent);
#endif
	CONNECT(eStreamWatchdog::getInstance()->AspectRatioChanged, eServiceHandlerDVB::aspectRatioChanged);

	int data = 0xFFFFFFFF;
	data &= ~(1<<4);  // exclude NVOD
	data &= ~(1<<1);  // exclude TV
	data &= ~(1<<2);  // exclude Radio

	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1), 0xFFFFFFFF),
			new eService( _("Providers (TV)"), eService::spfColCombi)
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1<<2, 0xFFFFFFFF ),
			new eService( _("Providers (Radio)"), eService::spfColCombi)
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, data, 0xFFFFFFFF),
			new eService( _("Providers (Data)"), eService::spfColCombi)
		);

	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, (1<<4)|(1<<1), 0xFFFFFFFF ), // TV and NVOD
			new eService( _("All services (TV)"), eService::spfColMulti)
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 1<<2, 0xFFFFFFFF ), // radio
			new eService( _("All services (Radio)"), eService::spfColMulti)
		);
	cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, data, 0xFFFFFFFF),
			new eService( _("All services (Data)"), eService::spfColMulti)
		);

	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
	{
		cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -4, (1<<4)|(1<<1)),
			new eService( _("Satellites (TV)"), eService::spfColSingle)
		);
		cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -4, 1<<2),
			new eService( _("Satellites (Radio)"), eService::spfColSingle)
		);
		cache.addPersistentService(
			eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -4, data),
			new eService( _("Satellites (Data)"), eService::spfColSingle)
		);
}
#ifndef DISABLE_FILE
	CONNECT(eServiceFileHandler::getInstance()->fileHandlers, eServiceHandlerDVB::addFile);
	recording=0;
	CONNECT(messages.recv_msg, eServiceHandlerDVB::gotMessage);
#endif
}

eServiceHandlerDVB::~eServiceHandlerDVB()
{
#ifndef DISABLE_FILE
	if (recording)
		eDVB::getInstance()->recEnd();
#endif
	if (eServiceInterface::getInstance()->unregisterHandler(id)<0)
		eFatal("couldn't unregister serviceHandler %d", id);
}

int eServiceHandlerDVB::play(const eServiceReference &service, int workaround )
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (service.type != eServiceReference::idDVB)
		return -1;
//	int oldflags=flags;
#ifndef DISABLE_FILE
	if ( !workaround )
		decoder=0;

	if (service.path.length())
	{
		if ( !workaround )
		{
			FILE *f = fopen( service.path.c_str(), "r" );
			if (!f)
			{
				eDebug("file %s not exist.. don't play", service.path.c_str() );
				return -1;
			}
			else
				fclose(f);
			if ( eDVB::getInstance()->recorder &&
				service.path == eDVB::getInstance()->recorder->getFilename() )
				startPlayback(service.path, 2);
			else
				startPlayback(service.path, 0);
		}
	}
	else
#endif
	{
		flags &= ~(flagIsSeekable|flagSupportPosition);
//		if (flags != oldflags)
			serviceEvent(eServiceEvent(eServiceEvent::evtFlagsChanged) );
	}

	if (sapi)
		return sapi->switchService((const eServiceReferenceDVB&)service);

	return -1;
}

int eServiceHandlerDVB::serviceCommand(const eServiceCommand &cmd)
{
	switch (cmd.type)
	{
#ifndef DISABLE_FILE
	case eServiceCommand::cmdRecordOpen:
	{
		if (!recording)
		{
			char *filename=reinterpret_cast<char*>(cmd.parm);
			current_filename=filename;
			eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
			eDVB::getInstance()->recBegin(filename, sapi ? sapi->service : eServiceReferenceDVB());
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
#ifdef TIMESHIFT
//			stopPlayback();
#endif
			recording=0;
			eDVB::getInstance()->recEnd();
		} else
			return -1;
		break;
	case eServiceCommand::cmdSetSpeed:
	{
		int parm=cmd.parm;
		if (!decoder && recording)
		{
			if ( parm == -1 ) // pause with must seek to begin
			{
				parm=0;
				startPlayback(current_filename, 2);
			}
			else
				startPlayback(current_filename, 1);
		}
		if ((state == statePlaying) || (state == statePause) || (state == stateStopped) || (state == stateSkipping))
		{
			if (parm < 0 || !decoder)
				return -1;
			decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::setSpeed, cmd.parm));
			if (parm == 0)
				state=statePause;
			else if (parm == 1)
				state=statePlaying;
			else
				state=stateSkipping;
		}
		else
			return -2;
		break;
	}
	case eServiceCommand::cmdSkip:
		if (!decoder)
		{
#ifdef TIMESHIFT
			if (recording && (cmd.parm < 0) )
				startPlayback(current_filename, 1);
			else
#endif
				return -1;
		}
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::skip, cmd.parm));
		break;
	case eServiceCommand::cmdSeekAbsolute:
		if (!decoder)
			return -1;
		decoder->messages.send(eDVRPlayerThread::eDVRPlayerThreadMessage(eDVRPlayerThread::eDVRPlayerThreadMessage::seekreal, cmd.parm));
		if ( cmd.parm == 0 && state == statePause )
			state = stateStopped;
		break;
	case eServiceCommand::cmdSeekReal:
		if (!decoder)
			return -1;
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
#endif // DISABLE_FILE
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

int eServiceHandlerDVB::stop(int workaround)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

	if (sapi)
		sapi->switchService(eServiceReferenceDVB());

#ifndef DISABLE_FILE
	if (!workaround)
		stopPlayback();
#endif

	return 0;
}

#ifndef DISABLE_FILE
void eServiceHandlerDVB::addFile(void *node, const eString &filename)
{
	if (filename.right(3).upper()==".TS")
	{
		struct stat s;
		if (::stat(filename.c_str(), &s))
			return;
		eServiceFileHandler::getInstance()->addReference(node, eServiceReference(id, 0, filename));
	}
}
#endif

struct eServiceHandlerDVB_addService
{
	Signal1<void,const eServiceReference&> &callback;
	int type;
	int DVBNamespace;
//	int pLockActive;
	eServiceHandlerDVB_addService(Signal1<void,const eServiceReference&> &callback, int type, int DVBNamespace)
	: callback(callback), type(type), DVBNamespace(DVBNamespace)//, pLockActive(0)
	{
//		pLockActive = eConfig::getInstance()->pLockActive();
	}
	void operator()(const eServiceReference &service)
	{
		eService *s = eTransponderList::getInstance()->searchService( service );
		if ( !s )  // dont show "removed services"
			return;
		else if ( s->spflags & eServiceDVB::dxDontshow )
			return;
		int t = ((eServiceReferenceDVB&)service).getServiceType();
		int nspace = ((eServiceReferenceDVB&)service).getDVBNamespace().get()&0xFFFF0000;
		if (t < 0)
			t=0;
		if (t >= 31)
			t=31;
		if ( type & (1<<t) && // right dvb service type
				 ( ( DVBNamespace == (int)0xFFFFFFFF) || // ignore namespace
				 ( (DVBNamespace&(int)0xFFFF0000) == nspace ) // right satellite
				 )
			 )
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
			eTransponderList::getInstance()->forEachServiceReference(eServiceHandlerDVB_addService(callback, ref.data[1], ref.data[2] ));
			break;
		case -3:  // normal dvb bouquet
		{
			eBouquet *b=eDVB::getInstance()->settings->getBouquet(ref.data[2]);
			if (!b)
				break;
			b->forEachServiceReference(eServiceHandlerDVB_addService(callback, ref.data[1], ref.data[3] ));
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
#ifndef DISABLE_FILE
	if (node.data[0]>=0)
	{
		eString l=node.path.mid(node.path.rfind('/')+1);
		if (!isUTF8(l))
			l=convertLatin1UTF8(l);
		if (node.descr)
			l=node.descr;
		int fd=open(node.path.c_str(), O_RDONLY);
		if (fd < 0)
			return 0;
		__u8 packet[188];
		eServiceDVB *dvb=0;
		do
		{
			if (::read(fd, packet, 188) != 188)
				break;
				// i know that THIS is not really a SIT parser :)
			if ((packet[0] != 0x47) || (packet[1] != 0x40) || (packet[2] != 0x1f) || (packet[3] != 0x10))
				break;
			int nameoffset = 6;
			if (memcmp(packet+0x15, "ENIGMA", 6))
			{
				//failed so check another
				if (!memcmp(packet+0x15, "NEUTRINONG", 10))
					nameoffset = 10;
				else
					break;
			}
			// we found our private descriptor:
			__u8 *descriptor=packet+0x13;
			int len=descriptor[1];
			dvb=new eServiceDVB(eServiceID((packet[0xf]<<8)|packet[0x10]), l.c_str());
			len-=nameoffset;
			descriptor+=2+nameoffset; // skip tag, len, ENIGMA or NEUTRINONG
			for (int i=0; i<len; i+=descriptor[i+1]+2)
			{
				int tag=descriptor[i];
				switch (tag)
				{
				case eServiceDVB::cVPID:
					dvb->set(eServiceDVB::cVPID, (descriptor[i+2]<<8)|(descriptor[i+3]));
					break;
				case eServiceDVB::cAPID:
					if (descriptor[i+4] == 0)
						dvb->set(eServiceDVB::cAPID, (descriptor[i+2]<<8)|(descriptor[i+3]));
					else
						dvb->set(eServiceDVB::cAC3PID, (descriptor[i+2]<<8)|(descriptor[i+3]));
					break;
				case eServiceDVB::cTPID:
					dvb->set(eServiceDVB::cTPID, (descriptor[i+2]<<8)|(descriptor[i+3]));
					break;
				case eServiceDVB::cPCRPID:
					dvb->set(eServiceDVB::cPCRPID, (descriptor[i+2]<<8)|(descriptor[i+3]));
					break;
				}
			}
		} while (0);
		close(fd);
		if (!dvb)
			return new eService(l);
		return dvb;
	}
#endif // DISABLE_FILE
	switch (node.data[0])
	{
	case -1: // for satellites...
	{
		std::map<int,tpPacket>::const_iterator it( eTransponderList::getInstance()->getNetworkNameMap().find( node.data[2] >> 16 ));
		if ( it == eTransponderList::getInstance()->getNetworkNameMap().end() )
			return 0;
		else
			return new eService( it->second.name+ _(" - provider"), eService::spfColCombi);
	}
	case -2: // for satellites...
	{
		std::map<int,tpPacket>::const_iterator it( eTransponderList::getInstance()->getNetworkNameMap().find( node.data[2] >> 16 ));
		if ( it == eTransponderList::getInstance()->getNetworkNameMap().end() )
			return 0;
		else
			return new eService( it->second.name+ _(" - services"), eService::spfColMulti);
	}
	case -3:
	{
		eBouquet *b=eDVB::getInstance()->settings->getBouquet(node.data[2]);
		if (!b)
			return 0;
		return new eService(b->bouquet_name.c_str(), eService::spfColCombi);
	}
	}
	return 0;
}

struct eServiceHandlerDVB_SatExist
{
	std::set<int> &existingSats;
	eServiceHandlerDVB_SatExist(std::set<int> &existSats)
	: existingSats(existSats)
	{
	}
	void operator()(const eTransponder &tp)
	{
		if ( tp.satellite.isValid() )
			existingSats.insert(tp.satellite.orbital_position);
	}
};

void eServiceHandlerDVB::loadNode(eServiceCache<eServiceHandlerDVB>::eNode &node, const eServiceReference &ref)
{
	int data = 0xFFFFFFFF;
	data &= ~(1<<4);  // exclude NVOD
	data &= ~(1<<1);  // exclude TV
	data &= ~(1<<2);  // exclude Radio
	switch (ref.type)
	{
	case eServiceReference::idStructure:
		switch (ref.data[0])
		{
		case eServiceStructureHandler::modeBouquets:
			break;
		case eServiceStructureHandler::modeRoot:
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -4, (1<<4)|(1<<1) ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -4, 1<<2 ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -4, data ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, (1<<4)|(1<<1), 0xFFFFFFFF ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 1<<2, 0xFFFFFFFF ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, data, 0xFFFFFFFF));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1), 0xFFFFFFFF ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1<<2, 0xFFFFFFFF ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, data, 0xFFFFFFFF));
			break;
		case eServiceStructureHandler::modeTvRadio:
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1), 0xFFFFFFFF ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1<<2, 0xFFFFFFFF ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, (1<<4)|(1<<1), 0xFFFFFFFF ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 1<<2, 0xFFFFFFFF ));
			break;
		case eServiceStructureHandler::modeTV:
			if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
				cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -4, (1<<4)|(1<<1) ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, (1<<4)|(1<<1), 0xFFFFFFFF ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1), 0xFFFFFFFF ));
			break;
		case eServiceStructureHandler::modeRadio:
			if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
				cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -4, 1<<2 ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, 1<<2, 0xFFFFFFFF ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1<<2, 0xFFFFFFFF ));
			break;
		case eServiceStructureHandler::modeData:
			if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )
				cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -4, data ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -2, data, 0xFFFFFFFF ));
			cache.addToNode(node, eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, data, 0xFFFFFFFF ));
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

				for ( std::list<eServiceReferenceDVB>::iterator s(i->list.begin()); s != i->list.end(); ++s)
				{
					int t = s->getServiceType();
					int nspace = ((eServiceReferenceDVB&)*s).getDVBNamespace().get()&0xFFFF0000;
					if (t < 0)
						t=0;
					if (t >= 31)
						t=31;
					if ( ref.data[1] & (1<<t) && // right dvb service type
							 ( ( ref.data[2] == (int)0xFFFFFFFF) || // ignore namespace
								 ( (ref.data[2]&(int)0xFFFF0000) == nspace ) // right satellite
							 )
						 )
						 {
							 cache.addToNode(node, eServiceReference(eServiceReference::idDVB, flags, -3, ref.data[1], i->bouquet_id, ref.data[2] ));
							 break;
						 }
				}
			}
			break;
		}
		case -4:  // handle Satellites
		{
			static int flags=eServiceReference::mustDescent|eServiceReference::canDescent|eServiceReference::isDirectory|eServiceReference::shouldSort;
			std::set<int> filledSats;
			eTransponderList::getInstance()->forEachTransponder( eServiceHandlerDVB_SatExist( filledSats ));
			for ( std::set<int>::iterator it( filledSats.begin()) ; it != filledSats.end(); it++ )
			{
				cache.addToNode(node, eServiceReference(eServiceReference::idDVB, flags, -1, ref.data[1], *it<<16 ));
				cache.addToNode(node, eServiceReference(eServiceReference::idDVB, flags, -2, ref.data[1], *it<<16 ));
			}
			break;
		}
	}
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

int eServiceHandlerDVB::getPosition(int what)
{
#ifndef DISABLE_FILE
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
#else
	return -1;
#endif
}

eAutoInitP0<eServiceHandlerDVB> i_eServiceHandlerDVB(eAutoInitNumbers::service+2, "eServiceHandlerDVB");
