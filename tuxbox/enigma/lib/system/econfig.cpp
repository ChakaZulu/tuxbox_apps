#include <lib/base/eerror.h>
#include <lib/system/econfig.h>
#include <lib/system/init.h>
#include <sys/stat.h>

eConfig *eConfig::instance;

eConfig::eConfig()
{
	if (!instance)
		instance=this;

	setName(CONFIGDIR "/enigma/registry");
	int e=open();
	if (e == NC_ERR_CORRUPT)
	{
		eFatal("CORRUTPED REGISTRY!");
		::remove(CONFIGDIR "/enigma/registry");
	}
	if (e)
	{
		if (createNew())
		{
			mkdir(CONFIGDIR "/enigma", 0777);
			if (createNew())
				eFatal("error while opening/creating registry - create " CONFIGDIR "/enigma");
		}
		if (open())
			eFatal("still can't open configfile");
	}
}

eConfig::~eConfig()
{
	if (instance==this)
		instance=0;
	close();
}

eAutoInitP0<eConfig> init_eRCConfig(0, "Configuration");
