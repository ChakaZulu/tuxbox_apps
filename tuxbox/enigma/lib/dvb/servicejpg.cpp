#ifndef DISABLE_FILE
#ifdef PICVIEWER

#include <config.h>
#include <lib/dvb/servicejpg.h>
#include <lib/dvb/servicefile.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/base/i18n.h>
#include <lib/gdi/fb.h>
#include <lib/picviewer/pictureviewer.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

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

	eServiceReference sref = eServiceInterface::getInstance()->service;
	printf("[SERVICEJPG] show %s\n", sref.path.c_str());
	ePictureViewer::getInstance()->displayImage(sref.path);

	return 0;
}

int eServiceHandlerJPG::stop(int workaround)
{
	printf("[SERVICEJPG] stop.\n");

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

eAutoInitP0<eServiceHandlerJPG> i_eServiceHandlerJPG(eAutoInitNumbers::service + 2, "eServiceHandlerJPG");

#endif
#endif //DISABLE_FILE
