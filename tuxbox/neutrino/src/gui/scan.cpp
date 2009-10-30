/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/scan.h>

#include <driver/rcinput.h>

#include <gui/color.h>

#include <gui/widget/menue.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/icons.h>

#include <system/settings.h>

#include <global.h>
#include <neutrino.h>

#ifdef HAVE_TRIPLEDRAGON
#elif HAVE_DVB_API_VERSION >= 3
#else
#include <ost/frontend.h>
#include <ost/sec.h>
#endif

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_init();
extern "C" int  tuxtxt_start(int tpid);
//extern "C" int  tuxtxt_stop();
extern "C" void tuxtxt_close();
#endif

CScanTs::CScanTs()
{
	frameBuffer = CFrameBuffer::getInstance();
	hheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	width       = 500;
	height = hheight + (9 * mheight); //9 lines
	x = ((720 - width) >> 1) - 20;
	y = (576 - height) >> 1;
	radar = 0;
	xpos_radar = x + 420;
	ypos_radar = y + hheight + (mheight >> 1);
	xpos1 = x + 10;
	found_transponder = 0;
	canceled = false;
}
#define get_set (CNeutrinoApp::getInstance()->getScanSettings())
#define NEUTRINO_SCAN_SETTINGS_FILE     CONFIGDIR "/scan.conf"
int CScanTs::exec(CMenuTarget* parent, const std::string &)
{
	diseqc_t		diseqcType = NO_DISEQC;
	neutrino_msg_t		msg;
	neutrino_msg_data_t	data;
	TP_params		TP;

	if (parent)
	{
		parent->hide();
	}

	frameBuffer->loadPal("scan.pal", 37, COL_MAXFREE);
	frameBuffer->loadPicture2FrameBuffer("scan.raw");

printf("[neutrino] TP_scan %d TP_freq %s TP_rate %s TP_fec %d TP_pol %d TP_mod %d TP_diseqc %d\n", get_set.TP_scan, get_set.TP_freq, get_set.TP_rate, get_set.TP_fec, get_set.TP_pol, get_set.TP_mod, (uint8_t)get_set.TP_diseqc);

#ifndef TUXTXT_CFG_STANDALONE
	if (g_settings.tuxtxt_cache)
		tuxtxt_close();
#endif
	// manual TP scan
	if(get_set.TP_scan == CScanTs::SCAN_ONE_TP)
	{
#ifdef HAVE_TRIPLEDRAGON
		TP.feparams.frequency = atoi(get_set.TP_freq);
		TP.feparams.symbolrate = atoi(get_set.TP_rate);
		TP.feparams.fec = get_set.TP_fec;
#elif HAVE_DVB_API_VERSION < 3
		TP.feparams.Frequency = atoi(get_set.TP_freq);
		TP.feparams.u.qpsk.SymbolRate = atoi(get_set.TP_rate);
		TP.feparams.u.qpsk.FEC_inner = (CodeRate)get_set.TP_fec;
#else
		TP.feparams.frequency = atoi(get_set.TP_freq);
		if(g_info.delivery_system == DVB_S) {
			TP.feparams.u.qpsk.symbol_rate = atoi(get_set.TP_rate);
			TP.feparams.u.qpsk.fec_inner = (fe_code_rate_t) get_set.TP_fec;
		} else {
			TP.feparams.u.qam.symbol_rate = atoi(get_set.TP_rate);
			TP.feparams.u.qam.fec_inner = (fe_code_rate_t) get_set.TP_fec;
			TP.feparams.u.qam.modulation = (fe_modulation_t) get_set.TP_mod;
		}
#endif
		TP.polarization = get_set.TP_pol;
		TP.diseqc = (uint8_t)get_set.TP_diseqc;
// printf("[neutrino] freq %d rate %d fec %d pol %d\n", TP.feparams.Frequency, TP.feparams.u.qpsk.SymbolRate, TP.feparams.u.qpsk.FEC_inner, TP.polarization);
// printf("[neutrino] freq %d rate %d fec %d pol %d\n", TP.feparams.frequency, TP.feparams.u.qpsk.symbol_rate, TP.feparams.u.qpsk.fec_inner, TP.polarization);
	//return menu_return::RETURN_REPAINT;
	}
	success = false;

	if (!frameBuffer->getActive())
		return menu_return::RETURN_EXIT_ALL;

	if(g_settings.video_Format != g_settings.video_backgroundFormat)
		g_Controld->setVideoFormat(g_settings.video_backgroundFormat);

	g_Sectionsd->Restart();
	g_Sectionsd->RegisterNeutrino();

	/* send diseqc type to zapit */
	diseqcType = CNeutrinoApp::getInstance()->getScanSettings().diseqcMode;
	g_Zapit->setDiseqcType(diseqcType);

	/* send diseqc repeat to zapit */
	g_Zapit->setDiseqcRepeat( CNeutrinoApp::getInstance()->getScanSettings().diseqcRepeat);
	g_Zapit->setScanBouquetMode( CNeutrinoApp::getInstance()->getScanSettings().bouquetMode);

	/* send satellite list to zapit */
	CZapitClient::ScanSatelliteList satList;
	CNeutrinoApp::getInstance()->getScanSettings().toSatList(satList);
	g_Zapit->setScanSatelliteList(satList);

	/* send scantype to zapit */
	g_Zapit->setScanType( CNeutrinoApp::getInstance()->getScanSettings().scanType );

	/* send motor position list to zapit */
	if (diseqcType == DISEQC_1_2)
	{
		CZapitClient::ScanMotorPosList motorPosList;
		CNeutrinoApp::getInstance()->getScanSettings().toMotorPosList(motorPosList);
		g_Zapit->setScanMotorPosList(motorPosList);
	}

	/* go */
	if (get_set.TP_scan == CScanTs::SCAN_ONE_TP)
	{
		success = g_Zapit->scan_TP(TP);
	}
	else if (get_set.TP_scan == CScanTs::SCAN_ONE_SAT)
	{
		success = g_Zapit->startScan(get_set.scan_mode, (uint8_t)get_set.TP_diseqc);
	}
	else	// CScanTs::SCAN_COMPLETE
	{
		success = g_Zapit->startScan(get_set.scan_mode);
	}
	start_time = time(NULL);
	paint();

	/* poll for messages */
	istheend = !success;
	while (!istheend)
	{
		paintRadar();

		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd_MS( 250 );

		do
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);
			msg = handleMsg(msg, data);
		}
		while (!(msg == CRCInput::RC_timeout));
	}

	if (!canceled)
		ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, success ? LOCALE_SCANTS_FINISHED : LOCALE_SCANTS_FAILED, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO);

	hide();

	g_Sectionsd->setPauseScanning(false);

#ifndef TUXTXT_CFG_STANDALONE
	if (g_settings.tuxtxt_cache)
	{
		int vtpid = g_RemoteControl->current_PIDs.PIDs.vtxtpid;
		tuxtxt_init();
		if (vtpid)
			tuxtxt_start(vtpid);
	}
#endif

	if(g_settings.video_Format != g_settings.video_backgroundFormat)
		g_Controld->setVideoFormat(g_settings.video_Format);

	return menu_return::RETURN_REPAINT;
}

int CScanTs::handleMsg(neutrino_msg_t msg, neutrino_msg_data_t data)
{
	char buffer[32];
	switch (msg)
	{
		case NeutrinoMessages::EVT_SCAN_SATELLITE:
			paintLine(xpos2, ypos_cur_satellite, width - xpos2, (char *)data);
			break;

		case NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS:
			sprintf(buffer, "%d", data);
			paintLine(xpos2, ypos_transponder, width - xpos2 - 95, buffer);
			found_transponder = data;
			break;

		case NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS:
			if (found_transponder == 0) data = 0;
			sprintf(buffer, "%d/%d", data, found_transponder);
			paintLine(xpos2, ypos_transponder, width - xpos2 - 95, buffer);
			break;

		case NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCY:
			sprintf(buffer, "%u", data);
			xpos_frequency = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(buffer, true);
			paintLine(xpos2, ypos_frequency, xpos_frequency, buffer);
			break;

		case NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCYP:
			(data == 0) ? sprintf(buffer, "-H") : sprintf(buffer, "-V");
			paintLine(xpos2 + xpos_frequency, ypos_frequency, 30, buffer);
			break;

		case NeutrinoMessages::EVT_SCAN_PROVIDER:
			paintLine(xpos2, ypos_provider, width - xpos2, (char*)data); // UTF-8
			break;

		case NeutrinoMessages::EVT_SCAN_SERVICENAME:
			paintLine(xpos2, ypos_channel, width - xpos2, (char *)data); // UTF-8
			break;

		case NeutrinoMessages::EVT_SCAN_NUM_CHANNELS:
			sprintf(buffer, " = %d", data);
			paintLine(xpos1 + 3 * 72, ypos_service_numbers + mheight, width - 3 * 72 - 10, buffer);
			break;

		case NeutrinoMessages::EVT_SCAN_FOUND_TV_CHAN:
			sprintf(buffer, "%d", data);
			paintLine(xpos1, ypos_service_numbers + mheight, 72, buffer);
			break;

		case NeutrinoMessages::EVT_SCAN_FOUND_RADIO_CHAN:
			sprintf(buffer, "%d", data);
			paintLine(xpos1 + 72, ypos_service_numbers + mheight, 72, buffer);
			break;

		case NeutrinoMessages::EVT_SCAN_FOUND_DATA_CHAN:
			sprintf(buffer, "%d", data);
			paintLine(xpos1 + 2 * 72, ypos_service_numbers + mheight, 72, buffer);
			break;

		case NeutrinoMessages::EVT_SCAN_COMPLETE:
		case NeutrinoMessages::EVT_SCAN_FAILED:
			success = (msg == NeutrinoMessages::EVT_SCAN_COMPLETE);
			istheend = true;
			msg = CRCInput::RC_timeout;
			break;
		case CRCInput::RC_home:
			if (get_set.TP_scan == CScanTs::SCAN_ONE_TP) // only if we scan a whole sat...
				break;

			if (ShowLocalizedMessage(LOCALE_SCANTS_ABORT_HEADER, LOCALE_SCANTS_ABORT_BODY, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo) == CMessageBox::mbrYes)
			{
				g_Zapit->stopScan();
				canceled = true;
			}
			break;
		default:
			if ((msg >= CRCInput::RC_WithData) && (msg < CRCInput::RC_WithData + 0x10000000))
				delete (unsigned char*) data;
			break;
	}
	return msg;
}

void CScanTs::paintRadar(void)
{
	if (radar % 5 == 0)
	{
		time_t t = time(NULL) - start_time;
		if (t < 0) // I'm not sure why this happens - maybe sectionsd restart?
		{
			fprintf(stderr, "CScanTs::paintRadar: time going backwards!\n");
			t = 0;
			start_time = time(NULL);
		}
		char c[10];
		snprintf(c, 10, "%ld:%02ld:%02ld", t/3600, (t/60)%60, t%60);
		paintLine(xpos1 + 5*72, ypos_service_numbers, 120, c);
	}
	char filename[30];
	sprintf(filename, "radar%d.raw", radar);
	radar = (radar + 1) % 10;
	frameBuffer->paintIcon8(filename, xpos_radar, ypos_radar, 17);
}

void CScanTs::hide()
{
	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->paintBackgroundBoxRel(0, 0, 720, 576);
}

void CScanTs::paintLineLocale(int _x, int *_y, int _width, const neutrino_locale_t l)
{
	frameBuffer->paintBoxRel(_x, *_y, _width, mheight, COL_MENUCONTENT_PLUS_0);
	//	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x, *y + mheight, width, g_Locale->getText(l), COL_MENUCONTENT, 0, true); // UTF-8
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x, *_y + mheight, _width, g_Locale->getText(l), COL_MENUCONTENTINACTIVE, 0, true); // UTF-8
	*_y += mheight;
}

void CScanTs::paintLine(int _x, int _y, int _width, const char * const txt)
{
	frameBuffer->paintBoxRel(_x, _y, _width, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x, _y + mheight, _width, txt, COL_MENUCONTENT, 0, true); // UTF-8
}

void CScanTs::paint()
{
	int ypos;

	ypos = y;
	
	int c_rad_mid = RADIUS_MID;
	
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD_PLUS_0, c_rad_mid, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(xpos1, ypos + hheight, width, g_Locale->getText(LOCALE_SCANTS_HEAD), COL_MENUHEAD, 0, true); // UTF-8
	frameBuffer->paintBoxRel(x, ypos + hheight, width, height - hheight, COL_MENUCONTENT_PLUS_0, c_rad_mid, CORNER_BOTTOM);

	frameBuffer->loadPal("radar.pal", 17, 37);

	ypos = y + hheight + (mheight >> 1);

	ypos_cur_satellite = ypos;

	if (g_info.delivery_system == DVB_S)
	{	//sat
		paintLineLocale(xpos1, &ypos, width - xpos1, LOCALE_SCANTS_ACTSATELLITE);
		xpos2 = xpos1 + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(g_Locale->getText(LOCALE_SCANTS_ACTSATELLITE), true); // UTF-8
	}
	if (g_info.delivery_system == DVB_C)
	{	//cable
		paintLineLocale(xpos1, &ypos, width - xpos1, LOCALE_SCANTS_ACTCABLE);
		xpos2 = xpos1 + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(g_Locale->getText(LOCALE_SCANTS_ACTCABLE), true); // UTF-8
	}

	ypos_transponder = ypos;
	paintLineLocale(xpos1, &ypos, width - xpos1, LOCALE_SCANTS_TRANSPONDERS);
	xpos2 = greater_xpos(xpos2, LOCALE_SCANTS_TRANSPONDERS);

	ypos_frequency = ypos;
	paintLineLocale(xpos1, &ypos, width - xpos1, LOCALE_SCANTS_FREQDATA);
	xpos2 = greater_xpos(xpos2, LOCALE_SCANTS_FREQDATA);

	ypos += mheight >> 1; // 1/2 blank line

	ypos_provider = ypos;
	paintLineLocale(xpos1, &ypos, width - xpos1, LOCALE_SCANTS_PROVIDER);
	xpos2 = greater_xpos(xpos2, LOCALE_SCANTS_PROVIDER);

	ypos_channel = ypos;
	paintLineLocale(xpos1, &ypos, width - xpos1, LOCALE_SCANTS_CHANNEL);
	xpos2 = greater_xpos(xpos2, LOCALE_SCANTS_CHANNEL);

	ypos += mheight >> 1; // 1/2 blank line

	ypos_service_numbers = ypos; paintLineLocale(xpos1         , &ypos, 72                 , LOCALE_SCANTS_NUMBEROFTVSERVICES   );
	ypos = ypos_service_numbers; paintLineLocale(xpos1 +     72, &ypos, 72                 , LOCALE_SCANTS_NUMBEROFRADIOSERVICES);
	ypos = ypos_service_numbers; paintLineLocale(xpos1 + 2 * 72, &ypos, 72                 , LOCALE_SCANTS_NUMBEROFDATASERVICES );
	ypos = ypos_service_numbers; paintLineLocale(xpos1 + 3 * 72, &ypos, width - 3 * 72 - 10, LOCALE_SCANTS_NUMBEROFTOTALSERVICES);
}

int CScanTs::greater_xpos(int xpos, const neutrino_locale_t txt)
{
	int txt_xpos = xpos1 + 10 + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(g_Locale->getText(txt), true); // UTF-8
	if (txt_xpos > xpos)
		return txt_xpos;
	else
		return xpos;
}
