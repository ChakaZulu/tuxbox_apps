#include <helpwindow.h>
#include <xmltree.h>
#include <unistd.h>

#include <lib/base/i18n.h>
#include <lib/dvb/edvb.h>
#include <lib/gdi/font.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/guiactions.h>
#include <lib/system/init_num.h>
#include <lib/system/info.h>

struct enigmaHelpWindowActions
{
	eActionMap map;
	eAction close, up, down;
	enigmaHelpWindowActions():
		map("helpwindow", _("Help window")),
		close(map, "close", _("close the help window"), eAction::prioDialog),
		up(map, "up", _("scroll up"), eAction::prioDialogHi),
		down(map, "down", _("scroll down"), eAction::prioDialogHi)
	{
	}
};

eAutoInitP0<enigmaHelpWindowActions> i_helpwindowActions(eAutoInitNumbers::actions, "enigma helpwindow actions");

eHelpWindow::eHelpWindow(ePtrList<eAction> &parseActionHelpList, int helpID):
	eWindow(1), curPage(0)
{
	int xpos=60, ypos=0, labelheight, imgheight;

	scrollbar = new eProgress(this);
	scrollbar->setName("scrollbar");
	scrollbar->setStart(0);
	scrollbar->setPerc(100);

	visible = new eWidget(this);
	visible->setName("visible");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eHelpWindow"))
		eFatal("skin load of \"eHelpWindow\" failed");

	scrollbox = new eWidget(visible);
	scrollbox->move(ePoint(0, 0));
	scrollbox->resize(eSize(visible->width(), visible->height()*8));

	const std::set<eString> styles=eActionMapList::getInstance()->getCurrentStyles();

	int t = eSystemInfo::getInstance()->getHwType();
	const char *hwstr =
		(t==eSystemInfo::DM500)?"dreambox" :
		(t==eSystemInfo::DM5600)?"dreambox" :
		(t==eSystemInfo::DM5620)?"dreambox" :
		(t==eSystemInfo::DM7000)?"dreambox" :
		"dbox2";

	entryBeg.push_back(0);
	int pageend=visible->height();

	for ( ePtrList<eAction>::iterator it( parseActionHelpList.begin() ); it != parseActionHelpList.end() ; it++ )
	{
		std::map< eString, keylist >::iterator b;
		
		for (std::set<eString>::const_iterator si(styles.begin()); si != styles.end(); ++si)
		{
			b=it->keys.find(*si);
			if (b == it->keys.end())
				continue;

			keylist &keys = b->second;
			for ( keylist::iterator i( keys.begin() ); i != keys.end() ; i++ )
			{
				imgheight=0;
				if ( strstr( i->producer->getDescription(), hwstr ) )
				{
					if (i->picture)
					{
						gPixmap *image=eSkin::getActive()->queryImage(i->picture);

						if (image)
						{
							label = new eLabel(scrollbox);
							label->setFlags(eLabel::flagVCenter);
							label->move(ePoint(0, ypos));
							label->resize(eSize(xpos,image->y));
							label->setBlitFlags(BF_ALPHATEST);
							label->setPixmap(image);
							label->setPixmapPosition(ePoint((xpos-10)/2-image->x/2, 0));
						}
					}

					label = new eLabel(scrollbox);
					label->setFlags(eLabel::flagVCenter);
					label->setFlags(RS_WRAP);
					label->move(ePoint(xpos, ypos));
					label->resize(eSize(visible->width()-xpos-20, 200));
							// since they are inited before language is set,
							// call gettext again.
					label->setText(gettext(it->getDescription()));
					labelheight=label->getExtend().height();
					label->resize(eSize(visible->width()-xpos-20, labelheight));

					ypos+=(labelheight>imgheight?labelheight:imgheight)+20;
					if ( ypos-20 > pageend )
					{
						entryBeg.push_back(ypos-(labelheight>imgheight?labelheight:imgheight)-20);
						pageend=entryBeg.back()+visible->height();
					}
					break;  // add only once :)
				}
			}
		}
	}

	if (helpID)
	{
		eString helptext=loadHelpText(helpID);

		label = new eLabel(scrollbox);
		label->setFlags(eLabel::flagVCenter);
		label->setFlags(RS_WRAP);
		label->move(ePoint(0, ypos));
		label->resize(eSize(visible->width(), 200));
		label->setText(helptext);
		labelheight = label->getExtend().height();
		label->resize(eSize(visible->width(), labelheight));
		int tmp = ypos+labelheight;
		while ( tmp > pageend )
		{
			entryBeg.push_back(ypos);
			pageend=entryBeg.back()+visible->height();
			ypos+=visible->height();
		}
	}

	cur = entryBeg.begin();
	doscroll=ypos>visible->height();

	if (!doscroll)
		scrollbar->hide();
	else
	  updateScrollbar();
	  
	addActionMap(&i_helpwindowActions->map);
}

eString eHelpWindow::loadHelpText(int helpIDtoLoad)
{
	FILE *in=fopen(eString((DATADIR)+eString("/enigma/resources/help.xml")).c_str(), "rt");
	if (!in)
	{
		eDebug("cannot open help.xml");
		return "";
	}

	XMLTreeParser *parser=new XMLTreeParser("ISO-8859-1");
	char buf[2048];
	
	int done;
	do
	{
		unsigned int len=fread(buf, 1, sizeof(buf), in);
		done=len<sizeof(buf);
		if (!parser->Parse(buf, len, done))
		{
			eFatal("parse error: %s at line %d", parser->ErrorString(parser->GetErrorCode()), parser->GetCurrentLineNumber());
			fclose(in);
			delete parser;
			return "";
		}
	} while (!done);
	fclose(in);

	XMLTreeNode *root=parser->RootNode();
	if (!root)
		return "";
	if (strcmp(root->GetType(), "enigmahelp"))
	{
		eFatal("not a enigma help file.");
		return "";
	}
	
	XMLTreeNode *node=parser->RootNode();
	
	for (node=node->GetChild(); node; node=node->GetNext())
		if (!strcmp(node->GetType(), "help"))
		{
			for (XMLTreeNode *xam=node->GetChild(); xam; xam=xam->GetNext())
				if (!strcmp(xam->GetType(), "helptext"))
				{
					if (helpIDtoLoad==atoi(xam->GetAttributeValue("id")))
					{
						const char *helptext=xam->GetAttributeValue("text");
						eString ht(helptext);
						delete parser;
						return ht;
					}
				}
		}

	delete parser;
	return "";
}

int eHelpWindow::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_helpwindowActions->up)
		{
			if (doscroll && *cur != entryBeg.front() ) // valid it
			{
				cur--;
				curPage--;
				scrollbox->move(ePoint(0, -(*cur)));
				updateScrollbar();
			}
		}
		else if (event.action == &i_helpwindowActions->down)
		{
			if (doscroll && *cur != entryBeg.back() ) // valid it
			{
				cur++;
				curPage++;
				scrollbox->move(ePoint(0, -(*cur)));
				updateScrollbar();
			}
		}
		else if (event.action == &i_helpwindowActions->close)
			close(0);
		else
			break;
		return 1;
	default:
		break;
	}
	return eWindow::eventHandler(event);
}

void eHelpWindow::updateScrollbar()
{
	int total=entryBeg.size()*visible->height();
	int start=curPage*visible->height()*100/total;
	int vis=visible->getSize().height()*100/total;
//	eDebug("total=%d, start = %d, vis = %d", total, start, vis);
	scrollbar->setParams(start,vis);
	scrollbar->show();
}

eHelpWindow::~eHelpWindow()
{
}

