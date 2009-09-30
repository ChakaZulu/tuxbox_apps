/*
 * $Id: dmx.h,v 1.13 2009/09/30 17:12:39 seife Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
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

#ifndef __zapit_dmx_h__
#define __zapit_dmx_h__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_OST_DMX_H

#include <ost/dmx.h>
#define dmx_sct_filter_params dmxSctFilterParams
#define dmx_filter            dmxFilter
#define dmx_output_t          dmxOutput_t
#define dmx_pes_type_t        dmxPesType_t
struct dmx_pes_filter_params
{
        dvb_pid_t                    pid;
        dmxInput_t                   input;
        dmxOutput_t                  output;
        dmxPesType_t                 pes_type;
        uint32_t                     flags;
        uint32_t                     ip;
        uint16_t                     port;
};

#elif defined(HAVE_TRIPLEDRAGON)
#include "td-demux-compat.h"
#else /* HAVE_OST_DMX_H */

#include <linux/dvb/dmx.h>

#endif /* HAVE_OST_DMX_H */

class CDemux
{
	private:
		int fd;
/*		struct dmx_caps dmx_caps; */ /* unused */

	public:
		CDemux(void);
		~CDemux(void);
		int sectionFilter(const unsigned short pid, const unsigned char * const filter, const unsigned char * const mask);
		int pesFilter(const unsigned short pid, const dmx_output_t output, const dmx_pes_type_t pes_type);
		int start(void);
		int stop(void);
		int read(unsigned char * const buf, const size_t n);
};

#endif /* __zapit_dmx_h__ */
