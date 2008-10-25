/*
 * $Id: epgsearch.cpp,v 1.1 2008/10/25 14:22:07 dbluelle Exp $
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

static int searchforEvent(EITEvent *ev, const eString &search, eString &titlefound, eServiceReferenceDVB &new_ref, int intExactMatch, int intCaseSensitive, int genre)
{
	eString title;
	eString sFind;
	
	int intFound = 0;

	// Genre Suchkriterium schlägt immer Titelsuche ;)
	if ( genre != 0)
	{
		int Range = 0;
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

		for (ePtrList<Descriptor>::iterator d(ev->descriptor); d != ev->descriptor.end(); ++d)
		{
			Descriptor *descriptor=*d;
			if(descriptor->Tag()==DESCR_CONTENT)
			{
				ContentDescriptor *cod=(ContentDescriptor*)descriptor;

				for(ePtrList<descr_content_entry_struct>::iterator ce(cod->contentList.begin()); ce != cod->contentList.end(); ++ce)
				{

					if ( genre < 32 )
					{
						if (genre  == ce->content_nibble_level_1*16+ce->content_nibble_level_2)
							intFound = 1;
					}
					else
					{
						int genreID = ce->content_nibble_level_1*16+ce->content_nibble_level_2;
						if ( (genreID >= genre) && (genreID <= Range))
							intFound = 1;
					}
				}
			}
		}
		if (intFound)
		{
			LocalEventData led;
			led.getLocalData(ev, &title, 0);
			titlefound = title;
		}
	}
	else
	{
		if (search != "")
		{
			LocalEventData led;
			
			led.getLocalData(ev, &title, 0);
			titlefound = title;
			if (intExactMatch || intCaseSensitive) 
				sFind = title;
			else
				sFind = title.upper();
			if (!intExactMatch)
			{
				if (sFind.find(search) != eString::npos)
					intFound = 1;
			}
			else
			{
				if (!strcmp(search.c_str(),sFind.c_str()))
					intFound = 1;
			}
		}
	}
	if (intFound)
	{
		for (ePtrList<Descriptor>::iterator d(ev->descriptor); d != ev->descriptor.end(); ++d)
		{
			Descriptor *descriptor=*d;
			if (descriptor->Tag() == DESCR_LINKAGE)
			{
					LinkageDescriptor *ld=(LinkageDescriptor*)descriptor;
					if (ld->linkage_type==0xB0)
					{
						eServiceReferenceDVB MySubService(new_ref.getDVBNamespace(),
							eTransportStreamID(ld->transport_stream_id),
							eOriginalNetworkID(ld->original_network_id),
							eServiceID(ld->service_id), 7);
						new_ref = MySubService;
						break;
					}
			}
		}
	}
	return intFound;
}

static void SearchInChannel(const eServiceReference &e, eString search, int begin, int intExactMatch, int intCaseSensitive, int genre)
{
	int duration = 0;
	if ((search != "") || (genre != 0))
	{
		eEPGCache *epgcache=eEPGCache::getInstance();
		eServiceReferenceDVB &ref = (eServiceReferenceDVB&)e;
		epgcache->Lock();
		const timeMap *evmap = epgcache->getTimeMap((eServiceReferenceDVB&)ref);
		if (!evmap)
		{
			epgcache->Unlock();
			// nix gefunden :-(
		}
		else
		{
			eServiceReferenceDVB &rref=(eServiceReferenceDVB&)ref;
			timeMap::const_iterator ibegin = evmap->begin(), iend = evmap->end();
			if (begin != 0)
			{
				ibegin = evmap->lower_bound(begin);
				if ((ibegin != evmap->end()) && (ibegin != evmap->begin()))
					--ibegin;
				else
					ibegin=evmap->begin();
		
				timeMap::const_iterator iend = evmap->upper_bound(begin + duration);
				if (iend != evmap->end())
					++iend;
			}
			int tsidonid =(rref.getTransportStreamID().get()<<16)|rref.getOriginalNetworkID().get();
			eService* current;
			eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
			if (sapi)
			{
				current = eDVB::getInstance()->settings->getTransponders()->searchService(e);
				if (current)
				{
					for (timeMap::const_iterator event(ibegin); event != iend; ++event)
					{
						EITEvent *ev = new EITEvent(*event->second, tsidonid);
						eString titleFound = "";
						int intFound = 0;
						eServiceReferenceDVB _ref = rref;
						intFound = searchforEvent( ev, search, titleFound, _ref, intExactMatch, intCaseSensitive, genre);
						if (intFound)
						{
							SEARCH_EPG_DATA tempSEARCH_EPG_DATA;
							tempSEARCH_EPG_DATA.ref = (eServiceReference)_ref;
							tempSEARCH_EPG_DATA.name = current->service_name;
							tempSEARCH_EPG_DATA.start_time = ev->start_time;
							tempSEARCH_EPG_DATA.duration = ev->duration;
							tempSEARCH_EPG_DATA.title = titleFound;
							SearchResultsEPG.push_back(tempSEARCH_EPG_DATA);
						}
						delete ev;
					}
				}
			}
			epgcache->Unlock();
			if ( SearchResultsEPG.size())
				sort(SearchResultsEPG.begin(), SearchResultsEPG.end(), sortByEventStart);
		}
	}
}

static void SearchInChannel(const eServiceReference &e, eString search, int intExactMatch, int intCaseSensitive, int TimeSpanSearch, tm beginTime, tm endTime, int Days, int Max_Duration)
{
	if (search != "")
	{
		eEPGCache *epgcache=eEPGCache::getInstance();
		eServiceReferenceDVB &ref = (eServiceReferenceDVB&)e;
		epgcache->Lock();
		const timeMap *evmap = epgcache->getTimeMap((eServiceReferenceDVB&)ref);
		if (!evmap)
		{
			epgcache->Unlock();
			// nix gefunden :-(
		}
		else
		{
			eServiceReferenceDVB &rref=(eServiceReferenceDVB&)ref;
			timeMap::const_iterator ibegin = evmap->begin(), iend = evmap->end();
			int tsidonid =(rref.getTransportStreamID().get()<<16)|rref.getOriginalNetworkID().get();
			eService* current;
			eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
			if (sapi)
			{
				current = eDVB::getInstance()->settings->getTransponders()->searchService(e);
				if (current)
				{
					for (timeMap::const_iterator event(ibegin); event != iend; ++event)
					{
						EITEvent *ev = new EITEvent(*event->second, tsidonid);
						int Event_duration = ev->duration;
						int dosearch = 0;
						// Anfangs+Endzeit
						if ( TimeSpanSearch )
						{
							tm Event_beginTime = *localtime( &ev->start_time);

							beginTime.tm_year = Event_beginTime.tm_year;
							beginTime.tm_mon = Event_beginTime.tm_mon;
							beginTime.tm_mday = Event_beginTime.tm_mday;
							time_t StartTime = mktime(&beginTime);

							time_t tmp = ev->start_time + ev->duration;
							tm Event_endTime = *localtime( &tmp );
							
							endTime.tm_year = Event_endTime.tm_year;
							endTime.tm_mon = Event_endTime.tm_mon;
							endTime.tm_mday = Event_endTime.tm_mday;
							time_t EndTime = mktime(&endTime);

							if (( StartTime <= ev->start_time) && ( EndTime >= tmp))
								dosearch = 1;
						}
						else
							dosearch = 1;
						// max. Laenge
						if ((Max_Duration != -1) && dosearch)
						{
							if ((Max_Duration*60) < Event_duration)
								dosearch = 0;
						}
						// Tage
						if ((Days != -1) && dosearch)
						{
							tm Event_beginTime = *localtime( &ev->start_time);	
							int i = ePlaylistEntry::Su;
							for ( int x = 0; x < Event_beginTime.tm_wday; x++ )
								i*=2;
							if (!( Days & i ))
								dosearch = 0;
						}
						if ( dosearch )
						{
							eString titleFound = "";
							int intFound = 0;
							eServiceReferenceDVB _ref = rref;
							intFound = searchforEvent( ev, search, titleFound, _ref, intExactMatch, intCaseSensitive, 0);
							if (intFound)
							{
								SEARCH_EPG_DATA tempSEARCH_EPG_DATA;
								tempSEARCH_EPG_DATA.ref = (eServiceReference)_ref;
								tempSEARCH_EPG_DATA.name = current->service_name;
								tempSEARCH_EPG_DATA.start_time = ev->start_time;
								tempSEARCH_EPG_DATA.duration = ev->duration;
								tempSEARCH_EPG_DATA.title = titleFound;
								SearchResultsEPG.push_back(tempSEARCH_EPG_DATA);
							}
						}
						delete ev;
					}
				}
			}
			epgcache->Unlock();
			if ( SearchResultsEPG.size())
				sort(SearchResultsEPG.begin(), SearchResultsEPG.end(), sortByEventStart);
		}
	}
}


class eSearchAllTVServices: public Object
{
	eServiceInterface &iface;
	eString search;
	time_t begin;
	int intExactMatch;
	int intCaseSensitive;
	int genre;
	
public:
	eSearchAllTVServices(eServiceInterface &iface, eString search, time_t begin, int intExactMatch, int intCaseSensitive, int genre): iface(iface), search(search), begin(begin), intExactMatch(intExactMatch), intCaseSensitive(intCaseSensitive), genre(genre)
	{
	}
	void addEntry(const eServiceReference &e)
	{
		int duration = 0;
		if ((search != "") || (genre != 0))
		{
			eEPGCache *epgcache=eEPGCache::getInstance();
			eServiceReferenceDVB &ref = (eServiceReferenceDVB&)e;
			epgcache->Lock();
			const timeMap *evmap = epgcache->getTimeMap((eServiceReferenceDVB&)ref);
			if (!evmap)
			{
				epgcache->Unlock();
				// nix gefunden :-(
			}
			else
			{
				eServiceReferenceDVB &rref=(eServiceReferenceDVB&)ref;
				timeMap::const_iterator ibegin = evmap->begin(), iend = evmap->end();
				if (begin != 0)
				{
					ibegin = evmap->lower_bound(begin);
					if ((ibegin != evmap->end()) && (ibegin != evmap->begin()))
						--ibegin;
					else
						ibegin=evmap->begin();
			
					timeMap::const_iterator iend = evmap->upper_bound(begin + duration);
					if (iend != evmap->end())
						++iend;
				}
				int tsidonid = (rref.getTransportStreamID().get()<<16)|rref.getOriginalNetworkID().get();
				eService *service=iface.addRef(e);
				if (service)
				{
					for (timeMap::const_iterator event(ibegin); event != iend; ++event)
					{
						EITEvent *ev = new EITEvent(*event->second, tsidonid);
						int intFound = 0;
						eString titleFound = "";
						eServiceReferenceDVB _ref = rref;
						intFound = searchforEvent( ev, search, titleFound, _ref, intExactMatch, intCaseSensitive, genre);
						if (intFound)
						{

							SEARCH_EPG_DATA tempSEARCH_EPG_DATA;
							tempSEARCH_EPG_DATA.ref = (eServiceReference)_ref;
							tempSEARCH_EPG_DATA.name = service->service_name;
							tempSEARCH_EPG_DATA.start_time = ev->start_time;
							tempSEARCH_EPG_DATA.duration = ev->duration;
							tempSEARCH_EPG_DATA.title = titleFound;
							SearchResultsEPG.push_back(tempSEARCH_EPG_DATA);
							
						}
						delete ev;
					}
				}
				epgcache->Unlock();
				if ( SearchResultsEPG.size())
					sort(SearchResultsEPG.begin(), SearchResultsEPG.end(), sortByEventStart);
				iface.removeRef(e);
			}
		}
	}
};

class eSearchAllTVServices2: public Object
{
	eServiceInterface &iface;
	eString search;
	int intExactMatch;
	int intCaseSensitive;
	int TimeSpanSearch;
	tm beginTime;
	tm endTime;
	int Days;
	int Max_Duration;
	
public:
	eSearchAllTVServices2(eServiceInterface &iface, eString search,int intExactMatch, int intCaseSensitive, int TimeSpanSearch, tm beginTime, tm endTime, int Days, int Max_Duration): iface(iface), search(search), intExactMatch(intExactMatch), intCaseSensitive(intCaseSensitive), TimeSpanSearch(TimeSpanSearch), beginTime(beginTime), endTime(endTime), Days(Days), Max_Duration(Max_Duration)
	{
	}
	void addEntry(const eServiceReference &e)
	{
		if (search != "")
		{
			eEPGCache *epgcache=eEPGCache::getInstance();
			eServiceReferenceDVB &ref = (eServiceReferenceDVB&)e;
			epgcache->Lock();
			const timeMap *evmap = epgcache->getTimeMap((eServiceReferenceDVB&)ref);
			if (!evmap)
			{
				epgcache->Unlock();
				// nix gefunden :-(
			}
			else
			{
				eServiceReferenceDVB &rref=(eServiceReferenceDVB&)ref;
				timeMap::const_iterator ibegin = evmap->begin(), iend = evmap->end();
				int tsidonid = (rref.getTransportStreamID().get()<<16)|rref.getOriginalNetworkID().get();
				eService *service=iface.addRef(e);
				if (service)
				{
					for (timeMap::const_iterator event(ibegin); event != iend; ++event)
					{
						EITEvent *ev = new EITEvent(*event->second, tsidonid);
						int Event_duration = ev->duration;
						int dosearch = 0;
						// Anfangs+Endzeit
						if ( TimeSpanSearch )
						{
							tm Event_beginTime = *localtime( &ev->start_time);

							beginTime.tm_year = Event_beginTime.tm_year;
							beginTime.tm_mon = Event_beginTime.tm_mon;
							beginTime.tm_mday = Event_beginTime.tm_mday;
							time_t StartTime = mktime(&beginTime);

							time_t tmp = ev->start_time + ev->duration;
							tm Event_endTime = *localtime( &tmp );
							
							endTime.tm_year = Event_endTime.tm_year;
							endTime.tm_mon = Event_endTime.tm_mon;
							endTime.tm_mday = Event_endTime.tm_mday;
							time_t EndTime = mktime(&endTime);

							if (( StartTime <= ev->start_time) && ( EndTime >= tmp))
								dosearch = 1;
						}
						else
							dosearch = 1;
						// max. Laenge
						if ((Max_Duration != -1) && dosearch)
						{
							if ((Max_Duration*60) < Event_duration)
								dosearch = 0;
						}
						// Tage
						if ((Days != -1) && dosearch)
						{
							tm Event_beginTime = *localtime( &ev->start_time);	
							int i = ePlaylistEntry::Su;
							for ( int x = 0; x < Event_beginTime.tm_wday; x++ )
								i*=2;
							if (!( Days & i ))
								dosearch = 0;
						}
						if ( dosearch )
						{
							eString titleFound = "";
							int intFound = 0;
							eServiceReferenceDVB _ref = rref;
							intFound = searchforEvent( ev, search, titleFound, _ref, intExactMatch, intCaseSensitive, 0);
							if (intFound)
							{
								SEARCH_EPG_DATA tempSEARCH_EPG_DATA;
								tempSEARCH_EPG_DATA.ref = (eServiceReference)_ref;
								tempSEARCH_EPG_DATA.name = service->service_name;
								tempSEARCH_EPG_DATA.start_time = ev->start_time;
								tempSEARCH_EPG_DATA.duration = ev->duration;
								tempSEARCH_EPG_DATA.title = titleFound;
								SearchResultsEPG.push_back(tempSEARCH_EPG_DATA);
							}
						}
						delete ev;
					}
				}
				epgcache->Unlock();
				if ( SearchResultsEPG.size())
					sort(SearchResultsEPG.begin(), SearchResultsEPG.end(), sortByEventStart);
			}
			iface.removeRef(e);
		}
	}
};


eEPGSearch::eEPGSearch(eServiceReference ref, EITEvent e): eWindow(1)
{
	eString descr;
	
	for (ePtrList<Descriptor>::const_iterator d(e.descriptor); d != e.descriptor.end(); ++d)
	{
		if ( d->Tag() == DESCR_SHORT_EVENT)
		{
			ShortEventDescriptor *s=(ShortEventDescriptor*)*d;
			descr=s->event_name;
			break;
		}
	}
	Titel = descr;
	init_eEPGSearch(ref);
}
eEPGSearch::eEPGSearch(eServiceReference ref, eString CurrentEventName): eWindow(1)
{
	Titel = CurrentEventName;
	init_eEPGSearch(ref);
}

void eEPGSearch::init_eEPGSearch(eServiceReference ref)
{
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
eEPGSearch::eEPGSearch()
{
	SearchName = "";
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
	hide();
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
	eMessageBox msg(Anzeige, _("EPG Search"), eMessageBox::iconInfo);
	msg.show();
	sleep(1);
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
	time_t begin = 0; //1184515200;
	eString mDescription = SearchName;
	eString current;
	int intFound = 2;
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

	if (chkAllServices->isChecked())
	{
		current = "1:15:fffffffe:12:ffffffff:0:0:0:0:0:";
		eServiceInterface *iface=eServiceInterface::getInstance();
		if (iface)
		{		
			if ((search != "") || (genre != 0))
			{
				eServiceReference current_service=string2ref(current);
				eSearchAllTVServices conv( *iface, search, begin, intExactMatch, intCaseSensitive, genre);
				Signal1<void,const eServiceReference&> signal;
				signal.connect(slot(conv, &eSearchAllTVServices::addEntry));
				iface->enterDirectory(current_service, signal);
				iface->leaveDirectory(current_service);
			}
		}
	}
	else
	{
		eServiceReference current_service=string2ref(sServiceReference);
		SearchInChannel(current_service, search, begin,intExactMatch, intCaseSensitive, genre);
	}
	if (SearchResultsEPG.size() )
		intFound = 0;
	return intFound;
}
int eEPGSearch::EPGSearching(eString title, eServiceReference SearchRef, int AllServices, int ExactMatch, int CaseSensitive, int genre)
{
	eString search;
	time_t begin = 0; //1184515200;
	eString current;
	int intFound = 0;
	search = title;

	
	if (!ExactMatch && !CaseSensitive)
		search = title.upper();

	SearchResultsEPG.clear();
	if (AllServices)
	{
		current = "1:15:fffffffe:12:ffffffff:0:0:0:0:0:";
		eServiceInterface *iface=eServiceInterface::getInstance();
		if (iface)
		{		
			if ((search != "") || (genre != 0))
			{
				eServiceReference current_service=string2ref(current);
				eSearchAllTVServices conv( *iface, search, begin, ExactMatch, CaseSensitive, genre);
				Signal1<void,const eServiceReference&> signal;
				signal.connect(slot(conv, &eSearchAllTVServices::addEntry));
				iface->enterDirectory(current_service, signal);
				iface->leaveDirectory(current_service);
			}
		}
	}
	else
		SearchInChannel(SearchRef, search, begin,ExactMatch, CaseSensitive, genre);
	if (SearchResultsEPG.size() )
		intFound = 1;
	return intFound;
}

int eEPGSearch::EPGSearching(eString title, int ExactMatch, int CaseSensitive, int TimeSpanSearch, tm beginTime, tm endTime, int Days, int Max_Duration )
{
	eString search;
	eString current;
	int intFound = 0;
	search = title;

	
	if (!ExactMatch && !CaseSensitive)
		search = title.upper();

	SearchResultsEPG.clear();
	current = "1:15:fffffffe:12:ffffffff:0:0:0:0:0:";
	eServiceInterface *iface=eServiceInterface::getInstance();
	if (iface)
	{		
		if (search != "")
		{
			eServiceReference current_service=string2ref(current);
			eSearchAllTVServices2 conv( *iface, search,ExactMatch, CaseSensitive, TimeSpanSearch, beginTime, endTime, Days, Max_Duration);
			Signal1<void,const eServiceReference&> signal;
			signal.connect(slot(conv, &eSearchAllTVServices2::addEntry));
			iface->enterDirectory(current_service, signal);
			iface->leaveDirectory(current_service);
		}
	}
	if (SearchResultsEPG.size() )
		intFound = 1;
	return intFound;
}

int eEPGSearch::EPGSearching(eString title, SearchReferences SearchRefs, int ExactMatch, int CaseSensitive, int TimeSpanSearch, tm beginTime, tm endTime, int Days, int Max_Duration )
{
	eString search;
	int intFound = 0;
	search = title;
	if (!ExactMatch && !CaseSensitive)
		search = title.upper();
	SearchResultsEPG.clear();
	for (SearchReferences::iterator a = SearchRefs.begin(); a != SearchRefs.end(); a++)
		SearchInChannel(a->SearchRef, search,ExactMatch, CaseSensitive, TimeSpanSearch, beginTime, endTime, Days, Max_Duration);
	if (SearchResultsEPG.size() )
		intFound = 1;
	return intFound;
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
