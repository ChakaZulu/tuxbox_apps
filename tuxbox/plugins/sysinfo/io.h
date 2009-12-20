/*
 * $Id: io.h,v 1.1 2009/12/20 16:22:58 rhabarber1848 Exp $
 *
 * sysinfo - d-box2 linux project
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
*/

#ifndef __IO_H__

#define __IO_H__

extern __u16 rc_last_code;
extern int rc_last_key;

int RCKeyPressed(void);
int GetRCCode();
int GetRCCodeNW();
int RCTranslate(int code);
void ClearKeys(void);

#endif
