#include "satconfig.h"
#include "eskin.h"
#include "elistbox.h"
#include "ebutton.h"

eSatelliteConfigurationManager::eSatelliteConfigurationManager()
{
	list=new eListbox(this);
	list->setName("list");

	close=new eButton(this);
	close->setName("close");

	sat_new=new eButton(this);
	sat_new->setName("new");
	
	sat_delete=new eButton(this);
	sat_delete->setName("delete");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eSatelliteConfigurationManager"))
		qFatal("skin load of \"eSatelliteConfigurationManager\" failed");

	new eListboxEntryText(list, "test1");
	new eListboxEntryText(list, "test2");
	new eListboxEntryText(list, "test3");
	new eListboxEntryText(list, "test4");
	new eListboxEntryText(list, "test5");
}

eSatelliteConfigurationManager::~eSatelliteConfigurationManager()
{
}

void eSatelliteConfigurationManager::newSatellite()
{
}

void eSatelliteConfigurationManager::deleteSatellite()
{
}
