/*
 * $Id: epgsearch.cpp,v 1.2 2009/03/05 17:32:39 dbluelle Exp $
 *
 * (C) 2008 by Dr. Best  <dr.best@dreambox-tools.info>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <enigma.h>
#include <epgsearch.h>
#include <lib/gdi/font.h> // eTextPara
#include <stdio.h>
using namespace std;
eEPGSearchDATA *eEPGSearchDATA::instance;
SearchEPGDATA SearchResultsEPG;


bool sortByEventStart(const SEARCH_EPG_DATA& a, const SEARCH_EPG_DATA& b)
{
        return a.start_time < b.start_time;
}


struct eSearchService
{
	eString search;
	int intExactMatch;
	int intCaseSensitive;
	int genre;
	int Range;
public:
	eSearchService(const eString &search, int intExactMatch, int intCaseSensitive, int genre, int Range):search(search),intExactMatch(intExactMatch),intCaseSensitive(intCaseSensitive),genre(genre),Range(Range)
	{
	}
	void operator()(const eServiceReference &e)
	{
		eEPGCache *epgcache=eEPGCache::getInstance();
		eServiceReferenceDVB &ref = (eServiceReferenceDVB&)e;
		const timeMap *evmap = epgcache->getTimeMap((eServiceReferenceDVB&)ref);
		if (evmap)
		{
			int tsidonid = (ref.getTransportStreamID().get()<<16)|ref.getOriginalNetworkID().get();
			timeMap::const_iterator ibegin = evmap->begin(), iend = evmap->end();
			for (timeMap::const_iterator event(ibegin); event != iend; ++event)
			{
				if (event->second->search(tsidonid,search,intExactMatch,intCaseSensitive,genre,Range))
				{
					EITEvent *ev = new EITEvent(*event->second, tsidonid);
					for (ePtrList<Descriptor>::iterator d(ev->descriptor); d != ev->descriptor.end(); ++d)
					{
						Descriptor *descriptor=*d;
						if (descriptor->Tag() == DESCR_LINKAGE)
						{
								LinkageDescriptor *ld=(LinkageDescriptor*)descriptor;
								if (ld->linkage_type==0xB0)
								{
									eServiceReferenceDVB MySubService(ref.getDVBNamespace(),
										eTransportStreamID(ld->transport_stream_id),
										eOriginalNetworkID(ld->original_network_id),
										eServiceID(ld->service_id), 7);
									ref = MySubService;
									break;
								}
						}
					}
					eString titlefound;
					LocalEventData led;
					led.getLocalData(ev, &titlefound, 0);
			
					SEARCH_EPG_DATA tempSEARCH_EPG_DATA;
					tempSEARCH_EPG_DATA.ref = (eServiceReference)ref;
					tempSEARCH_EPG_DATA.start_time = ev->start_time;
					tempSEARCH_EPG_DATA.duration = ev->duration;
					tempSEARCH_EPG_DATA.title = titlefound;
					SearchResultsEPG.push_back(tempSEARCH_EPG_DATA);
					delete ev;
				}
			}
		}
	}
};

eEPGSearch::eEPGSearch(eServiceReference& ref, const eString& CurrentEventName, EITEvent* e): eWindow(1)
{
	init_eEPGSearch(ref,CurrentEventName,e);
}
void eEPGSearch::init_eEPGSearch(eServiceReference& ref, const eString& CurrentEventName, EITEvent* e)
{
	SearchName = "";
	eString descr = CurrentEventName;
	if (e)
	{
		for (ePtrList<Descriptor>::const_iterator d(e->descriptor); d != e->descriptor.end(); ++d)
		{
			if ( d->Tag() == DESCR_SHORT_EVENT)
			{
				ShortEventDescriptor *s=(ShortEventDescriptor*)*d;
				descr=s->event_name;
				break;
			}
		}
	}
	Titel = descr;
	status  = new eStatusBar(this); status->setName("statusbar");
	
	sServiceReference = ref2string(ref);
	eString AnzeigeCheckBox = _("unknown");
	eService* current;
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi)
	{
		current = eDVB::getInstance()->settings->getTransponders()->searchService(ref);
		if (current)
			AnzeigeCheckBox = current->service_name;
	}
	sServiceReferenceSearch = AnzeigeCheckBox;
	
	InputName=new eTextInputField(this,0); InputName->setName("searchtext");
	InputName->setMaxChars(50); 
	
	intAllServices = 0;
	intExactMatch = 0;
	intCaseSensitive = 0;
	
	chkCurrentService = new eCheckbox(this, 0, 1);chkCurrentService->setName("CurrentService");
	CONNECT(chkCurrentService->checked, eEPGSearch::chkCurrentServiceStateChanged);
	
	chkAllServices = new eCheckbox(this, 1, 1);chkAllServices->setName("AllServices");
	CONNECT(chkAllServices->checked, eEPGSearch::chkAllServicesStateChanged);
	
	chkExactMatch = new eCheckbox(this, 0, 1);chkExactMatch->setName("ExactMatch");
	
	chkCaseSensitive = new eCheckbox(this, 0, 1);chkCaseSensitive->setName("CaseSensitive");


	cboGenre = new eComboBox(this);cboGenre->setName("Genre");
	new eListBoxEntryText(*cboGenre, "", (void *) (0));
	// Movies lasse ich in Einzelkategorien, des Rest fasse ich in Gruppen zusammen...
	new eListBoxEntryText(*cboGenre, _("Movie"), (void *) (16));
	new eListBoxEntryText(*cboGenre, _("Thriller"), (void *) (17));
	new eListBoxEntryText(*cboGenre, _("Adventure"), (void *) (18));
	new eListBoxEntryText(*cboGenre, _("SciFi"), (void *) (19));
	new eListBoxEntryText(*cboGenre, _("Comedy"), (void *) (20));
	new eListBoxEntryText(*cboGenre, _("Soap"), (void *) (21));
	new eListBoxEntryText(*cboGenre, _("Romance"), (void *) (22));
	new eListBoxEntryText(*cboGenre, _("Serious"), (void *) (23));
	new eListBoxEntryText(*cboGenre, _("Adult"), (void *) (24));
	new eListBoxEntryText(*cboGenre, _("News"), (void *) (32)); // Range = 32 bis 36
	new eListBoxEntryText(*cboGenre, _("Show"), (void *) (48)); // Range = 48 bis 51
	new eListBoxEntryText(*cboGenre, _("Sports"), (void *) (64)); // Range = 64 bis 75
	new eListBoxEntryText(*cboGenre, _("Children"), (void *) (80)); // Range = 80 bis 85
	new eListBoxEntryText(*cboGenre, _("Music"), (void *) (96)); // Range = 96 bis 102
	new eListBoxEntryText(*cboGenre, _("Culture"), (void *) (112)); // Range = 112 bis 123
	new eListBoxEntryText(*cboGenre, _("Social"), (void *) (128)); // Range = 128 bis 131
	new eListBoxEntryText(*cboGenre, _("Education"), (void *) (144)); // Range = 144 bis 151
	new eListBoxEntryText(*cboGenre, _("Hobbies"), (void *) (160)); // Range = 160 bis 167
	new eListBoxEntryText(*cboGenre, _("Live"), (void *) (176)); // Range = 176 bis 179

	cboGenre->setCurrent(0);
	CONNECT(cboGenre->selchanged, eEPGSearch::cboGenreChanged);
	
	bt_abort=new eButton(this);bt_abort->setName("Close");
        CONNECT(bt_abort->selected, eWidget::reject);
	
	bt_seach=new eButton(this);bt_seach->setName("Start");
	CONNECT(bt_seach->selected, eEPGSearch::Search);

	if (eSkin::getActive()->build(this, "eEPGSearch"))
		eFatal("skin load of \"eEPGSearch\" failed");

	chkCurrentService->setText(AnzeigeCheckBox);
	InputName->setText(Titel);
	setFocus(InputName);
	canCheck = 1;
}
void eEPGSearch::cboGenreChanged(eListBoxEntryText *item)
{
	if (item)
	{
		int ID = (int)item->getKey();
		if (ID == 0)
			InputName->setText(Titel);
		else
			InputName->setText("");
	}
}

void eEPGSearch::chkCurrentServiceStateChanged(int state)
{
	if (canCheck)
	{
		if (!chkCurrentService->isChecked())
		{
			canCheck = 0;
			chkAllServices->setCheck(1);
			canCheck = 1;
		}
		else
		{
			canCheck = 0;
			chkAllServices->setCheck(0);
			canCheck = 1;
		}
	}
}
void eEPGSearch::chkAllServicesStateChanged(int state)
{
	if (canCheck)
	{
		if (chkAllServices->isChecked())
		{
			canCheck = 0;
			chkCurrentService->setCheck(0);
			canCheck = 1;
		}
		else
		{
			canCheck = 0;
			chkCurrentService->setCheck(1);
			canCheck = 1;
		}
	}
}
void eEPGSearch::Search()
{
	eString h;
	if (chkAllServices->isChecked())
		h = _("all services");
	else
		h = sServiceReferenceSearch;

	eString Anzeige = "";
	int genre = 0;
	genre = (int)cboGenre->getCurrent()->getKey();
	if ( genre == 0)
	{
		Anzeige = eString(_("Searching for ")) + InputName->getText() + "\nin " + h + "...";
		SearchName = InputName->getText();
	}
	else
	{
		Anzeige = eString(_("Searching for ")) + eString(_("Genre:")) + cboGenre->getText() + "\nin " + h + "...";
		SearchName = eString(_("Genre:")) + cboGenre->getText();
	}
	//hide();
	eMessageBox msg(Anzeige, _("EPG Search"), eMessageBox::iconInfo);
	msg.show();
	int back = Searching(InputName->getText());
	msg.hide();
	close(back);
	
}
eString eEPGSearch::getSearchName()
{
	return SearchName;
}
int eEPGSearch::Searching(eString SearchName)
{
	eString search;
	eString mDescription = SearchName;
	if (chkExactMatch->isChecked())
	{
		intExactMatch = 1;
		search = mDescription;
	}
	else
		intExactMatch = 0;
	
	if (chkCaseSensitive->isChecked())
	{
		intCaseSensitive = 1;
		search = mDescription;
	}
	else
		intCaseSensitive = 0;
	
	if (!chkExactMatch->isChecked() && !chkCaseSensitive->isChecked())
	{
		search = mDescription.upper();
		intExactMatch = 0;
		intCaseSensitive = 0;
	}
	SearchResultsEPG.clear();

	int genre = 0;
	genre = (int)cboGenre->getCurrent()->getKey();
	int Range = 0;
	if ( genre )
	{
		switch (genre)
		{
			case 32:
				Range = 36;
				break;
			case 48:
				Range = 51;
				break;
			case 64:
				Range = 75;
				break;
			case 80:
				Range = 85;
				break;
			case 96:
				Range = 102;
				break;
			case 112:
				Range = 123;
				break;
			case 128:
				Range = 131;
				break;
			case 144:
				Range = 151;
				break;
			case 160:
				Range = 167;
				break;
			case 176:
				Range = 179;
				break;
			default:
				break;
		}
	}
	eEPGCache *epgcache=eEPGCache::getInstance();
	epgcache->Lock();

	if (chkAllServices->isChecked())
	{
		if ((search != "") || (genre != 0))
		{
			eTransponderList::getInstance()->forEachServiceReference(eSearchService(search, intExactMatch, intCaseSensitive, genre,Range));
		}
	}
	else
	{
		eServiceReference current_service=string2ref(sServiceReference);
		eSearchService ss(search, intExactMatch, intCaseSensitive, genre,Range);
		ss(current_service);
	}
	epgcache->Unlock();
	if ( SearchResultsEPG.size())
		sort(SearchResultsEPG.begin(), SearchResultsEPG.end(), sortByEventStart);
	return SearchResultsEPG.size() ? 0: 2;
}
eEPGSearchDATA::eEPGSearchDATA()
{
	if (!instance)
		instance=this;
	SearchResultsEPG.clear();
	
}
SearchEPGDATA eEPGSearchDATA::getSearchData()
{
	return SearchResultsEPG;
}
void eEPGSearchDATA::clearList()
{
	SearchResultsEPG.clear();	
}
