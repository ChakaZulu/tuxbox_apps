/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/include/zapit/settings.h,v 1.24 2009/09/30 17:12:39 seife Exp $
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
#include                	"config.h"
#else
#define CONFIGDIR       	"/var/tuxbox/config"
#endif

#define ZAPITCONFIGDIR  	CONFIGDIR "/zapit"

#define CONFIGFILE		ZAPITCONFIGDIR "/zapit.conf"
#define MOTORCONFIGFILE		ZAPITCONFIGDIR "/motor.conf"
#define SERVICES_XML		ZAPITCONFIGDIR "/services.xml"
#define MYSERVICES_XML		ZAPITCONFIGDIR "/myservices.xml"
#define STATICPIDS_XML		ZAPITCONFIGDIR "/staticpids.xml"
#define SERVICES_TMP    	"/tmp/services.tmp"
#define BOUQUETS_XML    	ZAPITCONFIGDIR "/bouquets.xml"
#define UBOUQUETS_XML		ZAPITCONFIGDIR "/ubouquets.xml"
#define BOUQUETS_TMP    	"/tmp/bouquets.tmp"
#define CURRENTSERVICES_XML	"/tmp/currentservices.xml"
#define CURRENTSERVICES_TMP	"/tmp/currentservices.tmp"
#define CURRENTBOUQUETS_XML	"/tmp/currentbouquets.xml"
#define CURRENTBOUQUETS_TMP	"/tmp/currentbouquets.tmp"

#define CABLES_XML      	"cables.xml"
#define SATELLITES_XML  	"satellites.xml"
#define TERRESTRIAL_XML 	"terrestrial.xml"

#ifdef HAVE_TRIPLEDRAGON
#include <tddevices.h>
#define AUDIO_DEVICE	"/dev/" DEVICE_NAME_AUDIO
#define DEMUX_DEVICE	"/dev/" DEVICE_NAME_DEMUX "0"
#define FRONTEND_DEVICE	"/dev/" DEVICE_NAME_TUNER "0"
#define VIDEO_DEVICE	"/dev/" DEVICE_NAME_VIDEO
#define AVS_DEVICE	"/dev/" DEVICE_NAME_AVS
#else
#if HAVE_DVB_API_VERSION < 3
#define AUDIO_DEVICE    	"/dev/dvb/card0/audio0"
#define DEMUX_DEVICE    	"/dev/dvb/card0/demux0"
#define FRONTEND_DEVICE 	"/dev/dvb/card0/frontend0"
#define VIDEO_DEVICE    	"/dev/dvb/card0/video0"
#define SEC_DEVICE		"/dev/dvb/card0/sec0"
#else
#define AUDIO_DEVICE    	"/dev/dvb/adapter0/audio0"
#define DEMUX_DEVICE    	"/dev/dvb/adapter0/demux0"
#define FRONTEND_DEVICE 	"/dev/dvb/adapter0/frontend0"
#define VIDEO_DEVICE    	"/dev/dvb/adapter0/video0"
#endif
#define AVS_DEVICE		"/dev/dbox/avs0"
#define SAA7126_DEVICE		"/dev/dbox/saa0"
#endif

#define CAMD_UDS_NAME  		"/tmp/camd.socket"


#endif /* __zapit__settings_h__ */
