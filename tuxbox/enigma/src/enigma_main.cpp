#include <enigma_main.h>

#include <errno.h>
#include <iomanip>
//#include <stdio.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <enigma_mainmenu.h>
#include <enigma_event.h>
#include <sselect.h>
#include <enigma.h>
#include <enigma_lcd.h>
#include <enigma_plugins.h>
#include <download.h>
#include <enigma.h>
#include <lib/base/i18n.h>
#include <lib/system/init.h>
#include <lib/system/econfig.h>
#include <lib/dvb/servicedvb.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/esection.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/iso639.h>
#include <lib/gdi/font.h>
#include <lib/gui/elabel.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/enumber.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/actions.h>
#include <lib/driver/rc.h>
#include <lib/driver/streamwd.h>
#include <lib/driver/eavswitch.h>
#include <lib/dvb/dvbservice.h>
#include <lib/gdi/lcd.h>
#include <lib/gdi/glcddc.h>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/serviceplaylist.h>

		// bis waldi das in nen .h tut
#define MOVIEDIR "/hdd/movie"

struct enigmaMainActions
{
	eActionMap map;
	eAction showMainMenu, standby_press, standby_repeat, standby_release, toggleInfobar, showServiceSelector,
		showSubservices, showAudio, pluginVTXT, showEPGList, showEPG, 
		nextService, prevService, playlistNextService, playlistPrevService,
		serviceListDown, serviceListUp, volumeUp, volumeDown, toggleMute,
		stop, pause, play, record, 
		startSkipForward, repeatSkipForward, stopSkipForward, 
		startSkipReverse, repeatSkipReverse, stopSkipReverse,
		showBouquets, showFavourites,
		modeTV, modeRadio, modeFile,
		toggleDVRFunctions;
	enigmaMainActions(): 
		map("enigmaMain", _("enigma Zapp")),
		showMainMenu(map, "showMainMenu", _("show main menu"), eAction::prioDialog),
		standby_press(map, "standby_press", _("go to standby (press)"), eAction::prioDialog),
		standby_repeat(map, "standby_repeat", _("go to standby (repeat)"), eAction::prioDialog),
		standby_release(map, "standby_release", _("go to standby (release)"), eAction::prioDialog),
		toggleInfobar(map, "toggleInfobar", _("toggle infobar"), eAction::prioDialog),
		showServiceSelector(map, "showServiceSelector", _("show service selector"), eAction::prioDialog),
		showSubservices(map, "showSubservices", _("show subservices/NVOD"), eAction::prioDialog),
		showAudio(map, "showAudio", _("show audio selector"), eAction::prioDialog),
		pluginVTXT(map, "pluginVTXT", _("show Videotext"), eAction::prioDialog),
		showEPGList(map, "showEPGList", _("show epg schedule list"), eAction::prioDialog),
		showEPG(map, "showEPG", _("show extended info"), eAction::prioDialog),
		nextService(map, "nextService", _("quickzap next"), eAction::prioDialog),
		prevService(map, "prevService", _("quickzap prev"), eAction::prioDialog),

		playlistNextService(map, "playlistNextService", _("history next"), eAction::prioDialog),
		playlistPrevService(map, "playlistPrevService", _("history prev"), eAction::prioDialog),
		
		serviceListDown(map, "serviceListDown", _("service list and down"), eAction::prioDialog),
		serviceListUp(map, "serviceListUp", _("service list and up"), eAction::prioDialog),
		volumeUp(map, "volumeUp", _("volume up"), eAction::prioDialog),
		volumeDown(map, "volumeDown", _("volume down"), eAction::prioDialog),
		toggleMute(map, "toggleMute", _("toggle mute flag"), eAction::prioDialog),
		
		stop(map, "stop", _("stop playback"), eAction::prioWidget),
		pause(map, "pause", _("pause playback"), eAction::prioWidget),
		play(map, "play", _("resume playback"), eAction::prioWidget),
		record(map, "record", _("record"), eAction::prioWidget),
		
		startSkipForward(map, "startSkipF", _("start skipping forward"), eAction::prioWidget),
		repeatSkipForward(map, "repeatSkipF", _("repeat skipping forward"), eAction::prioWidget),
		stopSkipForward(map, "stopSkipF", _("stop skipping forward"), eAction::prioWidget),

		startSkipReverse(map, "startSkipR", _("start skipping reverse"), eAction::prioWidget),
		repeatSkipReverse(map, "repeatSkipR", _("repeat skipping reverse"), eAction::prioWidget),
		stopSkipReverse(map, "stopSkipR", _("stop skipping reverse"), eAction::prioWidget),
		
		showBouquets(map, "showBouquets", _("show bouquet list"), eAction::prioWidget),
		showFavourites(map, "showFavourites", _("show favourites"), eAction::prioWidget),

		modeTV(map, "modeTV", _("switch to TV mode"), eAction::prioDialog),
		modeRadio(map, "modeRadio", _("switch to Radio mode"), eAction::prioDialog),
		modeFile(map, "modeFile", _("switch to File mode"), eAction::prioDialog),

		toggleDVRFunctions(map, "toggleDVRFunctions", _("toggle DVR panel"), eAction::prioDialog)
	{
	}
};
eAutoInitP0<enigmaMainActions> i_enigmaMainActions(5, "enigma main actions");

struct enigmaStandbyActions
{
	eActionMap map;
	eAction wakeUp;
	enigmaStandbyActions(): 
		map("enigmaStandby", _("enigma standby")),
		wakeUp(map, "wakeUp", _("wake up enigma"), eAction::prioDialog)
	{
	}
};
eAutoInitP0<enigmaStandbyActions> i_enigmaStandbyActions(5, "enigma standby actions");

class eZapStandby: public eWidget
{
protected:
	int eventHandler(const eWidgetEvent &);
public:
	eZapStandby();
};

int eZapStandby::eventHandler(const eWidgetEvent &event)
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_enigmaStandbyActions->wakeUp)
			close(0);
		else
			break;
		return 0;
	case eWidgetEvent::execBegin:
	{
		eDBoxLCD::getInstance()->switchLCD(0);
		system("/sbin/hdparm -y /dev/ide/host0/bus0/target0/lun0/disc");
		system("/sbin/hdparm -y /dev/ide/host0/bus0/target1/lun0/disc");
		eZapLCD *pLCD=eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdStandby->show();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 1));
		eAVSwitch::getInstance()->setInput(1);

		break;
	}
	case eWidgetEvent::execDone:
	{
		eZapLCD *pLCD=eZapLCD::getInstance();
		pLCD->lcdStandby->hide();
		pLCD->lcdMain->show();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 1));
		eAVSwitch::getInstance()->setInput(0);
		eDBoxLCD::getInstance()->switchLCD(1);
		break;
	}
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

eZapStandby::eZapStandby(): eWidget(0, 1)
{
	addActionMap(&i_enigmaStandbyActions->map);
}

eString getISO639Description(char *iso)
{
	for (unsigned int i=0; i<sizeof(iso639)/sizeof(*iso639); ++i)
	{
		if (!strnicmp(iso639[i].iso639foreign, iso, 3))
			return iso639[i].description1;
		if (!strnicmp(iso639[i].iso639int, iso, 3))
			return iso639[i].description1;
	}
	return eString()+iso[0]+iso[1]+iso[2];
}

void NVODStream::EITready(int error)
{
	eDebug("NVOD eit ready: %d", error);
	
	if (eit.ready && !eit.error)
	{
		for (ePtrList<EITEvent>::const_iterator event(eit.events); event != eit.events.end(); ++event)		// always take the first one
		{
			tm *begin=event->start_time!=-1?localtime(&event->start_time):0;

			if (begin)
				text << std::setfill('0') << std::setw(2) << begin->tm_hour << ':' << std::setw(2) << begin->tm_min;

			time_t endtime=event->start_time+event->duration;
			tm *end=event->start_time!=-1?localtime(&endtime):0;

			if (end)
				text << _(" to ") << std::setw(2) << end->tm_hour << ':' << std::setw(2) << end->tm_min;

			time_t now=time(0)+eDVB::getInstance()->time_difference;
			if ((event->start_time <= now) && (now < endtime))
			{
				int perc=(now-event->start_time)*100/event->duration;
				text << " (" << perc << "%, " << perc*3/100 << '.' << std::setw(2) << (perc*3)%100 << _(" Euro lost)");
			}
			break;
		}
	}
	else
		text << "Service " << std::setw(4) << std::hex << service.getServiceID().get();

	((eListBox<NVODStream>*)listbox)->sort(); // <<< without explicit cast the compiler nervs ;)

	if (listbox && listbox->isVisible())
		listbox->invalidate();

}

NVODStream::NVODStream(eListBox<NVODStream> *listbox, const NVODReferenceEntry *ref, int type)
	: eListBoxEntryTextStream((eListBox<eListBoxEntryTextStream>*)listbox), 
		service(eTransportStreamID(ref->transport_stream_id), eOriginalNetworkID(ref->original_network_id),
			eServiceID(ref->service_id), 5), eit(EIT::typeNowNext, ref->service_id, type)
{
	CONNECT(eit.tableReady, NVODStream::EITready);
	eit.start();
}

void eNVODSelector::selected(NVODStream* nv)
{
	if (nv)
	{
		nv->service.descr = _("NVOD");
		eServiceInterface::getInstance()->play(nv->service);
	}
	close(0);
}

eNVODSelector::eNVODSelector()
	:eListBoxWindow<NVODStream>(_("NVOD"), 10, 440)
{
	move(ePoint(100, 100));
	list.setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));
	CONNECT(list.selected, eNVODSelector::selected);
}

void eNVODSelector::clear()
{
	list.clearList();
}

void eNVODSelector::add(NVODReferenceEntry *ref)
{
	eServiceReference &s=eServiceInterface::getInstance()->service;
	if (s.type != eServiceReference::idDVB)
		return ;
	eServiceReferenceDVB &service=(eServiceReferenceDVB&)s;

	int type= ((service.getTransportStreamID()==eTransportStreamID(ref->transport_stream_id))
			&&	(service.getOriginalNetworkID()==eOriginalNetworkID(ref->original_network_id))) ? EIT::tsActual:EIT::tsOther;
	new NVODStream(&list, ref, type);
}

AudioStream::AudioStream(eListBox<AudioStream> *listbox, PMTEntry *stream)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*)listbox), isAC3(0), component_tag(-1), stream(stream)
{
  for (ePtrList<Descriptor>::iterator c(stream->ES_info); c != stream->ES_info.end(); ++c)
	{
		if (c->Tag()==DESCR_AC3)
			isAC3=1;
		else if (c->Tag()==DESCR_ISO639_LANGUAGE)
			text=getISO639Description(((ISO639LanguageDescriptor*)*c)->language_code);
		else if (c->Tag()==DESCR_STREAM_ID)
			component_tag=((StreamIdentifierDescriptor*)*c)->component_tag;
		else if (c->Tag()==DESCR_LESRADIOS)
		{
			text=eString().sprintf("%d.) ", (((LesRadiosDescriptor*)*c)->id));
			text+=((LesRadiosDescriptor*)*c)->name;
		}
	}
	if (!text)
		text.sprintf("PID %04x", stream->elementary_PID);
	if (isAC3)
		text+=" (AC3)";
  if (component_tag!=-1)
	{
    eServiceHandler *service=eServiceInterface::getInstance()->getService();
		if (service)
		{
      EIT *eit=service->getEIT();
			if (eit)
        parseEIT(eit);
      else
        CONNECT( eDVB::getInstance()->tEIT.tableReady, AudioStream::EITready );
		}
	}
}

void AudioStream::parseEIT(EIT* eit)
{
  for (ePtrList<EITEvent>::iterator e(eit->events); e != eit->events.end(); ++e)
  {
  	if ((e->running_status>=2)||(!e->running_status))		// currently running service
    {
  		for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
      {
				if (d->Tag()==DESCR_COMPONENT)
        {
          if (((ComponentDescriptor*)*d)->component_tag == component_tag )
          {
  					text=((ComponentDescriptor*)*d)->text;
            if (isAC3)
              text+=" (AC3)";
          }
        }
      }
    }
  }
	eit->unlock();  
}

void AudioStream::EITready(int error)
{
  if (!error)
	{
		EIT *eit=eDVB::getInstance()->tEIT.getCurrent();
    parseEIT(eit);
  }
}

void eAudioSelector::selected(AudioStream *l)
{
	eServiceHandler *service=eServiceInterface::getInstance()->getService();
	if (l && service)
	{
		service->setPID(l->stream);
		//service->setDecoder();
	}

	close(0);
}

eAudioSelector::eAudioSelector()
	:eListBoxWindow<AudioStream>(_("Audio"), 10, 330)
{
	move(ePoint(100, 100));
	CONNECT(list.selected, eAudioSelector::selected);
}

void eAudioSelector::clear()
{
	list.clearList();
}

void eAudioSelector::add(PMTEntry *pmt)
{
	new AudioStream(&list, pmt);
}

SubService::SubService(eListBox<SubService> *listbox, const LinkageDescriptor *descr)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*) listbox),
		service(eTransportStreamID(descr->transport_stream_id), 
			eOriginalNetworkID(descr->original_network_id),
			eServiceID(descr->service_id), 1)
{
	text=(const char*)descr->private_data;
}

eSubServiceSelector::eSubServiceSelector()
	:eListBoxWindow<SubService>(_("multiple Services"), 10, 330)
{
	move(ePoint(100, 100));
	CONNECT(list.selected, eSubServiceSelector::selected);
}

void eSubServiceSelector::selected(SubService *ss)
{
	if (ss)
	{
		ss->service.setServiceType(5);  // faked service type
		ss->service.descr = ss->getText();
		eServiceInterface::getInstance()->play(ss->service);
	}
	close(0);
}

void eSubServiceSelector::clear()
{
	list.clearList();
}

void eSubServiceSelector::add(const LinkageDescriptor *ref)
{
	new SubService(&list, ref);
}

void eServiceNumberWidget::selected(int *res)
{
	if (!res)
	{
		close(-1);
		return;
	}
	close(number->getNumber());
}

void eServiceNumberWidget::timeout()
{
	close(number->getNumber());
}

eServiceNumberWidget::eServiceNumberWidget(int initial)
										:eWindow(0)
{
	setText("Channel");
	move(ePoint(200, 140));
	resize(eSize(280, 120));
	eLabel *label;
	label=new eLabel(this);
	label->setText("Channel:");
	label->move(ePoint(50, 15));
	label->resize(eSize(110, eSkin::getActive()->queryValue("fontsize", 20)+4));
	
	number=new eNumber(this, 1, 1, 999, 3, 0, 1, label);
	number->move(ePoint(160, 15));
	number->resize(eSize(50, eSkin::getActive()->queryValue("fontsize", 20)+4));
	number->setNumber(initial);
	CONNECT(number->selected, eServiceNumberWidget::selected);
	
	timer=new eTimer(eApp);
	timer->start(2000);
	CONNECT(timer->timeout, eServiceNumberWidget::timeout);	
}

eServiceNumberWidget::~eServiceNumberWidget()
{
	if (timer)
		delete timer;
}

eZapMain *eZapMain::instance;

void eZapMain::redrawWidget(gPainter *painter, const eRect &where)
{
}

void eZapMain::eraseBackground(gPainter *painter, const eRect &where)
{
}

eZapMain::eZapMain()
	:eWidget(0, 1)
	,mute( eZap::getInstance()->getDesktop( eZap::desktopFB ) )
	,volume( eZap::getInstance()->getDesktop( eZap::desktopFB ) )
	,VolumeBar( &volume ), pMsg(0), message_notifier(eApp, 0), timeout(eApp)
	,clocktimer(eApp), messagetimeout(eApp), progresstimer(eApp)
	,volumeTimer(eApp), state( stateNormal )
{
	if (!instance)
		instance=this;

// Mute Symbol
	gPixmap *pm = eSkin::getActive()->queryImage("mute_symbol");
	int x = eSkin::getActive()->queryValue("mute.pos.x", 0),
		  y = eSkin::getActive()->queryValue("mute.pos.y", 0);

	if (pm && x && y )
	{
		mute.setPixmap(pm);
		mute.move( ePoint(x, y) );
		mute.resize( eSize( pm->x, pm->y ) );
		mute.pixmap_position = ePoint(0,0);
		mute.hide();
		mute.setBlitFlags( BF_ALPHATEST );
	}

// Volume Pixmap
	pm = eSkin::getActive()->queryImage("volume_grafik");
	x = eSkin::getActive()->queryValue("volume.grafik.pos.x", 0),
	y = eSkin::getActive()->queryValue("volume.grafik.pos.y", 0);

	if (pm && x && y )
	{
		volume.setPixmap(pm);
		volume.move( ePoint(x, y) );
		volume.resize( eSize( pm->x, pm->y ) );
		volume.pixmap_position = ePoint(0,0);
		volume.hide();
		volume.setBlitFlags( BF_ALPHATEST );
	}

// Volume Slider
	x = eSkin::getActive()->queryValue("volume.slider.pos.x", 0),
	y = eSkin::getActive()->queryValue("volume.slider.pos.y", 0);
	
	if ( x && y )
		VolumeBar.move( ePoint(x, y) );

	x = eSkin::getActive()->queryValue("volume.slider.width", 0),
	y = eSkin::getActive()->queryValue("volume.slider.height", 0);

	if ( x && y )
		VolumeBar.resize( eSize( x, y ) );

	VolumeBar.setLeftColor( eSkin::getActive()->queryColor("volume_left") );
	VolumeBar.setRightColor( eSkin::getActive()->queryColor("volume_right") );
	VolumeBar.setBorder(0);
	
	dvrFunctions=new eWidget(this);
	dvrFunctions->setName("dvrFunctions");
	dvrFunctions->hide();
	
	dvrfunctions=0;

	isVT=0;
	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "ezap_main"))
		eFatal("skin load of \"ezap_main\" failed");

	eDebug("[PROFILE] eZapMain");
	lcdmain.show();
	eDebug("<-- show lcd.");

	ASSIGN(ChannelNumber, eLabel, "ch_number");
	ASSIGN(ChannelName, eLabel, "ch_name");

	ASSIGN(EINow, eLabel, "e_now_title");
	ASSIGN(EINext, eLabel, "e_next_title");
	
	ASSIGN(EINowDuration, eLabel, "e_now_duration");
	ASSIGN(EINextDuration, eLabel, "e_next_duration");

	ASSIGN(EINowTime, eLabel, "e_now_time");
	ASSIGN(EINextTime, eLabel, "e_next_time");

	ASSIGN(Description, eLabel, "description");
//	ASSIGN(VolumeBar, eProgress, "volume_bar");
	ASSIGN(Progress, eProgress, "progress_bar");
	
	ASSIGN(ButtonRedEn, eLabel, "button_red_enabled");
	ASSIGN(ButtonGreenEn, eLabel, "button_green_enabled");
	ASSIGN(ButtonYellowEn, eLabel, "button_yellow_enabled");
	ASSIGN(ButtonBlueEn, eLabel, "button_blue_enabled");
	ASSIGN(ButtonRedDis, eLabel, "button_red_disabled");
	ASSIGN(ButtonGreenDis, eLabel, "button_green_disabled");
	ASSIGN(ButtonYellowDis, eLabel, "button_yellow_disabled");
	ASSIGN(ButtonBlueDis, eLabel, "button_blue_disabled");

	ASSIGN(DolbyOn, eLabel, "osd_dolby_on");
	ASSIGN(CryptOn, eLabel, "osd_crypt_on");
	ASSIGN(WideOn, eLabel, "osd_format_on");
	ASSIGN(DolbyOff, eLabel, "osd_dolby_off");
	ASSIGN(CryptOff, eLabel, "osd_crypt_off");
	ASSIGN(WideOff, eLabel, "osd_format_off");
	DolbyOn->hide();
	CryptOn->hide();
	WideOn->hide();
	DolbyOff->show();
	CryptOff->show();
	WideOff->show();

	ButtonRedEn->hide();
	ButtonRedDis->show();
	ButtonGreenEn->hide();
	ButtonGreenDis->show();
	ButtonYellowEn->hide();
	ButtonYellowDis->show();
	ButtonBlueEn->hide();
	ButtonBlueDis->show();

	Clock=new eLabel(this);
	ASSIGN(Clock, eLabel, "time");

	cur_start=cur_duration=-1;
	cur_event_text="";
	cur_event_id=-1;

	CONNECT(eEPGCache::getInstance()->EPGAvail, eZapMain::setEPGButton);

	CONNECT(eServiceInterface::getInstance()->serviceEvent, eZapMain::handleServiceEvent);

	CONNECT(timeout.timeout, eZapMain::timeOut);

	CONNECT(clocktimer.timeout, eZapMain::clockUpdate);
	CONNECT(messagetimeout.timeout, eZapMain::nextMessage);
	
	CONNECT(progresstimer.timeout, eZapMain::updateProgress);

	CONNECT(eDVB::getInstance()->timeUpdated, eZapMain::clockUpdate);
	CONNECT(eAVSwitch::getInstance()->volumeChanged, eZapMain::updateVolume);

	CONNECT(message_notifier.recv_msg, eZapMain::gotMessage);

	CONNECT( volumeTimer.timeout, eZapMain::hideVolumeSlider );

	actual_eventDisplay=0;

	clockUpdate();
	standbyTime=-1;
	
	skipcounter=0;
	skipping=0;
	
	state = stateNormal;
	
	addActionMap(&i_enigmaMainActions->map);
	addActionMap(&i_numberActions->map);
	
	gotPMT();
	gotSDT();
	gotEIT();

	playlistref=eServiceReference(eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 1, 0);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeFavourite), playlistref);
	
	curlist=(ePlaylist*)eServiceInterface::getInstance()->addRef(playlistref);
	
	if (!curlist)
		eFatal("couldn't allocate playlist!");

	for (int i = modeTV; i <= modeFile; i++)
	{
		favouriteref[i]=eServiceReference(eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 2, i);
		favourite[i]=(ePlaylist*)eServiceInterface::getInstance()->addRef(favouriteref[i]);
		ASSERT(favourite[i]);
		char *descr="";
		switch (i)
		{
		case modeTV: descr=_("TV"); break;
		case modeRadio: descr=_("Radio"); break;
		case modeFile: descr=_("File"); break;
		}
		favourite[i]->service_name=eString(_("Favourites (")) + eString(descr) + ")";
		favourite[i]->load(eString().sprintf( CONFIGDIR "/enigma/favourite.%d.epl", i).c_str());
		int modemap[]={eServiceStructureHandler::modeTV, eServiceStructureHandler::modeRadio, eServiceStructureHandler::modeFile};
		eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(modemap[i]), favouriteref[i]);
		eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeFavourite), favouriteref[i]);
	}

	recordingsref=eServiceReference(eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 1, 2);
	recordings=(ePlaylist*)eServiceInterface::getInstance()->addRef(recordingsref);
	ASSERT(recordings);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeFile), recordingsref);
	recordings->service_name=_("recorded movies");
	recordings->load(MOVIEDIR "/recordings.epl");

	CONNECT(eZap::getInstance()->getServiceSelector()->addServiceToList, eZapMain::doPlaylistAdd);
	CONNECT(eZap::getInstance()->getServiceSelector()->addServiceToFavourite, eZapMain::addServiceToFavourite);
	CONNECT(eZap::getInstance()->getServiceSelector()->showMenu, eZapMain::showServiceMenu);
	CONNECT(eZap::getInstance()->getServiceSelector()->showFavourite, eZapMain::showFavourite);
	CONNECT(eZap::getInstance()->getServiceSelector()->setMode, eZapMain::setModeD);
	CONNECT(eZap::getInstance()->getServiceSelector()->moveEntry, eZapMain::moveService);
	
	last_mode=mode=-1;

	// read for all modes last servicePath from registry
	mode = modeTV;  // begin by modeTV
	while ( mode < modeEnd )  //.. modeRadio .. modeFile .. modeEnd
	{
		char* str;
		if ( !eConfig::getInstance()->getKey( eString().sprintf("/ezap/ui/modes/%i", mode).c_str(), str) )
			modeLast[mode++].setString(str);
		else
		{
			modeLast[mode]=eServiceStructureHandler::getRoot(mode+1);
			modeLast[mode++].down( eServiceReference() );
		}
	}

	int tmp;
	// read last mode from registry
  if ( eConfig::getInstance()->getKey("/ezap/ui/lastmode", tmp ) )
		tmp = 0;  // defaut TV Mode
		
	if (tmp < 0)
		tmp=0;
		
	mode=-1;  // fake mode for first call of setMode
	curlist->load(CONFIGDIR "/enigma/playlist.epl");

	int style;
	if ( eConfig::getInstance()->getKey("/ezap/ui/serviceSelectorStyle", style ) )
		style=eServiceSelector::styleSingleColumn;  // default we use single Column Style

	eZap::getInstance()->getServiceSelector()->setStyle(style);	
	// now we set the last mode
	setMode(tmp);
	
	if (curlist->current != curlist->list.end())
		playService(*curlist->current, psDontAdd);
	startMessages();
	
	recstatus=new eRecordingStatus();
	recstatus->hide();

}

eZapMain::~eZapMain()
{
	if ( state & (stateRecording|recDVR) )
		recordDVR(0, 0);
/*	else if ( state & (stateRecording|recVCR) )
		recordVCR(0);*/
	
	delete recstatus;
	recstatus=0;

	getPlaylistPosition();
	if (mode != -1)
		getServiceSelectorPath(modeLast[mode]);

  // save last mode to registry
 	eConfig::getInstance()->setKey("ezap/ui/lastmode", (last_mode==-1) ? mode : last_mode);

	// save for all modes the servicePath to registry
	mode=0;	
	while ( mode < modeEnd )
		eConfig::getInstance()->setKey( eString().sprintf("/ezap/ui/modes/%i", mode).c_str(), modeLast[mode++].toString().c_str() );

	for (int i = modeTV; i <= modeFile; i++)
	{
		favourite[i]->save(eString().sprintf( CONFIGDIR "/enigma/favourite.%d.epl", i).c_str());
		eServiceInterface::getInstance()->removeRef(favouriteref[i]);
	}

	curlist->save();
	eServiceInterface::getInstance()->removeRef(playlistref);

	recordings->save();
	eServiceInterface::getInstance()->removeRef(recordingsref);
	
	if (instance == this)
		instance = 0;
	eZapLCD *pLCD=eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdShutdown->show();
	gLCDDC::getInstance()->setUpdate(0);
	eDBoxLCD::getInstance()->switchLCD(0); // BITTE lasst das doch einfach drin :/

	eConfig::getInstance()->setKey("/ezap/ui/serviceSelectorStyle", eZap::getInstance()->getServiceSelector()->getStyle() );
}

void eZapMain::set16_9Logo(int aspect)
{
	if (aspect)
	{
		WideOff->hide();
		WideOn->show();
	} else
	{
		WideOn->hide();
		WideOff->show();
	}
}

void eZapMain::setEPGButton(bool b)
{
	if (b)
	{
		isEPG=1;
		ButtonRedDis->hide();
		ButtonRedEn->show();
	}
	else
	{
		isEPG=0;
		ButtonRedEn->hide();
		ButtonRedDis->show();
	}
}

void eZapMain::setVTButton(bool b)
{
	if (b)
	{
		ButtonBlueDis->hide();
		ButtonBlueEn->show();
	}
	else
	{
		ButtonBlueEn->hide();
		ButtonBlueDis->show();
	}
}

void eZapMain::setAC3Logo(bool b)
{
	if (b)
	{
		DolbyOff->hide();
		DolbyOn->show();
	} else
	{
		DolbyOn->hide();
		DolbyOff->show();
	}
}

void eZapMain::setSmartcardLogo(bool b)
{
	if (b)
	{
		CryptOff->hide();
		CryptOn->show();
	} else
	{
		CryptOn->hide();
		CryptOff->show();
	}
}

void eZapMain::setEIT(EIT *eit)
{
	int numsub=0;
	subservicesel.clear();
	
	cur_event_text="";
	
	if (eit)
	{
		eString nowtext, nexttext, nowtime="", nexttime="", descr;
		int val=0;
		int p=0;
		
		for (ePtrList<EITEvent>::iterator i(eit->events); i != eit->events.end(); ++i)
		{
			EITEvent *event=*i;
			if ((event->running_status>=2) || ((!p) && (!event->running_status)))
			{
				cur_event_id=event->event_id;
				cur_start=event->start_time;
				cur_duration=event->duration;
				clockUpdate();

				for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
					if (d->Tag()==DESCR_LINKAGE)
					{
						LinkageDescriptor *ld=(LinkageDescriptor*)*d;
						if (ld->linkage_type!=0xB0)
							continue;
						subservicesel.add(ld);
						numsub++;
					}
			}
			for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
			{
				Descriptor *descriptor=*d;
				if (descriptor->Tag()==DESCR_SHORT_EVENT)
				{
					ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
					if (event->event_id == cur_event_id)
						cur_event_text=ss->event_name;
					switch (p)
					{
					case 0:
						nowtext=ss->event_name;
						val|=1;
						descr=ss->text;
						break;
					case 1:
						nexttext=ss->event_name;
						val|=2;
						break;
					}
				}
			}
			tm *t=event->start_time!=-1?localtime(&event->start_time):0;
			eString start="";
			if (t && event->duration)
				start.sprintf("%02d:%02d", t->tm_hour, t->tm_min);
			eString duration;
			if (event->duration>0)
				duration.sprintf("%d min", event->duration/60);
			else
				duration="";
			switch (p)
			{
			case 0:
				EINowDuration->setText(duration);
				nowtime=start;
				break;
			case 1:
				EINextDuration->setText(duration);
				nexttime=start;
				break;
			}
			Description->setText(descr);
			p++;
		}
		if (val&1)
		{
			EINow->setText(nowtext);
			EINowTime->setText(nowtime);
		}

		if (val&2)
		{
			EINext->setText(nexttext);
			EINextTime->setText(nexttime);
		}
	} else
	{
		EINow->setText(_("no EPG available"));
		EINext->setText("");
		EINowDuration->setText("");
		EINextDuration->setText("");
		EINowTime->setText("");
		EINextTime->setText("");
	}
	if (numsub>1)
		flags|=ENIGMA_SUBSERVICES;
	else
		flags&=~ENIGMA_SUBSERVICES;
	if (flags&(ENIGMA_NVOD|ENIGMA_SUBSERVICES))
	{
		ButtonGreenEn->show();
		ButtonGreenDis->hide();
	}
	else
	{
		ButtonGreenDis->show();
		ButtonGreenEn->hide();
	}
	ePtrList<EITEvent> dummy;
	if (actual_eventDisplay)
		actual_eventDisplay->setList(eit?eit->events:dummy);
}

void eZapMain::updateProgress()
{
  eZapLCD *pLCD=eZapLCD::getInstance();
	if (serviceFlags & eServiceHandler::flagSupportPosition)
	{
    eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (!handler)
			return;
		int total=handler->getPosition(eServiceHandler::posQueryLength);
		int current=handler->getPosition(eServiceHandler::posQueryCurrent);
		if ((total > 0) && (current != -1))
		{
      Progress->setPerc(current*100/total);
      Progress->show();
			pLCD->lcdMain->Progress->setPerc(current*100/total);
			pLCD->lcdMain->Progress->show();
      int min=total-current;
			int sec=min%60;
			min/=60;
			int sign=-1;
			ChannelNumber->setText(eString().sprintf("%s%d:%02d", (sign==-1)?"-":"", min, sec));
		} else
		{
      pLCD->lcdMain->Progress->hide();
			Progress->hide();
			ChannelNumber->setText("");
		}
	}
	else
	{
    time_t c=time(0)+eDVB::getInstance()->time_difference;
		tm *t=localtime(&c);
		if (t && eDVB::getInstance()->time_difference)
		{
			if ((cur_start <= c) && (c < cur_start+cur_duration))
			{
        Progress->setPerc((c-cur_start)*100/cur_duration);
        Progress->show();
				pLCD->lcdMain->Progress->setPerc((c-cur_start)*100/cur_duration);
				pLCD->lcdMain->Progress->show();
			} else
			{
				Progress->hide();
				pLCD->lcdMain->Progress->hide();
			}
		} else
		{
			Progress->hide();
			pLCD->lcdMain->Progress->hide();
		}
	}
}

void eZapMain::setPlaylistPosition()
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return;
	if (curlist->current != curlist->list.end())
		if (curlist->current->current_position != -1)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekReal, curlist->current->current_position));
}

void eZapMain::getPlaylistPosition()
{
	int time=-1;
	if (serviceFlags & eServiceHandler::flagIsSeekable)
	{
		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (!handler)
			return;
		time=handler->getPosition(eServiceHandler::posQueryRealCurrent);
	
		if ( curlist->current != curlist->list.end() && curlist->current->service == eServiceInterface::getInstance()->service )
			curlist->current->current_position=time;
	}
}


void eZapMain::handleNVODService(SDTEntry *sdtentry)
{
	nvodsel.clear();
	for (ePtrList<Descriptor>::iterator i(sdtentry->descriptors); i != sdtentry->descriptors.end(); ++i)
		if (i->Tag()==DESCR_NVOD_REF)
			for (ePtrList<NVODReferenceEntry>::iterator e(((NVODReferenceDescriptor*)*i)->entries); e != ((NVODReferenceDescriptor*)*i)->entries.end(); ++e)
				nvodsel.add(*e);
	eService *service=eServiceInterface::getInstance()->addRef(eServiceInterface::getInstance()->service);
	if (service)
		nvodsel.setText(service->service_name.c_str());
	eServiceInterface::getInstance()->removeRef(eServiceInterface::getInstance()->service);
}

void eZapMain::showServiceSelector(int dir, int selcurrent)
{
	hide();

	eZapLCD* pLCD = eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdMenu->show();

	entered_playlistmode=0;
	
	eServiceSelector *e = eZap::getInstance()->getServiceSelector();
	e->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);

	getServiceSelectorPath(modeLast[mode]);

	e->selectService(eServiceInterface::getInstance()->service);
	const eServiceReference *service = e->choose(dir);	// reset path only when NOT showing specific list
	
	pLCD->lcdMain->show();
	pLCD->lcdMenu->hide();

	if (!service)
	{
		eServicePath p = modeLast[mode];
		setServiceSelectorPath(modeLast[mode]);
		return;
	}
	
	if (*service == eServiceInterface::getInstance()->service)
		return;

	getServiceSelectorPath(modeLast[mode]);

	if (eZap::getInstance()->getServiceSelector()->getPath().current() != playlistref)
	{
		if (!entered_playlistmode)
			playlistmode=0;
		playService(*service, playlistmode?psAdd:0);
	} else
		playService(*service, psDontAdd);
}

void eZapMain::nextService(int add)
{
	const eServiceReference *service=eZap::getInstance()->getServiceSelector()->next();
	if (!service)
		return;
	else
		getServiceSelectorPath( modeLast[mode] );
	
	if (service->flags & eServiceReference::mustDescent)
		return;

	playService(*service, add?0:psDontAdd);
}

void eZapMain::prevService()
{
	const eServiceReference *service=eZap::getInstance()->getServiceSelector()->prev();
	if (!service)
		return;
	else
		getServiceSelectorPath( modeLast[mode] );
	
	if (service->flags & eServiceReference::mustDescent)
		return;

	playService(*service, psDontAdd);
}

void eZapMain::playlistPrevService()
{
	eDebug("playlist prev");
	getPlaylistPosition();
	if (curlist->current != curlist->list.begin())
	{
		if (playlistmode)
			curlist->current->current_position=-1;
		curlist->current--;
		playService(*curlist->current, psDontAdd);
	}
}

void eZapMain::playlistNextService()
{
	eDebug("playlist next");
	getPlaylistPosition();
	if (curlist->current != curlist->list.end())
	{
		if (playlistmode)
			curlist->current->current_position=-1;
		curlist->current++;
		if (curlist->current == curlist->list.end())
		{
			curlist->current--;
			return;
		}
		playService(*curlist->current, psDontAdd);
	}
}

void eZapMain::volumeUp()
{
	eAVSwitch::getInstance()->changeVolume(0, -2);
	if (!volume.isVisible())
	{
		volume.show();
		volumeTimer.start(5000, true);
	}
//		if (!isVisible())
//			show();
//		timeout.start(1000, 1);
}

void eZapMain::volumeDown()
{
	eAVSwitch::getInstance()->changeVolume(0, +2);
	if (!volume.isVisible())
	{
		volume.show();
		volumeTimer.start(2000, true);
	}
//		if (!isVisible())
//			show();
//		timeout.start(1000, 1);
}

void eZapMain::hideVolumeSlider()
{
	if ( volume.isVisible() )
		volume.hide();
}

void eZapMain::toggleMute()
{
	eAVSwitch::getInstance()->toggleMute();
}

void eZapMain::showMainMenu()
{
	if (isVisible())
		hide();

	eZapLCD* pLCD = eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdMenu->show();

	eMainMenu mm;
	mm.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	mm.show();
	if (mm.exec() == 1)
		eZap::getInstance()->quit();
	mm.hide();

	pLCD->lcdMenu->hide();
	pLCD->lcdMain->show();
}

void eZapMain::standbyPress()
{
	standbyTime = time(0);
}

void eZapMain::standbyRepeat()
{
	if (standbyTime == -1)		// just waking up
	{
		state=stateNormal;
		return;
	}
	int diff = time(0) - standbyTime;
	if (diff > 2)
		standbyRelease();
}

void eZapMain::standbyRelease()
{
	if (standbyTime == -1)		// just waking up
	{
		state=stateNormal;
		return;
	}
	int diff = time(0) - standbyTime;
	standbyTime=-1;
	if (diff > 2)
	{
		if (handleState(1))
			eZap::getInstance()->quit();
	} else
	{
		eZapStandby standby;
		if (isVisible())
			hide();
		standby.show();
		state=stateSleeping;
		standby.exec();
		standby.hide();
	}
}

void eZapMain::showInfobar()
{
	if ( !isVisible() )
		show();

	if (eServiceInterface::getInstance()->service.type == eServiceReference::idDVB)
		timeout.start(10000, 1);
}

void eZapMain::hideInfobar()
{
	if (eServiceInterface::getInstance()->service.type == eServiceReference::idDVB)
	{
		timeout.stop();
		hide();
	}
}

void eZapMain::play()
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return;
	
	if (skipping)
	{
		skipping=0;
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 1));
	} else if (handler->getState() == eServiceHandler::statePause)
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 1));
	else
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekAbsolute, 0));
	updateProgress();
}

void eZapMain::stop()
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return;
	handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 0));
	handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekAbsolute, 0));
	updateProgress();
}

void eZapMain::pause()
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return;
	if (handler->getState() == eServiceHandler::statePause)
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 1));
	else
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 0));
}

int eZapMain::recordDVR(int onoff, int user, eString name)
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return -1;
	
	if (onoff) //  start recording
	{
		if ((state & (stateMask)) == stateRecording)
			recordDVR(0, 0); // try to stop recording.. should not happen
		if ((state & (stateMask)) == stateRecording)
			return -2; // already recording
		eServiceReference ref=eServiceInterface::getInstance()->service;
		if ( (ref.type != eServiceReference::idDVB) || 
				(ref.data[0] < 0) || ref.path.size())
		{
			if (user)
			{
				eMessageBox box(_("Sorry, you cannot record this service."), _("record"), eMessageBox::iconWarning|eMessageBox::btOK );
				box.show();
				box.exec();
				box.hide();
			}
			return -1; // cannot record this service
		} 

		eString filename;
		
		int suffix=0;
		struct stat s;
		do
		{
			filename=MOVIEDIR "/";
			filename+=name;
			if (suffix)
				filename+=eString().sprintf(" [%d]", suffix);
			suffix++;
			filename+=".ts";
		} while (!stat(filename.c_str(), &s));

		eDebug("start recording... to %s", filename.c_str());
		if (handler->serviceCommand(
			eServiceCommand(
				eServiceCommand::cmdRecordOpen, 
				reinterpret_cast<int>(
						strcpy(new char[strlen(filename.c_str())+1], filename.c_str())
					)
				)
			))
		{
			eDebug("couldn't record... :/");
			return -3; // couldn't record... warum auch immer.
		} else
		{
			eServiceReference ref2=ref;
			ref2.path=filename;
			ePlaylistEntry en(ref2);
			en.type=ePlaylistEntry::PlaylistEntry|ePlaylistEntry::boundFile;
			recordings->list.push_back(en); // add to playlist
			recordings->save(MOVIEDIR "/recordings.epl");
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStart));
			eDebug("ok, recording...");
			oldstate=state; // because of sleeping etc.
			state=stateRecording|recDVR;
			recstatus->show();
		}
		return 0;
	} else
	{
		if ((state & stateMask) != stateRecording)
			return -1; // not recording
		state=oldstate;
		
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStop));
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordClose));
		recstatus->hide();
		if (user)
		{
			setMode(modeFile);
			showBouquetList(1);
			
		}
		return 0;
	}
}

void eZapMain::startSkip(int dir)
{
	skipcounter=0;
	eDebug("startSkip: %d", dir);
	{
		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekBegin));
	}
	if (!isVisible())
		show();
	timeout.stop();
}

void eZapMain::repeatSkip(int dir)
{
	eDebug("repeatSkip: %d", dir);
	if (!skipping)
	{
		skipcounter++;
		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSkip, (dir == skipForward)?5000:-5000));
		updateProgress();
	}
}

void eZapMain::stopSkip(int dir)
{
	eDebug("stopSkip: %d", dir);
#if 0
	if (!skipcounter)
	{
		if (dir == skipForward)
			skipping++;
		else
			skipping--;
			
		if (skipping > 3)
			skipping=3;
		else if (skipping < -3)
			skipping=-3;

		int speed;
		if (!skipping)
			speed=1;
		else if (skipping < 0)
			speed=-(1<<(-skipping));
		else
			speed=1<<skipping;

		eDebug("speed: %d", speed);
		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, speed));
	}
#endif
	{
		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekEnd));
	}

/*	if (isVisible())
		timeout.start(1000, 1);*/
}

void eZapMain::showServiceMenu(eServiceSelector *sel)
{
	eServiceReference ref=sel->getSelected();
	eServiceReference path=sel->getPath().current();

	sel->hide();
	eServiceContextMenu m(ref, path);
	m.show();
	int res=m.exec();
	m.hide();
	switch (res)
	{
	case 0: // cancel
		break;
	case 1: // delete service
	{
		if (eServiceInterface::getInstance()->service == ref)
			eServiceInterface::getInstance()->stop();
		ePlaylist *pl=0;
		if (path.type == eServicePlaylistHandler::ID)
			pl=(ePlaylist*)eServiceInterface::getInstance()->addRef(path);
		if (!pl)
		{
			eMessageBox box(_("Sorry, you cannot delete this service."), _("delete service"), eMessageBox::iconWarning|eMessageBox::btOK );
			box.show();
			box.exec();
			box.hide();
			break;
		}
		eMessageBox box(_("Really delete this service?"), _("delete service"), eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo);
		box.show();
		int r=box.exec();
		box.hide();
		if (r == eMessageBox::btYes)
		{
			std::list<ePlaylistEntry>::iterator it=std::find(pl->list.begin(), pl->list.end(), ref);
			pl->deleteService(it);
			pl->save();
			sel->actualize();
		}
		eServiceInterface::getInstance()->removeRef(path);
		break;
	}
	case 2: // move service
	{
			// enable move mode in listbox - we get a callback
			// when move has finished
		sel->setMoveMode(1);
		break;
	}
	case 3: // add service to playlist
		doPlaylistAdd(sel->getSelected());
		break;
	case 4: // add service to favourite
		addServiceToFavourite(sel);
	}
	sel->show();
}

void eZapMain::showFavourite(eServiceSelector *)
{
	if (last_mode == -1)	// not in playlist mode
	{
		playlistmode=1;
		setMode(modePlaylist);
	} else
		setMode(last_mode);
}

void eZapMain::playService(const eServiceReference &service, int flags)
{
	int first=0;
	if (flags & psDontAdd)
	{
		if ((curlist->current != curlist->list.end() ) && (curlist->current->service != service))
			getPlaylistPosition();
		eServiceInterface::getInstance()->play(service);

		std::list<ePlaylistEntry>::iterator i=std::find(curlist->list.begin(), curlist->list.end(), service);
		if (i != curlist->list.end())
		{
			curlist->current=i;
			setPlaylistPosition();
		}
		return;
	}
		// we assume that no "isDirectory" is set - we won't open the service selector again.
	if (!(flags & psAdd))
	{
		if (!playlistmode)		// dem user liebgewonnene playlists nicht einfach killen
			while (curlist->list.size() > 10)
				curlist->list.pop_front();
		if ((!playlistmode) && (service.flags & eServiceReference::mustDescent)) // a playlist/complete directory..
		{
			curlist->list.clear();
			first=1;
			playlistmode=1;
		} else
		{
			curlist->current=curlist->list.end();
			if (curlist->current == curlist->list.begin())
				first=1;
		}
	}
	addService(service);
	if (! (flags & psAdd))
	{
		if (first)
			curlist->current = curlist->list.begin();
		if (curlist->current != curlist->list.end())
			eServiceInterface::getInstance()->play(*curlist->current);
		else
		{
			Description->setText(_("This directory doesn't contain anything playable!"));
			postMessage(eZapMessage(0, _("Play"), _("This directory doesn't contain anything playable!")), 1);
		}
	}
	if (eZap::getInstance()->getServiceSelector()->getPath().current() == playlistref)
		eZap::getInstance()->getServiceSelector()->actualize();
}

void eZapMain::addService(const eServiceReference &service)
{
	if (service.flags & eServiceReference::mustDescent) // recursive add services..
	{
		Signal1<void,const eServiceReference&> signal;
		CONNECT(&signal, eZapMain::addService);
		eServiceInterface::getInstance()->enterDirectory(service, signal);
		eServiceInterface::getInstance()->leaveDirectory(service);
	} else
	{
		int last=0;
		if (curlist->current != curlist->list.end() && *curlist->current == service)
		{
			++curlist->current;
			last=1;
		}
		curlist->list.remove(service);
		curlist->list.push_back(ePlaylistEntry(service));
		if ((curlist->current == curlist->list.end()) || last)
			--curlist->current;
	}
}

void eZapMain::doPlaylistAdd(const eServiceReference &service)
{
	entered_playlistmode=1;
	if (!playlistmode)
	{
		playlistmode=1;
		curlist->list.clear();
		curlist->current=curlist->list.begin();
		playService(service, 0);
	} else
		playService(service, psAdd);
}

void eZapMain::addServiceToFavourite(eServiceSelector *sel)
{
	const eServiceReference &service=sel->getSelected();
	if ((mode > modeFile) || (mode < 0))
		return;
	
	for (std::list<ePlaylistEntry>::iterator i(favourite[mode]->list.begin()); i != favourite[mode]->list.end(); ++i)
		if (i->service == service)
		{
			eMessageBox box(_("This service is already in your favourite list."), _("Add Channel to Favourite"), eMessageBox::iconWarning|eMessageBox::btOK);
			sel->hide();
			box.show();
			box.exec();
			box.hide();
			sel->show();
			return;
		}

	eMessageBox box(_("Really add this channel to your favourite list?"), _("Add Channel to Favourite"), eMessageBox::iconQuestion|eMessageBox::btYes|eMessageBox::btNo);
	sel->hide();
	box.show();
	int res=box.exec();
	box.hide();
	sel->show();
	
	if ( res == eMessageBox::btYes )
	{
		favourite[mode]->list.push_back(service);
		favourite[mode]->save();
	}
}

void eZapMain::showSubserviceMenu()
{
	if (!(flags & (ENIGMA_NVOD|ENIGMA_SUBSERVICES)))
		return;

	eZapLCD* pLCD = eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdMenu->show();
	if (flags&ENIGMA_NVOD)
	{
		if (isVisible())
		{
			timeout.stop();
			hide();
		}
		nvodsel.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		nvodsel.show();
		nvodsel.exec();
		nvodsel.hide();
	}
	else if (flags&ENIGMA_SUBSERVICES)
	{
		if (isVisible())
		{
			timeout.stop();
			hide();
		}
		subservicesel.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		subservicesel.show();
		subservicesel.exec();
		subservicesel.hide();
	}
	pLCD->lcdMenu->hide();
	pLCD->lcdMain->show();
}

void eZapMain::showAudioMenu()
{
	if (flags&ENIGMA_AUDIO)
	{
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
		if (isVisible())
		{
			timeout.stop();
			hide();
		}
		audiosel.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		audiosel.show();
		audiosel.exec();
		audiosel.hide();
		pLCD->lcdMenu->hide();
		pLCD->lcdMain->show();
	}
}

extern eString getInfo(const char *file, const char *info);

void eZapMain::runVTXT()
{
	if (isVT)
	{
		eZapPlugins plugins;
		
		struct dirent **namelist;

		int n = scandir(PLUGINDIR "/", &namelist, 0, alphasort);

		if (n < 0)
		{
			eDebug("Read Plugin Directory");
			eMessageBox msg(_("Couldn't read plugin directory"), _("Error"), eMessageBox::iconError|eMessageBox::btOK );
			msg.show();
			msg.exec();
			msg.hide();
			return;
		}

		int executed=0;
		for(int count=0;count<n;count++)
		{
			eString	FileName(PLUGINDIR "/");
			FileName+=namelist[count]->d_name;
			if ( !executed && FileName.find(".cfg") != eString::npos )
			{
				eString aneedvtxtpid=getInfo(FileName.c_str(), "needvtxtpid");
				eString aneedlcd=getInfo(FileName.c_str(), "needlcd");
				// we search tuxtxt... this use lcd and vtxtpid... not perfekt... but i havo no other idea
				if ((aneedvtxtpid.isNull() ? false : atoi(aneedvtxtpid.c_str())) &&
            (aneedlcd.isNull() ? false : atoi(aneedlcd.c_str())) )
				{
					plugins.execPluginByName(namelist[count]->d_name);
					executed=1;
				}
			}
			free(namelist[count]);
		}
		free(namelist);
	}
}

void eZapMain::showEPGList()
{
	eServiceReferenceDVB &service=(eServiceReferenceDVB&)eServiceInterface::getInstance()->service;
	if (service.type != eServiceReference::idDVB)
		return;
	if (isEPG)
	{
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
		eEPGSelector wnd(service);
		wnd.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		if (isVisible())
		{
			timeout.stop();
			hide();
		}
		wnd.show();
		wnd.exec();
		wnd.hide();
		pLCD->lcdMenu->hide();
		pLCD->lcdMain->show();
	}
}

void eZapMain::showEPG()
{
	const eService *service=eServiceInterface::getInstance()->addRef( eServiceInterface::getInstance()->service );

	if (!service && eServiceInterface::getInstance()->service.type == eServiceReference::idDVB && !(eServiceInterface::getInstance()->service.flags & eServiceReference::isDirectory) )
		return;
		
	if (isVisible())
	{
		timeout.stop();
		hide();
	}

	if (isEPG)
	{
		const eventMap* pMap = eEPGCache::getInstance()->getEventMap( (eServiceReferenceDVB&)eServiceInterface::getInstance()->service );
		if (pMap)  // EPG vorhanden
		{
			eventMap::const_iterator It = pMap->begin();
				
			ePtrList<EITEvent> events;
			events.setAutoDelete(true);
			
			while (It != pMap->end())
			{
				events.push_back( new EITEvent(*It->second) );
				It++;
			}
			eEventDisplay ei( service->service_name.c_str(), &events );			
			actual_eventDisplay=&ei;
			eZapLCD* pLCD = eZapLCD::getInstance();
			pLCD->lcdMain->hide();
			pLCD->lcdMenu->show();
			ei.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
			ei.show();
			ei.exec();
			ei.hide();
			pLCD->lcdMenu->hide();
			pLCD->lcdMain->show();
			actual_eventDisplay=0;
		}
	}
	else	
	{
		EIT *eit=eDVB::getInstance()->getEIT();
		ePtrList<EITEvent> dummy;
		eEventDisplay ei(service->service_name.c_str(), eit?&eit->events:&dummy);
		if (eit)
			eit->unlock();		// HIER liegt der hund begraben.
		actual_eventDisplay=&ei;
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
		ei.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		ei.show();
		ei.exec();
		ei.hide();
		pLCD->lcdMenu->hide();
		pLCD->lcdMain->show();
		actual_eventDisplay=0;
	}
	eServiceInterface::getInstance()->removeRef( eServiceInterface::getInstance()->service );
}

bool eZapMain::handleState(int notimer)
{
	eString text, caption;
//	caption=_("Warning!");
	bool b=false;
	if ( (state & (stateMask|recDVR)) == (stateRecording|recDVR) )
	{
		if ( state & stateInTimerMode )
			text=_("Currently an timer based digital recording is in progress!");
		else
			text=_("Currently an digital recording is in progress!");
	}
	else if ( (state & (stateMask|recDVR)) == (stateRecording|recVCR) )
		return false;  // we cancel all actions...
	else if ( ((state & stateMask) == stateInTimerMode ) && notimer )
		text=_("Currently an timer based event is in progress!");
	else		// not recording
		return true; 
// show this message
/*	{
		eMessageBox box(text,caption, eMessageBox::iconWarning|eMessageBox::btOK );		
		box.show();
		box.exec();
		box.hide();
	} */
	caption=_("Really do this?");
	if ( (state & (stateMask|recDVR)) == (stateRecording|recDVR) )
	{
		text+="\n";
		if ( state & stateInTimerMode )
			text+=_("This stops the timer, and digital recording!");
		else
			text+=_("This stops the recording!");
	}
	else
	{
		text+="\n";
		text+=_("This stops the timer based event!");
	}
// show this message
	{
		eMessageBox box(text,caption, eMessageBox::iconQuestion|eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btNo );		
		box.show();
		b = (box.exec() == eMessageBox::btYes);
		box.hide();
	}
	return b;
//	return true;
}

int eZapMain::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
	{
		int num=0;
		stopMessages();
		if (event.action == &i_enigmaMainActions->showMainMenu)
		{
			int oldmode=mode;
			showMainMenu();
			if (mode != oldmode)
				showServiceSelector(-1, 0);
		} else if (event.action == &i_enigmaMainActions->standby_press)
			standbyPress();
		else if (event.action == &i_enigmaMainActions->standby_repeat)
			standbyRepeat();
		else if (event.action == &i_enigmaMainActions->standby_release)
			standbyRelease();
		else if ((!isVisible()) && (event.action == &i_enigmaMainActions->toggleInfobar))
			showInfobar();
		else if (isVisible() && (event.action == &i_enigmaMainActions->toggleInfobar))
			hideInfobar();
		else if (event.action == &i_enigmaMainActions->showServiceSelector && handleState(0))
			showServiceSelector(-1, 1);
		else if (event.action == &i_enigmaMainActions->showSubservices && handleState(0))
			showSubserviceMenu();
		else if (event.action == &i_enigmaMainActions->showAudio && handleState(0))
			showAudioMenu();
		else if (event.action == &i_enigmaMainActions->pluginVTXT && handleState(1))
			runVTXT();
		else if (event.action == &i_enigmaMainActions->showEPGList)
			showEPGList();
		else if (event.action == &i_enigmaMainActions->nextService && handleState(0))
			nextService();
		else if (event.action == &i_enigmaMainActions->prevService && handleState(0))
			prevService();
		else if (event.action == &i_enigmaMainActions->playlistNextService && handleState(0))
			playlistNextService();
		else if (event.action == &i_enigmaMainActions->playlistPrevService && handleState(0))
			playlistPrevService();
		else if (event.action == &i_enigmaMainActions->serviceListDown && handleState(0))
			showServiceSelector(eServiceSelector::dirDown, 1);
		else if (event.action == &i_enigmaMainActions->serviceListUp && handleState(0))
			showServiceSelector(eServiceSelector::dirUp, 1);
		else if (event.action == &i_enigmaMainActions->volumeUp)
			volumeUp();
		else if (event.action == &i_enigmaMainActions->volumeDown)
			volumeDown();
		else if (event.action == &i_enigmaMainActions->toggleMute)
			toggleMute();
		else if (dvrfunctions && event.action == &i_enigmaMainActions->play)
			play();
		else if (dvrfunctions && event.action == &i_enigmaMainActions->stop)
			stop();
		else if (dvrfunctions && event.action == &i_enigmaMainActions->pause)
			pause();
		else if (dvrfunctions && event.action == &i_enigmaMainActions->record && handleState(0))
		{
 			if ((state & stateMask) == stateRecording)
 				recordDVR(0, 1);
 			else
 			{
				eString name;
				eServiceReference ref=eServiceInterface::getInstance()->service;
				eService *service=eServiceInterface::getInstance()->addRef(ref);
				if (service)
				{
					name=service->service_name;
					eServiceInterface::getInstance()->removeRef(ref);
				} else
					name+="record";
				if (cur_event_text != "")
					name+=" - " + cur_event_text;
				recordDVR(1, 1, name);
			}
		}
		else if (dvrfunctions && event.action == &i_enigmaMainActions->startSkipForward)
			startSkip(skipForward);
		else if (dvrfunctions && event.action == &i_enigmaMainActions->repeatSkipForward)
			repeatSkip(skipForward);
		else if (dvrfunctions && event.action == &i_enigmaMainActions->stopSkipForward)
			stopSkip(skipForward);
		else if (dvrfunctions && event.action == &i_enigmaMainActions->startSkipReverse)
			startSkip(skipReverse);
		else if (dvrfunctions && event.action == &i_enigmaMainActions->repeatSkipReverse)
			repeatSkip(skipReverse);
		else if (dvrfunctions && event.action == &i_enigmaMainActions->stopSkipReverse)
			stopSkip(skipReverse);
		else if (event.action == &i_enigmaMainActions->showEPG)
			showEPG();
		else if (event.action == &i_numberActions->key1)
			num=1;
		else if (event.action == &i_numberActions->key2)
			num=2;
		else if (event.action == &i_numberActions->key3)
			num=3;
		else if (event.action == &i_numberActions->key4)
			num=4;
		else if (event.action == &i_numberActions->key5)
			num=5;
		else if (event.action == &i_numberActions->key6)
			num=6;
		else if (event.action == &i_numberActions->key7)
			num=7;
		else if (event.action == &i_numberActions->key8)
			num=8;
		else if (event.action == &i_numberActions->key9)
			num=9;
		else if (event.action == &i_enigmaMainActions->showBouquets && handleState(0))
		{
			showBouquetList(0);
		} else if (event.action == &i_enigmaMainActions->showFavourites && handleState(0))
		{
			showFavourites();
		} else if (event.action == &i_enigmaMainActions->modeRadio && handleState(0))
		{
			setMode(modeRadio);
			showServiceSelector(-1, 1);
		} else if (event.action == &i_enigmaMainActions->modeTV && handleState(0))
		{
			setMode(modeTV);
			showServiceSelector(-1, 1);
		} else if (event.action == &i_enigmaMainActions->toggleDVRFunctions)
		{
			showDVRFunctions(!dvrfunctions);
			if (!isVisible())
				showInfobar();
		} else
		{
			startMessages();
			break;
		}
		startMessages();
		if ( num && handleState(0))
		{
			if (isVisible())
				hide();
			eServiceNumberWidget s(num);
			s.show();
			num = s.exec();
			if (num != -1)
			{
				if ( eZap::getInstance()->getServiceSelector()->selectService( num ) )
				{
					getServiceSelectorPath(modeLast[mode]);
					playService(modeLast[mode].current(), psDontAdd);
				}
				else
					setServiceSelectorPath(modeLast[mode]);
			}
			num = 0;
  	}
		return 1;
	}
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

void eZapMain::handleServiceEvent(const eServiceEvent &event)
{
	switch (event.type)
	{
	case eServiceEvent::evtStateChanged:
		break;
	case eServiceEvent::evtFlagsChanged:
	{
		serviceFlags = eServiceInterface::getInstance()->getService()->getFlags();
		setSmartcardLogo( serviceFlags & eServiceHandler::flagIsScrambled );
		if (serviceFlags & eServiceHandler::flagSupportPosition)
		{
			eDebug("support position query..");
			progresstimer.start(1000);
  		updateProgress();
    } else
		{	
			eDebug("doesn't support position query");
			progresstimer.stop();
		}
		showDVRFunctions(serviceFlags & eServiceHandler::flagIsSeekable);

		break;
	}
	case eServiceEvent::evtAspectChanged:
	{
		int aspect = eServiceInterface::getInstance()->getService()->getAspectRatio();
		set16_9Logo(aspect);
		break;
	}
	case eServiceEvent::evtStart:
	{
		int err = eServiceInterface::getInstance()->getService()->getErrorInfo();
		serviceFlags = eServiceInterface::getInstance()->getService()->getFlags();
		startService(eServiceInterface::getInstance()->service, err);
		cur_event_text="";
		cur_event_id=0;
		break;
	}
	case eServiceEvent::evtStop:
		leaveService();
		break;
	case eServiceEvent::evtGotEIT:
		gotEIT();
		break;
	case eServiceEvent::evtGotSDT:
		gotSDT();
		break;
	case eServiceEvent::evtGotPMT:
		gotPMT();
		break;
	case eServiceEvent::evtEnd:
	{
		eDebug("evtEnd - next entry!");
		if (curlist->current != curlist->list.end())
		{
			curlist->current->current_position=-1;
			++curlist->current;
		}
		if (curlist->current != curlist->list.end() )
		{
			curlist->current->current_position=-1;		// start from beginning.
			eServiceInterface::getInstance()->play(*curlist->current);
		}
		else if (!playlistmode)
			nextService(1);
		else
			eDebug("ende im gelaende.");
		break;
	}
	}
}

void eZapMain::startService(const eServiceReference &_serviceref, int err)
{
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
		return;
		
	eService *service=eServiceInterface::getInstance()->addRef(_serviceref);

	if (_serviceref.type == eServiceReference::idDVB)
	{
		isVT = Decoder::parms.tpid != -1;

		const eServiceReferenceDVB &serviceref=(const eServiceReferenceDVB&)_serviceref;

		setVTButton(isVT);

			// es wird nur dann versucht einen service als referenz-service zu uebernehmen, wenns den ueberhaupt
			// gibt.

		if (service)
			switch (serviceref.getServiceType())
			{
			case 1:	// TV
			case 2: // radio
			case 4: // nvod ref
				refservice=serviceref;
				break;
			}

		eService *rservice=0;
		if (refservice != serviceref)
			rservice=eDVB::getInstance()->settings->getTransponders()->searchService(refservice);
	
		if (refservice.getServiceType()==4)
			flags|=ENIGMA_NVOD;
		else
			flags&=~ENIGMA_NVOD;

		eString name="";

		if (rservice)
			name=rservice->service_name + " - ";
	
		if (service)
			name+=service->service_name;
		else
			switch (serviceref.getServiceType())
			{
			case 5: // nvod stream or linkage subservice ( type faked in SubServiceSelector::selected )
				name+=serviceref.descr;
			}

		if (!name.length())
			name="unknown service";

		ChannelName->setText(name);
	        eZapLCD* pLCD = eZapLCD::getInstance();
	        pLCD->lcdMain->setServiceName(name);

		switch (err)
		{
		case 0:
			Description->setText("");
			postMessage(eZapMessage(0), 1);
			break;
		case -EAGAIN:
			Description->setText(_("One moment please..."));
			postMessage(eZapMessage(0, _("switch"), _("One moment please...")), 1);
			break;
		case -ENOENT:
			Description->setText(_("Service could not be found !"));
			postMessage(eZapMessage(0, _("switch"), _("Service could not be found !")), 1);
			eDebug("Sender konnte nicht gefunden werden.");
			break;
		case -ENOCASYS:
		{
			int serviceFlags = eServiceInterface::getInstance()->getService()->getFlags();
			if( serviceFlags & eServiceHandler::flagIsScrambled )
			{
				Description->setText(_("This service could not be descrambled"));
				postMessage(eZapMessage(0, _("switch"), _("This service could not be descrambled")), 1);
				eDebug("This service could not be descrambled");
			}
			break;
		}
		case -ENOSTREAM:
			Description->setText(_("This service sends (currently) no signal"));
			postMessage(eZapMessage(0, _("switch"), _("This service sends (currently) no signal")), 1);
			eDebug("This service sends (currently) no signal");
			break;
		case -ENOSYS:
			Description->setText(_("This content could not be displayed"));
			eDebug("This content could not be displayed");
			postMessage(eZapMessage(0, _("switch"), _("This content could not be displayed")), 1);
			break;
		case -ENVOD:
			Description->setText(_("NVOD Service - select a starttime, please"));
			eDebug("NVOD Service - select a starttime, please");
			postMessage(eZapMessage(0, _("switch"), _("NVOD Service - select a starttime, please")), 1);
			break;
		default:
			Description->setText(_("Unknown error!!"));
			eDebug("Unknown error!!");
			postMessage(eZapMessage(0, _("switch"), _("Unknown error!!")), 1);
			break;
		}

		int num=-1;
	
		if (rservice)
			num=eZap::getInstance()->getServiceSelector()->getServiceNum(_serviceref);
		else if (service)
			num=eZap::getInstance()->getServiceSelector()->getServiceNum(_serviceref);

		if (num != -1)
			ChannelNumber->setText(eString().sprintf("%d", num));
		else
			ChannelNumber->setText("");

		if (flags&(ENIGMA_NVOD|ENIGMA_SUBSERVICES))
		{
			ButtonGreenDis->hide();
			ButtonGreenEn->show();
		}
		else
		{
			ButtonGreenEn->hide();
			ButtonGreenDis->show();	
		}

		if (flags&ENIGMA_AUDIO)
		{
			ButtonYellowDis->hide();
			ButtonYellowEn->show();
		}
		else
		{
			ButtonYellowEn->hide();
			ButtonYellowDis->show();
		}
	} else
	{
		eString name;
		postMessage(eZapMessage(0), 1);
		if (service)
			name=service->service_name;
		else
			name="bla :(";

		ChannelName->setText(name);	
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->setServiceName(name);
	}
	
	if (!eZap::getInstance()->focus)
		show();

	eServiceInterface::getInstance()->removeRef(_serviceref);

// Quick und Dirty ... damit die aktuelle Volume sofort angezeigt wird.
	eAVSwitch::getInstance()->sendVolumeChanged();

	if ( eServiceInterface::getInstance()->service.type == eServiceReference::idDVB )
		timeout.start((sapi->getState() == eServiceHandler::statePlaying)?10000:2000, 1);
}

void eZapMain::gotEIT()
{
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
		return;

	EIT *eit=sapi->getEIT();
	int old_event_id=cur_event_id;
	
	setEIT(eit);

	if (eit)
	{
		int state=0;
		if (old_event_id != cur_event_id)
		{
			eConfig::getInstance()->getKey("/ezap/osd/showOSDOnEITUpdate", state);

			if (!eZap::getInstance()->focus && state)
			{
				if (!isVisible())
					show();
				if (eServiceInterface::getInstance()->service.type == eServiceReference::idDVB)
					timeout.start((sapi->getState() == eServiceHandler::statePlaying)?10000:2000, 1);
			}
		}
		eit->unlock();
	}
}

void eZapMain::gotSDT()
{
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
		return;
		
	SDT *sdt=sapi->getSDT();
	if (!sdt)
		return;

	switch (((eServiceReferenceDVB&)eServiceInterface::getInstance()->service).getServiceType())
	{
	case 0x4:	// nvod reference
	{
		for (ePtrList<SDTEntry>::iterator i(sdt->entries); i != sdt->entries.end(); ++i)
		{
			if (eServiceID(i->service_id)==((eServiceReferenceDVB&)eServiceInterface::getInstance()->service).getServiceID())
			{
				handleNVODService(*i);
			}
		}
		break;
	}
	}
	sdt->unlock();
}

void eZapMain::gotPMT()
{
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
		return;
		
	PMT *pmt=sapi->getPMT();
	if (!pmt)
		return;

	bool isAc3 = false;
	eDebug("got pmt");
	int numaudio=0;
	audiosel.clear();
	for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
	{
		PMTEntry *pe=*i;
		int isaudio=0;
		if (pe->stream_type==3)
			isaudio=1;
		if (pe->stream_type==4)
			isaudio=1;
		if (pe->stream_type==6)
		{
			for (ePtrList<Descriptor>::iterator d(pe->ES_info); d != pe->ES_info.end(); ++d)
				if (d->Tag()==DESCR_AC3)
				{
					isaudio++;
					isAc3=true;
				}
				
		}
		if (isaudio)
		{
			audiosel.add(pe);
			numaudio++;
		}
	}
	if (numaudio>1)
		flags|=ENIGMA_AUDIO;
	else
		flags&=~ENIGMA_AUDIO;
		
	setAC3Logo(isAc3);
	
	pmt->unlock();
}

void eZapMain::timeOut()
{
	if (eZap::getInstance()->focus==this)
		hide();
}

void eZapMain::leaveService()
{
	eDebug("leaving service");

//	flags=0;
	
	ChannelName->setText("");
	ChannelNumber->setText("");
	Description->setText("");

	EINow->setText("");
	EINowDuration->setText("");
	EINowTime->setText("");
	EINext->setText("");
	EINextDuration->setText("");
	EINextTime->setText("");

	Progress->hide();
}

void eZapMain::clockUpdate()
{
	time_t c=time(0)+eDVB::getInstance()->time_difference;
	tm *t=localtime(&c);
	eZapLCD *pLCD=eZapLCD::getInstance();
	if (t && eDVB::getInstance()->time_difference)
	{
		eString s;
		s.sprintf("%02d:%02d", t->tm_hour, t->tm_min);
		clocktimer.start((70-t->tm_sec)*1000);
		Clock->setText(s);
		pLCD->lcdMain->Clock->setText(s);
		pLCD->lcdStandby->Clock->setText(s);
	} else
	{
		Clock->setText("--:--");
		pLCD->lcdMain->Clock->setText("--:--");
		pLCD->lcdStandby->Clock->setText("--:--");
		clocktimer.start(60000);
	}
	updateProgress();
}

void eZapMain::updateVolume(int vol)
{
	if ( vol == 63 && !mute.isVisible() )
	{
		if ( volume.isVisible() )
			volume.hide();
		mute.show();
	}
	else if (vol < 63 && mute.isVisible() )
		mute.hide();

	VolumeBar.setPerc((63-vol)*100/63);
}

void eZapMain::postMessage(const eZapMessage &message, int clear)
{
	eLocker l(messagelock);
	
	int c=0;
	if (clear)
	{
		for (std::list<eZapMessage>::iterator i(messages.begin()); i != messages.end(); )
		{
			if (message.isSameType(*i))
			{
				if (i == messages.begin())
				{
					c=1;
					++i;
				} else
					i = messages.erase(i);
			} else
				++i;
		}
	}
	if (!message.isEmpty())
		messages.push_back(message);
	message_notifier.send(c);
}

void eZapMain::gotMessage(const int &c)
{
	if ((!c) && pMsg) // noch eine gueltige message vorhanden
	{
		return;
	}
	if ((!isVisible()) && eZap::getInstance()->focus)
	{
		eDebug("message will be delivered later.");
		pauseMessages();
		message_notifier.send(c);
		return;
	}
	pauseMessages();
	while (!messages.empty())
	{
		nextMessage();
		if (pMsg)
			break;
	}
	startMessages();
}

void eZapMain::nextMessage()
{
	eZapMessage msg;
	messagelock.lock();
	
	messagetimeout.stop();

	if (pMsg)
	{
#if 0
		if (pMsg->in_loop)
			pMsg->close();
#endif
		pMsg->hide();
		delete pMsg;
		pMsg=0;
		messages.pop_front();
	}

	std::list<eZapMessage>::iterator i(messages.begin());
 	if (i != messages.end())
 	{
 		msg=messages.front();
		messagelock.unlock();
		int showonly=msg.getTimeout()>=0;
		if (!showonly)
			hide();
		pMsg = new eMessageBox(msg.getBody(), msg.getCaption(), showonly?0:eMessageBox::btOK);
		pMsg->show();
		if (!showonly)
		{
			pMsg->exec();
			pMsg->hide();
			delete pMsg;
			pMsg=0;
			messages.pop_front();
		} else if (msg.getTimeout())
			messagetimeout.start(msg.getTimeout()*1000, 1);
	} else
		messagelock.unlock();
}

void eZapMain::stopMessages()
{
	pauseMessages();
	if (pMsg)
	{
		delete pMsg;
		pMsg=0;
		messages.pop_front();
	}
}

void eZapMain::startMessages()
{
	message_notifier.start();
}

void eZapMain::pauseMessages()
{
	message_notifier.stop();
}

void eZapMain::setMode(int newmode, int user)
{
	if ( handleState(0) )
	{
		if ( newmode == modeFile && mode != newmode )
			eEPGCache::getInstance()->pauseEPG();
		else if ( mode == modeFile && mode != newmode && newmode != -1 )
			eEPGCache::getInstance()->restartEPG();
	
		eDebug("setting mode to %d", newmode);
		if (mode != -1)
			getServiceSelectorPath(modeLast[mode]);
		if (mode == newmode)
			user=0;
	
		if (newmode == modePlaylist)
			last_mode=mode;
		else
			last_mode=-1;
	
		mode=newmode;
	
		if (user)
			playService(modeLast[mode].current(), psDontAdd);
	
		if (mode != -1)
			setServiceSelectorPath(modeLast[mode]);
	}
}
	
void eZapMain::setModeD(int newmode)
{
	setMode(newmode, 0);
}

void eZapMain::setServiceSelectorPath(eServicePath path)
{
	eServiceReference ref=path.current();
	path.up();
//	eServicePath p = path;
//	eDebug("Setting currentService to %s", ref.toString().c_str() );
//	eDebug("setting path to %s", p.toString().c_str());
	eZap::getInstance()->getServiceSelector()->setPath(path, ref);
}

void eZapMain::getServiceSelectorPath(eServicePath &path)
{
//	eDebug("selected = %s",eZap::getInstance()->getServiceSelector()->getSelected().toString().c_str() );
	path=eZap::getInstance()->getServiceSelector()->getPath();
	path.down(eZap::getInstance()->getServiceSelector()->getSelected());
//	eDebug("stored path for mode %d: %s", mode, eServicePath(path).toString().c_str());
}

void eZapMain::showBouquetList(int last)
{
	eServicePath b=eServiceStructureHandler::getRoot(mode+1);
	switch (mode)
	{
	case modeTV:
		b.down(eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, (1<<4)|(1<<1) ));
		break;
	case modeRadio:
		b.down(eServiceReference(eServiceReference::idDVB, eServiceReference::flagDirectory|eServiceReference::shouldSort, -1, 1 << 2 ));
		break;
	case modeFile:
		b.down(recordingsref);
		break;
	default:
		return;
	}
	if ((!last) || (!recordings->list.size()))
		b.down( eServiceReference() );
	else
		b.down( recordings->list.back().service);
	setServiceSelectorPath(b);
	showServiceSelector(-1, !last);
}

void eZapMain::showDVRFunctions(int show)
{
	dvrfunctions=show;

	if (dvrfunctions)
		dvrFunctions->show();
	else
		dvrFunctions->hide();
}

void eZapMain::showFavourites()
{
	eServicePath b=eServiceStructureHandler::getRoot(mode+1);
	if (mode > modeFile)
		return;
	b.down( favouriteref[mode] );
	b.down( eServiceReference() );
	setServiceSelectorPath(b);
	showServiceSelector(-1, 1);
}

void eZapMain::moveService(const eServiceReference &path, const eServiceReference &ref, const eServiceReference &afterref)
{
	ePlaylist *pl=0;
	if (path.type == eServicePlaylistHandler::ID)
		pl=(ePlaylist*)eServiceInterface::getInstance()->addRef(path);
	if (!pl)
		return;

	std::list<ePlaylistEntry>::iterator 
		it=std::find(pl->list.begin(), pl->list.end(), ref);
	std::list<ePlaylistEntry>::iterator after;
	
	if (afterref)
		after=std::find(pl->list.begin(), pl->list.end(), afterref);
	else
		after=pl->list.end();
	
	pl->moveService(it, after);
	pl->save();

	eServiceInterface::getInstance()->removeRef(path);
}

eServiceContextMenu::eServiceContextMenu(const eServiceReference &ref, const eServiceReference &path): eListBoxWindow<eListBoxEntryText>(_("Service Menu"), 5), ref(ref)
{
	move(ePoint(150, 200));
	new eListBoxEntryText(&list, _("back"), (void*)0);
	if (path.type == eServicePlaylistHandler::ID)
	{
		new eListBoxEntryText(&list, _("move service"), (void*)2);
		new eListBoxEntryText(&list, _("delete service"), (void*)1);
	}
	new eListBoxEntryText(&list, _("add service to favourites"), (void*)4);
	new eListBoxEntryText(&list, _("add service to playlist"), (void*)3);
	CONNECT(list.selected, eServiceContextMenu::entrySelected);
}

void eServiceContextMenu::entrySelected(eListBoxEntryText *test)
{
	if (!test)
		close(0);
	else
		close((int)test->getKey());
}

eRecordingStatus::eRecordingStatus()
{
	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eRecordingStatus"))
		eFatal("skin load of \"eRecordingStatus\" failed");
}
