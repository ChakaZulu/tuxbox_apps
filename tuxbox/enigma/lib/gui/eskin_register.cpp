#include "eskin_register.h"
#include "config.h"

#include <core/gui/eskin.h>
#include <core/gdi/gfbdc.h>
#include <core/system/init.h>



class eSkinInit
{
	eSkin default_skin;
public:
	eSkinInit()
	{
		if (default_skin.load( DATADIR "/enigma/skins/default.esml"))
			eFatal("skin load failed (" DATADIR "/enigma/skins/default.esml)");
		if (default_skin.load( DATADIR "/enigma/skins/neutrino.esml"))
			eDebug("skin load failed (" DATADIR "/enigma/skins/neutrino.esml)");
/*		if (default_skin.load( DATADIR "/enigma/skins/dream.esml"))
			qWarning("skin load failed (" DATADIR "/enigma/skins/dream.esml)"); */
/*		if (default_skin.load( DATADIR "/enigma/skins/chkdesign.esml"))
			qWarning("skin load failed (" DATADIR "/enigma/skins/chkdesign.esml)"); */
		default_skin.setPalette(gFBDC::getInstance());
		default_skin.makeActive();
	}
};

eAutoInitP0<eSkinInit> init_skin(2, "skin subsystem");
