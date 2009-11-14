#ifndef DISABLE_CI

#include <enigma_ci.h>
#include <enigma.h>
#include <lib/base/i18n.h>
#include <lib/driver/rc.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/dvbci.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/statusbar.h>
#include <lib/gui/eskin.h>
#include <lib/gdi/font.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>

enigmaCI::enigmaCI()
	:ci_messages(eApp,1), ci2_messages(eApp,1)
{
	init_enigmaCI();
}

void enigmaCI::init_enigmaCI()
{
	CONNECT(ci_messages.recv_msg, enigmaCI::updateCIinfo );

	DVBCI=eDVB::getInstance()->DVBCI;

	if( eSystemInfo::getInstance()->hasCI() > 1 )
	{
		setText(_("Common Interface Modules"));
		move(ePoint(160, 90));
		cresize(eSize(350, 330));
		DVBCI2=eDVB::getInstance()->DVBCI2;
		CONNECT(ci2_messages.recv_msg, enigmaCI::updateCI2info );
	}

	CONNECT(CreateSkinnedButton("reset")->selected, enigmaCI::resetPressed);
	CONNECT(CreateSkinnedButton("init")->selected, enigmaCI::initPressed);

	app=CreateSkinnedButton("app");
	CONNECT(app->selected, enigmaCI::appPressed);		

	eCheckbox *twoServices;
	twoServices = CreateSkinnedCheckbox("twoServices",0,"/ezap/ci/handleTwoServices");
	twoServices->setFlags(RS_WRAP|eLabel::flagVCenter);
	CONNECT(twoServices->checked, enigmaCI::handleTwoServicesChecked);

	if( eSystemInfo::getInstance()->hasCI() > 1 )
	{
		CONNECT(CreateSkinnedButton("reset2")->selected, enigmaCI::reset2Pressed);
		CONNECT(CreateSkinnedButton("init2")->selected, enigmaCI::init2Pressed);
		app2=CreateSkinnedButton("app2");
		CONNECT(app2->selected, enigmaCI::app2Pressed);
		twoServices ->hide();
	}
	else
	{
		CreateSkinnedButton("reset2")->hide();
		CreateSkinnedButton("init2")->hide();
		CreateSkinnedButton("app2")->hide();
	}

	CONNECT(DVBCI->ci_progress, enigmaCI::gotCIinfoText);
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::getAppName));

	if( eSystemInfo::getInstance()->hasCI() > 1 )
	{
		CONNECT(DVBCI2->ci_progress, enigmaCI::gotCI2infoText);
		DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::getAppName));
	}
}

enigmaCI::~enigmaCI()
{
}

void enigmaCI::handleTwoServicesChecked(int val)
{
	eConfig::getInstance()->setKey("/ezap/ci/handleTwoServices", val);
}

void enigmaCI::gotCIinfoText(const char *text)
{
	// called from CI thread !!
	if (text)
		ci_messages.send(text);
}

void enigmaCI::gotCI2infoText(const char *text)
{
	// called from CI2 thread !!
	if (text)
		ci2_messages.send(text);
}

void enigmaCI::updateCIinfo(const char * const &buffer)
{
	eDebug("new info %s",buffer);
	app->setText(buffer);
}

void enigmaCI::updateCI2info(const char * const &buffer)
{
	eDebug("new info %s",buffer);
	app2->setText(buffer);
}

void enigmaCI::resetPressed()
{
	app->setText(_("resetting....please wait"));
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::reset));
}

void enigmaCI::reset2Pressed()
{
	app2->setText(_("resetting....please wait"));
	DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::reset));
}

void enigmaCI::initPressed()
{
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::init));
}

void enigmaCI::init2Pressed()
{
	DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::init));
}

void enigmaCI::appPressed()
{
	hide();
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_begin));
	enigmaCIMMI::getInstance(DVBCI)->exec();
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_end));
	show();
}

void enigmaCI::app2Pressed()
{
	hide();
	DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_begin));
	enigmaCIMMI::getInstance(DVBCI2)->exec();
	DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_end));
	show();
}

// -----------  CI MMI ----------------
std::map<eDVBCI*,enigmaCIMMI*> enigmaCIMMI::exist;

enigmaCIMMI* enigmaCIMMI::getInstance( eDVBCI* ci )
{
	std::map<eDVBCI*, enigmaCIMMI*>::iterator it = exist.find(ci);
	if ( it == exist.end() )
		exist[ci]=new enigmaCIMMI(ci);
	return exist[ci];
}

enigmaCIMMI::enigmaCIMMI( eDVBCI *ci )
	:ci(ci)
{
	setText(_("Common Interface Module - mmi"));
	lText->setText(_("waiting for CI answer..."));
	int newHeight = size.height() - getClientSize().height() + lText->getExtend().height() + 10 + 20;
	resize( eSize( size.width(), newHeight ) );
}

void enigmaCIMMI::sendAnswer( AnswerType ans, int param, unsigned char *data )
{
	switch(ans)
	{
		case ENQAnswer:
			ci->messages.send( eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_enqansw, param, data));
			break;
		case LISTAnswer:
		case MENUAnswer:
			ci->messages.send( eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::mmi_menuansw,param));
			break;
	}
}

void enigmaCIMMI::beginExec()
{
	conn = CONNECT(ci->ci_mmi_progress, enigmaMMI::gotMMIData );
}

#endif // DISABLE_CI
