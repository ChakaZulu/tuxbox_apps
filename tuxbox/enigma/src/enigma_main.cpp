#include <enigma_main.h>

#include <errno.h>
#include <iomanip>
//#include <stdio.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <enigma_mainmenu.h>
#include <enigma_event.h>
#include <sselect.h>
#include <enigma.h>
#include <enigma_lcd.h>
#include <enigma_plugins.h>
#include <timer.h>
#include <download.h>
#include <lib/base/i18n.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/econfig.h>
#include <lib/dvb/servicedvb.h>
#include <lib/dvb/epgcache.h>
#include <lib/dvb/esection.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/iso639.h>
#include <lib/dvb/servicemp3.h>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/serviceplaylist.h>
#include <lib/dvb/frontend.h>
#include <lib/gdi/font.h>
#include <lib/gui/elabel.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/enumber.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/actions.h>
#include <lib/gui/echeckbox.h>
#include <lib/driver/rc.h>
#include <lib/driver/streamwd.h>
#include <lib/driver/eavswitch.h>
#include <lib/dvb/dvbservice.h>
#include <lib/gdi/lcd.h>
#include <lib/gdi/glcddc.h>
#include <tuxbox.h>

		// bis waldi das in nen .h tut
#define MOVIEDIR "/hdd/movie"

struct enigmaMainActions
{
	eActionMap map;
	eAction showMainMenu, standby_press, standby_repeat, standby_release, 
		showInfobar, hideInfobar, showInfobarEPG, showServiceSelector,
		showSubservices, showAudio, pluginVTXT, showEPGList, showEPG, 
		nextService, prevService, playlistNextService, playlistPrevService,
		serviceListDown, serviceListUp, volumeUp, volumeDown, toggleMute,
		stop, pause, play, record, 
		startSkipForward, repeatSkipForward, stopSkipForward, 
		startSkipReverse, repeatSkipReverse, stopSkipReverse,
		showBouquets,
		modeTV, modeRadio, modeFile,
		toggleDVRFunctions, toggleIndexmark;
	enigmaMainActions(): 
		map("enigmaMain", _("enigma Zapp")),
		showMainMenu(map, "showMainMenu", _("show main menu"), eAction::prioDialog),
		standby_press(map, "standby_press", _("go to standby (press)"), eAction::prioDialog),
		standby_repeat(map, "standby_repeat", _("go to standby (repeat)"), eAction::prioDialog),
		standby_release(map, "standby_release", _("go to standby (release)"), eAction::prioDialog),

		showInfobar(map, "showInfobar", _("show infobar"), eAction::prioDialog),
		hideInfobar(map, "hideInfobar", _("hide infobar"), eAction::prioDialog),
		showInfobarEPG(map, "showInfobarEPG", _("show infobar or EPG"), eAction::prioDialog),
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

		modeTV(map, "modeTV", _("switch to TV mode"), eAction::prioDialog),
		modeRadio(map, "modeRadio", _("switch to Radio mode"), eAction::prioDialog),
		modeFile(map, "modeFile", _("switch to File mode"), eAction::prioDialog),

		toggleDVRFunctions(map, "toggleDVRFunctions", _("toggle DVR panel"), eAction::prioDialog),
		toggleIndexmark(map, "toggleIndexmark", _("toggle index mark"), eAction::prioDialog)
	{
	}
};

eAutoInitP0<enigmaMainActions> i_enigmaMainActions(eAutoInitNumbers::actions, "enigma main actions");

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

eAutoInitP0<enigmaStandbyActions> i_enigmaStandbyActions(eAutoInitNumbers::actions, "enigma standby actions");

class eZapStandby: public eWidget
{
	static eZapStandby *instance;
protected:
	int eventHandler(const eWidgetEvent &);
public:
	void wakeUp();
	static eZapStandby *getInstance() { return instance; }
	eZapStandby();
	~eZapStandby()
	{
		instance=0;
	}
};
eZapStandby* eZapStandby::instance=0;

void eZapStandby::wakeUp()
{
	close(0);  
}

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
		eZapLCD *pLCD=eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdStandby->show();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSetSpeed, 0));
		eAVSwitch::getInstance()->setInput(1);
		eAVSwitch::getInstance()->setTVPin8(0);
		system("/bin/sync");
		system("/sbin/hdparm -y /dev/ide/host0/bus0/target0/lun0/disc");
		system("/sbin/hdparm -y /dev/ide/host0/bus0/target1/lun0/disc");

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
		eStreamWatchdog::getInstance()->reloadSettings();
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
	if (!instance)
		instance=this;
	else
		eFatal("more than ob eZapStandby instance is created!");
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

void eZapSeekIndices::load(const eString &filename)
{
	this->filename=filename;
	FILE *f=fopen(filename.c_str(), "rt");
	if (!f)
		return;
	
	while (1)
	{
		int real, time;
		if (fscanf(f, "%d %d\n", &real, &time) != 2)
			break;
		add(real, time);
	}
	fclose(f);
	changed=0;
}

void eZapSeekIndices::save()
{
	FILE *f=fopen(filename.c_str(), "wt");
	if (!f)
		return;
		
	for (std::map<int,int>::const_iterator i(index.begin()); i != index.end(); ++i)
		fprintf(f, "%d %d", i->first, i->second);
	fclose(f);
	changed=0;
}

void eZapSeekIndices::add(int real, int time)
{
	index.insert(std::pair<int,int>(real,time));
}

void eZapSeekIndices::remove(int real)
{
	index.erase(real);
}

int eZapSeekIndices::getNext(int real, int dir)
{
	int diff=-1, r=-1;
	for (std::map<int,int>::const_iterator i(index.begin()); i != index.end(); ++i)
	{
		if ((dir > 0) && (i->first <= real))
			continue;
		if ((dir < 0) && (i->first >= real))
			break;
		int d=abs(i->first-real);
		if (d < diff)
		{
			diff=d;
			r=i->first;
		} else
			break;
	}
	return r;
}

int eZapSeekIndices::getTime(int real)
{
	std::map<int,int>::const_iterator i=index.find(real);
	if (i != index.end())
		return i->second;
	return -1;
}

std::map<int,int> &eZapSeekIndices::getIndices()
{
	return index;
}

int NVODStream::validate()
{
	text.str(eString());
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

		if ( now > endtime )
			return 0;

		if ((event->start_time <= now) && (now < endtime))
		{
			int perc=(now-event->start_time)*100/event->duration;
			text << " (" << perc << "%, " << perc*3/100 << '.' << std::setw(2) << (perc*3)%100 << _(" Euro lost)");
		}
		return 1;
	}
	return 0;
}

void NVODStream::EITready(int error)
{
	eDebug("NVOD eit ready: %d", error);

	if ( error )
		delete this;
	else if ( eit.ready && !error && !valid && validate() )
	{
		valid=1;
		listbox->append( this );
		((eListBox<NVODStream>*)listbox)->sort(); // <<< without explicit cast the compiler nervs ;)
	}

/*
	if (listbox && listbox->isVisible())
		listbox->invalidate();
	*/
}

NVODStream::NVODStream(eListBox<NVODStream> *listbox, eDVBNamespace dvb_namespace, const NVODReferenceEntry *ref, int type)
	: eListBoxEntryTextStream((eListBox<eListBoxEntryTextStream>*)listbox), 
		service(dvb_namespace, eTransportStreamID(ref->transport_stream_id), eOriginalNetworkID(ref->original_network_id),
			eServiceID(ref->service_id), 5), eit(EIT::typeNowNext, ref->service_id, type)
{
	CONNECT(eit.tableReady, NVODStream::EITready);
	listbox->remove(this);
	valid=0;
	eit.start();
}

eString NVODStream::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int state )
{
	if (valid && validate())
		return eListBoxEntryTextStream::redraw(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, state);

	valid=0;
	listbox->remove( this );
	eit.start();
	return _("not valid!");
}

void eNVODSelector::selected(NVODStream* nv)
{
	if (nv)
		eServiceInterface::getInstance()->play(nv->service);

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

void eNVODSelector::add(eDVBNamespace dvb_namespace, NVODReferenceEntry *ref)
{
	eServiceReference &s=eServiceInterface::getInstance()->service;
	if (s.type != eServiceReference::idDVB)
		return ;
	eServiceReferenceDVB &service=(eServiceReferenceDVB&)s;

	int type= ((service.getTransportStreamID()==eTransportStreamID(ref->transport_stream_id))
			&&	(service.getOriginalNetworkID()==eOriginalNetworkID(ref->original_network_id))) ? EIT::tsActual:EIT::tsOther;
	new NVODStream(&list, dvb_namespace, ref, type);
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
			{
				parseEIT(eit);
			}
			else
			{
				CONNECT( eDVB::getInstance()->tEIT.tableReady, AudioStream::EITready );
			}
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
		EIT *eit=eDVB::getInstance()->getEIT();
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

SubService::SubService(eListBox<SubService> *listbox, eDVBNamespace dvb_namespace, const LinkageDescriptor *descr)
	:eListBoxEntryText((eListBox<eListBoxEntryText>*) listbox),
		service(dvb_namespace, 
			eTransportStreamID(descr->transport_stream_id), 
			eOriginalNetworkID(descr->original_network_id),
			eServiceID(descr->service_id), 6)
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
		ss->service.descr = ss->getText();
		eServiceInterface::getInstance()->play(ss->service);
	}
	close(0);
}

void eSubServiceSelector::clear()
{
	list.clearList();
}

void eSubServiceSelector::add(eDVBNamespace dvb_namespace, const LinkageDescriptor *ref)
{
	new SubService(&list, dvb_namespace, ref);
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
	setText(_("Channel"));
	move(ePoint(200, 140));
	resize(eSize(280, 120));
	eLabel *label;
	label=new eLabel(this);
	label->setText(_("Channel:"));
	label->move(ePoint(50, 15));
	label->resize(eSize(110, eSkin::getActive()->queryValue("fontsize", 20)+4));
	
	number=new eNumber(this, 1, 1, 9999, 4, 0, 1, label);
	number->move(ePoint(160, 15));
	number->resize(eSize(60, eSkin::getActive()->queryValue("fontsize", 20)+4));
	number->setNumber(initial);
	CONNECT(number->selected, eServiceNumberWidget::selected);
	CONNECT(number->numberChanged, eServiceNumberWidget::numberChanged );
	
	timer=new eTimer(eApp);
	timer->start(2000);
	CONNECT(timer->timeout, eServiceNumberWidget::timeout);	
}

eServiceNumberWidget::~eServiceNumberWidget()
{
	if (timer)
		delete timer;
}

void eServiceNumberWidget::numberChanged()
{
	timer->start(2000);
}

eZapMain *eZapMain::instance;

void eZapMain::onRotorStart( int newPos )
{
	if (!pRotorMsg)
	{
		pRotorMsg = new eMessageBox( eString().sprintf(_("Please wait while the motor is driving to %d.%d°%c ...."),abs(newPos)/10,abs(newPos)%10,newPos<0?'W':'E'), _("Message"), 0);
		pRotorMsg->show();
	}
}

void eZapMain::onRotorStop()
{
	if (pRotorMsg)
	{
		pRotorMsg->hide();
		delete pRotorMsg;
		pRotorMsg=0;
	}
}

void eZapMain::onRotorTimeout()
{
	if (pRotorMsg)
	{
		pRotorMsg->hide();
		delete pRotorMsg;
		pRotorMsg=0;
	}
/*	pRotorMsg = new eMessageBox( _("Rotor has timeouted... check your LNB Cable, or if you use rotor drive speed for running detection then decrease the °/sec value")
									, _("Message"),
									eMessageBox::btOK|eMessageBox::iconInfo);
	pRotorMsg->show();
	timeout.start(10000, true);*/
}

void eZapMain::eraseBackground(gPainter *painter, const eRect &where)
{
}

eZapMain::eZapMain()
	:eWidget(0, 1)
	,mute( eZap::getInstance()->getDesktop( eZap::desktopFB ) )
	,volume( eZap::getInstance()->getDesktop( eZap::desktopFB ) )
	,VolumeBar( &volume ), pMsg(0), pRotorMsg(0), message_notifier(eApp, 0), timeout(eApp)
	,clocktimer(eApp), messagetimeout(eApp), progresstimer(eApp)
	,volumeTimer(eApp), recStatusBlink(eApp), currentSelectedUserBouquet(0)
	,wasSleeping(0), state( 0 )
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

	dvrFunctions=new eLabel(this);
	dvrFunctions->setName("dvrFunctions");
	dvrFunctions->hide();

	DVRSpaceLeft=new eLabel(dvrFunctions);
	DVRSpaceLeft->setName("TimeLeft");

	nonDVRfunctions=new eWidget(this);
	nonDVRfunctions->setName("nonDVRfunctions");

	dvrfunctions=0;

	recstatus=new eLabel(this);
	recstatus->setName("recStatus");
	recstatus->hide();

	isVT=0;
	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "ezap_main"))
		eFatal("skin load of \"ezap_main\" failed");

	lcdmain.show();

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

	CONNECT(volumeTimer.timeout, eZapMain::hideVolumeSlider );
	CONNECT(recStatusBlink.timeout, eZapMain::blinkRecord);

	CONNECT( eFrontend::getInstance()->rotorRunning, eZapMain::onRotorStart );
	CONNECT( eFrontend::getInstance()->rotorStopped, eZapMain::onRotorStop );
	CONNECT( eFrontend::getInstance()->rotorTimeout, eZapMain::onRotorTimeout );

	actual_eventDisplay=0;

//	clockUpdate();
	standbyTime=-1;

	skipcounter=0;
	skipping=0;

	addActionMap(&i_enigmaMainActions->map);
	addActionMap(&i_numberActions->map);

	gotPMT();
	gotSDT();
	gotEIT();

	eplPath = CONFIGDIR "/enigma";

	// create Playlist
	eServicePlaylistHandler::getInstance()->addNum( 0 );
	playlistref=eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, 0 );
	playlist=(ePlaylist*)eServiceInterface::getInstance()->addRef(playlistref);
	ASSERT(playlist);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeRoot), playlistref);
	playlist->load((eplPath+"/playlist.epl").c_str());

	// create recordingslist..
	eServicePlaylistHandler::getInstance()->addNum( 1 );
	recordingsref=eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, 1);
	recordings=(ePlaylist*)eServiceInterface::getInstance()->addRef(recordingsref);
	ASSERT(recordings);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeFile), recordingsref);
	recordings->service_name=_("recorded movies");
	recordings->load(MOVIEDIR "/recordings.epl");

	// create user bouquet tv list
	eServicePlaylistHandler::getInstance()->addNum( 2 );
	userTVBouquetsRef=eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, 2);
	userTVBouquets=(ePlaylist*)eServiceInterface::getInstance()->addRef(userTVBouquetsRef);
	ASSERT(userTVBouquets);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeRoot), userTVBouquetsRef);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeTvRadio), userTVBouquetsRef);
	userTVBouquets->service_name=_("User - bouquets (TV)");
	userTVBouquets->load((eplPath+"/userbouquets.tv.epl").c_str());

	// create user bouquet radio list
	eServicePlaylistHandler::getInstance()->addNum( 3 );
	userRadioBouquetsRef=eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, 3);
	userRadioBouquets=(ePlaylist*)eServiceInterface::getInstance()->addRef(userRadioBouquetsRef);
	ASSERT(userRadioBouquets);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeRoot), userRadioBouquetsRef);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeTvRadio), userRadioBouquetsRef);
	userRadioBouquets->service_name=_("User - bouquets (Radio)");
	userRadioBouquets->load((eplPath+"/userbouquets.radio.epl").c_str());

	// create user bouquet file list
	eServicePlaylistHandler::getInstance()->addNum( 4 );
	userFileBouquetsRef=eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, 4);
	userFileBouquets=(ePlaylist*)eServiceInterface::getInstance()->addRef(userFileBouquetsRef);
	ASSERT(userFileBouquets);
	eServicePlaylistHandler::getInstance()->newPlaylist(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeRoot), userFileBouquetsRef);
	userFileBouquets->service_name=_("User - bouquets (File)");
	userFileBouquets->load((eplPath+"/userbouquets.file.epl").c_str());

	int i=0;
	for (int d = modeTV; d <= modeFile; d++)
	{
		ePlaylist* parentList=0;
		switch(d)
		{
			case modeTV:
				parentList = userTVBouquets;
				break;
			case modeRadio:
				parentList = userRadioBouquets;
				break;
			case modeFile:
				parentList = userFileBouquets;
				break;
		}
		for ( std::list<ePlaylistEntry>::iterator it(parentList->getList().begin()); it != parentList->getList().end(); it++, i++)
		{
			eServicePlaylistHandler::getInstance()->addNum( it->service.data[1] );
			eServiceReference ref = eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, it->service.data[1] );
			ref.path = it->service.path;
			(ePlaylist*)eServiceInterface::getInstance()->addRef(ref);
		}
	}

	if(!i)  // no favlist loaded...
	{    
		// create default user bouquet lists (favourite lists)
		for (int i = modeTV; i <= modeFile; i++)
		{
			eServiceReference ref = eServicePlaylistHandler::getInstance()->newPlaylist();
			ePlaylist* parentList=0;
			eString path;
			eString name;
			switch(i)
			{
				case modeTV:
					parentList = userTVBouquets;
					path = eplPath+'/'+eString().sprintf("userbouquet.%x.tv",ref.data[1]);
					name = _("Favourites (TV)");
				break;
				case modeRadio:
					parentList = userRadioBouquets;
					path = eplPath+'/'+eString().sprintf("userbouquet.%x.radio",ref.data[1]);
					name = _("Favourites (Radio)");
				break;
				case modeFile:
					parentList = userFileBouquets;
					path = eplPath+'/'+eString().sprintf("userbouquet.%x.file",ref.data[1]);
					name = _("Favourites (File)");
				break;
			}
			addUserBouquet( parentList, path, name, ref, true );
		}
	}

	CONNECT(eZap::getInstance()->getServiceSelector()->addServiceToPlaylist, eZapMain::doPlaylistAdd);
	CONNECT(eZap::getInstance()->getServiceSelector()->addServiceToUserBouquet, eZapMain::addServiceToUserBouquet);
	CONNECT(eZap::getInstance()->getServiceSelector()->removeServiceFromUserBouquet, eZapMain::removeServiceFromUserBouquet );
	CONNECT(eZap::getInstance()->getServiceSelector()->showMenu, eZapMain::showServiceMenu);
	CONNECT_2_1(eZap::getInstance()->getServiceSelector()->setMode, eZapMain::setMode, 0);
	CONNECT(eZap::getInstance()->getServiceSelector()->rotateRoot, eZapMain::rotateRoot);
	CONNECT(eZap::getInstance()->getServiceSelector()->moveEntry, eZapMain::moveService);

	// read for all modes last servicePaths from registry
	for (mode=modeTV; mode < modeEnd; mode++)
	{
		char* str;
		// normale dvb bouquet pathes...
		if ( !eConfig::getInstance()->getKey( eString().sprintf("/ezap/ui/modes/%d/path0", mode).c_str(), str) )
		{                                                        
			modeLast[mode][0].setString(str);
//			eDebug(str);
			free(str);
		}
		else  // no path in registry... create default..
		{
			modeLast[mode][0]=eServiceStructureHandler::getRoot(mode+1);
			modeLast[mode][0].down( eServiceReference() );
		}
		if ( !eConfig::getInstance()->getKey( eString().sprintf("/ezap/ui/modes/%d/path1", mode).c_str(), str) )
		{
			modeLast[mode][1].setString(str);
//			eDebug(str);
			free(str);
		}
		else  // no path in registry... create default..
		{
			modeLast[mode][1]=(mode==modeTV)?userTVBouquetsRef:(mode==modeRadio)?userRadioBouquetsRef:userFileBouquetsRef;
			modeLast[mode][1].down( eServiceReference() );
		}
		if ( !eConfig::getInstance()->getKey( eString().sprintf("/ezap/ui/modes/%d/path2", mode).c_str(), str) )
		{
			modeLast[mode][2].setString(str);
//			eDebug(str);
			free(str);
		}
		else
		{
			modeLast[mode][2]=playlistref;
			modeLast[mode][2].down( eServiceReference() );
		}
	}

	// set serviceSelector style
	int style;
	if ( eConfig::getInstance()->getKey("/ezap/ui/serviceSelectorStyle", style ) )
		style=eServiceSelector::styleSingleColumn;  // default we use single Column Style

	eZap::getInstance()->getServiceSelector()->setStyle(style);

	mode=-1;  // fake mode for first call of setMode 

	// get last mode form registry ( TV Radio File )
	int curMode;
	if ( eConfig::getInstance()->getKey("/ezap/ui/lastmode", curMode ) )
		curMode = 0;  // defaut TV Mode

	if (curMode < 0)
		curMode=0;

	setMode(curMode);  // do it..

#if 1  // temporary reenable this...
	if (playlist->current != playlist->getConstList().end())  // we was in playlist mode??
		playService(*playlist->current, psDontAdd);  // then play the last service

	startMessages();
#endif

	dvrFunctions->zOrderRaise();
	nonDVRfunctions->zOrderRaise();
}

eZapMain::~eZapMain()
{
	if ( state & stateRecording )
	{
		if ( state & stateInTimerMode )
		{
			eTimerManager::getInstance()->abortEvent( ePlaylistEntry::errorUserAborted );
		}
		else
			recordDVR(0,1);
	}

	getPlaylistPosition();

	// get current selected serviceselector path
	if ( mode != -1 ) // valid mode?
		getServiceSelectorPath(modeLast[mode][0]);

  // save last mode to registry
	eConfig::getInstance()->setKey("ezap/ui/lastmode", mode );

	// save for all modes the servicePath to registry
	for (mode=modeTV; mode < modeEnd; mode++ )
	{
		for (int i=0; i < 3; i++)
		{
			eString str = modeLast[mode][i].toString();
			eConfig::getInstance()->setKey( eString().sprintf("/ezap/ui/modes/%d/path%d", mode, i).c_str(), str.c_str() );
		}
	}

	// save and destroy all userBouquetLists
	userTVBouquets->save();
	for (std::list<ePlaylistEntry>::iterator it(userTVBouquets->getList().begin()); it != userTVBouquets->getList().end(); it++ )
		eServiceInterface::getInstance()->removeRef(it->service);
	eServiceInterface::getInstance()->removeRef(userTVBouquetsRef);

	// save and destroy userRadioBouquetList
	userRadioBouquets->save();
	for (std::list<ePlaylistEntry>::iterator it(userRadioBouquets->getList().begin()); it != userRadioBouquets->getList().end(); it++ )
		eServiceInterface::getInstance()->removeRef(it->service);
	eServiceInterface::getInstance()->removeRef(userRadioBouquetsRef);
	
	// save and destroy userFileBouquetList
	userFileBouquets->save();
	for (std::list<ePlaylistEntry>::iterator it(userFileBouquets->getList().begin()); it != userFileBouquets->getList().end(); it++ )
		eServiceInterface::getInstance()->removeRef(it->service);
	eServiceInterface::getInstance()->removeRef(userFileBouquetsRef);
	
	// save and destroy playlist
	playlist->save();
	eServiceInterface::getInstance()->removeRef(playlistref);

	// save and destroy recordingslist
	recordings->save();
	eServiceInterface::getInstance()->removeRef(recordingsref);

	if (instance == this)
		instance = 0;

	eZapLCD *pLCD=eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdShutdown->show();
	gLCDDC::getInstance()->setUpdate(0);
	if (tuxbox_get_submodel() == TUXBOX_SUBMODEL_DREAMBOX_DM7000)
		eDBoxLCD::getInstance()->switchLCD(0);

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
	cur_event_text="";

	if (eit)
	{
		eString nowtext, nexttext, nowtime="", nexttime="", descr;
		int val=0;
		int p=0;

		for (ePtrList<EITEvent>::iterator i(eit->events); i != eit->events.end(); ++i)
		{
			EITEvent *event=*i;
			eServiceReferenceDVB &ref=(eServiceReferenceDVB&)eServiceInterface::getInstance()->service;
			if ( ref.getServiceType() != 6 )
			{
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
							subservicesel.add(ref.getDVBNamespace(), ld);
							flags|=ENIGMA_SUBSERVICES;
						}
				}
			}
			for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
			{
				Descriptor *descriptor=*d;
				if (descriptor->Tag()==DESCR_SHORT_EVENT)
				{
					ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;

					switch (p)
					{
					case 0:
						nowtext=ss->event_name;
						cur_event_text=ss->event_name;
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
	if (flags&(ENIGMA_NVOD|ENIGMA_SUBSERVICES))
	{
		ButtonGreenEn->show();
		ButtonGreenDis->hide();
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
	if (playlist->current != playlist->getConstList().end())
		if (playlist->current->current_position != -1)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSeekReal, playlist->current->current_position));
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

		if ( playlist->current != playlist->getConstList().end() && playlist->current->service == eServiceInterface::getInstance()->service )
			playlist->current->current_position=time;
	}
}


void eZapMain::handleNVODService(SDTEntry *sdtentry)
{
	nvodsel.clear();
	for (ePtrList<Descriptor>::iterator i(sdtentry->descriptors); i != sdtentry->descriptors.end(); ++i)
		if (i->Tag()==DESCR_NVOD_REF)
			for (ePtrList<NVODReferenceEntry>::iterator e(((NVODReferenceDescriptor*)*i)->entries); e != ((NVODReferenceDescriptor*)*i)->entries.end(); ++e)
				nvodsel.add(((eServiceReferenceDVB&)eServiceInterface::getInstance()->service).getDVBNamespace(), *e);
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

	getServiceSelectorPath(modeLast[mode][0]);

	e->selectService(eServiceInterface::getInstance()->service);
	const eServiceReference *service = e->choose(dir);	// reset path only when NOT showing specific list

	pLCD->lcdMain->show();
	pLCD->lcdMenu->hide();

	if (!service)
	{
		setServiceSelectorPath(modeLast[mode][0]);
		return;
	}

	if (*service == eServiceInterface::getInstance()->service)
		return;

	if ( handleState() )
	{
		getServiceSelectorPath(modeLast[mode][0]);

		if (eZap::getInstance()->getServiceSelector()->getPath().current() != playlistref)
		{
			if (!entered_playlistmode)
				playlistmode=0;
			playService(*service, playlistmode?psAdd:0);
		} else
			playService(*service, psDontAdd);
	}
}

void eZapMain::nextService(int add)
{
	const eServiceReference *service=eZap::getInstance()->getServiceSelector()->next();
		
	if (!service)
		return;
	else
		getServiceSelectorPath( modeLast[mode][0] );

	if (service->flags & eServiceReference::mustDescent)
		return;

	if (service->type & eServiceReference::idDVB)
		add = 1;

	playService(*service, add?0:psDontAdd);
}

void eZapMain::prevService()
{
	const eServiceReference *service=eZap::getInstance()->getServiceSelector()->prev();

	if (!service)
		return;
	else
		getServiceSelectorPath( modeLast[mode][0] );

	if (service->flags & eServiceReference::mustDescent)
		return;

	if (service->type & eServiceReference::idDVB)
		playService(*service, 0);
	else
		playService(*service, psDontAdd);
}

void eZapMain::playlistPrevService()
{
	getPlaylistPosition();
	if ( playlist->current != playlist->getConstList().begin())
	{
		if (playlistmode)
			playlist->current->current_position=-1;
		playlist->current--;
		playService(*playlist->current, psDontAdd);
	}
}

void eZapMain::playlistNextService()
{
	getPlaylistPosition();
	if (playlist->current != playlist->getConstList().end())
	{
		if (playlistmode)
			playlist->current->current_position=-1;
		playlist->current++;
		if (playlist->current == playlist->getConstList().end())
		{
			playlist->current--;
			return;
		}
		playService(*playlist->current, psDontAdd);
	}
}

void eZapMain::volumeUp()
{
	eAVSwitch::getInstance()->changeVolume(0, -2);
	if (!volume.isVisible())
		volume.show();
	volumeTimer.start(2000, true);
}

void eZapMain::volumeDown()
{
	eAVSwitch::getInstance()->changeVolume(0, +2);
	if (!volume.isVisible())
		volume.show();
	volumeTimer.start(2000, true);
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

void eZapMain::toggleTimerMode()
{
	if ( state & stateInTimerMode)
		state &= ~stateInTimerMode;
	else
		state |= stateInTimerMode;
}

void eZapMain::standbyPress()
{
	standbyTime = time(0);
}

void eZapMain::standbyRepeat()
{
	int diff = time(0) - standbyTime;
	if (diff > 2)
		standbyRelease();
}

void eZapMain::standbyRelease()
{
	if ( standbyTime == -1) // we come from standby ?
		return;

	int diff = time(0) - standbyTime;
	standbyTime=-1;
	if (diff > 2)
	{
		if (handleState())
			eZap::getInstance()->quit();
	} else
	{
		eZapStandby standby;
		if (isVisible())
			hide();
		standby.show();
		state |= stateSleeping;
		standby.exec();
		standby.hide();
		state &= ~stateSleeping;
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
	else if ( handler->getState() != eServiceHandler::statePlaying )
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

void eZapMain::record()
{
	if ( state & stateRecording )
	{
		if ( state & stateInTimerMode )
			eTimerManager::getInstance()->abortEvent( ePlaylistEntry::errorUserAborted );
		else
			recordDVR(0,1);
	}
	else
		recordDVR(1, 1);
}

int freeRecordSpace()
{
	struct statfs s;
	int free;
	if (statfs(MOVIEDIR, &s)<0)
		free=-1;
	else
		free=s.f_bfree/1000*s.f_bsize/1000;
	return free;
}

int eZapMain::recordDVR(int onoff, int user, int event_id)
{
	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return -1;

	if (onoff) //  start recording
	{
		if (state & stateRecording)
			recordDVR(0, 0); // try to stop recording.. should not happen
		if (state & stateRecording)
			return -2; // already recording
		eServiceReference ref_=eServiceInterface::getInstance()->service;
		if ( (ref_.type != eServiceReference::idDVB) ||
				(ref_.data[0] < 0) || ref_.path.size())
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

		eServiceReferenceDVB &ref=(eServiceReferenceDVB&)ref_;
		// build filename
		eString servicename, descr = _("no description available");

		eService *service=0;
		EITEvent *event=0;

		if ( ref.getServiceType() > 4 )  // nvod or linkage
			service = eServiceInterface::getInstance()->addRef(refservice);
		else
			service = eServiceInterface::getInstance()->addRef(ref);

		if (event_id != -1)
			event = eEPGCache::getInstance()->lookupEvent( ref, event_id );

		if (service)
		{
			servicename = service->service_name;
			static char strfilter[4] = { 0xC2, 0x87, 0x86, 0x00 };
			// filter short name brakets...
			for (eString::iterator it(servicename.begin()); it != servicename.end();)
				strchr( strfilter, *it ) ? it = servicename.erase(it) : it++;

			if ( ref.getServiceType() > 4 )  // nvod or linkage
				eServiceInterface::getInstance()->removeRef(refservice);
			else
				eServiceInterface::getInstance()->removeRef(ref);
		}
		else
			servicename="record";

		if (cur_event_text.length())
			descr = cur_event_text;

		else if ( event )
		{
			for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
			{
				Descriptor *descriptor=*d;
				if (descriptor->Tag()==DESCR_SHORT_EVENT)
				{
					ShortEventDescriptor *ss=(ShortEventDescriptor*)descriptor;
					descr = ss->event_name;
					delete event;
					break;
				}
			}
		}

		eString filename = servicename + " - " + descr;
		eString cname="";

		for (unsigned int i=0; i<filename.length(); ++i)
		{
			if (strchr("abcdefghijklkmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_- ()", filename[i]) || (filename[i] >= 0x80)) 	// allow UTF-8
				cname+=filename[i];
			else
				cname+='_';
		}

		int suffix=0;
		struct stat s;
		do
		{
			filename=MOVIEDIR "/";
			filename+=cname;
			if (suffix)
				filename+=eString().sprintf(" [%d]", suffix);
			suffix++;
			filename+=".ts";
		} while (!stat(filename.c_str(), &s));

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
			handleStandby();

			eServiceReference ref2=ref;
			ref2.path=filename;
			ref2.descr=descr;
			ePlaylistEntry en(ref2);
			en.type=ePlaylistEntry::PlaylistEntry|ePlaylistEntry::boundFile;
			recordings->getList().push_back(en); // add to playlist
			recordings->save(MOVIEDIR "/recordings.epl");
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStart));
			state |= (stateRecording|recDVR);
			recstatus->show();

// DETECT HARDDISC or NFS mount
			FILE *f=fopen("/proc/mounts", "r");
			char line[1024];
			int ok=0;
			if (!f)
				return -3;
			while (fgets(line, 1024, f))
			{
				if ( (strstr(line, "/dev/ide/host") || strstr(line, "nfs rw")) &&
					strstr(line, "/hdd") )
				{
					ok=1;
					break;
				}
			}
			if ( ok )
				recStatusBlink.start(500, 1);
			else
				return -3;
		}
		return 0;
	}
	else   // stop recording
	{
		if ( !(state & stateRecording) )
			return -1; // not recording

		state &= ~(stateRecording|recDVR);

		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStop));
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordClose));
		DVRSpaceLeft->hide();
		recStatusBlink.stop();
		recstatus->hide();
		if (user)
		{
			setMode(modeFile);
			showBouquetList(1);
		}
		else
		{
			handleStandby();
		}
		return 0;
	}
}

void eZapMain::handleStandby()
{
	if (state & stateSleeping)
	{
		wasSleeping=1;
		eZapStandby::getInstance()->wakeUp();
	}
	else if (wasSleeping==1) // before record we was in sleep mode...
	{
		wasSleeping=0;
		message_notifier.send(eZapMain::messageGoSleep);
	}
	else if (wasSleeping==2)
	{
		wasSleeping=0;
		message_notifier.send(eZapMain::messageShutdown);
	}
}

void eZapMain::startSkip(int dir)
{
	skipcounter=0;
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
	if (!skipping)
	{
		skipcounter++;
		int time=5000;
		if (skipcounter > 10)
			time=20000;
		else if (skipcounter > 30)
			time=120000;
		else if (skipcounter > 60)
			time=600000;
		eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (handler)
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdSkip, (dir == skipForward)?time:-time));
		updateProgress();
	}
}

void eZapMain::stopSkip(int dir)
{
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

ePlaylist *eZapMain::addUserBouquet( ePlaylist *list, const eString &path, const eString &name, eServiceReference& ref, bool save )
{
	ePlaylist *pl = (ePlaylist*)eServiceInterface::getInstance()->addRef(ref);
	pl->service_name = name;
	pl->load( path.c_str() );
	pl->save();
	eServiceInterface::getInstance()->removeRef(ref);
	ref.path=path;
	pl = (ePlaylist*)eServiceInterface::getInstance()->addRef(ref);
	pl->load( path.c_str() );
	list->getList().push_back( ref );
	list->getList().back().type = ePlaylistEntry::PlaylistEntry|ePlaylistEntry::boundFile;
	list->save();
	if (save)
		pl->save();
	return pl;
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
		bool removeEntry=true;

		std::list<ePlaylistEntry>::iterator it=std::find(pl->getList().begin(), pl->getList().end(), ref);
		if (it == pl->getList().end())
			break;

		// remove parent playlist ref...
		eServiceInterface::getInstance()->removeRef(path);

		if ( it->service.type == eServiceReference::idDVB )
		{
			if ( (it->type & (ePlaylistEntry::PlaylistEntry|ePlaylistEntry::boundFile))==(ePlaylistEntry::PlaylistEntry|ePlaylistEntry::boundFile) )
			{
				eMessageBox box(_("This is a recorded stream!\nReally delete?"), _("Delete recorded stream"), eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo);
				box.show();
				int r=box.exec();
				box.hide();
				if (r != eMessageBox::btYes)
					removeEntry=false;
			}
		}
		else if ( it->service.type == eServicePlaylistHandler::ID )
			eServicePlaylistHandler::getInstance()->removePlaylist( it->service );

		if (removeEntry)  // handle recorded streams..
		{
			eServiceInterface::getInstance()->removeRef(ref);
			pl->deleteService(it);
			pl->save();
			sel->actualize();
		}

		break;
	}
	case 2: // enable/disable movemode
	{
		if ( sel->toggleMoveMode() )
		{
			currentSelectedUserBouquetRef=sel->getPath().current();
			currentSelectedUserBouquet=(ePlaylist*)eServiceInterface::getInstance()->addRef( currentSelectedUserBouquetRef );
		}
		else
		{
			currentSelectedUserBouquet->save();
			currentSelectedUserBouquet=0;
			eServiceInterface::getInstance()->removeRef( currentSelectedUserBouquetRef );
			currentSelectedUserBouquetRef=eServiceReference();
		}
		break;
	}
	case 3: // add service to playlist
		doPlaylistAdd(sel->getSelected());
		break;
	case 4: // add service to UserBouquet
		addServiceToUserBouquet(sel);
	break;
	case 5: // toggle edit User Bouquet Mode
		if ( sel->toggleEditMode() ) // toggleMode.. and get new state !!!
		{
			UserBouquetSelector s( mode == modeTV?userTVBouquets->getList():mode==modeRadio?userRadioBouquets->getList():userFileBouquets->getList() );
			s.show();
			s.exec();
			s.hide();
			if (s.curSel)
			{
				currentSelectedUserBouquetRef = s.curSel;
				currentSelectedUserBouquet = (ePlaylist*)eServiceInterface::getInstance()->addRef( s.curSel );
				for (std::list<ePlaylistEntry>::const_iterator i(currentSelectedUserBouquet->getConstList().begin()); i != currentSelectedUserBouquet->getConstList().end(); ++i)
					eListBoxEntryService::hilitedEntrys.insert(*i);
			}
		}
		else
		{
			currentSelectedUserBouquet->save();
			currentSelectedUserBouquet=0;
			eServiceInterface::getInstance()->removeRef( currentSelectedUserBouquetRef );
			currentSelectedUserBouquetRef=eServiceReference();
		}
	break;
	case 6: // add new user bouquet
	{
		TextEditWindow wnd(_("Enter name for the new bouquet:"));
		hide();
		wnd.setText(_("Add new user bouquet"));
		wnd.show();
		wnd.setEditText(" ");
		wnd.setMaxChars(50);
		int ret = wnd.exec();
		wnd.hide();
		show();
		if ( !ret )
		{
			int actualize=0;
			eServicePath path = sel->getPath();
			path.up();
			eServiceReference newList = eServicePlaylistHandler::getInstance()->newPlaylist();
			switch ( mode )
			{
				case modeTV:
				{
					addUserBouquet( userTVBouquets, eplPath+'/'+eString().sprintf("userbouquet.%x.tv",newList.data[1]), wnd.getEditText(), newList, true );
					actualize = (path.current() == userTVBouquetsRef);
				}
				break;
				case modeRadio:
				{
					addUserBouquet( userRadioBouquets, eplPath+'/'+eString().sprintf("userbouquet.%x.radio",newList.data[1]), wnd.getEditText(), newList, true );
					actualize = (path.current() == userRadioBouquetsRef);
				}
				break;
				case modeFile:
				{
					addUserBouquet( userFileBouquets, eplPath+'/'+eString().sprintf("userbouquet.%x.file",newList.data[1]), wnd.getEditText(), newList, true );
					actualize = (path.current() == userRadioBouquetsRef);
				}
				break;
			}
			if (actualize)
				sel->actualize();
		}
	}
	break;

	case 7: // rename user bouquet
	{
		ePlaylist *p=(ePlaylist*)eServiceInterface::getInstance()->addRef(ref);
		TextEditWindow wnd(_("Enter new name for the user bouquet:"));
		hide();
		wnd.setText(_("Rename user bouquet"));
		wnd.show();
		wnd.setEditText(p->service_name);
		wnd.setMaxChars(50);		
		int ret = wnd.exec();
		wnd.hide();
		show();
		if ( !ret )
		{
			if ( p->service_name != wnd.getEditText() )
			{
				p->service_name=wnd.getEditText();
				p->save();
				sel->actualize();
			}
		}
		eServiceInterface::getInstance()->removeRef(ref);
	}
	break;
	
	case 8: // duplicate normal bouquet as user bouquet
	{
		// create new user bouquet
		currentSelectedUserBouquetRef = eServicePlaylistHandler::getInstance()->newPlaylist();

		// get name of the source bouquet
		eString name;

		const eService *pservice=eServiceInterface::getInstance()->addRef(ref);
		if (pservice)
		{
			if ( pservice->service_name.length() )
				name = pservice->service_name;
			else
				name = _("unnamed bouquet");
			eServiceInterface::getInstance()->removeRef(ref);
		}

		switch ( mode )
		{
			case modeTV:
				currentSelectedUserBouquet = addUserBouquet( userTVBouquets, eplPath+'/'+eString().sprintf("userbouquet.%x.tv",currentSelectedUserBouquetRef.data[1]), name, currentSelectedUserBouquetRef, false );
			break;
			case modeRadio:
				currentSelectedUserBouquet = addUserBouquet( userRadioBouquets, eplPath+'/'+eString().sprintf("userbouquet.%x.radio",currentSelectedUserBouquetRef.data[1]), name, currentSelectedUserBouquetRef, false );
			break;
		}

		eServiceInterface *iface=eServiceInterface::getInstance();
		ASSERT(iface);

		Signal1<void,const eServiceReference&> signal;
		CONNECT(signal, eZapMain::addServiceToCurUserBouquet);

		iface->enterDirectory(ref, signal);
		iface->leaveDirectory(ref);	// we have a copy

		currentSelectedUserBouquet->save();
		currentSelectedUserBouquet=0;
		currentSelectedUserBouquetRef = eServiceReference();
	}
	break;

	case 9: // rename recorded movie
	{
		std::list<ePlaylistEntry>::iterator it=std::find(recordings->getList().begin(), recordings->getList().end(), ref);
		if (it == recordings->getList().end())
			break;

		if ( it->service.type == eServiceReference::idDVB )
		{
			if ( it->type & ePlaylistEntry::boundFile )
			{
				TextEditWindow wnd(_("Enter new name for the recorded movie:"));
				hide();
				wnd.setText(_("Rename recorded movie"));
				wnd.show();
				if ( it->service.descr.length() )
					wnd.setEditText(it->service.descr);
				else
				{
					int i = it->service.path.rfind('/');
					++i;
					wnd.setEditText(it->service.path.mid( i, it->service.path.length()-i ));
				}
				wnd.setMaxChars(50);
				int ret = wnd.exec();
				wnd.hide();
				show();
				if ( !ret )
				{
					if ( it->service.descr != wnd.getEditText() )
					{
						it->service.descr = wnd.getEditText();
						recordings->save();
						sel->actualize();
					}
				}
			}
		}
	}
	break;
	}
	sel->show();
}

void eZapMain::addServiceToCurUserBouquet(const eServiceReference& service)
{
	currentSelectedUserBouquet->getList().push_back(service);
}

void eZapMain::playService(const eServiceReference &service, int flags)
{
	int first=0;
	if (flags & psDontAdd)
	{
		if ((playlist->current != playlist->getConstList().end() ) && (playlist->current->service != service))
			getPlaylistPosition();
		eServiceInterface::getInstance()->play(service);

		std::list<ePlaylistEntry>::iterator i=std::find(playlist->getList().begin(), playlist->getList().end(), service);
		if (i != playlist->getList().end())
		{
			playlist->current=i;
			setPlaylistPosition();
		}
		return;
	}
		// we assume that no "isDirectory" is set - we won't open the service selector again.
	if (!(flags & psAdd))
	{
		if (!playlistmode)		// dem user liebgewonnene playlists nicht einfach killen
			while (playlist->getConstList().size() > 10)
				playlist->getList().pop_front();
		if ((!playlistmode) && (service.flags & eServiceReference::mustDescent)) // a playlist/complete directory..
		{
			playlist->getList().clear();
			first=1;
			playlistmode=1;
		} else
		{
			playlist->current=playlist->getList().end();
			if (playlist->current == playlist->getList().begin())
				first=1;
		}
	}
	addService(service);
	if (! (flags & psAdd))
	{
		if (first)
			playlist->current = playlist->getList().begin();
		if (playlist->current != playlist->getConstList().end())
			eServiceInterface::getInstance()->play(*playlist->current);
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
		if (playlist->current != playlist->getConstList().end() && *playlist->current == service)
		{
			++playlist->current;
			last=1;
		}
		playlist->getList().remove(service);
		playlist->getList().push_back(ePlaylistEntry(service));
		if ((playlist->current == playlist->getConstList().end()) || last)
			--playlist->current;
	}
}

void eZapMain::doPlaylistAdd(const eServiceReference &service)
{
	entered_playlistmode=1;
	if (!playlistmode)
	{
		playlistmode=1;
		playlist->getList().clear();
		playlist->current=playlist->getList().begin();
		playService(service, 0);
	} else
		playService(service, psAdd);
}

void eZapMain::addServiceToUserBouquet(eServiceSelector *sel, int dontask)
{
	const eServiceReference &service=sel->getSelected();
	if ((mode > modeFile) || (mode < 0))
		return;

	if (!dontask)
	{
		UserBouquetSelector s( mode == modeTV?userTVBouquets->getList():mode==modeRadio?userRadioBouquets->getList():userFileBouquets->getList() );
		sel->hide();
		s.show();
		s.exec();
		s.hide();

		if (s.curSel != eServiceReference() )
		{
			currentSelectedUserBouquetRef = s.curSel;
			currentSelectedUserBouquet = (ePlaylist*)eServiceInterface::getInstance()->addRef( currentSelectedUserBouquetRef );
		}
	}

	if (currentSelectedUserBouquet)
	{
		if (!dontask)
		{
			for (std::list<ePlaylistEntry>::const_iterator i(currentSelectedUserBouquet->getConstList().begin()); i != currentSelectedUserBouquet->getConstList().end(); ++i)
				if (i->service == service)
				{
					eMessageBox box(_("This service is already in this user bouquet."), _("Add channel to user bouquet"), eMessageBox::iconWarning|eMessageBox::btOK);
					box.show();
					box.exec();
					box.hide();
					sel->show();
					goto ret;
				}
		}
		currentSelectedUserBouquet->getList().push_back(service);
	}
ret:
	if (!dontask)
	{
		currentSelectedUserBouquet->save();
		currentSelectedUserBouquet=0;
		eServiceInterface::getInstance()->removeRef( currentSelectedUserBouquetRef );
		currentSelectedUserBouquetRef = eServiceReference();
	}
}

void eZapMain::removeServiceFromUserBouquet(eServiceSelector *sel )
{
	const eServiceReference &service=sel->getSelected();
	if ((mode > modeFile) || (mode < 0))
		return;

	if (currentSelectedUserBouquet)
		currentSelectedUserBouquet->getList().remove(service);
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
				// we search tuxtxt... this use lcd and vtxtpid... not perfect... but i havo no other idea
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
	if ( eServiceInterface::getInstance()->service.type != eServiceReference::idDVB)
		return;

	int stype = ((eServiceReferenceDVB&)eServiceInterface::getInstance()->service).getServiceType();

	eServiceReferenceDVB& ref = ( stype > 4 ) ? refservice : (eServiceReferenceDVB&)eServiceInterface::getInstance()->service;
	const eService *service=0;

	service = eServiceInterface::getInstance()->addRef( ref );

	if (!service && !(ref.flags & eServiceReference::isDirectory) )
		return;

	if (isVisible())
	{
		timeout.stop();
		hide();
	}

	if (isEPG)
	{
		const eventMap* pMap = eEPGCache::getInstance()->getEventMap( ref );
		if (pMap)  // EPG vorhanden
		{
			eventMap::const_iterator It = pMap->begin();

			ePtrList<EITEvent> events;
			events.setAutoDelete(false);

			while (It != pMap->end())
			{
				events.push_back( new EITEvent(*It->second) );
				It++;
			}
			actual_eventDisplay=new eEventDisplay( service->service_name.c_str(), &events );
		}
	}
	else
	{
		EIT *eit=eDVB::getInstance()->getEIT();
		ePtrList<EITEvent> dummy;
		actual_eventDisplay=new eEventDisplay( service->service_name.c_str(), eit?&eit->events:&dummy);
		if (eit)
			eit->unlock();		// HIER liegt der hund begraben.
	}
	eZapLCD* pLCD = eZapLCD::getInstance();
	pLCD->lcdMain->hide();
	pLCD->lcdMenu->show();
	actual_eventDisplay->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	actual_eventDisplay->show();
	actual_eventDisplay->exec();
	actual_eventDisplay->hide();
	pLCD->lcdMenu->hide();
	pLCD->lcdMain->show();
	delete actual_eventDisplay;
	actual_eventDisplay=0;

	eServiceInterface::getInstance()->removeRef( ref );
}

bool eZapMain::handleState(int justask)
{
	eString text, caption;
	bool b=false;
	if ( state & stateRecording )
	{
		if ( state & stateInTimerMode )
		{
			if (state & recDVR )
				text=_("A timer based digital recording is currently in progress!\n"
							"This stops the timer, and digital recording!");
			else
				return true; // we wouldn't destroy the VCR Recording *g*
/*				text=_("Currently an timer based VCR recording is in progress!\n"
							"This stops the timer, and the VCR recording!");*/
		}
		else
		{
			if (state & recDVR )
				text=_("A digital recording is currently in progress!\n"
							"This stops the digital recording!");
			else
				return true; // we wouldn't destroy the VCR Recording *g*
/*				text=_("Currently an VCR recording is in progress!\n"
							"This stops the VCR recording!");*/
		}
	}
	else if ( state & stateInTimerMode )
	{
		text=_("A timer event is currently in progress!\n"
					"This stops the timer event!");
	}
	else		// not timer event or recording in progress
		return true;

	eMessageBox box(text, _("Really do this?"), eMessageBox::iconQuestion|eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btNo );
	box.show();
	b = (box.exec() == eMessageBox::btYes);
	box.hide();

	if (b && !justask)
	{
		if (state & stateInTimerMode)
		{
			eTimerManager::getInstance()->abortEvent( ePlaylistEntry::errorUserAborted );
		}
		else if (state & stateRecording)
		{
			if (state & recDVR)
				recordDVR(0,0);
			else
				; // here send LIRC VCR Stop
		}
	}
	return b;
}

void eZapMain::blinkRecord()
{
	if (state & stateRecording)
	{
		if (isVisible())
		{
			if (recstatus->isVisible())
				recstatus->hide();
			else
				recstatus->show();

			if ( hddDev != 1 )
			{
				static int cnt=0;
				static int swp=0;
				int fds=freeRecordSpace();
				if (!(cnt++ % 7))
					swp^=1;
				if (swp)
					DVRSpaceLeft->setText(eString().sprintf("%d.%02d GB\nfree", fds/1024, (fds%1024)/10.34 ));
				else
				{
					int min = fds/33;
					
					if (min<60)
						DVRSpaceLeft->setText(eString().sprintf("~%d min free", min ));
					else
						DVRSpaceLeft->setText(eString().sprintf("~%dh%02d\nfree", min/60, min%60 ));
				}
			}
		}
		recStatusBlink.start(500, 1);
	}
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
		}
		else if (event.action == &i_enigmaMainActions->standby_press)
		{
			if ( handleState() )
				standbyPress();
		}
		else if (event.action == &i_enigmaMainActions->standby_repeat)
			standbyRepeat();
		else if (event.action == &i_enigmaMainActions->standby_release)
			standbyRelease();
		else if ( !isVisible() && event.action == &i_enigmaMainActions->showInfobar)
			showInfobar();
		else if (event.action == &i_enigmaMainActions->hideInfobar)
			hideInfobar();
		else if ( isVisible() && event.action == &i_enigmaMainActions->showInfobarEPG)
			showEPG();
		else if (event.action == &i_enigmaMainActions->showServiceSelector)
		{
//			if (handleState())
			showServiceSelector(-1, 1);
		}
		else if (event.action == &i_enigmaMainActions->showSubservices )
		{
			if ( handleState() )
				showSubserviceMenu();
		}
		else if (event.action == &i_enigmaMainActions->showAudio)
		{
//			if ( handleState() )
				showAudioMenu();
		}
		else if (event.action == &i_enigmaMainActions->pluginVTXT)
		{
//			if ( handleState() )
				runVTXT();
		}
		else if (event.action == &i_enigmaMainActions->showEPGList)
			showEPGList();
		else if (event.action == &i_enigmaMainActions->nextService)
		{
			if ( handleState() )
				nextService();
		}	else if (event.action == &i_enigmaMainActions->prevService)
		{
			if ( handleState() )
				prevService();
		}
		else if (event.action == &i_enigmaMainActions->playlistNextService)
		{
			if ( handleState() )
				playlistNextService();
		}
		else if (event.action == &i_enigmaMainActions->playlistPrevService)
		{
			if ( handleState() )
				playlistPrevService();
		}
		else if (event.action == &i_enigmaMainActions->serviceListDown)
		{
//			if ( handleState() )
				showServiceSelector(eServiceSelector::dirDown, 1);
		}
		else if (event.action == &i_enigmaMainActions->serviceListUp)
		{
//			if ( handleState() )
				showServiceSelector(eServiceSelector::dirUp, 1);
		}
		else if (event.action == &i_enigmaMainActions->volumeUp)
			volumeUp();
		else if (event.action == &i_enigmaMainActions->volumeDown)
			volumeDown();
		else if (event.action == &i_enigmaMainActions->toggleMute)
			toggleMute();
		else if (dvrfunctions && event.action == &i_enigmaMainActions->play)
			play();
		else if (dvrfunctions && event.action == &i_enigmaMainActions->stop)
		{
			if ( state & stateRecording )
			{
				if ( handleState(1) )
				{
					record();
				}
			}
			else
				stop();
		}
		else if (dvrfunctions && event.action == &i_enigmaMainActions->pause)
			pause();
		else if (dvrfunctions && event.action == &i_enigmaMainActions->record )
		{
			if ( state & stateRecording && !(state & stateInTimerMode) )
			{
				eRecordContextMenu menu;
				if (isVisible())
					hide();
				menu.show();
				int ret = menu.exec();
				menu.hide();
				switch ( ret )
				{
					case 1: // stop record now
						record();
					break;

					case 2: // set Record Duration...
					{
						eTimerInput e;
						e.show();
						EITEvent *evt = (EITEvent*) e.exec();

						if (evt != (EITEvent*)-1)
						{
							eTimerManager::getInstance()->addEventToTimerList( &e, &eServiceInterface::getInstance()->service, evt, ePlaylistEntry::stateWaiting|ePlaylistEntry::typeShutOffTimer|ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recDVR );
							wasSleeping = e.getCheckboxState();
							delete evt;
						}

						e.hide();
					}
					break;

					case 3: // set Record Stop Time...
					{
						eRecTimeInput e;
						e.show();
						EITEvent *evt = (EITEvent*) e.exec();
						if (evt != (EITEvent*)-1)
						{
							eTimerManager::getInstance()->addEventToTimerList( &e, &eServiceInterface::getInstance()->service, evt, ePlaylistEntry::stateWaiting|ePlaylistEntry::typeShutOffTimer|ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recDVR );
							wasSleeping = e.getCheckboxState();
							delete evt;
						}
						e.hide();
					}
					break;

					case 0:
					default:
						;
				}
			}
			else if ( handleState(1) )
			{
				record();
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
		else if (event.action == &i_numberActions->key0)
		{
			if ( playlist->getConstList().size() > 1 )
			{
				std::list<ePlaylistEntry>::iterator prev,last;
				last = playlist->getList().end();
				last--;
				prev=last;
				prev--;
				playService( prev->service, 0 );
				playlist->getList().push_back( *prev );
				playlist->current = playlist->getList().end();
				playlist->current--;
				playlist->getList().erase(prev);
			}
		}
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
		else if (event.action == &i_enigmaMainActions->showBouquets)
		{
//			if ( handleState() )
				showBouquetList(0);
		}
		else if (event.action == &i_enigmaMainActions->modeRadio)
		{
			if ( handleState() )
			{
				setMode(modeRadio, 1);
				showServiceSelector(-1, 1);
			}
		}
		else if (event.action == &i_enigmaMainActions->modeTV)
		{
			if ( handleState() )
			{
				setMode(modeTV, 2);
				showServiceSelector(-1, 1);
			}
		}
		else if (event.action == &i_enigmaMainActions->toggleDVRFunctions)
		{
			showDVRFunctions(!dvrfunctions);
			showInfobar();
		}
		else if (event.action == &i_enigmaMainActions->toggleIndexmark)
		{
			toggleIndexmark();
		}
		else
		{
			startMessages();
			break;
		}
		startMessages();
		if ( num && handleState())
		{
			if (isVisible())
				hide();
			eServiceNumberWidget s(num);
			s.show();
			num = s.exec();
			if (num != -1)
			{
				if (num < 200)	// < 200 is in favourite list
				{
					ePlaylist *p=0;
					switch(mode)
					{
						case modeTV:
							if ( userTVBouquets->getList().size() )
								p=userTVBouquets;
						break;
						case modeRadio:
							if ( userRadioBouquets->getList().size() )
								p=userRadioBouquets;
						break;
						case modeFile:
							if ( userFileBouquets->getList().size() )
								p=userFileBouquets;
						break;
					}
					if ( p )
					{
						ePlaylist *pl = (ePlaylist*)eServiceInterface::getInstance()->addRef( p->getList().front().service );
						std::list<ePlaylistEntry>::iterator it(pl->getList().begin());
						for (; it != pl->getList().end(); it++ )
							if (!--num)
								break;

						if (!num)
							playService( it->service, 0);

						eServiceInterface::getInstance()->removeRef( p->getList().front().service );
					}
				}
				else
				{
					eServiceReferenceDVB s=eDVB::getInstance()->settings->getTransponders()->searchServiceByNumber(num);
					if (s)
						playService(s, 0);
				}
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
			progresstimer.start(1000);
			updateProgress();
		} else
		{
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
	case eServiceEvent::evtRecordFailed:
		if ( state&stateInTimerMode )
			eTimerManager::getInstance()->abortEvent( ePlaylistEntry::errorNoSpaceLeft );
		else
			recordDVR(0,1);  // stop Recording
		break;
	case eServiceEvent::evtEnd:
	{
		int serviceFlags = eServiceInterface::getInstance()->getService()->getFlags();
		if(! (serviceFlags & eServiceHandler::flagIsTrack) )
			break;

		if (playlist->current != playlist->getConstList().end())
		{
			playlist->current->current_position=-1;
			++playlist->current;
		}
		if (playlist->current != playlist->getConstList().end() )
		{
			playlist->current->current_position=-1;		// start from beginning.
			eServiceInterface::getInstance()->play(*playlist->current);
		}
		else if (!playlistmode)
			nextService(1);
		else
			eDebug("ende im gelaende.");
		break;
	}
	case eServiceEvent::evtStatus:
	{
		eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
		if (!sapi)
			return;
		eDebug("evtStatus");
		showDVRFunctions(0);
		EINow->setText(sapi->getInfo(0));
		break;
	}
	case eServiceEvent::evtInfoUpdated:
	{
		eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
		if (!sapi)
			return;
		eDebug("evtInfoUpdated");
		showDVRFunctions(0);
		EINow->setText(sapi->getInfo(1));
		EINext->setText(sapi->getInfo(2));
		break;
	}}
}

void eZapMain::startService(const eServiceReference &_serviceref, int err)
{
	skipcounter=0;
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();

	if (!sapi)
		return;

	eService *service=eServiceInterface::getInstance()->addRef(_serviceref);

	if (_serviceref.type == eServiceReference::idDVB && !_serviceref.path.length() )
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
		if ( refservice != serviceref && !( refservice.flags & eServiceReference::flagDirectory ) )
		{
			rservice=eDVB::getInstance()->settings->getTransponders()->searchService(refservice);

			if (refservice.getServiceType()==4) // nvod ref service
				flags|=ENIGMA_NVOD;
		}
		else
		{
			if (serviceref.getServiceType()==4) // nvod ref service
				flags|=ENIGMA_NVOD;
		}

		if (serviceref.getServiceType()==6)  // linkage service
			flags|=ENIGMA_SUBSERVICES;
		else
			subservicesel.clear();

		eString name="";

		if (rservice)
		{
			name=rservice->service_name;
			if (serviceref.getServiceType() == 6)  // linkage service..
				name+=" - " + serviceref.descr;
		}
		else if (service)
			name=service->service_name;

		if (!name.length())
			name="unknown service";

		eZapLCD::getInstance()->lcdMain->setServiceName(name);

		if ( !serviceref.path.length() )  // no recorded movie
		{
			int opos=0;
			if (rservice)
				opos = refservice.data[4]>>16;
			else
				opos = serviceref.data[4]>>16;
			name+=eString().sprintf(" (%d.%d°%c)", abs(opos / 10), abs(opos % 10), opos>0?'E':'W');
		}

		ChannelName->setText(name);

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
			break;
		case -ENOCASYS:
		{
			int serviceFlags = eServiceInterface::getInstance()->getService()->getFlags();
			if( serviceFlags & eServiceHandler::flagIsScrambled )
			{
				Description->setText(_("This service could not be descrambled"));
				postMessage(eZapMessage(0, _("switch"), _("This service could not be descrambled"), 2), 1);
				eDebug("This service could not be descrambled");
			}
			break;
		}
		case -ENOSTREAM:
			Description->setText(_("This service doesn't currently send a signal"));
			postMessage(eZapMessage(0, _("switch"), _("This service doesn't currently send a signal"), 2), 1);
			eDebug("This service doesn't currently send a signal");
			break;
		case -ENOSYS:
			Description->setText(_("This content could not be displayed"));
			eDebug("This content could not be displayed");
			postMessage(eZapMessage(0, _("switch"), _("This content could not be displayed"), 2), 1);
			break;
		case -ENVOD:
			Description->setText(_("NVOD Service - please select a starttime"));
			eDebug("NVOD Service - please select a starttime");
			postMessage(eZapMessage(0, _("switch"), _("NVOD Service - please select a starttime"), 5), 1);
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
		if ((num == -1) && service)
			num=eZap::getInstance()->getServiceSelector()->getServiceNum(_serviceref);
		if ((num == -1) && service && service->dvb)
			num=service->dvb->service_number;

		if (num != -1)
		{
#if 1
			if (tuxbox_get_submodel() == TUXBOX_SUBMODEL_DREAMBOX_DM5600)
			{
				eDebug("write number to led-display");
				int fd=::open("/dev/dbox/fp0",O_RDWR);
				::ioctl(fd,4,&num);
				::close(fd);
			}
#endif
			ChannelNumber->setText(eString().sprintf("%d", num));
		}
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

		if (service && service->id3)
		{
			eString artist="unknown artist", album="unknown album", title="", num="";
			eString line2;
			if (service->id3->tags.count("TALB"))
				album=service->id3->tags.find("TALB")->second;
			if (service->id3->tags.count("TIT2"))
				title=service->id3->tags.find("TIT2")->second;
			if (service->id3->tags.count("TPE1"))
				artist=service->id3->tags.find("TPE1")->second;
			if (service->id3->tags.count("TRCK"))
				num=service->id3->tags.find("TRCK")->second;
			EINow->setText(artist + ": " + album);
			line2="";
			if (num)
				line2+="[" + num + "] ";
			line2+=title;
			EINext->setText(line2);
		}
	}

	if (!eZap::getInstance()->focus)
	{
		eDebug("!eZap::getInstance()->focus");
//		showDVRFunctions(0);
		show();
	}

	eServiceInterface::getInstance()->removeRef(_serviceref);

// Quick und Dirty ... damit die aktuelle Volume sofort angezeigt wird.
	eAVSwitch::getInstance()->sendVolumeChanged();

	if ( eServiceInterface::getInstance()->service.type == eServiceReference::idDVB )
		timeout.start((sapi->getState() == eServiceHandler::statePlaying)?5000:2000, 1);
}

void eZapMain::gotEIT()
{
	eServiceHandler *sapi=eServiceInterface::getInstance()->getService();
	if (!sapi)
	{
		eDebug("no sapi");
		return;
	}

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
	if (pRotorMsg && pRotorMsg->isVisible() )
	{
		pRotorMsg->hide();
		delete pRotorMsg;
		pRotorMsg=0;
	}
	else if (eZap::getInstance()->focus==this)
		hide();
}

void eZapMain::leaveService()
{
	ButtonGreenDis->show();
	ButtonGreenEn->hide();
	ButtonYellowDis->show();
	ButtonYellowEn->hide();

	flags&=~(ENIGMA_NVOD|ENIGMA_SUBSERVICES|ENIGMA_AUDIO);

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
	else if ( mute.isVisible() )
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
	if ( c == eZapMain::messageGoSleep )
	{
		eDebug("goto Standby (sleep)");
		standbyPress();
		standbyRelease();
		return;
	}
	else if ( c == eZapMain::messageShutdown )
	{
		eZap::getInstance()->quit();
		return;
	}

	if ((!c) && pMsg) // noch eine gueltige message vorhanden
	{
		return;
	}
	if ((!isVisible()) && eZap::getInstance()->focus)
	{
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
	if ( handleState() )
	{
		if ( newmode == modeFile )
			playlist->service_name=_("Playlist");
		else
			playlist->service_name=_("History");

		if ( newmode == modeFile && mode != newmode )
			eEPGCache::getInstance()->pauseEPG();
		else if ( mode == modeFile && mode != newmode && newmode != -1 )
			eEPGCache::getInstance()->restartEPG();

		eDebug("setting mode to %d", newmode);

		// save oldMode
		if (mode != -1)
			getServiceSelectorPath(modeLast[mode][0]);

		if (mode == newmode)
			user=0;

		if ( newmode != -1 )
			mode=newmode;

		if (user)
		{
			eDebug("playservice");
			playService(modeLast[mode][0].current(), psDontAdd);
		}

		if (mode != -1)
		{
			eDebug("setServiceSelectorPath");
			setServiceSelectorPath(modeLast[mode][0]);
		}
	}
}

void eZapMain::setServiceSelectorPath(eServicePath path)
{
	eServiceReference ref=path.current();
	path.up();
	eServicePath p = path;
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
	if ((!last) || (!recordings->getConstList().size()))
		b.down( eServiceReference() );
	else
		b.down( recordings->getConstList().back().service);
	setServiceSelectorPath(b);
	showServiceSelector(-1, !last);
}

void eZapMain::showDVRFunctions(int show)
{
	dvrfunctions=show;

	if (dvrfunctions)
	{
		nonDVRfunctions->hide();
		dvrFunctions->show();
	} else
	{
		dvrFunctions->hide();
		nonDVRfunctions->show();
	}
}

void eZapMain::moveService(const eServiceReference &path, const eServiceReference &ref, const eServiceReference &afterref)
{
	std::list<ePlaylistEntry>::iterator it=std::find(currentSelectedUserBouquet->getList().begin(), currentSelectedUserBouquet->getList().end(), ref),
																			after;

	if (afterref)
		after=std::find(currentSelectedUserBouquet->getList().begin(), currentSelectedUserBouquet->getList().end(), afterref);
	else
		after=currentSelectedUserBouquet->getList().end();

	currentSelectedUserBouquet->moveService(it, after);
}


void eZapMain::toggleIndexmark()
{
	if (!(serviceFlags & eServiceHandler::flagIsSeekable))
		return;

	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
	if (!handler)
		return;

	int real=handler->getPosition(eServiceHandler::posQueryRealCurrent), time=handler->getPosition(eServiceHandler::posQueryCurrent);

	int nearest=indices.getNext(real, 0);
	if ((nearest == -1) || abs(indices.getTime(nearest)-real) > 5000)
	{
		eDebug("added indexmark");
		indices.add(real, time);
	} else
	{
		eDebug("removed indexmark.");
		indices.remove(nearest);
	}
	redrawIndexmarks();
}

void eZapMain::redrawIndexmarks()
{
	indexmarks.clear();
	for (int i=indices.getNext(-1, 1); i != -1; i=indices.getNext(i, 1))
	{
		int time=indices.getTime(i);
		eDebug("adding indexmark at %d (%d)", time, i);
	}
}

eServiceContextMenu::eServiceContextMenu(const eServiceReference &ref, const eServiceReference &path): eListBoxWindow<eListBoxEntryText>(_("Service Menu"), 5), ref(ref)
{
	move(ePoint(150, 200));
	new eListBoxEntryText(&list, _("back"), (void*)0);
	if (/*!(ref.flags & eListBoxEntryService::flagIsReturn) &&*/ !(ref.flags & eServiceReference::flagDirectory))
		new eListBoxEntryText(&list, _("add service to playlist"), (void*)3);
	if (path.type == eServicePlaylistHandler::ID)
	{
		new eListBoxEntryText(&list, _("delete"), (void*)1);
		if (ref.type == eServicePlaylistHandler::ID)
			new eListBoxEntryText(&list, _("rename"), (void*)7);
		else if ( ref.type == eServiceReference::idDVB && ref.path.size() )
			new eListBoxEntryText(&list, _("rename"), (void*)9);

		// move Mode ( only in Favourite lists... )
		if ( eZap::getInstance()->getServiceSelector()->movemode )
			new eListBoxEntryText(&list, _("disable move mode"), (void*)2);
		else
			new eListBoxEntryText(&list, _("enable move mode"), (void*)2);
		// delete Service ( only in Favourite lists... )
	}
	else
	{
		// add current service to favourite
		if ( !(ref.flags & eServiceReference::flagDirectory) )
			new eListBoxEntryText(&list, _("add to user bouquet"), (void*)4);
		else if (ref.data[0] == -2 || ref.data[0] == -3 )
			new eListBoxEntryText(&list, _("copy to user bouquets"), (void*)8);
		// Favourite Mode ( simple add services to favourite list )
		if ( eZap::getInstance()->getServiceSelector()->editMode )
			new eListBoxEntryText(&list, _("disable edit mode"), (void*)5);
		else
			new eListBoxEntryText(&list, _("enable edit mode"), (void*)5);
	}
		// add current service to playlist
	new eListBoxEntryText(&list, _("create new user bouquet"), (void*)6);
	CONNECT(list.selected, eServiceContextMenu::entrySelected);
}

void eServiceContextMenu::entrySelected(eListBoxEntryText *test)
{
	if (!test)
		close(0);
	else
		close((int)test->getKey());
}

eRecordContextMenu::eRecordContextMenu()
	: eListBoxWindow<eListBoxEntryText>(_("Record Menu"), 5)
{
	move(ePoint(150, 200));
	new eListBoxEntryText(&list, _("back"), (void*)0);
	new eListBoxEntryText(&list, _("stop record now"), (void*)1);
	new eListBoxEntryText(&list, _("set record duration"), (void*)2);
	new eListBoxEntryText(&list, _("set record stop time"), (void*)3);
	CONNECT(list.selected, eRecordContextMenu::entrySelected);
}

void eRecordContextMenu::entrySelected( eListBoxEntryText *sel )
{
	if (!sel)
		close(0);
	else
		close((int)sel->getKey());
}

eRecStopWindow::eRecStopWindow(eWidget *parent, int len, int min, int max, int maxdigits, int *init, int isactive, eWidget* descr, int grabfocus, const char* deco )
{
	num = new eNumber( parent, len, min, max, maxdigits, init, isactive, descr, grabfocus, deco );
	Shutdown = new eCheckbox(this);
	Shutdown->setName("shutdown");
	Standby = new eCheckbox(this);
	Standby->setName("standby");
	set = new eButton(this);
	set->setName("set");
	cancel = new eButton(this);
	cancel->setName("abort");
	CONNECT( num->selected, eRecStopWindow::fieldSelected );
	CONNECT( Shutdown->checked, eRecStopWindow::ShutdownChanged );
	CONNECT( Standby->checked, eRecStopWindow::StandbyChanged );
	CONNECT( cancel->selected, eWidget::reject );
}

void eRecStopWindow::StandbyChanged( int checked )
{
	if ( checked )
		Shutdown->setCheck( 0 );
}

void eRecStopWindow::ShutdownChanged( int checked )
{
	if ( checked )
		Standby->setCheck( 0 );
}

int eRecStopWindow::getCheckboxState()
{
	return Standby->isChecked()?1:Shutdown->isChecked()?2:0;
}

eTimerInput::eTimerInput()
:eRecStopWindow( this, 1, 1, 240, 3, 0, 0 )
{
	eLabel *l = new eLabel(this);
	l->setName("lrec_duration");
	num->setDescr(l);
	num->setName("rec_duration");
	num->setNumber(10);
	if (eSkin::getActive()->build(this, "recording_duration"))
		eFatal("skin load of \"recording_duration\" failed");
	CONNECT( set->selected, eTimerInput::setPressed );
}

void eTimerInput::setPressed()
{
	EITEvent *evt = new EITEvent();
	evt->start_time = time(0)+eDVB::getInstance()->time_difference;
	evt->duration = num->getNumber()*60;
	evt->event_id = -1;
	evt->free_CA_mode = -1;
	evt->running_status = -1;
	close((int)evt);
}

eRecTimeInput::eRecTimeInput()
:eRecStopWindow( this, 2, 0, 59, 2, 0, 0 )
{
	eLabel *l = new eLabel(this);
	l->setName("lrec_end_time");
	num->setDescr(l);
	num->setName("rec_end_time");
	num->setFlags( eNumber::flagFillWithZeros|eNumber::flagTime );

	time_t now = time(0)+eDVB::getInstance()->time_difference;
	struct tm *t = localtime( &now );
	num->setNumber(0, t->tm_hour);
	num->setNumber(1, t->tm_min);

	if (eSkin::getActive()->build(this, "recording_end_time"))
		eFatal("skin load of \"recording_end_time\" failed");

	CONNECT( set->selected, eRecTimeInput::setPressed );
}

extern time_t normalize( struct tm & t );

void eRecTimeInput::setPressed()
{
	int hour = num->getNumber(0);
	int min = num->getNumber(1);

	time_t now = time(0)+eDVB::getInstance()->time_difference;
	struct tm *t = localtime( &now );

	if ( hour*60+min < t->tm_hour*60+t->tm_min )
	{
		t->tm_mday++;
		t->tm_hour = hour;
		t->tm_min = min;
		normalize(*t);
	}
	else
	{
		t->tm_hour = hour;
		t->tm_min = min;
	}

	time_t tmp = mktime( t );

	EITEvent *evt = new EITEvent();
	evt->start_time = time(0)+eDVB::getInstance()->time_difference;
	evt->duration = tmp - evt->start_time;
	evt->event_id = -1;
	evt->free_CA_mode = -1;
	evt->running_status = -1;
	close((int)evt);
}

TextEditWindow::TextEditWindow( const char *InputFieldDescr, const char* useableChars )
	:eWindow(0)
{
	input = new eTextInputField(this);
	input->setName("inputfield");
	input->setMaxChars(20);
	input->setHelpText(_("press ok to start edit mode"));
	if (useableChars)
		input->setUseableChars( useableChars );

	descr = new eLabel(this);
	descr->setName("descr");
	descr->setText(InputFieldDescr);

	image = new eLabel(this);
	image->setName("image");

	save = new eButton(this);
	save->setName("save");
	CONNECT( save->selected, TextEditWindow::accept );
	
	cancel = new eButton(this);
	cancel->setName("cancel");
	CONNECT( cancel->selected, TextEditWindow::reject );

	eStatusBar *n = new eStatusBar(this);
	n->setName("statusbar");

	if (eSkin::getActive()->build(this, "TextEditWindow"))
		eWarning("TextEditWindo widget build failed!");
}

UserBouquetSelector::UserBouquetSelector( std::list<ePlaylistEntry>&list )
	:eListBoxWindow<eListBoxEntryText>(_("User Bouquets"), 8, 400),
	SourceList(list)
{
	move(ePoint(100,80));

	for (std::list<ePlaylistEntry>::iterator it( SourceList.begin() ); it != SourceList.end(); it++)
	{
		ePlaylist *pl = (ePlaylist*)eServiceInterface::getInstance()->addRef( it->service );
		new eListBoxEntryText( &this->list, pl->service_name, &it->service );
		eServiceInterface::getInstance()->removeRef( it->service );
	}
	CONNECT( this->list.selected, UserBouquetSelector::selected );
}

void UserBouquetSelector::selected( eListBoxEntryText *sel )
{
	if (sel && sel->getKey())
		curSel=*((eServiceReference*)sel->getKey());

	close(0);
}
