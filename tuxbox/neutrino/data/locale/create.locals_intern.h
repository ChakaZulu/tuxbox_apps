#!/bin/bash
# usage: cut -d' ' -f1 deutsch.locale | sort | uniq | ./create.locals_intern.h
cat > locals_intern.h <<EOF
#ifndef __locals_intern__
#define __locals_intern__

/*
 * \$Id\$
 *
 * (C) 2004 by thegoodguy <thegoodguy@berlios.de>
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

const char * locale_real_names[] =
{
	"INTERNAL ERROR - PLEASE REPORT",
EOF
while read id; do
	if [[ \
	      "$id" != "cam.wrong"                    && \
	      "$id" != "flashupdate.action"           && \
	      "$id" != "flashupdate.actionwriteflash" && \
	      "$id" != "flashupdate.reboot"           && \
	      "$id" != "infoviewer.cantdecode"        && \
	      "$id" != "mainsettings.plugins"         && \
	      "$id" != "mainsettings.scan"            && \
	      "$id" != "miscsettings.boxtype"         && \
	      "$id" != "parentallock.onstart"         && \
	      "$id" != "recordingmenu.vcr_devicename" && \
	      "$id" != "satsetup.extended_lnb"        && \
	      "$id" != "scants.services"              && \
	      "$id" != "streamfeatures.info"          && \
	      "$id" != "streaminfo.ecm_invalid"       && \
	      "$id" != "streaminfo.not_crypted"       && \
	      "$id" != "timerlist.empty"                 \
	      ]] ;
	then
		echo $'\t'"\"$id\"," >> locals_intern.h;
	fi
done
cat >> locals_intern.h <<EOF
};
#endif
EOF
# // infoviewer.cantdecode
# miscsettings.startbhdriver: only for HAVE_DVB_API_VERSION == 1
# // parentallock.onstart
# // streamfeatures.info
# streaminfo.signal: // streaminfo2.cpp
# // timerlist.empty