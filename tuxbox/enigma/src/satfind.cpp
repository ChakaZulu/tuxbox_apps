#include "satfind.h"
#include <lib/base/ebase.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/elabel.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/echeckbox.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvbservice.h>

eSatfind::eSatfind(eFrontend *fe)
	:eWindow(0), updateTimer(eApp), fe(fe)
{
	p_snr=new eProgress(this);
	p_snr->setName("snr");

	p_agc=new eProgress(this);
	p_agc->setName("agc");

	p_ber=new eProgress(this);
	p_ber->setName("ber");

	c_sync=new eCheckbox(this, 0, 0);
	c_sync->setName("sync");

	c_lock=new eCheckbox(this, 0, 0);
	c_lock->setName("lock");

	lsnr_num=new eLabel(this);
	lsnr_num->setName("snr_num");

	lsync_num=new eLabel(this);
	lsync_num->setName("agc_num");

	lber_num=new eLabel(this);
	lber_num->setName("ber_num");

	ok=new eButton(this);
	ok->setName("ok");

	CONNECT(ok->selected, eWidget::reject );
	CONNECT(updateTimer.timeout, eSatfind::update);

	if (eSkin::getActive()->build(this, "eSatfind"))
		return;
}

int eSatfind::eventHandler( const eWidgetEvent& e)
{
	switch (e.type)
	{
		case eWidgetEvent::execBegin:
			updateTimer.start(500, false);
		break;
		case eWidgetEvent::execDone:
			updateTimer.stop();
		break;
		default:
			return eWindow::eventHandler(e);
	}
	return 0;
}

void eSatfind::update()
{                    
	int snr=fe->SNR()*100/65535,
			agc=fe->SignalStrength()*100/65535,
			ber=fe->BER();
	p_agc->setPerc(agc);
	p_snr->setPerc(snr);
	p_ber->setPerc((int)log2(ber));
	lsnr_num->setText(eString().sprintf("%d%%",snr));
	lsync_num->setText(eString().sprintf("%d%%",agc));
	lber_num->setText(eString().sprintf("%d",ber));
	int status=fe->Status();
	if (!(status & FE_HAS_LOCK))
	{
		eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

		if (sapi && sapi->transponder)
			sapi->transponder->tune();
	}
	c_lock->setCheck(!!(status & FE_HAS_LOCK));
	c_sync->setCheck(!!(status & FE_HAS_SYNC));
}
