#ifndef __satconfig_h
#define __satconfig_h

#include <core/gui/listbox.h>

class eButton;

class eSatelliteConfiguration: public eWindow
{
public:
	eSatelliteConfiguration(int sat);
};

class eSatelliteConfigurationManager: public eWindow
{
	eButton *button_close;
	int eventFilter(const eWidgetEvent &event);
public:
	void okPressed();
public:
	eSatelliteConfigurationManager();
	~eSatelliteConfigurationManager();
};

#endif
