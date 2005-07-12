/******************************************************************************
 *                      <<< TuxTxt - Teletext Plugin >>>                      *
 *                                                                            *
 *             (c) Thomas "LazyT" Loewe 2002-2003 (LazyT@gmx.net)             *
 *                                                                            *
 *    continued 2004-2005 by Roland Meier <RolandMeier@Siemens.com>           *
 *                       and DBLuelle <dbluelle@blau-weissoedingen.de>        *
 *                                                                            *
 ******************************************************************************/

#define TUXTXT_CFG_STANDALONE 0  // 1:plugin only 0:use library

#include <config.h>

#ifndef DREAMBOX
#include <tuxbox.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>

#include <linux/fb.h>

#if HAVE_DVB_API_VERSION < 3
#include <dbox/avia_gt_pig.h>
#else
#include <linux/input.h>
#include <linux/videodev.h>
#endif

#include <sys/ioctl.h>
#include <sys/mman.h>

#include <dbox/avs_core.h>
#include <dbox/saa7126_core.h>
#include <dbox/fp.h>
#include <plugin.h>
#include <dbox/lcd-ks0713.h>


#include "tuxtxt_def.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H

/* devices */
#define AVS "/dev/dbox/avs0"
#define SAA "/dev/dbox/saa0"
#if HAVE_DVB_API_VERSION < 3
#define PIG "/dev/dbox/pig0"
#else
#define PIG "/dev/v4l/video0"
#endif


#if TUXTXT_CFG_STANDALONE
#include "tuxtxt_common.h"
#else
/* variables and functions from libtuxtxt */
extern tuxtxt_cache_struct tuxtxt_cache;
extern int tuxtxt_init();
extern void tuxtxt_close();
extern void tuxtxt_start(int tpid);  // Start caching
extern int  tuxtxt_stop(); // Stop caching
extern void tuxtxt_next_dec(int *i); /* skip to next decimal */
extern void tuxtxt_prev_dec(int *i); /* counting down */
extern int tuxtxt_is_dec(int i);
extern int tuxtxt_next_hex(int i);
extern void tuxtxt_decode_btt();
extern void tuxtxt_decode_adip(); /* additional information table */
#endif


#define TUXTXTCONF CONFIGDIR "/tuxtxt/tuxtxt2.conf"

/* fonts */
#define TUXTXTTTF FONTDIR "/tuxtxt.ttf"
#define TUXTXTOTB FONTDIR "/tuxtxt.otb"
/* alternative fontdir */
#define TUXTXTTTFVAR "/var/tuxtxt/tuxtxt.ttf"
#define TUXTXTOTBVAR "/var/tuxtxt/tuxtxt.otb"

int TTFWidthFactor16, TTFHeightFactor16, TTFShiftX, TTFShiftY; /* parameters for adapting to various TTF fonts */
int fontheight, fontwidth, fontwidth_normal, fontwidth_small, fontwidth_topmenumain, fontwidth_topmenusmall, ascender;
int ymosaic[4];
int displaywidth;
#define fontwidth_small_lcd 8

#define TV43STARTX (ex - 146) //(StartX + 2 + (40-nofirst)*fontwidth_topmenumain + (40*fontwidth_topmenumain/abx))
#define TV169FULLSTARTX (sx+ 8*40) //(sx +(ex +1 - sx)/2)
#define TVENDX ex
#define TVENDY (StartY + 25*fontheight)
#define TV43WIDTH 144 /* 120 */
#define TV43HEIGHT 116 /* 96 */
#define TV43STARTY (TVENDY - TV43HEIGHT)
#define TV169FULLSTARTY sy
#define TV169FULLWIDTH  (ex - sx)/2
#define TV169FULLHEIGHT (ey - sy)

#define TOPMENUSTARTX TV43STARTX+2
#define TOPMENUENDX TVENDX
#define TOPMENUSTARTY StartY
#define TOPMENUENDY TV43STARTY

#define TOPMENULINEWIDTH ((TOPMENUENDX-TOPMENU43STARTX+fontwidth_topmenusmall-1)/fontwidth_topmenusmall)
#define TOPMENUINDENTBLK 0
#define TOPMENUINDENTGRP 1
#define TOPMENUINDENTDEF 2
#define TOPMENUSPC 0
#define TOPMENUCHARS (TOPMENUINDENTDEF+12+TOPMENUSPC+4)

#define FLOFSIZE 4

/* spacing attributes */
#define alpha_black         0x00
#define alpha_red           0x01
#define alpha_green         0x02
#define alpha_yellow        0x03
#define alpha_blue          0x04
#define alpha_magenta       0x05
#define alpha_cyan          0x06
#define alpha_white         0x07
#define flash               0x08
#define steady              0x09
#define end_box             0x0A
#define start_box           0x0B
#define normal_size         0x0C
#define double_height       0x0D
#define double_width        0x0E
#define double_size         0x0F
#define mosaic_black        0x10
#define mosaic_red          0x11
#define mosaic_green        0x12
#define mosaic_yellow       0x13
#define mosaic_blue         0x14
#define mosaic_magenta      0x15
#define mosaic_cyan         0x16
#define mosaic_white        0x17
#define conceal             0x18
#define contiguous_mosaic   0x19
#define separated_mosaic    0x1A
#define esc                 0x1B
#define black_background    0x1C
#define new_background      0x1D
#define hold_mosaic         0x1E
#define release_mosaic      0x1F

/* rc codes */
#if HAVE_DVB_API_VERSION < 3
#define KEY_0       0x5C00
#define KEY_1       0x5C01
#define KEY_2       0x5C02
#define KEY_3       0x5C03
#define KEY_4       0x5C04
#define KEY_5       0x5C05
#define KEY_6       0x5C06
#define KEY_7       0x5C07
#define KEY_8       0x5C08
#define KEY_9       0x5C09
#define KEY_POWER   0x5C0C
#define KEY_UP      0x5C0E
#define KEY_DOWN    0x5C0F
#define KEY_VOLUMEUP    0x5C16
#define KEY_VOLUMEDOWN  0x5C17
#define KEY_HOME    0x5C20
#define KEY_SETUP   0x5C27
#define KEY_MUTE    0x5C28
#define KEY_RED     0x5C2D
#define KEY_RIGHT   0x5C2E
#define KEY_LEFT    0x5C2F
#define KEY_OK      0x5C30
#define KEY_BLUE    0x5C3B
#define KEY_YELLOW  0x5C52
#define KEY_GREEN   0x5C55
#define KEY_HELP    0x5C82
#endif
#define RC_0        0x00
#define RC_1        0x01
#define RC_2        0x02
#define RC_3        0x03
#define RC_4        0x04
#define RC_5        0x05
#define RC_6        0x06
#define RC_7        0x07
#define RC_8        0x08
#define RC_9        0x09
#define RC_RIGHT    0x0A
#define RC_LEFT     0x0B
#define RC_UP       0x0C
#define RC_DOWN     0x0D
#define RC_OK       0x0E
#define RC_MUTE     0x0F
#define RC_STANDBY  0x10
#define RC_GREEN    0x11
#define RC_YELLOW   0x12
#define RC_RED      0x13
#define RC_BLUE     0x14
#define RC_PLUS     0x15
#define RC_MINUS    0x16
#define RC_HELP     0x17
#define RC_DBOX     0x18
#define RC_HOME     0x1F




typedef enum /* object type */
{
	OBJ_PASSIVE,
	OBJ_ACTIVE,
	OBJ_ADAPTIVE
} tObjType;

const unsigned char *ObjectSource[] =
{
	"(illegal)",
	"Local",
	"POP",
	"GPOP"
};
const unsigned char *ObjectType[] =
{
	"Passive",
	"Active",
	"Adaptive",
	"Passive"
};


/* messages */
#define ShowInfoBar     0
//#define PageNotFound    1
#define ShowServiceName 2
#define NoServicesFound 3

/* framebuffer stuff */
unsigned char *lfb = 0;
struct fb_var_screeninfo var_screeninfo;
struct fb_fix_screeninfo fix_screeninfo;

/* freetype stuff */
FT_Library      library;
FTC_Manager     manager;
FTC_SBitCache   cache;
FTC_SBit        sbit;
#if HAVE_DVB_API_VERSION < 3
#define FONTTYPE FTC_Image_Desc
#else
#define FONTTYPE FTC_ImageTypeRec
#endif

FT_Face			face;
FONTTYPE typettf;

const unsigned short int nationaltable23[13][2] =
{
	{ '#', 367 }, /* 0  CS/SK   */
	{ '£', '$' }, /* 1    EN    */
	{ '#', 'õ' }, /* 2    ET    */
	{ 'é', 'ï' }, /* 3    FR    */
	{ '#', '$' }, /* 4    DE    */
	{ '£', '$' }, /* 5    IT    */
	{ '#', '$' }, /* 6  LV/LT   */
	{ '#', 329 }, /* 7    PL    */
	{ 'ç', '$' }, /* 8  PT/ES   */
	{ '#', '¤' }, /* 9    RO    */
	{ '#', 'Ë' }, /* A SR/HR/SL */
	{ '#', '¤' }, /* B SV/FI/HU */
	{ '£', 287 }, /* C    TR   ? */
};
const unsigned short int nationaltable40[13] =
{
	269, /* 0  CS/SK   */
	'@', /* 1    EN    */
	352, /* 2    ET    */
	'à', /* 3    FR    */
	'§', /* 4    DE    */
	'é', /* 5    IT    */
	352, /* 6  LV/LT   */
	261, /* 7    PL    */
	'¡', /* 8  PT/ES   */
	354, /* 9    RO    */
	268, /* A SR/HR/SL */
	'É', /* B SV/FI/HU */
	304, /* C    TR    */
};
const unsigned short int nationaltable5b[13][6] =
{
	{ 357, 382, 'ý', 'í', 345, 'é' }, /* 0  CS/SK   */
	{8592, '½',8594,8593, '#', 822 }, /* 1    EN    */
	{ 'Ä', 'Ö', 381, 'Ü', 'Õ', 353 }, /* 2    ET    */
	{ 'ë', 'ê', 'ù', 'î', '#', 'è' }, /* 3    FR    */
	{ 'Ä', 'Ö', 'Ü', '^', '_', '°' }, /* 4    DE    */
	{ '°', 'ç',8594,8593, '#', 'ù' }, /* 5    IT    */
	{ 'é', 553, 381, 269, 363, 353 }, /* 6  LV/LT   */
	{ 437, 346, 321, 263, 'ó', 281 }, /* 7    PL    */
	{ 'á', 'é', 'í', 'ó', 'ú', '¿' }, /* 8  PT/ES   */
	{ 'Â', 350, 461, 'Î', 305, 355 }, /* 9    RO    */
	{ 262, 381, 272, 352, 'ë', 269 }, /* A SR/HR/SL */
	{ 'Ä', 'Ö', 'Å', 'Ü', '_', 'é' }, /* B SV/FI/HU */
	{ 350, 'Ö', 'Ç', 'Ü', 486, 305 }, /* C    TR    */
};
const unsigned short int nationaltable7b[13][4] =
{
	{ 'á', 283, 'ú', 353 }, /* 0  CS/SK   */
	{ '¼',8214, '¾', '÷' }, /* 1    EN    */
	{ 'ä', 'ö', 382, 'ü' }, /* 2    ET    */
	{ 'â', 'ô', 'û', 'ç' }, /* 3    FR    */
	{ 'ä', 'ö', 'ü', 'ß' }, /* 4    DE    */
	{ 'à', 'ò', 'è', 'ì' }, /* 5    IT    */
	{ 261, 371, 382, 303 }, /* 6  LV/LT   */
	{ 380, 347, 322, 378 }, /* 7    PL    */
	{ 'ü', 'ñ', 'è', 'à' }, /* 8  PT/ES   */
	{ 'â', 351, 462, 'î' }, /* 9    RO    */
	{ 263, 382, 273, 353 }, /* A SR/HR/SL */
	{ 'ä', 'ö', 'å', 'ü' }, /* B SV/FI/HU */
	{ 351, 'ö', 231, 'ü' }, /* C    TR    */
};
const unsigned short int arrowtable[] =
{
	8592, 8594, 8593, 8595, 'O', 'K', 8592, 8592
};

/* national subsets */
const char countrystring[] =
"  CS/SK   (#$@[\\]^_`{|}~) "   /*  0 czech, slovak */
"    EN    (#$@[\\]^_`{|}~) "   /*  1 english */
"    ET    (#$@[\\]^_`{|}~) "   /*  2 estonian */
"    FR    (#$@[\\]^_`{|}~) "   /*  3 french */
"    DE    (#$@[\\]^_`{|}~) "   /*  4 german */
"    IT    (#$@[\\]^_`{|}~) "   /*  5 italian */
"  LV/LT   (#$@[\\]^_`{|}~) "   /*  6 latvian, lithuanian */
"    PL    (#$@[\\]^_`{|}~) "   /*  7 polish */
"  PT/ES   (#$@[\\]^_`{|}~) "   /*  8 portuguese, spanish */
"    RO    (#$@[\\]^_`{|}~) "   /*  9 romanian */
" SR/HR/SL (#$@[\\]^_`{|}~) "   /* 10 serbian, croatian, slovenian */
" SV/FI/HU (#$@[\\]^_`{|}~) "   /* 11 swedish, finnish, hungarian */
"    TR    (#$@[\\]^_`{|}~) "   /* 12 turkish */
" RU/BUL/SER/CRO/UKR (cyr)  "   /* 13 cyrillic */
"    EK                     "   /* 14 greek */
;
#define COUNTRYSTRING_WIDTH 26
#define MAX_NATIONAL_SUBSET (sizeof(countrystring) / COUNTRYSTRING_WIDTH - 1)

enum
{
	NAT_DEFAULT = 4,
	NAT_DE = 4,
	NAT_MAX_FROM_HEADER = 12,
	NAT_RU = 13,
	NAT_GR = 14
};

const unsigned char countryconversiontable[] = { 1, 4, 11, 5, 3, 8, 0, 9 };


/* some data */
char versioninfo[16];
int hotlist[10];
int maxhotlist;

int pig, avs, saa, rc, fb, lcd;
int sx, ex, sy, ey;
int PosX, PosY, StartX, StartY;
int lastpage;
int inputcounter;
int zoommode, screenmode, transpmode, hintmode, boxed, nofirst, savedscreenmode, showflof, show39, showl25;
char dumpl25;
int catch_row, catch_col, catched_page, pagecatching;
int prev_100, prev_10, next_10, next_100;
int fnc_old, saa_old, screen_mode1, screen_mode2, color_mode, national_subset, auto_national, swapupdown, showhex, menulanguage;
int pids_found, current_service, getpidsdone;
int SDT_ready;
int pc_old_row, pc_old_col;     /* for page catching */
int temp_page;	/* for page input */
char saveconfig, hotlistchanged;
signed char clearbbcolor = -1;
int usettf;
short pop, gpop, drcs, gdrcs;
unsigned char tAPx, tAPy;	/* temporary offset to Active Position for objects */
unsigned char axdrcs[12+1+10+1];
#define aydrcs (axdrcs + 12+1)
unsigned char FullRowColor[25];
unsigned char FullScrColor;
tstPageinfo *pageinfo = 0;/* pointer to cached info of last decoded page */
const int fncmodes[] = {AVS_FNCOUT_EXT43, AVS_FNCOUT_EXT169};
const int saamodes[] = {SAA_WSS_43F, SAA_WSS_169F};

FILE *conf;


unsigned short RCCode;

struct _pid_table
{
	int  vtxt_pid;
	int  service_id;
	int  service_name_len;
	char service_name[24];
	int  national_subset;
}pid_table[128];

unsigned char restoreaudio = 0;
/* 0 Nokia, 1 Philips, 2 Sagem */
/* typ_vcr/dvb: 	v1 a1 v2 a2 v3 a3 (vcr_only: fblk) */
const int avstable_ioctl[7] =
{
	AVSIOSVSW1, AVSIOSASW1, AVSIOSVSW2, AVSIOSASW2, AVSIOSVSW3, AVSIOSASW3, AVSIOSFBLK
};
const int avstable_ioctl_get[7] =
{
	AVSIOGVSW1, AVSIOGASW1, AVSIOGVSW2, AVSIOGASW2, AVSIOGVSW3, AVSIOGASW3, AVSIOGFBLK
};
const unsigned char avstable_scart[3][7] =
{
	{ 3, 2, 1, 0, 1, 1, 2 },
	{ 3, 3, 2, 2, 3, 2, 2 },
	{ 2, 1, 0, 0, 0, 0, 0 },
};
unsigned char avstable_dvb[3][7] =
{
	{ 5, 1, 1, 0, 1, 1, 0 },
	{ 1, 1, 1, 1, 1, 1, 0 },
	{ 0, 0, 0, 0, 0, 0, 0 },
};

/* language dependent texts */
#define MAXMENULANGUAGE 5 /* 0 deutsch, 1 englisch, 2 französisch, 3 niederländisch, 4 griechisch, 5 italienisch */

#define Menu_StartX (StartX + fontwidth*9/2)
#define Menu_StartY (StartY + fontheight)
#define Menu_Height 24
#define Menu_Width 31

const char MenuLine[] =
{
	4,9,12,13,16,19,20,21
};

enum
{
	M_HOT=0,
	M_PID,
	M_SC1,
	M_SC2,
	M_COL,
	M_AUN,
	M_NAT,
	M_LNG,
	M_Number
};

#define M_Start M_HOT
#define M_MaxDirect M_AUN

const char hotlistpagecolumn[] =	/* last(!) column of page to show in each language */
{
	22, 26, 28, 27, 28, 27
};
const char hotlisttextcolumn[] =
{
	24, 14, 14, 15, 14, 15
};
const char hotlisttext[][2*5] =
{
	{ "dazu entf." },
	{ " add rem. " },
	{ "ajoutenlev" },
	{ "toev.verw." },
	{ "pqoshavaiq" },
	{ "agg. elim." },
};

const char configonoff[][2*3] =
{
	{ "ausein" },
	{ "offon " },
	{ "desact" },
	{ "uitaan" },
	{ "emeape" },
	{ "offon " }
};
const char menuatr[Menu_Height*Menu_Width] =
{
	"0000000000000000000000000000002"
	"0111111111111111111111111111102"
	"0000000000000000000000000000002"
	"3333333333333333333333333333332"
	"3144444444444444444444444444432"
	"3556655555555555555555555555532"
	"3555555555555555555555555555532"
	"3333333333333333333333333333332"
	"3144444444444444444444444444432"
	"3555555555555555555555555555532"
	"3333333333333333333333333333332"
	"3444444444444444444444444444432"
	"3155555555555555555555555555532"
	"3155555555555555555555555555532"
	"3333333333333333333333333333332"
	"3144444444444444444444444444432"
	"3555555555555555555555555555532"
	"3333333333333333333333333333332"
	"3144444444444444444444444444432"
	"3555555555555555555555555555532"
	"3555555555555555555555555555532"
	"3555555555555555555555555555532"
	"3333333333333333333333333333332"
	"2222222222222222222222222222222"
};
const char configmenu[][Menu_Height*Menu_Width] =
{
	{
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
		"àááááááááááááááááááááááááááááâè"
		"ã     Konfigurationsmenue    äé"
		"åææææææææææææææææææææææææææææçé"
		"ã                            äé"
		"ã1 Favoriten: Seite 111 dazu äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2     Teletext-Auswahl      äé"
		"ãí          suchen          îäé"
		"ã                            äé"
		"ã      Bildschirmformat      äé"
		"ã3  Standard-Modus 16:9      äé"
		"ã4  TextBild-Modus 16:9      äé"
		"ã                            äé"
		"ã5        Helligkeit         äé"
		"ãí                          îäé"
		"ã                            äé"
		"ã6  nationaler Zeichensatz   äé"
		"ãautomatische Erkennung      äé"
		"ãí   DE    (#$@[\\]^_`{|}~)  îäé"
		"ãí Sprache/Language deutsch îäé"
		"åææææææææææææææææææææææææææææçé"
		"ëìììììììììììììììììììììììììììììê"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã     Configuration menu     äé"
		"åææææææææææææææææææææææææææææçé"
		"ã                            äé"
		"ã1 Favorites:  add page 111  äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2     Teletext selection    äé"
		"ãí          search          îäé"
		"ã                            äé"
		"ã        Screen format       äé"
		"ã3 Standard mode 16:9        äé"
		"ã4 Text/TV mode  16:9        äé"
		"ã                            äé"
		"ã5        Brightness         äé"
		"ãí                          îäé"
		"ã                            äé"
		"ã6   national characterset   äé"
		"ã automatic recognition      äé"
		"ãí   DE    (#$@[\\]^_`{|}~)  îäé"
		"ãí Sprache/language english îäé"
		"åææææææææææææææææææææææææææææçé"
		"ëìììììììììììììììììììììììììììììê"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã    Menu de configuration   äé"
		"åææææææææææææææææææææææææææææçé"
		"ã                            äé"
		"ã1 Favorites: ajout. page 111äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2  Selection de teletext    äé"
		"ãí        recherche         îäé"
		"ã                            äé"
		"ã      Format de l'#cran     äé"
		"ã3 Mode standard 16:9        äé"
		"ã4 Texte/TV      16:9        äé"
		"ã                            äé"
		"ã5          Clarte           äé"
		"ãí                          îäé"
		"ã                            äé"
		"ã6     police nationale      äé"
		"ãreconn. automatique         äé"
		"ãí   DE    (#$@[\\]^_`{|}~)  îäé"
		"ãí Sprache/language francaisîäé"
		"åææææææææææææææææææææææææææææçé"
		"ëìììììììììììììììììììììììììììììê"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã      Configuratiemenu      äé"
		"åææææææææææææææææææææææææææææçé"
		"ã                            äé"
		"ã1 Favorieten: toev. pag 111 äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2     Teletekst-selectie    äé"
		"ãí          zoeken          îäé"
		"ã                            äé"
		"ã     Beeldschermformaat     äé"
		"ã3   Standaardmode 16:9      äé"
		"ã4   Tekst/TV mode 16:9      äé"
		"ã                            äé"
		"ã5        Helderheid         äé"
		"ãí                          îäé"
		"ã                            äé"
		"ã6    nationale tekenset     äé"
		"ãautomatische herkenning     äé"
		"ãí   DE    (#$@[\\]^_`{|}~)  îäé"
		"ãí Sprache/Language nederl. îäé"
		"åææææææææææææææææææææææææææææçé"
		"ëìììììììììììììììììììììììììììììê"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã      Lemou quhliseym       äé"
		"åææææææææææææææææææææææææææææçé"
		"ã                            äé"
		"ã1 Vaboqi:    pqosh. sek. 111äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2     Epikocg Teketent      äé"
		"ãí        amafgtgsg         îäé"
		"ã                            äé"
		"ã       Loqvg ohomgr         äé"
		"ã3 Tqopor pqotupor   16:9    äé"
		"ã4 Tqopor eij. jeil. 16:9    äé"
		"ã                            äé"
		"ã5      Vyteimotgta          äé"
		"ãí                          îäé"
		"ã                            äé"
		"ã6    Ehmijg tuposeiqa       äé"
		"ãautolatg amacmyqisg         äé"
		"ãí   DE    (#$@[\\]^_`{|}~)  îäé"
		"ãí Ckyssa/Language ekkgmija îäé"
		"åææææææææææææææææææææææææææææçé"
		"ëìììììììììììììììììììììììììììììê"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè"
		"ã   Menu di configurazione   äé"
		"åææææææææææææææææææææææææææææçé"
		"ã                            äé"
		"ã1  Preferiti:  agg. pag.111 äé"
		"ãíîñò                        äé"
		"ã+-?                         äé"
		"ã                            äé"
		"ã2   Selezione televideo     äé"
		"ãí         ricerca          îäé"
		"ã                            äé"
		"ã      Formato schermo       äé"
		"ã3  Modo standard 16:9       äé"
		"ã4  Text/Mod.TV 16:9         äé"
		"ã                            äé"
		"ã5        Luminosità         äé"
		"ãí                          îäé"
		"ã                            äé"
		"ã6   nazionalita'caratteri   äé"
		"ã riconoscimento automatico  äé"
		"ãí   DE    (#$@[\\]^_`{|}~)  îäé"
		"ãí Lingua/Language Italiana îäé"
		"åææææææææææææææææææææææææææææçé"
		"ëìììììììììììììììììììììììììììììê"
	}
};

const char catchmenutext[][80] =
{
	{ "        íïðî w{hlen   ñò anzeigen       "
	  "0000000011110000000000110000000000000000" },
	{ "        íïðî select   ñò show           "
	  "0000000011110000000000110000000000000000" },
	{ "  íïðî selectionner   ñò montrer        "
	  "0011110000000000000000110000000000000000" },
	{ "        íïðî kiezen   ñò tonen          "
	  "0000000011110000000000110000000000000000" },
	{ "        íïðî epikocg  ñò pqobokg        "
	  "0000000011110000000000110000000000000000" },
	{ "        íïðîseleziona ñò mostra         "
	  "0000000011110000000000110000000000000000" }
};

const char message_3[][38] =
{
	{ "ã   suche nach Teletext-Anbietern   äé" },
	{ "ã  searching for teletext Services  äé" },
	{ "ã  recherche des services teletext  äé" },
	{ "ã zoeken naar teletekst aanbieders  äé" },
	{ "ã     amafgtgsg voqeym Teketent     äé" },
	{ "ã     attesa opzioni televideo      äé" }
};
const char message_3_blank[] = "ã                                   äé";
const char message_7[][38] =
{
	{ "ã kein Teletext auf dem Transponder äé" },
	{ "ã   no teletext on the transponder  äé" },
	{ "ã pas de teletext sur le transponderäé" },
	{ "ã geen teletekst op de transponder  äé" },
	{ "ã jalela Teketent ston amaletadotg  äé" },
	{ "ã nessun televideo sul trasponder   äé" }
};
const char message_8[][38] =
{
/*    00000000001111111111222222222233333333334 */
/*    01234567890123456789012345678901234567890 */
	{ "ã  warte auf Empfang von Seite 100  äé" },
	{ "ã waiting for reception of page 100 äé" },
	{ "ã attentre la réception de page 100 äé" },
	{ "ãwachten op ontvangst van pagina 100äé" },
	{ "ã     amalemy kgxg sekidar 100      äé" },
	{ "ã   attesa ricezione pagina 100     äé" }
};
const char message8pagecolumn[] = /* last(!) column of page to show in each language */
{
	33, 34, 34, 35, 29, 30
};
#if 0
const char message_9[][38] =
{
/*    0000000000111111111122222222223 */
/*    0123456789012345678901234567890 */
	{ "ã     Seite 100 existiert nicht!    äé" },
	{ "ã      Page 100 does not exist!     äé" },
	{ "ã      Page 100 n'existe pas!       äé" },
	{ "ã    Pagina 100 bestaat niet!       äé" },
	{ "ã    G sekida 100 dem upaqwei!      äé" },
	{ "ã      Pagina 100 non esiste!       äé" }
};
#define MESSAGE9PAGECOLUMN 14
#endif

enum /* options for charset */
{
	C_G0P = 0, /* primary G0 */
	C_G0S, /* secondary G0 */
	C_G1C, /* G1 contiguous */
	C_G1S, /* G1 separate */
	C_G2,
	C_G3,
	C_OFFSET_DRCS = 32
	/* 32..47: 32+subpage# GDRCS (offset/20 in page_char) */
	/* 48..63: 48+subpage#  DRCS (offset/20 in page_char) */
};

/* struct for page attributes */
typedef struct
{
	unsigned char fg      :6; /* foreground color */
	unsigned char bg      :6; /* background color */
	unsigned char charset :6; /* see enum above */
	unsigned char doubleh :1; /* double height */
	unsigned char doublew :1; /* double width */
	/* ignore at Black Background Color Substitution */
	/* black background set by New Background ($1d) instead of start-of-row default or Black Backgr. ($1c) */
	/* or black background set by level 2.5 extensions */
	unsigned char IgnoreAtBlackBgSubst:1;
	unsigned char concealed:1; /* concealed information */
	unsigned char inverted :1; /* colors inverted */
//	unsigned char flash   :5; /* flash mode */
} tstPageAttr;

enum /* indices in atrtable */
{
	ATR_WB, /* white on black */
	ATR_PassiveDefault, /* Default for passive objects: white on black, ignore at Black Background Color Substitution */
	ATR_L250, /* line25 */
	ATR_L251, /* line25 */
	ATR_L252, /* line25 */
	ATR_L253, /* line25 */
	ATR_TOPMENU0, /* topmenu */
	ATR_TOPMENU1, /* topmenu */
	ATR_TOPMENU2, /* topmenu */
	ATR_TOPMENU3, /* topmenu */
	ATR_MSG0, /* message */
	ATR_MSG1, /* message */
	ATR_MSG2, /* message */
	ATR_MSG3, /* message */
	ATR_MSGDRM0, /* message */
	ATR_MSGDRM1, /* message */
	ATR_MSGDRM2, /* message */
	ATR_MSGDRM3, /* message */
	ATR_MENUHIL0, /* hilit menu line */
	ATR_MENUHIL1, /* hilit menu line */
	ATR_MENUHIL2, /* hilit menu line */
	ATR_MENU0, /* menu line */
	ATR_MENU1, /* menu line */
	ATR_MENU2, /* menu line */
	ATR_MENU3, /* menu line */
	ATR_MENU4, /* menu line */
	ATR_MENU5, /* menu line */
	ATR_MENU6, /* menu line */
	ATR_CATCHMENU0, /* catch menu line */
	ATR_CATCHMENU1 /* catch menu line */
};

/* colortable */
enum
{
	black = 0,
	red, /* 1 */
	green, /* 2 */
	yellow, /* 3 */
	blue,	/* 4 */
	magenta,	/* 5 */
	cyan,	/* 6 */
	white, /* 7 */
	menu1 = (4*8),
	menu2,
	menu3,
	transp,
	transp2,
	SIZECOLTABLE
};

//const (avoid warnings :<)
tstPageAttr atrtable[] =
{
	{ white  , black , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_WB */
	{ white  , black , C_G0P, 0, 0, 1 ,0, 0}, /* ATR_PassiveDefault */
	{ white  , red   , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_L250 */
	{ black  , green , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_L251 */
	{ black  , yellow, C_G0P, 0, 0, 0 ,0, 0}, /* ATR_L252 */
	{ white  , blue  , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_L253 */
	{ magenta, black , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_TOPMENU0 */
	{ green  , black , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_TOPMENU1 */
	{ yellow , black , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_TOPMENU2 */
	{ cyan   , black , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_TOPMENU3 */
	{ menu2  , menu3 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MSG0 */
	{ yellow , menu3 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MSG1 */
	{ menu2  , transp, C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MSG2 */
	{ white  , menu3 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MSG3 */
	{ menu2  , menu1 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MSGDRM0 */
	{ yellow , menu1 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MSGDRM1 */
	{ menu2  , black , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MSGDRM2 */
	{ white  , menu1 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MSGDRM3 */
	{ menu1  , blue  , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MENUHIL0 5a Z */
	{ white  , blue  , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MENUHIL1 58 X */
	{ menu2  , transp, C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MENUHIL2 9b › */
	{ menu2  , menu1 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MENU0 ab « */
	{ yellow , menu1 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MENU1 a4 ¤ */
	{ menu2  , transp, C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MENU2 9b › */
	{ menu2  , menu3 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MENU3 cb Ë */
	{ cyan   , menu3 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MENU4 c7 Ç */
	{ white  , menu3 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MENU5 c8 È */
	{ white  , menu1 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_MENU6 a8 ¨ */
	{ yellow , menu1 , C_G0P, 0, 0, 0 ,0, 0}, /* ATR_CATCHMENU0 a4 ¤ */
	{ white  , menu1 , C_G0P, 0, 0, 0 ,0, 0}  /* ATR_CATCHMENU1 a8 ¨ */
};
/* buffers */
unsigned char  lcd_backbuffer[120*64 / 8];
unsigned char  page_char[40 * 25];
tstPageAttr page_atrb[40 * 25];

//unsigned short page_atrb[40 * 25]; /*  ?????:h:cc:bbbb:ffff -> ?=reserved, h=double height, c=charset (0:G0 / 1:G1c / 2:G1s), b=background, f=foreground */


/* colormaps */
const unsigned short defaultcolors[] =	/* 0x0bgr */
{
	0x000, 0x00f, 0x0f0, 0x0ff, 0xf00, 0xf0f, 0xff0, 0xfff,
	0x000, 0x007, 0x070, 0x077, 0x700, 0x707, 0x770, 0x777,
	0x50f, 0x07f, 0x7f0, 0xbff, 0xac0, 0x005, 0x256, 0x77c,
	0x333, 0x77f, 0x7f7, 0x7ff, 0xf77, 0xf7f, 0xff7, 0xddd,
	0x420, 0x210, 0x420, 0x000, 0x000
};

unsigned short rd0[] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0x00<<8, 0x00<<8, 0x00<<8, 0,      0      };
unsigned short gn0[] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0x20<<8, 0x10<<8, 0x20<<8, 0,      0      };
unsigned short bl0[] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0x40<<8, 0x20<<8, 0x40<<8, 0,      0      };
unsigned short tr0[] = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0x0000 , 0x0000 , 0x0A00 , 0xFFFF, 0x3000 };
struct fb_cmap colormap_0 = {0, SIZECOLTABLE, rd0, gn0, bl0, tr0};

/* tables for color table remapping, first entry (no remapping) skipped, offsets for color index */
const unsigned char MapTblFG[] = {  0,  0,  8,  8, 16, 16, 16 };
const unsigned char MapTblBG[] = {  8, 16,  8, 16,  8, 16, 24 };


/* shapes */
enum
{
	S_END = 0,
	S_FHL, /* full horizontal line: y-offset */
	S_FVL, /* full vertical line: x-offset */
	S_BOX, /* rectangle: x-offset, y-offset, width, height */
	S_TRA, /* trapez: x0, y0, l0, x1, y1, l1 */
	S_INV, /* invert */
	S_LNK, /* call other shape: shapenumber */
	S_CHR  /* Character from freetype hibyte, lowbyte*/
};

/* shape coordinates */
enum
{
	S_W13 = 5, /* width*1/3 */
	S_W12, /* width*1/2 */
	S_W23, /* width*2/3 */
	S_W11, /* width */
	S_WM3, /* width-3 */
	S_H13, /* height*1/3 */
	S_H12, /* height*1/2 */
	S_H23, /* height*2/3 */
	S_H11, /* height */
	S_NrShCoord
};

/* G3 characters */
unsigned char aG3_20[] = { S_TRA, 0, S_H23, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_21[] = { S_TRA, 0, S_H23, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_22[] = { S_TRA, 0, S_H12, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_23[] = { S_TRA, 0, S_H12, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_24[] = { S_TRA, 0, 0, 1, 0, S_H11, S_W12, S_END };
unsigned char aG3_25[] = { S_TRA, 0, 0, 1, 0, S_H11, S_W11, S_END };
unsigned char aG3_26[] = { S_INV, S_LNK, 0x66 };
unsigned char aG3_27[] = { S_INV, S_LNK, 0x67 };
unsigned char aG3_28[] = { S_INV, S_LNK, 0x68 };
unsigned char aG3_29[] = { S_INV, S_LNK, 0x69 };
unsigned char aG3_2a[] = { S_INV, S_LNK, 0x6a };
unsigned char aG3_2b[] = { S_INV, S_LNK, 0x6b };
unsigned char aG3_2c[] = { S_INV, S_LNK, 0x6c };
unsigned char aG3_2d[] = { S_INV, S_LNK, 0x6d };
unsigned char aG3_2e[] = { S_BOX, 1, 0, 2, S_H11, S_END };
unsigned char aG3_2f[] = { S_BOX, 0, 0, S_W11, S_H11, S_END }; /* FIXME: grey  */
unsigned char aG3_30[] = { S_TRA, S_W11, S_H23, 0, S_W12, S_H11, S_W12, S_END };
unsigned char aG3_31[] = { S_TRA, S_W11, S_H23, 0, 0, S_H11, S_W11, S_END };
unsigned char aG3_32[] = { S_TRA, S_W11, S_H12, 0, S_W12, S_H11, S_W12, S_END };
unsigned char aG3_33[] = { S_TRA, S_W11, S_H12, 0, 0, S_H11, S_W11, S_END };
unsigned char aG3_34[] = { S_TRA, S_W11, 0, S_W12, S_H11, S_W12, S_END };
unsigned char aG3_35[] = { S_TRA, S_W11, 0, 0, 0, S_H11, S_W11, S_END };
unsigned char aG3_36[] = { S_INV, S_LNK, 0x76 };
unsigned char aG3_37[] = { S_INV, S_LNK, 0x77 };
unsigned char aG3_38[] = { S_INV, S_LNK, 0x78 };
unsigned char aG3_39[] = { S_INV, S_LNK, 0x79 };
unsigned char aG3_3a[] = { S_INV, S_LNK, 0x7a };
unsigned char aG3_3b[] = { S_INV, S_LNK, 0x7b };
unsigned char aG3_3c[] = { S_INV, S_LNK, 0x7c };
unsigned char aG3_3d[] = { S_INV, S_LNK, 0x7d };
unsigned char aG3_3e[] = { S_BOX, S_WM3, 0, 2, S_H11, S_END };
unsigned char aG3_3f[] = { S_BOX, 0, 0, S_W11, S_H11, S_END };
unsigned char aG3_40[] = { S_BOX, 0, S_H12, S_W11, 4, S_BOX, S_W12, 0, 2, S_H12, S_END };
unsigned char aG3_41[] = { S_BOX, 0, S_H12, S_W11, 4, S_BOX, S_W12, S_H12, 2, S_H12, S_END };
unsigned char aG3_42[] = { S_BOX, S_W12, 0, 2, S_H11, S_BOX, S_W12, S_H12, S_W12, 4, S_END };
unsigned char aG3_43[] = { S_BOX, S_W12, 0, 2, S_H11, S_BOX, 0, S_H12, S_W12, 4, S_END };
unsigned char aG3_44[] = { S_TRA, S_W13, 0, S_W13, 0, S_H12, S_W13, S_TRA, 0, S_H12, S_W13, S_W13, S_H11, S_W13, S_END };
unsigned char aG3_45[] = { S_TRA, S_W12, 0, S_W13, S_W23, S_H12, S_W13, S_TRA, S_W23, S_H12, S_W13, S_W12, S_H11, S_W13, S_END };
unsigned char aG3_46[] = { S_TRA, 0, S_H12, 1, S_W13, S_H11, S_W12, S_TRA, S_W11, S_H12, 0, S_W13, S_H11, S_W12, S_END };
unsigned char aG3_47[] = { S_TRA, S_W13, 0, S_W12, 0, S_H12, 1, S_TRA, S_W13, 0, S_W12, S_W11, S_H12, 0, S_END };
unsigned char aG3_48[] = { S_TRA, S_W13, 0, S_W13, 0, S_H12, 1, S_END };
unsigned char aG3_49[] = { S_TRA, S_W12, 0, S_W13, S_W11, S_H12, 0, S_END };
unsigned char aG3_4a[] = { S_TRA, 0, S_H12, 1, S_W13, S_H11, S_H13, S_END };
unsigned char aG3_4b[] = { S_TRA, S_W11, S_H12, 0, S_W12, S_H11, S_H13, S_END };
unsigned char aG3_4c[] = { S_BOX, 0, S_H12, S_W11, 4, S_BOX, S_W12, 0, 2, S_H11, S_END };
unsigned char aG3_4d[] = { S_BOX, S_W13, S_H13, S_W12, S_H12, S_END };/* FIXME (circles) */
unsigned char aG3_4e[] = { S_BOX, S_W13, S_H13, S_W12, S_H12, S_END };/* FIXME */
unsigned char aG3_4f[] = { S_BOX, S_W13, S_H13, S_W12, S_H12, S_END };/* FIXME */
unsigned char aG3_50[] = { S_BOX, S_W12, 0, 2, S_H11, S_END };
unsigned char aG3_51[] = { S_BOX, 0, S_H12, S_W11, 2, S_END };
unsigned char aG3_52[] = { S_BOX, S_W12, S_H12, 2, S_H12, S_BOX, S_W12, S_H12, S_W12, 2, S_END };
unsigned char aG3_53[] = { S_BOX, S_W12, S_H12, 2, S_H12, S_BOX, 0, S_H12, S_W12, 2, S_END };
unsigned char aG3_54[] = { S_BOX, S_W12, 0, 2, S_H12, S_BOX, S_W12, S_H12, S_W12, 2, S_END };
unsigned char aG3_55[] = { S_BOX, S_W12, 0, 2, S_H12, S_BOX, 0, S_H12, S_W12, 2, S_END };
unsigned char aG3_56[] = { S_BOX, S_W12, 0, 2, S_H11, S_BOX, S_W12, S_H12, S_W12, 2, S_END };
unsigned char aG3_57[] = { S_BOX, S_W12, 0, 2, S_H11, S_BOX, 0, S_H12, S_W12, 2, S_END };
unsigned char aG3_58[] = { S_BOX, 0, S_H12, S_W11, 2, S_BOX, S_W12, S_H12, 2, S_H12, S_END };
unsigned char aG3_59[] = { S_BOX, 0, S_H12, S_W11, 2, S_BOX, S_W12, 0, 2, S_H12, S_END };
unsigned char aG3_5a[] = { S_BOX, 0, S_H12, S_W11, 2, S_BOX, S_W12, 0, 2, S_H11, S_END };
unsigned char aG3_5b[] = { S_CHR, 0x21, 0x92};
unsigned char aG3_5c[] = { S_CHR, 0x21, 0x90};
unsigned char aG3_5d[] = { S_CHR, 0x21, 0x91};
unsigned char aG3_5e[] = { S_CHR, 0x21, 0x93};
unsigned char aG3_5f[] = { S_BOX, S_W13, S_H13, S_W12, S_H12, S_END };/* FIXME */
unsigned char aG3_60[] = { S_INV, S_LNK, 0x20 };
unsigned char aG3_61[] = { S_INV, S_LNK, 0x21 };
unsigned char aG3_62[] = { S_INV, S_LNK, 0x22 };
unsigned char aG3_63[] = { S_INV, S_LNK, 0x23 };
unsigned char aG3_64[] = { S_INV, S_LNK, 0x24 };
unsigned char aG3_65[] = { S_INV, S_LNK, 0x25 };
unsigned char aG3_66[] = { S_TRA, 0, 0, S_W12, 0, S_H13, 1, S_END };
unsigned char aG3_67[] = { S_TRA, 0, 0, S_W11, 0, S_H13, 1, S_END };
unsigned char aG3_68[] = { S_TRA, 0, 0, S_W12, 0, S_H12, 1, S_END };
unsigned char aG3_69[] = { S_TRA, 0, 0, S_W11, 0, S_H12, 1, S_END };
unsigned char aG3_6a[] = { S_TRA, 0, 0, S_W12, 0, S_H11, 1, S_END };
unsigned char aG3_6b[] = { S_BOX, 0, 0, S_W11, S_H13, S_TRA, 0, S_H13, S_W11, 0, S_H23, 1, S_END };
unsigned char aG3_6c[] = { S_TRA, 0, 0, 1, 0, S_H12, S_W12, S_TRA, 0, S_H12, S_W12, 0, S_H11, 1, S_END };
unsigned char aG3_6d[] = { S_TRA, 0, 0, S_W11, S_W12, S_H12, 1, S_END };
unsigned char aG3_6e[] = { S_BOX, S_W13, S_H13, S_W12, S_H12, S_END }; /* FIXME empty*/
unsigned char aG3_6f[] = { S_BOX, S_W13, S_H13, S_W12, S_H12, S_END };
unsigned char aG3_70[] = { S_INV, S_LNK, 0x30 };
unsigned char aG3_71[] = { S_INV, S_LNK, 0x31 };
unsigned char aG3_72[] = { S_INV, S_LNK, 0x32 };
unsigned char aG3_73[] = { S_INV, S_LNK, 0x33 };
unsigned char aG3_74[] = { S_INV, S_LNK, 0x34 };
unsigned char aG3_75[] = { S_INV, S_LNK, 0x35 };
unsigned char aG3_76[] = { S_TRA, S_W12, 0, S_W12, S_W11, S_H13, 0, S_END };
unsigned char aG3_77[] = { S_TRA, 0, 0, S_W11, S_W11, S_H13, 0, S_END };
unsigned char aG3_78[] = { S_TRA, S_W12, 0, S_W12, S_W11, S_H12, 0, S_END };
unsigned char aG3_79[] = { S_TRA, 0, 0, S_W11, S_W11, S_H12, 0, S_END };
unsigned char aG3_7a[] = { S_TRA, S_W12, 0, S_W12, S_W11, S_H23, 0, S_END };
unsigned char aG3_7b[] = { S_BOX, 0, 0, S_W11, S_H13, S_TRA, 0, S_H13, S_W11, 0, S_H23, 1, S_END };
unsigned char aG3_7c[] = { S_TRA, S_W11, 0, 0, S_W12, S_H12, S_W12, S_TRA, S_W12, S_H12, S_W12, S_W11, S_H11, 0, S_END };
unsigned char aG3_7d[] = { S_TRA, S_W12, S_H12, 1, 0, S_H11, S_W11, S_END };

unsigned char *aShapes[] =
{
	aG3_20, aG3_21, aG3_22, aG3_23, aG3_24, aG3_25, aG3_26, aG3_27, aG3_28, aG3_29, aG3_2a, aG3_2b, aG3_2c, aG3_2d, aG3_2e, aG3_2f,
	aG3_30, aG3_31, aG3_32, aG3_33, aG3_34, aG3_35, aG3_36, aG3_37, aG3_38, aG3_39, aG3_3a, aG3_3b, aG3_3c, aG3_3d, aG3_3e, aG3_3f,
	aG3_40, aG3_41, aG3_42, aG3_43, aG3_44, aG3_45, aG3_46, aG3_47, aG3_48, aG3_49, aG3_4a, aG3_4b, aG3_4c, aG3_4d, aG3_4e, aG3_4f,
	aG3_50, aG3_51, aG3_52, aG3_53, aG3_54, aG3_55, aG3_56, aG3_57, aG3_58, aG3_59, aG3_5a, aG3_5b, aG3_5c, aG3_5d, aG3_5e, aG3_5f,
	aG3_60, aG3_61, aG3_62, aG3_63, aG3_64, aG3_65, aG3_66, aG3_67, aG3_68, aG3_69, aG3_6a, aG3_6b, aG3_6c, aG3_6d, aG3_6e, aG3_6f,
	aG3_70, aG3_71, aG3_72, aG3_73, aG3_74, aG3_75, aG3_76, aG3_77, aG3_78, aG3_79, aG3_7a, aG3_7b, aG3_7c, aG3_7d
};




/* lcd layout */
const char lcd_layout[] =
{
#define ____ 0x0
#define ___X 0x1
#define __X_ 0x2
#define __XX 0x3
#define _X__ 0x4
#define _X_X 0x5
#define _XX_ 0x6
#define _XXX 0x7
#define X___ 0x8
#define X__X 0x9
#define X_X_ 0xA
#define X_XX 0xB
#define XX__ 0xC
#define XX_X 0xD
#define XXX_ 0xE
#define XXXX 0xF

#define i <<4|

	____ i _XXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXX_ i ____,
	___X i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i X___,
	__XX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XX__ i ____,XXX_ i _X__,_XXX i __X_,__XX i X___,___X i XX__,X___ i XXX_,____ i _XXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XX__,
	_XXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,X_XX i XXXX,_X_X i X_XX,X_X_ i XX_X,XX_X i _XXX,XXX_ i X_XX,_XXX i _X_X,XXXX i X_XX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXX_,
	_XXX i XXXX,X___ i ____,____ i ____,____ i __XX,X_XX i XXXX,_X_X i X_XX,X_X_ i XX_X,XX_X i _XXX,XXX_ i X_XX,_XXX i _X_X,XXXX i X_XX,X___ i ____,____ i ____,____ i ___X,XXXX i XXX_,
	XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,X_XX i XXXX,_X_X i X_XX,X_X_ i XX_X,XX_X i _XXX,XXX_ i X_XX,_XXX i _X_X,XXXX i X_XX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,
	XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XX__ i XX__,XX_X i X_XX,X_X_ i XXXX,XX_X i X__X,X__X i X_XX,XXXX i _XX_,_XX_ i _XXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,
	XXXX i XX__,____ i ____,____ i ____,____ i __XX,XXX_ i XX_X,XX_X i X_XX,X_XX i _XXX,__XX i XX_X,X_XX i XX_X,XX__ i XXXX,_XX_ i XXXX,X___ i ____,____ i ____,____ i ____,__XX i XXXX,
	XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXX_ i XX_X,XX_X i X_XX,X_X_ i XXXX,XX_X i XX_X,X_XX i X_XX,XXXX i _XXX,_XX_ i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,
	XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXX_ i XX_X,XX_X i X_XX,X_X_ i XX_X,XX_X i XX_X,X_XX i X_XX,_XXX i _XXX,_XX_ i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,
	XXX_ i ____,____ i ____,____ i ____,____ i __XX,XXX_ i XX_X,XX_X i XXXX,X_X_ i XX_X,XX_X i XX_X,X_XX i X_XX,_XXX i _XXX,_XX_ i XXXX,X___ i ____,____ i ____,____ i ____,____ i _XXX,
	XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXX_ i XX_X,XX_X i XXXX,X_X_ i XX_X,XX_X i XX_X,X_XX i X_XX,_XXX i _XXX,_XX_ i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,
	XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i __XX,XXX_ i ____,_XXX i __X_,__XX i XXX_,_XXX i XX__,X___ i XXXX,X__X i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,
	XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,
	XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i XX__,_XXX i XXX_,__XX i ___X,XXXX i X___,XX__ i _XXX,XXX_ i __XX,____ i ___X,X___ i XXXX,XX__ i _XX_,__XX i XXXX,___X i XXXX,X___ i XX_X,XX__ i _XXX,XXX_ i __XX,XXXX i ___X,
	X__X i __X_,X___ i ___X,_X__ i X_X_,____ i _X_X,__X_ i X___,___X i _X__,X___ i __X_,_X_X i ____,__X_ i X__X,_X__ i ____,X_X_ i ____,_X_X i __X_,__X_ i X___,___X i _X__,____ i X__X,
	X__X i __X_,X___ i ___X,_X__ i X_X_,____ i _X_X,__X_ i X___,___X i _X__,X___ i __X_,_X_X i ____,__X_ i X__X,_X__ i ____,X_X_ i ____,_X_X i __X_,__X_ i X___,___X i _X__,____ i X__X,
	X__X i __X_,X___ i ___X,_X__ i X_X_,____ i _X_X,__X_ i X___,___X i _X__,X___ i __X_,_X_X i ____,__X_ i X__X,_X__ i ____,X_X_ i ____,_X_X i __X_,__X_ i X___,___X i _X__,____ i X__X,
	X__X i __X_,X__X i ___X,_X__ i X__X,X__X i X__X,__X_ i X__X,___X i _X__,X___ i __X_,_X_X i __XX,XX__ i X__X,_X__ i XXXX,__X_ i _X__,_X_X i __X_,__X_ i X__X,___X i _X__,XXXX i ___X,
	X__X i __X_,X__X i ___X,_X__ i X___,X__X i ___X,__X_ i X___,___X i _X__,X___ i __X_,_X_X i ____,__X_ i X__X,_X__ i ____,X_X_ i ___X,X__X i __X_,__X_ i X__X,___X i _X__,X___ i X__X,
	X__X i __X_,X__X i ___X,_X__ i X___,X__X i ___X,__X_ i X___,___X i _X__,X___ i __X_,_X_X i ____,__X_ i X__X,_X__ i ____,X_X_ i ____,_X_X i __X_,__X_ i X__X,___X i _X__,X___ i X__X,
	X__X i __X_,X__X i ___X,_X__ i X___,X__X i ___X,__X_ i X__X,___X i _X__,XXXX i __X_,_X__ i XXX_,__X_ i X__X,_X__ i XXXX,__X_ i _X__,_X_X i __X_,__X_ i X__X,___X i _X__,X___ i X__X,
	X__X i __X_,X__X i ___X,_X__ i X___,X__X i ___X,__X_ i X__X,___X i _X__,____ i X_X_,_X_X i ____,__X_ i X__X,_X__ i ____,X_X_ i _X__,_X_X i ____,__X_ i X__X,___X i _X__,____ i X__X,
	X__X i __X_,X__X i ___X,_X__ i X___,X__X i ___X,__X_ i X__X,___X i _X__,____ i X_X_,_X_X i ____,__X_ i X__X,_X__ i ____,X_X_ i _X__,_X_X i ____,__X_ i X__X,___X i _X__,____ i X__X,
	X___ i XX__,XXX_ i XXXX,__XX i ____,_XX_ i ____,XX__ i _XX_,XXX_ i __XX,XXXX i ___X,X___ i XXXX,XX__ i _XX_,__XX i XXXX,___X i X_XX,X___ i XXXX,XX__ i _XX_,XXX_ i __XX,XXXX i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,XXXX i XXXX,X___ i XXXX,XX__ i _XXX,XXXX i XX__,_XXX i XXX_,__XX i XXXX,___X i XXXX,X___ i ____,__XX i XXXX,___X i X___,XXXX i XX__,_XXX i XXX_,__XX i XXXX,____ i ___X,
	X___ i ___X,____ i ____,_X_X i ____,__X_ i X___,____ i __X_,X___ i ___X,_X__ i ____,X_X_ i ____,_X__ i ____,_X__ i ____,X_X_ i _X_X,____ i __X_,X___ i ___X,_X__ i ____,X___ i ___X,
	X___ i ___X,____ i ____,_X_X i ____,__X_ i X___,____ i __X_,X___ i ___X,_X__ i ____,X_X_ i ____,_X__ i ____,_X__ i ____,X_X_ i _X_X,____ i __X_,X___ i ___X,_X__ i ____,X___ i ___X,
	X___ i ___X,____ i ____,_X_X i ____,__X_ i X___,____ i __X_,X___ i ___X,_X__ i ____,X_X_ i ____,_X__ i ____,_X__ i XX__,X_X_ i _X_X,____ i __X_,X___ i ___X,_X__ i ____,X___ i ___X,
	X___ i ___X,__X_ i __X_,_X_X i __X_,__X_ i X__X,___X i __X_,X__X i XXX_,_X__ i X___,X__X i X__X,X___ i ____,_X__ i ____,X_X_ i _X__,XX__ i XX__,_XX_ i _XX_,_X__ i XXXX,____ i ___X,
	X___ i ___X,__X_ i __X_,_X_X i __X_,__X_ i X__X,___X i __X_,X___ i ___X,_X__ i X___,X___ i X__X,____ i ____,_X__ i __XX,__X_ i _X__,_X__ i X___,__X_ i _X__,_X__ i ____,X___ i ___X,
	X___ i ___X,__X_ i __X_,_X_X i __X_,__X_ i X__X,___X i __X_,X___ i ___X,_X__ i X___,X___ i X__X,____ i ____,_X__ i ____,X_X_ i _X__,_X__ i X___,__X_ i _X__,_X__ i ____,X___ i ___X,
	X___ i ___X,__X_ i __X_,_X_X i __X_,__X_ i X__X,___X i __X_,X__X i XXX_,_X__ i X___,X___ i X__X,____ i ____,_X__ i XX__,X_X_ i _X__,_X__ i X___,__X_ i _X__,_X__ i XXXX,____ i ___X,
	X___ i ___X,__X_ i __X_,_X_X i ____,__X_ i X__X,___X i __X_,X___ i ___X,_X__ i X___,X___ i X__X,____ i ____,_X__ i ____,X_X_ i _X__,_X__ i X___,__X_ i _X__,_X__ i ____,X___ i ___X,
	X___ i ___X,__X_ i __X_,_X_X i ____,__X_ i X__X,___X i __X_,X___ i ___X,_X__ i X___,X___ i X__X,____ i ____,_X__ i ____,X_X_ i _X__,_X__ i X___,__X_ i _X__,_X__ i ____,X___ i ___X,
	X___ i ____,XX_X i XX_X,X___ i XXXX,XX__ i _XX_,XXX_ i XX__,_XXX i XXX_,__XX i _XXX,____ i _XX_,____ i ____,__XX i XXXX,___X i X___,__XX i ____,___X i X___,__XX i XXXX,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	X___ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ___X,
	_X__ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i __X_,
	_X__ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i __X_,
	__X_ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i _X__,
	___X i X___,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,____ i ____,___X i X___,
	____ i _XXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXXX i XXXX,XXX_ i ____,

#undef i
};

/* lcd digits */
const char lcd_digits[] =
{
	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,0,0,1,1,1,1,0,0,0,
	0,0,1,1,0,0,1,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,1,0,0,1,1,0,0,
	0,0,0,1,1,1,1,0,0,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,1,1,1,1,0,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,0,1,1,1,1,0,
	1,1,0,1,1,1,0,0,1,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,1,0,0,1,1,
	0,0,0,0,0,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,1,1,1,1,0,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,1,1,1,1,0,
	1,0,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,0,0,0,0,1,
	0,0,0,0,1,1,0,0,1,1,
	0,0,0,0,0,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,0,0,0,0,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,1,1,
	0,1,1,1,1,1,1,1,1,0,

	/* 10: - */
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,1,1,1,1,1,1,0,0,
	0,1,0,0,0,0,0,0,1,0,
	0,1,0,0,0,0,0,0,1,0,
	0,1,0,0,0,0,0,0,1,0,
	0,0,1,1,1,1,1,1,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,

	/* 11: / */
	0,0,0,0,1,1,1,1,0,0,
	0,0,0,1,0,0,0,0,1,0,
	0,0,0,1,0,0,0,0,1,0,
	0,0,0,1,0,0,0,0,1,0,
	0,0,0,1,0,0,0,0,1,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,0,1,0,0,0,0,1,0,0,
	0,1,0,0,0,0,1,0,0,0,
	0,1,0,0,0,0,1,0,0,0,
	0,1,0,0,0,0,1,0,0,0,
	0,1,0,0,0,0,1,0,0,0,
	0,0,1,1,1,1,0,0,0,0,

	/* 12: ? */
	0,1,1,1,1,1,1,1,1,0,
	1,1,0,0,0,0,0,0,1,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	0,1,1,1,1,1,0,0,0,1,
	1,1,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,0,1,1,
	1,1,1,1,1,1,1,1,1,0,
	1,0,0,0,0,1,0,0,0,0,
	1,0,0,0,0,1,0,0,0,0,
	1,0,0,0,0,1,0,0,0,0,
	1,1,0,0,1,1,0,0,0,0,
	0,1,1,1,1,0,0,0,0,0,

	/* 13: " " */
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
};

/* functions */
void ConfigMenu(int Init);
void CleanUp();
void PageInput(int Number);
void ColorKey(int);
void PageCatching();
void CatchNextPage(int, int);
void GetNextPageOne(int up);
void GetNextSubPage(int offset);
void SwitchZoomMode();
void SwitchScreenMode(int newscreenmode);
void SwitchTranspMode();
void SwitchHintMode();
void CreateLine25();
void CopyBB2FB();
void RenderCatchedPage();
void RenderCharFB(int Char, tstPageAttr *Attribute);
void RenderCharBB(int Char, tstPageAttr *Attribute);
void RenderCharLCD(int Digit, int XPos, int YPos);
void RenderMessage(int Message);
void RenderPage();
void DecodePage();
void UpdateLCD();
int  Init();
int  GetNationalSubset(char *country_code);
int  GetTeletextPIDs();
int  GetRCCode();
int  eval_triplet(int iOData, tstCachedPage *pstCachedPage,
						unsigned char *pAPx, unsigned char *pAPy,
						unsigned char *pAPx0, unsigned char *pAPy0,
						unsigned char *drcssubp, unsigned char *gdrcssubp,
						signed char *endcol, tstPageAttr *attrPassive);
void eval_object(int iONr, tstCachedPage *pstCachedPage,
					  unsigned char *pAPx, unsigned char *pAPy,
					  unsigned char *pAPx0, unsigned char *pAPy0,
					  tObjType ObjType);

/* Local Variables: */
/* indent-tabs-mode:t */
/* tab-width:3 */
/* c-basic-offset:3 */
/* comment-column:0 */
/* fill-column:120 */
/* End: */
