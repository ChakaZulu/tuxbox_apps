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
 * $Id: enigma_info.cpp,v 1.5 2002/07/02 00:19:33 Ghostrider Exp $
 */

#include "enigma_info.h"

#include <apps/enigma/streaminfo.h>
#include <apps/enigma/showbnversion.h>

#include <core/dvb/edvb.h>
#include <core/gui/ewindow.h>
#include <core/gui/eskin.h>
#include <core/gui/elabel.h>
#include <core/gui/emessage.h>
#include <core/base/i18n.h>

eZapInfo::eZapInfo()
	:eListBoxWindow<eListBoxEntryMenu>(_("Infos"), 8, 220)
{
	move(ePoint(150, 136));
	CONNECT((new eListBoxEntryMenu(&list, _("[back]")))->selected, eZapInfo::sel_close);
	CONNECT((new eListBoxEntryMenu(&list, _("Streaminfo")))->selected, eZapInfo::sel_streaminfo);
	CONNECT((new eListBoxEntryMenu(&list, _("Show BN version")))->selected, eZapInfo::sel_bnversion);
	CONNECT((new eListBoxEntryMenu(&list, _("About...")))->selected, eZapInfo::sel_about);
	
}

eZapInfo::~eZapInfo()
{
}

void eZapInfo::sel_close()
{
	close(0);
}

void eZapInfo::sel_streaminfo()
{
	hide();	
	eStreaminfo si;
	si.setLCD(LCDTitle, LCDElement);
	si.show();
	si.exec();
	si.hide();
	show();
}

void eZapInfo::sel_bnversion()
{
	hide();	
	ShowBNVersion bn;
	bn.setLCD(LCDTitle, LCDElement);
	bn.show();
	bn.exec();
	bn.hide();
	show();
}

void eZapInfo::sel_about()
{
	hide();
	eMessageBox msgbox("insert non-peinlichen ABOUT text here...","About enigma");
	msgbox.show();
	msgbox.exec();
	msgbox.hide();
	show();
}
