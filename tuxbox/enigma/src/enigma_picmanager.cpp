#include <lib/base/i18n.h>
#include <lib/gui/elabel.h>
#include <lib/gui/enumber.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/eskin.h>
#include <lib/system/econfig.h>
#include <lib/gui/numberactions.h>
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include "enigma_picmanager.h"

struct PicViewerStyleSelectorActions
{
	eActionMap map;
	eAction infoPressed;
	PicViewerStyleSelectorActions():
		map("PicViewerStyleSelector", _("Picture Viewer Actions")),
		infoPressed(map, "infoPressed", _("open the Picture Viewer with selected style"), eAction::prioDialog)
	{
	}
};
eAutoInitP0<PicViewerStyleSelectorActions> i_PicViewerStyleSelectorActions(eAutoInitNumbers::actions, "Picture Viewer Style Selector");

ePicViewerStyleSelector::ePicViewerStyleSelector(int ssel)
		:eListBoxWindow<eListBoxEntryText>(_("Picture Viewer 1.1 - Actions"), 5, 350, true)
		,ssel(ssel)
{
	eListBoxEntrySeparator *sep;
	addActionMap(&i_PicViewerStyleSelectorActions->map);
	cmove(ePoint(100, 100));
	int last = 0;
	eConfig::getInstance()->getKey("/ezap/lastPicViewerStyle", last);
	eListBoxEntryText *sel[3];
	sel[0] = new eListBoxEntryText(&list,_("Slide"), (void *)0, 0, _("Show selected slide") );
	sel[1] = new eListBoxEntryText(&list,_("Slideshow"), (void *)1, 0, _("Show slideshow (of all pictures in directory)"));
	sep = new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	sel[2] = new eListBoxEntryText(&list,_("Settings"), (void *)2, 0, _("Customize your slideshow"));

	list.setCurrent(sel[last]);
	CONNECT(list.selected, ePicViewerStyleSelector::entrySelected);
}

int ePicViewerStyleSelector::eventHandler( const eWidgetEvent &event )
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

void ePicViewerStyleSelector::entrySelected( eListBoxEntryText* e )
{
	if (e)
	{
		if ((int)e->getKey() < 2)
		{
			eConfig::getInstance()->setKey("/ezap/lastPicViewerStyle", (int)e->getKey());
			close( (int)e->getKey() );
		}
		else
		{
			ePicViewerSettings s;
			s.show();
			s.exec();
			s.hide();
		}
	}
	else
		close(-1);
}

ePicViewerSettings::ePicViewerSettings():eWindow(0)
{
	int slideshowtimeout = 5;
	eConfig::getInstance()->getKey("/picviewer/slideshowtimeout", slideshowtimeout);
	int sortpictures = 1;
	eConfig::getInstance()->getKey("/picviewer/sortpictures", sortpictures);
	int wraparound = 1;
	eConfig::getInstance()->getKey("/picviewer/wraparound", wraparound);
	int startwithselectedpic = 0;
	eConfig::getInstance()->getKey("/picviewer/startwithselectedpic", startwithselectedpic);
	int includesubdirs = 0;
	eConfig::getInstance()->getKey("/picviewer/includesubdirs", includesubdirs);

	int fd = eSkin::getActive()->queryValue("fontsize", 20);

	setText(_("Slideshow Settings"));
	cmove(ePoint(100, 80));

	int y = 10, dy = 35, h = fd + 6;


	eLabel *l = new eLabel(this);
	l->setText(_("Timeout"));
	l->move(ePoint(10, y));
	l->resize(eSize(100, h));

	timeout = new eListBox<eListBoxEntryText>(this, l);
	timeout->loadDeco();
	timeout->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	timeout->move(ePoint(110, y));
	timeout->resize(eSize(35, 34));
	eListBoxEntryText* entries[30];
	for (int i = 0; i < 30; i++)
	{
		eString num = eString().sprintf("%d", i + 1);
		entries[i] = new eListBoxEntryText(timeout, num.c_str(), (void *)new eString(num.c_str()));
	}
	timeout->setCurrent(entries[slideshowtimeout - 1]);
	timeout->setHelpText(_("select slideshow timeout (left, right)"));

	y += dy;


	sort = new eCheckbox(this, sortpictures, 1);
	sort->setText(_("Sort pictures"));
	sort->move(ePoint(10, y));
	sort->resize(eSize(300, h));
	sort->setHelpText(_("sort pictures alphabetically"));

	y += dy;

	wrap = new eCheckbox(this, wraparound, 1);
	wrap->setText(_("Wrap around"));
	wrap->move(ePoint(10, y));
	wrap->resize(eSize(300, h));
	wrap->setHelpText(_("Wrap around at end of slideshow"));

	y += dy;

	start = new eCheckbox(this, startwithselectedpic, 1);
	start->setText(_("Start with selected picture"));
	start->move(ePoint(10, y));
	start->resize(eSize(300, h));
	start->setHelpText(_("Start slideshow with selected picture"));

	y += dy;

	subdirs = new eCheckbox(this, includesubdirs, 1);
	subdirs->setText(_("Include subdirectories"));
	subdirs->move(ePoint(10, y));
	subdirs->resize(eSize(300, h));
	subdirs->setHelpText(_("Include subdirectories in slideshow"));

	y += dy + 20;

	ok = new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(10, y));
	ok->resize(eSize(130, h));
	ok->setHelpText(_("close window and save entries"));
	ok->loadDeco();
	CONNECT(ok->selected, ePicViewerSettings::okPressed);

	abort = new eButton(this);
	abort->loadDeco();
	abort->setText(_("abort"));
	abort->setShortcut("red");
	abort->setShortcutPixmap("red");
	abort->move(ePoint(130 + 10 + 10, y));
	abort->resize(eSize(130, h));
	abort->setHelpText(_("close window (no changes are saved)"));
	CONNECT(abort->selected, ePicViewerSettings::abortPressed);

	y = y + 40 + 35;
	cresize(eSize(350, y));

	statusbar = new eStatusBar(this);
	statusbar->move(ePoint(0, clientrect.height() - 30 ) );
	statusbar->resize(eSize( clientrect.width(), 30) );
	statusbar->loadDeco();
}

ePicViewerSettings::~ePicViewerSettings()
{
}

void ePicViewerSettings::fieldSelected(int *number)
{
	focusNext(eWidget::focusDirNext);
}

void ePicViewerSettings::okPressed()
{
	eConfig::getInstance()->setKey("/picviewer/slideshowtimeout", atoi(((eString *)timeout->getCurrent()->getKey())->c_str()));
	eConfig::getInstance()->setKey("/picviewer/sortpictures", (int)sort->isChecked());
	eConfig::getInstance()->setKey("/picviewer/wraparound", (int)wrap->isChecked());
	eConfig::getInstance()->setKey("/picviewer/startwithselectedpic", (int)start->isChecked());
	eConfig::getInstance()->setKey("/picviewer/includesubdirs", (int)subdirs->isChecked());

	close(1);
}

void ePicViewerSettings::abortPressed()
{
	close(0);
}

