/*
 * enigma_info.cpp
 *
 * Copyright (C) 2002 Felix Domke <tmbinc@tuxbox.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: enigma_info.cpp,v 1.1 2002/05/06 16:17:16 tmbinc Exp $
 */

#include "enigma_info.h"
#include "streaminfo.h"
#include "showbnversion.h"
#include "elistbox.h"
#include "ewindow.h"
#include "edvb.h"
#include "eskin.h"
#include "elabel.h"
#include "emessage.h"

#include <core/base/i18n.h>

eZapInfo::eZapInfo()
	:eLBWindow(_("Infos"), eListbox::tBorder, 8, eSkin::getActive()->queryValue("fontsize", 20), 220)
{
	move(ePoint(150, 136));
	CONNECT((new eListboxEntryText(list, _("[back]")))->selected, eZapInfo::sel_close);
	CONNECT((new eListboxEntryText(list, _("Streaminfo")))->selected, eZapInfo::sel_streaminfo);
	CONNECT((new eListboxEntryText(list, _("Show BN version")))->selected, eZapInfo::sel_bnversion);
	CONNECT((new eListboxEntryText(list, _("About...")))->selected, eZapInfo::sel_about);
	
}

eZapInfo::~eZapInfo()
{
}

void eZapInfo::sel_close(eListboxEntry *)
{
	close(0);
}

void eZapInfo::sel_streaminfo(eListboxEntry *)
{
	eStreaminfo si;
	si.setLCD(LCDTitle, LCDElement);
	hide();	
	si.show();
	si.exec();
	si.hide();
	show();
}

void eZapInfo::sel_bnversion(eListboxEntry *)
{
	ShowBNVersion bn;
	bn.setLCD(LCDTitle, LCDElement);
	hide();	
	bn.show();
	bn.exec();
	bn.hide();
	show();
}

void eZapInfo::sel_about(eListboxEntry *)
{
	hide();
	eMessageBox msgbox("insert non-peinlichen ABOUT text here...","About enigma");
	msgbox.show();
	msgbox.exec();
	msgbox.hide();
	show();
}
