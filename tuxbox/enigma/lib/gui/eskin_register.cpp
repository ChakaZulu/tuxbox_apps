#include "eskin_register.h"
#include "eskin.h"
#include "gfbdc.h"
#include "init.h"
#include "config.h"

class eSkinInit
{
	eSkin default_skin;
public:
	eSkinInit()
	{
		if (default_skin.load( DATADIR "/enigma/skins/default.esml"))
			qFatal("skin load failed (" DATADIR "/enigma/skins/default.esml)");
		default_skin.setPalette(gFBDC::getInstance());
		default_skin.makeActive();
	}
};

eAutoInitP0<eSkinInit, 2> init_skin("skin subsystem");
