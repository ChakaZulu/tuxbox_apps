#ifndef __APPS_ENIGMA_CHANNELINFO__
#define __APPS_ENIGMA_CHANNELINFO__

#include <core/gui/ewidget.h>
#include <core/gui/elabel.h>
#include <core/dvb/si.h>
#include <core/dvb/dvb.h>
#include <core/dvb/service.h>

class eChannelInfo : public eDecoWidget
{	
	eLabel ctime, cname, cdescr, cdolby, cstereo, cformat, cscrambled;

  eString name, descr, starttime;
	int cflags;

	static const char *genresTableShort[];

	void getServiceInfo( const eServiceReferenceDVB& );
	int LayoutIcon(eLabel *, int,  int );
	void ParseEITInfo(EITEvent *e);
	eServiceReferenceDVB current;

	void redrawWidget(gPainter *, const eRect &);
	int eventHandler(const eWidgetEvent &event);
	EIT *eit;
	void EITready(int err);
	void closeEIT();
public:
	enum
	{
		cflagStereo = 1,
		cflagDolby = 2,
		cflagWide = 4,
		cflagScrambled = 8
	};

	eChannelInfo( eWidget*, const char* deco="eStatusBar" );
	~eChannelInfo()
	{
		closeEIT();
	}
	void update( const eServiceReferenceDVB& );
	void clear();
};

#endif // __APPS_ENIGMA_CHANNELINFO__

