#ifdef PICVIEWER

#include <algorithm>
#include <list>

#include <enigma.h>
#include <enigma_main.h>
#include <picviewer.h>

#include <lib/base/i18n.h>
#include <lib/driver/rc.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/service.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/serviceplaylist.h>
#include <lib/dvb/servicejpg.h>
#include <lib/gdi/font.h>
#include <lib/gui/actions.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include <lib/gui/numberactions.h>
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/base/message.h>
#include <lib/picviewer/pictureviewer.h>

struct PicViewerStyleSelectorActions
{
	eActionMap map;
	eAction infoPressed;
	PicViewerStyleSelectorActions():
		map("PicViewerStyleSelector", _("Picture Viewer Style Selector")),
		infoPressed(map, "infoPressed", _("open the picture with selected style"), eAction::prioDialog)
	{
	}
};
eAutoInitP0<PicViewerStyleSelectorActions> i_PicViewerStyleSelectorActions(eAutoInitNumbers::actions, "Picture Viewer Style Selector actions");

ePicViewerStyleSelector::ePicViewerStyleSelector(int ssel)
		:eListBoxWindow<eListBoxEntryText>(_("Picture Viewer Style"), 5, 350, true)
		,ssel(ssel)
{
	addActionMap(&i_PicViewerStyleSelectorActions->map);
	move(ePoint(100, 100));
	int last=1;

	eListBoxEntryText *sel[3];
	sel[0] = new eListBoxEntryText(&list,_("Show Picture"), (void *)1, 0, _("show this picture"));
	sel[1] = new eListBoxEntryText(&list,_("Show Slideshow"), (void *)2, 0, _("show slideshow of all pictures in this directory"));

	list.setCurrent(sel[last-1]);
	CONNECT(list.selected, ePicViewerStyleSelector::entrySelected);
}

int ePicViewerStyleSelector::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
			if (event.action == &i_PicViewerStyleSelectorActions->infoPressed)
				entrySelected(list.getCurrent());
			else
				break;
			return 1;
		default:
			break;
	}
	return eWindow::eventHandler(event);
}

void ePicViewerStyleSelector::entrySelected(eListBoxEntryText* e)
{
	if (e)
	{
		int selection = (int)e->getKey();
		switch(selection)
		{
			case 1: printf("[PICVIEWER] show slide now...\n");
//				ePictureViewer::getInstance()->displayImage("picture.jpg");
				break;
			case 2:
				printf("[PICVIEWER] show slideshow now...\n");
				/*
				for (all pics in directory)
					ePictureViewer::getInstance()->displayImage("picture.jpg");
				*/
				break;
		}
	}
	close(-1);
}
#endif

