/*
 * $Id: tuxwetter.h,v 1.2 2009/12/27 12:08:02 rhabarber1848 Exp $
 *
 * tuxwetter - d-box2 linux project
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

#ifndef __TUXWETTER_H__

#define __TUXWETTER_H__

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H

/* tested with freetype 2.3.9, and 2.1.4 */
#if FREETYPE_MAJOR >= 2 && FREETYPE_MINOR >= 3
#define FT_NEW_CACHE_API
#endif

#define MISS_FILE	"/var/tuxbox/config/tuxwetter/missing_translations.txt"

//rc codes

#if HAVE_DVB_API_VERSION == 1

#define	RC1_0		0x5C00
#define	RC1_1		0x5C01
#define	RC1_2		0x5C02
#define	RC1_3		0x5C03
#define	RC1_4		0x5C04
#define	RC1_5		0x5C05
#define	RC1_6		0x5C06
#define	RC1_7		0x5C07
#define	RC1_8		0x5C08
#define	RC1_9		0x5C09
#define	RC1_STANDBY	0x5C0C
#define	RC1_UP		0x5C0E
#define	RC1_DOWN	0x5C0F
#define	RC1_PLUS	0x5C16
#define	RC1_MINUS	0x5C17
#define	RC1_HOME	0x5C20
#define	RC1_DBOX	0x5C27
#define	RC1_MUTE	0x5C28
#define	RC1_RED		0x5C2D
#define	RC1_RIGHT	0x5C2E
#define	RC1_LEFT	0x5C2F
#define	RC1_OK		0x5C30
#define	RC1_BLUE	0x5C3B
#define	RC1_YELLOW	0x5C52
#define	RC1_GREEN	0x5C55
#define	RC1_HELP	0x5C82

#endif

#ifdef HAVE_DREAMBOX_HARDWARE
#define KEY_0		0x5C00
#define KEY_1		0x5C01
#define KEY_2		0x5C02
#define KEY_3		0x5C03
#define KEY_4		0x5C04
#define KEY_5		0x5C05
#define KEY_6		0x5C06
#define KEY_7		0x5C07
#define KEY_8		0x5C08
#define KEY_9		0x5C09
#define KEY_POWER	0x5C0C
#define KEY_UP		0x5C0E
#define KEY_DOWN	0x5C0F
#define KEY_VOLUMEUP	0x5C16
#define KEY_VOLUMEDOWN	0x5C17
#define KEY_HOME	0x5C20
#define KEY_SETUP	0x5C27
#define KEY_MUTE	0x5C28
#define KEY_RED		0x5C2D
#define KEY_RIGHT	0x5C2E
#define KEY_LEFT	0x5C2F
#define KEY_OK		0x5C30
#define KEY_BLUE	0x5C3B
#define KEY_YELLOW	0x5C52
#define KEY_GREEN	0x5C55
#define KEY_HELP	0x5C82
#endif

#define	RC_0		0x00
#define	RC_1		0x01
#define	RC_2		0x02
#define	RC_3		0x03
#define	RC_4		0x04
#define	RC_5		0x05
#define	RC_6		0x06
#define	RC_7		0x07
#define	RC_8		0x08
#define	RC_9		0x09
#define	RC_RIGHT	0x0A
#define	RC_LEFT		0x0B
#define	RC_UP		0x0C
#define	RC_DOWN		0x0D
#define	RC_OK		0x0E
#define	RC_MUTE		0x0F
#define	RC_STANDBY	0x10
#define	RC_GREEN	0x11
#define	RC_YELLOW	0x12
#define	RC_RED		0x13
#define	RC_BLUE		0x14
#define	RC_PLUS		0x15
#define	RC_MINUS	0x16
#define	RC_HELP		0x17
#define	RC_DBOX		0x18
#define	RC_HOME		0x1F

//freetype stuff

extern unsigned char FONT[64];

enum {LEFT, CENTER, RIGHT};
enum {SMALL, MED, BIG};
//enum {FT_PIC=1, FT_TEXT, FT_HTML, FT_TXHTM};

FT_Error 		error;
FT_Library		library;
FTC_Manager		manager;
FTC_SBitCache		cache;
FTC_SBit		sbit;
#if FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 0
FTC_Image_Desc		desc;
#else
FTC_ImageTypeRec	desc;
#endif
FT_Face			face;
FT_UInt			prev_glyphindex;
FT_Bool			use_kerning;

//devs

int fb, rc, lcd;

//framebuffer stuff

//enum {DUMMY=127, CMCST, CMCS, CMCT, CMC, CMCIT, CMCI, CMHT, CMH, WHITE, BLUE0, TRANSP, BLUE1, CMS, GREEN, YELLOW, RED};
enum {EMPTY=127, CMCST, CMCS, CMCT, CMC, CMCIT, CMCI, CMHT, CMH, WHITE, BLUE0, GTRANSP, CMS, ORANGE, GREEN, YELLOW, RED};
#define TRANSP 0

extern unsigned char *lfb, *lbb;
extern unsigned char *proxyadress, *proxyuserpwd;
extern int instance;
int get_instance(void);
void put_instance(int pval);

struct fb_fix_screeninfo fix_screeninfo;
struct fb_var_screeninfo var_screeninfo;

int startx, starty, sx, ex, sy, ey;
char online;

#if HAVE_DVB_API_VERSION == 3

struct input_event ev;

#endif

unsigned short rccode;

#endif

#define BUFSIZE 4096
#define FB_DEVICE	"/dev/fb/0"

#if HAVE_DVB_API_VERSION < 3
int key_count;
unsigned short lastkey;
#define RC_DEVICE	"/dev/dbox/rc0"
#else
#define RC_DEVICE	"/dev/input/event0"
#endif

