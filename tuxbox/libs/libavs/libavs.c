/*
 *   libavs.c - audio/video switch library (dbox-II-project)
 *
 *   Homepage: http://dbox2.elxsi.de
 *
 *   Copyright (C) 2002 Gillem gillem@berlios.de
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 *   $Log: libavs.c,v $
 *   Revision 1.1  2002/03/04 16:10:11  gillem
 *   - initial release
 *
 *
 *
 *   $Revision: 1.1 $
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "dbox/avs_core.h"

#include "libavs.h"

int dbg;
int avs_fd=-1;
int saa_fd=-1;
int avs_type;

void sys_error( char * text )
{
    if(dbg)
	perror(text);
}

int avsInit( int debug )
{
    int ret = -1;
    
    dbg = debug;
    
    if ( (saa_fd = open( SAA_DEVICE, O_RDWR )) == -1 )
    {
	sys_error("saa open: ");
    }
    else if ( (avs_fd = open( AVS_DEVICE, O_RDWR )) == -1 )
    {
	sys_error("avs open: ");
    }
    else
    {
	if ( ioctl( avs_fd, AVSIOGTYPE, &avs_type ) != 0 )
	{
	    sys_error("ioctl: ");
	}
	else
	{
	    ret = 0;
	}
    }
    
    return ret;
}

void avsDeInit()
{
    if( avs_fd != -1 )
    {
	close(avs_fd);
    }
    
    if( saa_fd != -1 )
    {
	close(saa_fd);
    }
}

int avsSetVolume( int vol )
{
    int ret = -1;
    
    if ( (vol >= 0) && (vol < 63) )
    {
	if ( ioctl( avs_fd, AVSIOSVOL, &vol ) != 0 )
	{
	    sys_error("ioctl: ");
	}
	else
	{
	    ret = 0;
	}
    }
    
    return ret;
}

int avsGetVolume( void )
{
    int ret = -1;
    int vol;
    
    if ( ioctl( avs_fd, AVSIOGVOL, &vol ) != 0 )
    {
	sys_error("ioctl: ");
    }
    else
    {
	ret = vol;
    }
    
    return ret;
}

int avsSetMute( int mute )
{
    int ret = -1;
    
    if ( ioctl( avs_fd, AVSIOSMUTE, &mute ) != 0 )
    {
	sys_error("ioctl: ");
    }
    else
    {
	ret = 0;
    }
    
    return ret;
}

/*
 * the very nice routing, please bug report and NO dau report ...
 *
 * 1. port (vcr,tv,cinch)
 * 2. source (mute,encoder,vcr)
 * 3. mode (only encoder)
 * 4. set avs/saa(mode) specific values and route
 *
 */
int avsSetRoute( ePort port, eSource source, eMode mode )
{
	int ret = -1;
	int videoport,audioport;
	int videosource,audiosource;
	int video;
	int audio;
	int m;
    
	switch(port)
	{
		case epVCR:
		{
			switch(source)
			{
				case esVCR:
				{
					video = 1;
					audio = 1;

					switch(avs_type)
					{
						case CXA2092:
						case CXA2126:
						{
							videoport = AVSIOSVSW2;
							videosource = 2;
							audioport = AVSIOSASW2;
							audiosource = 0;
							ret = 0;
							break;
						}
					}

					break;
				}

				case esMUTE:
				{
					video = 1;
					audio = 0;

					switch(avs_type)
					{
						case CXA2092:
						case CXA2126:
						{
							videoport = AVSIOSVSW2;
							videosource = 7;
							ret = 0;
							break;
						}
					}

					break;
				}

				case esDIGITALENCODER:
				{
					/*    C   | CVBS/Y
					 *
					 * 0: R/C | CVBS/Y
					 * 1: C   | CVBS/Y (spare inputs)
					 * 4: R/C | G/CVBS/Y
					 *
					 */
					break;
				}

				default:
				{
					break;
				}
			}
		}

		case epTV:
		{
			switch(source)
			{
				case esVCR:
				{
					video = 1;
					audio = 1;

					switch(avs_type)
					{
						case CXA2092:
						case CXA2126:
						{
							videoport = AVSIOSVSW1;
							videosource = 2;
							audioport = AVSIOSASW1;
							audiosource = 0;
							ret = 0;
							break;
						}
					}

					break;
				}

				case esMUTE:
				{
					video = 1;
					audio = 0;

					switch(avs_type)
					{
						case CXA2092:
						case CXA2126:
						{
							videoport = AVSIOSVSW1;
							videosource = 7;
							ret = 0;
							break;
						}
					}

					break;
				}

				case esDIGITALENCODER:
				{
					/*
					 *
					 * 0:
					 * 1:
					 * 4:
					 * 5:
					 * 6:
					 *
					 */
					break;
				}

				default:
				{
					break;
				}
			}
		}

		case epCINCH:
		{
			break;
		}

		default:
		{
			break;
		}
	}

	if ( ret == 0 )
	{
		if ( video == 1 )
		{
			if ( ioctl( avs_fd, videoport, &videosource ) != 0 )
			{
				sys_error("ioctl: ");
				ret = -1;
			}
		}
    
		if ( audio == 1 )
		{
			if ( ioctl( avs_fd, audioport, &audiosource ) != 0 )
			{
				sys_error("ioctl: ");
				ret = -1;
			}
		}
	}

	return ret;
}
