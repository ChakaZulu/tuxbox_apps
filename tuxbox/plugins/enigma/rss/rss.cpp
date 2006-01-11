/* 
Dreambox RSS reader
Copyright (C) 2004 Bjorn Hijmans (bjorn@hijmans.nl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "rss.h"

extern "C" int plugin_exec( PluginParam *par );

int plugin_exec( PluginParam *par )
{
	rssMain dlg;
	dlg.show();
	int result=dlg.exec();
	dlg.hide();

	return result;
}

eString stringReplace( const eString &toReplace, char from, char to )
{	char buffer[8192];
	strcpy(buffer, (char *)toReplace.c_str());
	char *p = buffer;
	while ( *p != '\0' ) 
	{	if ( *p == from )
			*p = to;
		p++;
	}
	return eString( buffer );
}

eString removeTags(eString in)
{	eString out;
	// Find first <
	int startpos = 0; int length = 0;
	for(unsigned int i = 0; i < in.length(); i++)
	{	length++;
		if(in.mid(i, 1) == "<")
		{	out = out + in.mid(startpos, length - 1);
			// Look for >
			for(unsigned int j = i; j < in.length(); j++)
			{	if(in.mid(j, 1) == ">")
				{	i = j;
					startpos = i + 1;
					length = 0;
					break;
				}
			}
		}
	}
	if(out == "")
	{	out = in;
	}
	return out;
}
			

rssMain::rssMain(): eWindow(1)
{	cmove(ePoint(140, 140));
	cresize(eSize(440, 296));
	setText("Dreambox RSS reader");

	theList = new eListBox<eListBoxEntryText>(this);
	theList->move(ePoint(10, 10));
	theList->resize(eSize(clientrect.width() - 20, clientrect.height() - 20));
	theList->loadDeco();
	theList->setColumns(1);

	setFocus(theList);

	if theConfigParser.parse("/var/tuxbox/config/feeds.xml")
		theConfigParser.parse(CONFIGDIR"/feeds.xml")
	//theConfigParser.parse("/var/tuxbox/plugins/feeds.xml");

	theRssFeed = new rssFeed();
	theRssFeed->hide();

	inDownloadFlag = 0;

	printFeeds();	

	CONNECT(theList->selected, rssMain::selectedItem);
}

void rssMain::printFeeds()
{	std::list<ConfigItem>::iterator i;
	for(i = theConfigParser.configItems.begin(); i != theConfigParser.configItems.end(); ++i)
	{	new eListBoxEntryText(theList, i->name.c_str(), (void *) &(i->id));
	}
}

void rssMain::selectedItem(eListBoxEntryText *item)
{	if(item && !inDownloadFlag)
	{	inDownloadFlag = 1;

		int * iKey = (int *) (item->getKey());

		eString url;
		eString name;

		std::list<ConfigItem>::iterator i;
		for(i = theConfigParser.configItems.begin(); i != theConfigParser.configItems.end(); ++i)
		{	if(i->id == *iKey)
			{	url = i->url;
				name = i->name;
			}
		}

		CONNECT(theFetcher.downloadDone, rssMain::downloadDone);		
		downloadDoneFlag = 0;
		currentName = name;
		theFetcher.fetch(url);
	}
}

void rssMain::downloadDone(int err)
{	if(!downloadDoneFlag)
	{	downloadDoneFlag = 1;
		this->hide();
		theRssFeed->printRSSFeed(currentName);
		theRssFeed->show();
		theRssFeed->exec();
		theRssFeed->hide();
		this->show();
		setFocus(theList);
		inDownloadFlag = 0;
	}
}

rssFeed::rssFeed(): eWindow(1)
{	
	//720x576
	//560x376

	move(ePoint(80, 100));
	resize(eSize(560, 376));
	setText("Feed");

	theList = new eListBox<eListBoxEntryText>(this);
	theList->move(ePoint(10, 10));
	theList->resize(eSize(clientrect.width() - 20, clientrect.height() - 20));
	theList->loadDeco();
	theList->setColumns(1);

	setFocus(theList);

	CONNECT(theList->selected, rssFeed::selectedItem);
}

void rssFeed::printRSSFeed(eString title)
{	eString dest = "/var/tmp/rss.tmp";
	theRSSParser.parse(dest);
	
	setFocus(theList);
	setText(title);
	
	theList->beginAtomic();
	theList->clearList();
	
	if(theList)
	{	// Iterate through list
		std::list<NewsItem>::iterator i;
		for(i = theRSSParser.newsItems.begin(); i != theRSSParser.newsItems.end(); ++i)
		{	new eListBoxEntryText(theList, i->title.c_str(), (void *) (i->id));
		}
	}
	theList->endAtomic();

}

void rssFeed::selectedItem(eListBoxEntryText *item)
{	if(item)
	{	int iKey = (int) (item->getKey());

		eString description;
		eString title;

		std::list<NewsItem>::iterator i;
		for(i = theRSSParser.newsItems.begin(); i != theRSSParser.newsItems.end(); ++i)
		{	if(i->id == iKey)
			{	description = i->description;
				title = i->title;
			}
		}

		//eMessageBox msg(description.c_str(), title.c_str(), eMessageBox::btOK);
		//msg.show();     msg.exec();     msg.hide();
		
		rssDetail dlg(title.c_str(), description.c_str());
		dlg.show();
		dlg.exec();
		dlg.hide();
	}
	else
	{	reject();
	}
}

rssMain::~rssMain()
{
	// we have to do almost nothing here. all widgets are automatically removed
	// since they are children of the main dialog. the eWidget-destructor will do to this.
}

void Fetcher::fetch(eString url)
{	tempPath = "/var/tmp/rss.tmp";
	int error = 0;
	connectionP = eHTTPConnection::doRequest(url.c_str(), eApp, &error);
	
	if(!connectionP || error)
	{	eMessageBox msg("Error downloading " + url + "(" + eString().sprintf("%d", error) + ")", _("Details"), eMessageBox::btOK);
		msg.show();     msg.exec();     msg.hide();
	}
	else
	{	CONNECT(connectionP->transferDone, Fetcher::transferDone);
		CONNECT(connectionP->createDataSource, Fetcher::createDownloadSink);
		connectionP->local_header["User-Agent"] = "RSS";
		connectionP->start();
	}
}

void Fetcher::transferDone(int err)
{	
	if(!err)
	{	connectionP = NULL;

		// Tell caller download is ready
		/*emit*/ downloadDone(err);
	}
	else
	{	eString sMsg = "Error " + eString().sprintf("%d", err);
		eMessageBox msg(sMsg, _("User Abort"), eMessageBox::iconWarning|eMessageBox::btOK);
		msg.show();     msg.exec();     msg.hide();
	}
}

eHTTPDataSource * Fetcher::createDownloadSink(eHTTPConnection *conn)
{	dataSinkP = new eHTTPDownload(connectionP, (char *)tempPath.c_str());

	return(dataSinkP);
}

void ConfigParser::save(ConfigItem i)
{	configItems.push_back(i);
}

void ConfigParser::parse(eString file)
{	XMLTreeParser * parser;
	FILE *in = fopen(file.c_str(), "rt");

        if(in) 
	{	parser = new XMLTreeParser("ISO-8859-1");
		char buf[2048];

		int done;
		do 
		{	unsigned int len=fread(buf, 1, sizeof(buf), in);
			done = ( len < sizeof(buf) );
			if ( ! parser->Parse( buf, len, done ) ) 
			{	eMessageBox msg(_("Configfile parse error"), _("User Abort"), eMessageBox::iconWarning|eMessageBox::btOK);
		                msg.show();     msg.exec();     msg.hide();
				delete parser;
				parser = NULL;
				return;
			}
		} 
		while (!done);

                fclose(in);

		XMLTreeNode * root = parser->RootNode();
		if(!root)
		{	eMessageBox msg(_("Configfile parse error"), _("User Abort"), eMessageBox::iconWarning|eMessageBox::btOK);
		        msg.show();     msg.exec();     msg.hide();	
			return;
		}
		if ( strcmp( root->GetType(), "feeds") ) 
		{	eMessageBox msg(_("Invalid configfile"), _("User Abort"), eMessageBox::iconWarning|eMessageBox::btOK);
		        msg.show();     msg.exec();     msg.hide();
	                return;
	        }
	
		ConfigItem thisItem;
		int idcnt = 0;

		for(XMLTreeNode * node = root->GetChild(); node; node = node->GetNext())
		{	if(!strcmp(node->GetType(), "feed"))
			{	for(XMLTreeNode * i = node->GetChild(); i; i = i->GetNext())
				{	if(!strcmp(i->GetType(), "name"))
					{	thisItem.name = i->GetData();
					}
					if(!strcmp(i->GetType(), "url"))
					{	thisItem.url = i->GetData();
					}
				}
				thisItem.id = idcnt++;
				save(thisItem);
			}
		}


		delete parser;
	}
	else
	{	eMessageBox msg(_("Config file not found"), _("User Abort"), eMessageBox::iconWarning|eMessageBox::btOK);
		msg.show();     msg.exec();     msg.hide();
	}
}

rssDetail::rssDetail(const char *title, const char *desc) : eWindow(0)
{
	setText(title);

	move(ePoint(100, 120));
	resize(eSize(520, 336));	

	scrollbar = new eProgress(this);
        scrollbar->setStart(0);
        scrollbar->setPerc(100);
        scrollbar->setDirection(1);
        scrollbar->move(ePoint(clientrect.width() - 20, 0));
        scrollbar->resize(eSize(20, clientrect.height() - 30));
        
	descrWidget = new eLabel(this);
	descrWidget->move(ePoint(20, 20));
	descrWidget->resize(eSize(clientrect.width() - 50, clientrect.height() - 50 - 5));
	
	descrLabel = new eLabel(descrWidget);
	descrLabel->setText(desc);
	descrLabel->move(ePoint(0, 0));
	descrLabel->resize(eSize(10, 10));
	descrLabel->setFlags(RS_WRAP);

	float lineheight=fontRenderClass::getInstance()->getLineHeight( descrLabel->getFont() );
	int lines = descrWidget->getSize().height() / (int)lineheight;
	//pageHeight = (int)(lines * lineheight);
	int newheight = lines * (int)lineheight + (int)(round(lineheight) - (int)lineheight);
	descrWidget->resize( eSize( descrWidget->getSize().width(), newheight + (int)lineheight/6 ));
	descrLabel->resize( 
		eSize(
			descrWidget->getSize().width(), 
			descrWidget->getSize().height() * 4
		 )
	);

	const eRect &cr = getClientRect();
	eButton * ok = new eButton(this);
	ok->setText("OK");
	ok->move(ePoint(10, cr.height() - 30));
	ok->resize(eSize(78, 20));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");

        CONNECT(ok->selected, eWidget::accept);

	updateScrollbar();
}

void rssDetail::updateScrollbar()
{
	total = descrWidget->getSize().height();
	int pages=1;
	while( total < descrLabel->getExtend().height() ) {
		total += descrWidget->getSize().height();
		pages++;
	}

	int start =- ( descrLabel->getPosition().y() * 100 ) /total;
	int vis = descrWidget->getSize().height() *100/total;

	scrollbar->setParams(start, vis);
	scrollbar->show();
	if (pages == 1)
		total = 0;
}

int rssDetail::eventHandler(const eWidgetEvent &event)
{
	switch (event.type) {
        	case eWidgetEvent::evtKey:
			if ( (event.key)->flags == KEY_STATE_DOWN ) {
				switch ( (event.key)->code  ) {
					case RC_UP:
						handleUp();
						break;
					case RC_DN:
						handleDown();
						break;
				}
			}
			break;
		default:
			break;
	}

	return eWindow::eventHandler(event);
}

void rssDetail::handleDown( void )
{
	ePoint curPos = descrLabel->getPosition();
	if ( 
		total &&
		(total - descrWidget->getSize().height() ) >= abs( curPos.y() - descrWidget->getSize().height() ) 
	) {
		descrLabel->move( 
			ePoint( 
				curPos.x(), 
				curPos.y() - descrWidget->getSize().height() 
			)
		);
		updateScrollbar();
	}
}

void rssDetail::handleUp( void )
{
	ePoint curPos = descrLabel->getPosition();
	if ( 
		total && 
		( curPos.y() < 0 )
	) {
		descrLabel->move( 
			ePoint( 
				curPos.x(), 
				curPos.y() + descrWidget->getSize().height() 
			) 
		);
		updateScrollbar();
	}
}

void rssDetail::redrawWidget(gPainter *d, const eRect &area)
{	eWindow::redrawWidget( d, area );
}


void RSSParser::save(NewsItem i)
{	newsItems.push_back(i);
}

void RSSParser::parse(eString file)
{	XMLTreeParser * parser;
	FILE *in = fopen(file.c_str(), "rt");
	newsItems.clear();

        if(in) 
	{	parser = new XMLTreeParser("ISO-8859-1");
		char buf[2048];

		int done;
		do 
		{	unsigned int len=fread(buf, 1, sizeof(buf), in);
			done = ( len < sizeof(buf) );
			if ( ! parser->Parse( buf, len, done ) ) 
			{	eMessageBox msg(_("XML parse error (general xml)"), _("Error"), eMessageBox::iconWarning|eMessageBox::btOK);
		                msg.show();     msg.exec();     msg.hide();
				delete parser;
				parser = NULL;
				return;
			}
		} 
		while (!done);

                fclose(in);

		XMLTreeNode * root = parser->RootNode();
		if(!root)
		{	eMessageBox msg(_("XML parse error (rootnode)"), _("Error"), eMessageBox::iconWarning|eMessageBox::btOK);
		        msg.show();     msg.exec();     msg.hide();	
			return;
		}
		if (!strcmp( root->GetType(), "rss")) 
		{	NewsItem thisItem;
			int idcnt = 0;

			for(XMLTreeNode * node = root->GetChild(); node; node = node->GetNext())
			{	if(!strcmp(node->GetType(), "channel"))
				{	for(XMLTreeNode * itemNode = node->GetChild(); itemNode; itemNode = itemNode->GetNext())
					{	if(!strcmp(itemNode->GetType(), "item"))
						{	for(XMLTreeNode * i = itemNode->GetChild(); i; i = i->GetNext())
							{	if(!strcmp(i->GetType(), "title"))
								{	thisItem.title = i->GetData();
									thisItem.title.removeChars('\n');
									thisItem.title.removeChars('\r');
								}
								if(!strcmp(i->GetType(), "description"))
								{	eString desc = i->GetData();
									desc.removeChars('\n');
									desc.removeChars('\r');
									desc = removeTags(desc);
									thisItem.description = desc;
								}
							}
							thisItem.id = idcnt++;
							save(thisItem);
						}
					}
				}
			}
		}
		else if(!strcmp(root->GetType(), "rdf:RDF"))
		{	NewsItem thisItem;
			int idcnt = 0;

			for(XMLTreeNode * node = root->GetChild(); node; node = node->GetNext())
			{	if(!strcmp(node->GetType(), "item"))
				{	for(XMLTreeNode * i = node->GetChild(); i; i = i->GetNext())
					{	if(!strcmp(i->GetType(), "title"))
						{	thisItem.title = i->GetData();
							thisItem.title.removeChars('\n');
							thisItem.title.removeChars('\r');
						}
						if(!strcmp(i->GetType(), "description"))
						{	eString desc = i->GetData();
							desc.removeChars('\n');
							desc.removeChars('\r');
							desc = removeTags(desc);
							thisItem.description = desc;
						}
					}
					thisItem.id = idcnt++;
					save(thisItem);
				}
			}
		}
		else
		{	eString sMsg = eString().sprintf("Unknown file format (%s)", root->GetType());
			eMessageBox msg(sMsg, _("Error"), eMessageBox::iconWarning|eMessageBox::btOK);
			msg.show();     msg.exec();     msg.hide();
		}

		delete parser;
	}
	else
	{	eMessageBox msg(_("RSS file not found"), _("Error"), eMessageBox::iconWarning|eMessageBox::btOK);
		msg.show();     msg.exec();     msg.hide();
	}
}
