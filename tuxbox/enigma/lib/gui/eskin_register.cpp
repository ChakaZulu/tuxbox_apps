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
		if (default_skin.load( DATADIR "/enigma/skins/neutrino.esml"))
			qWarning("skin load failed (" DATADIR "/enigma/skins/neutrino.esml)");
/*		if (default_skin.load( DATADIR "/enigma/skins/dream.esml"))
			qWarning("skin load failed (" DATADIR "/enigma/skins/dream.esml)"); */
/*		if (default_skin.load( DATADIR "/enigma/skins/chkdesign.esml"))
			qWarning("skin load failed (" DATADIR "/enigma/skins/chkdesign.esml)"); */
		default_skin.setPalette(gFBDC::getInstance());
		default_skin.makeActive();
	}
};

eAutoInitP0<eSkinInit> init_skin(2, "skin subsystem");
