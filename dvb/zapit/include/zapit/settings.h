/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/include/zapit/settings.h,v 1.14 2005/06/19 14:28:22 barf Exp $
 *
 * zapit's settings - d-box2 linux project
 *
 * (C) 2002 by thegoodguy <thegoodguy@berlios.de>
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

#ifndef __zapit__settings_h__
#define __zapit__settings_h__


#ifdef HAVE_CONFIG_H
#include                "config.h"
#else
#define CONFIGDIR       "/var/tuxbox/config"
#endif

#define ZAPITCONFIGDIR  CONFIGDIR "/zapit"

#define CONFIGFILE      ZAPITCONFIGDIR "/zapit.conf"
#define MOTORCONFIGFILE ZAPITCONFIGDIR "/motor.conf"
#define SERVICES_XML    ZAPITCONFIGDIR "/services.xml"
#define MYSERVICES_XML  ZAPITCONFIGDIR "/myservices.xml"
#define ANTISERVICES_XML ZAPITCONFIGDIR "/antiservices.xml"
#define SERVICES_TMP    "/tmp/services.tmp"
#define BOUQUETS_XML    ZAPITCONFIGDIR "/bouquets.xml"
#define BOUQUETS_TMP    "/tmp/bouquets.tmp"

#define CABLES_XML      DATADIR "/cables.xml"
#define SATELLITES_XML  DATADIR "/satellites.xml"
#define TERRESTRIAL_XML DATADIR "/terrestrial.xml"

#define AUDIO_DEVICE    "/dev/dvb/adapter0/audio0"
#define DEMUX_DEVICE    "/dev/dvb/adapter0/demux0"
#define FRONTEND_DEVICE "/dev/dvb/adapter0/frontend0"
#define VIDEO_DEVICE    "/dev/dvb/adapter0/video0"

#define CAMD_UDS_NAME  "/tmp/camd.socket"


#endif /* __zapit__settings_h__ */
