#ifndef __satconfig_h
#define __satconfig_h

#include "ewindow.h"
class eListbox;
class eButton;

class eSatelliteConfiguration: public eWindow
{
public:
	eSatelliteConfiguration(int sat);
};

class eSatelliteConfigurationManager: public eWindow
{
	Q_OBJECT
	eListbox *list;
	eButton *close, *sat_new, *sat_delete;
public slots:
	void newSatellite();
	void deleteSatellite();
public:
	eSatelliteConfigurationManager();
	~eSatelliteConfigurationManager();
};

#endif
