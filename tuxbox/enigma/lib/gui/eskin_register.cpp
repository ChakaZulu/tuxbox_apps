#include "eskin_register.h"
#include "config.h"

#include <core/gui/eskin.h>
#include <core/gdi/gfbdc.h>
#include <core/system/init.h>
#include <core/system/econfig.h>

#define DEFAULTSKIN "chkv2.esml"

class eSkinInit
{
	eSkin default_skin;
public:
	eSkinInit()
	{
		if (default_skin.load( DATADIR "/enigma/skins/default.esml"))
			eFatal("skin load failed (" DATADIR "/enigma/skins/default.esml)");

		char *temp=DEFAULTSKIN;
		eConfig::getInstance()->getKey("/ezap/ui/skin", temp);

		eString skinfile=DATADIR "/enigma/skins/";
		skinfile+=temp;
				
		if (default_skin.load(skinfile.c_str()))
		{
			eWarning("failed to load user defined skin %s, falling back to " DEFAULTSKIN, skinfile.c_str());
			if (default_skin.load(DATADIR "/enigma/skins/" DEFAULTSKIN))
				eFatal("couldn't load fallback skin " DATADIR "/enigma/skins/" DEFAULTSKIN);
		}

		default_skin.setPalette(gFBDC::getInstance());
		default_skin.makeActive();
	}
};

eAutoInitP0<eSkinInit> init_skin(2, "skin subsystem");
