#include "econfig.h"
#include "init.h"
#include "config.h"
#include <sys/stat.h>
#include "qglobal.h"

eConfig *eConfig::instance;

eConfig::eConfig()
{
	if (!instance)
		instance=this;

	setName(CONFIGDIR "/enigma/registry");
	if (open())
	{
		if (createNew())
		{
			mkdir(CONFIGDIR "/enigma", 0777);
			if (createNew())
				qFatal("error while opening/creating registry - create " CONFIGDIR "/enigma");
		}
		if (open())
			qFatal("still can't open configfile");
	}
}

eConfig::~eConfig()
{
	if (instance==this)
		instance=0;
	close();
}

eAutoInitP0<eConfig> init_eRCConfig(0, "Configuration");
