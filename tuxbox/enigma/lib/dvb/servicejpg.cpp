#ifndef DISABLE_FILE

#include <config.h>
#include <lib/dvb/servicejpg.h>
#include <lib/dvb/servicefile.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/base/i18n.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

#include <src/picviewer.h>

ePicViewerStyleSelector e(1);

eServiceHandlerJPG::eServiceHandlerJPG(): eServiceHandler(0x2000)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this) < 0)
		eFatal("couldn't register serviceHandler %d", id);
	CONNECT(eServiceFileHandler::getInstance()->fileHandlers, eServiceHandlerJPG::addFile);
}

eServiceHandlerJPG::~eServiceHandlerJPG()
{
	eServiceInterface::getInstance()->unregisterHandler(id);
}

void eServiceHandlerJPG::addFile(void *node, const eString &filename)
{
	if (filename.right(4).upper() == ".JPG")
	{
		struct stat s;
		if (!(::stat(filename.c_str(), &s)))
		{
			eServiceReference ref(id, 0, filename);
			eString name = filename;
			while (int pos = name.find("/") != eString::npos)
				name = name.mid(pos + 1, name.length() - pos - 1);
			ref.descr = name;
			eServiceFileHandler::getInstance()->addReference(node, ref);
		}
	}
}

int eServiceHandlerJPG::play(const eServiceReference &service, int workaround )
{
	printf("[SERVICEJPG] start...\n");

//	state = statePlaying;

//	flags = flagIsSeekable|flagSupportPosition;
//	flags |= flagIsTrack;

//	serviceEvent(eServiceEvent(eServiceEvent::evtStart));
//	serviceEvent(eServiceEvent(eServiceEvent::evtFlagsChanged) );

#ifndef DISABLE_LCD
//	e.setLCD(LCDTitle, LCDElement);
#endif
	e.show();
	int ret = e.exec();
//	e.hide();
	switch (ret)
	{
		case 1:
			printf("[SERVICEJPG] show slide now...\n");
//			showSlide((eServiceReferenceDVB&)selected);
			break;
		case 2:
			printf("[SERVICEJPG] show slideshow now...\n");
//			showSlideShow((eServiceReferenceDVB&)selected);
			break;
		default:
			break;
	}

	return 0;
}

int eServiceHandlerJPG::stop(int workaround)
{
	if (!workaround)
	{
		e.hide();
		printf("[SERVICEJPG] stop.\n");
//		removeRef(runningService);
//		runningService=eServiceReference();
	}
//	serviceEvent(eServiceEvent(eServiceEvent::evtStop));
	return 0;
}

eService *eServiceHandlerJPG::addRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->addRef(service);
}

void eServiceHandlerJPG::removeRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->removeRef(service);
}

eAutoInitP0<eServiceHandlerJPG> i_eServiceHandlerJPG(eAutoInitNumbers::service+2, "eServiceHandlerJPG");

#endif //DISABLE_FILE
