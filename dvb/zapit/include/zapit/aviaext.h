/*
 * $Id: aviaext.h,v 1.1 2005/01/18 10:33:34 diemade Exp $
 *
 * (C) 2005 by Axel Buehning 'DieMade'
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

#ifndef __zapit_aviaext_h__
#define __zapit_aviaext_h__

class CAViAext
{
	private:
		/* aviaext device */
		int fd;

	public:
		/* construct & destruct */
		CAViAext(void);
		~CAViAext(void);

		void iecOn(void);
		void iecOff(void);
		int iecState(void);
		void playbackSPTS(void);
		void playbackPES(void);
		int playbackState(void);
};

#endif /* __zapit_aviaext_h__ */

