/******************************************************************************
 *                      <<< TuxTxt - Teletext Plugin >>>                      *
 *                                                                            *
 *             (c) Thomas "LazyT" Loewe 2002-2003 (LazyT@gmx.net)             *
 ******************************************************************************/

#include <config.h>
#ifndef DREAMBOX
#include <tuxbox.h>
#endif
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if HAVE_DVB_API_VERSION < 3
#include <ost/dmx.h>
#else
#include <linux/dvb/dmx.h>
#endif
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

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H

#define PAGESIZE (40*24)

/* devices */
#define AVS "/dev/dbox/avs0"
#define SAA "/dev/dbox/saa0"
#if HAVE_DVB_API_VERSION < 3
#define DMX "/dev/dvb/card0/demux0"
#define PIG "/dev/dbox/pig0"
#else
#define DMX "/dev/dvb/adapter0/demux0"
#define PIG "/dev/v4l/video0"
#endif

#define CFGTTF 1
#define TUXTXTCONF CONFIGDIR "/tuxtxt/tuxtxt.conf"

/* fonts */
#define TUXTXT0 FONTDIR "/tuxtxt0.fon"  /* G0 8+16pt */
#define TUXTXT1 FONTDIR "/tuxtxt1.fon"  /* G1 8+16pt */
#define TUXTXT2 FONTDIR "/tuxtxt2.fon"  /* NS 8+16pt */
#if 0
#if CFGTTF 
#define TUXTXTTTF "/var/tuxtxt/TuxTxt.ttf" /* TTF */
#else
#define TUXTXT0R "/var/tuxtxt/tuxtxt0r.fon" /* G0 12pt */
#define TUXTXT1R "/var/tuxtxt/tuxtxt1r.fon" /* G1 12pt */
#define TUXTXT2R "/var/tuxtxt/tuxtxt2r.fon" /* NS 12pt */
#endif
#else
#if CFGTTF 
#define TUXTXTTTF FONTDIR "/tuxtxt.ttf" /* TTF */
#else
#define TUXTXT0R FONTDIR "/tuxtxt0r.fon" /* G0 12pt */
#define TUXTXT1R FONTDIR "/tuxtxt1r.fon" /* G1 12pt */
#define TUXTXT2R FONTDIR "/tuxtxt2r.fon" /* NS 12pt */
#endif
#endif

#if CFGTTF 
int fontheight, fontwidth, fontwidth_normal, fontwidth_small, fontwidth_topmenumain, fontwidth_topmenusmall, ascender;
int ymosaic[4];
#define TTFWIDTHFACTOR 4 /* FIXME: otherwise too much space btw chars */

#define fontwidth_small_lcd 8
#else
#define fontheight 20
#define fontwidth (type0.font.pix_width)
#define fontwidth_normal 16
#define fontwidth_small 8
#define fontwidth_topmenumain 12
#define fontwidth_topmenusmall 8
#endif

#define TV43STARTX (StartX + 10 + 40*fontwidth_topmenumain)
#define TV169STARTX (StartX + 40*fontwidth_small)
#define TVENDX (StartX + 40*fontwidth_normal)
#define TV43WIDTH 120
#define TV169WIDTH (TVENDX-TV169STARTX-9)
#define TV169FULLWIDTH 320
#define TV43STARTY (TVENDY - TV43WIDTH*3/4-20)
#define TV169STARTY (TVENDY - TV169WIDTH-50)
#define TVENDY (StartY + 25*fontheight)
#define TV43HEIGHT 96
#define TV169HEIGHT (TVENDY-TV169STARTY)
#define TV169FULLHEIGHT 576

#define TOPMENU43STARTX (TV43STARTX+2)
#define TOPMENU169STARTX (TV43STARTX+2)
#define TOPMENUENDX TVENDX
#define TOPMENUSTARTY StartY
#define TOPMENU43ENDY TV43STARTY
#define TOPMENU169ENDY TV43STARTY

#define TOPMENULINEWIDTH ((TOPMENUENDX-TOPMENU43STARTX+fontwidth_topmenusmall-1)/fontwidth_topmenusmall)
#define TOPMENUINDENTBLK 0
#define TOPMENUINDENTGRP 1
#define TOPMENUINDENTDEF 2
#define TOPMENUSPC 0
#define TOPMENUCHARS (TOPMENUINDENTDEF+12+TOPMENUSPC+3+5)

/*  1: blk-1, grp-1, grp+1, blk+1 */
/*  2: blk-1, grp+1, grp+2, blk+1 */
#define LINE25MODE 2

/* colortable */
#define black   0x01
#define red     0x02
#define green   0x03
#define yellow  0x04
#define blue    0x05
#define magenta 0x06
#define cyan    0x07
#define white   0x08
#define transp  0x09
#define menu1   0x0A
#define menu2   0x0B
#define menu3   0x0C
#define transp2 0x0D

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

/* messages */
#define ShowInfoBar     0
#define PageNotFound    1
#define ShowServiceName 2
#define NoServicesFound 3

/* functions */
void ConfigMenu(int Init);
void CleanUp();
void PageInput(int Number);
void Prev100();
void Prev10();
void Next10();
void Next100();
void PageCatching();
void CatchNextPage(int, int);
void GetNextPageOne();
void GetPrevPageOne();
void GetNextSubPage();
void GetPrevSubPage();
void SwitchZoomMode();
void SwitchScreenMode(int newscreenmode);
void SwitchTranspMode();
void SwitchHintMode();
void CreateLine25();
void CopyBB2FB();
void RenderCatchedPage();
void RenderCharFB(int Char, int Attribute);
void RenderCharBB(int Char, int Attribute);
void RenderCharLCD(int Digit, int XPos, int YPos);
void RenderMessage(int Message);
void RenderPage();
void DecodePage();
void UpdateLCD();
void *CacheThread(void *arg);
int  Init();
int  GetNationalSubset(char *country_code);
int  GetTeletextPIDs();
int  GetRCCode();

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

#if CFGTTF 
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

#else
FONTTYPE type0, type1, type2;
FONTTYPE type0r, type1r, type2r;
#endif


/* some data */
unsigned char basictop[0x900];
unsigned char adip[0x900][13];  /* FIXME: pointers and malloc? */
char versioninfo[16];
char bttok;
int adippg[10];
int maxadippg;
int hotlist[10];
int maxhotlist;

int dmx, pig, avs, saa, rc, fb, lcd;
int sx, ex, sy, ey;
int vtxtpid;
int PosX, PosY, StartX, StartY;
int cached_pages, page_receiving, current_page[9], current_subpage[9];
int page, subpage, lastpage, pageupdate, zap_subpage_manual;
int current_national_subset;
int inputcounter;
int zoommode, screenmode, transpmode, hintmode, boxed;
int catch_row, catch_col, catched_page, pagecatching;
int prev_100, prev_10, next_10, next_100;
int fnc_old, saa_old, screen_mode1, screen_mode2, color_mode, national_subset, auto_national, swapupdown, showhex, menulanguage;
int clear_page, clear_subpage;
int pids_found, current_service, getpidsdone;
int SDT_ready;
int pc_old_row, pc_old_col;     /* for page catching */
int temp_page;	/* for page input */
char saveconfig, hotlistchanged;
signed char clearbbcolor = -1;

const int fncmodes[] = {AVS_FNCOUT_EXT43, AVS_FNCOUT_EXT169};
const int saamodes[] = {SAA_WSS_43F, SAA_WSS_169F};

FILE *conf;

pthread_t thread_id;
void *thread_result;

unsigned short RCCode;

struct _pid_table
{
	int  vtxt_pid;
	int  service_id;
	int  service_name_len;
	char service_name[24];
	int  national_subset;
}pid_table[128];

/* national subsets */
const char countrystring[] =
"  CS/SK  (#$@[\\]^_`{|}~)  "   /* czech, slovak */
"    EN (#$@[\\]^_`{|}~)    "   /* english */
"    ET (#$@[\\]^_`{|}~)    "   /* estonian */
"    FR (#$@[\\]^_`{|}~)    "   /* french */
"    DE (#$@[\\]^_`{|}~)    "   /* german */
"    IT (#$@[\\]^_`{|}~)    "   /* italian */
"  LV/LT  (#$@[\\]^_`{|}~)  "   /* latvian, lithuanian */
"    PL (#$@[\\]^_`{|}~)    "   /* polish */
"  PT/ES  (#$@[\\]^_`{|}~)  "   /* portuguese, spanish */
"    RO (#$@[\\]^_`{|}~)    "   /* romanian */
" SR/HR/SL (#$@[\\]^_`{|}~) "   /* serbian, croatian, slovenian */
" SV/FI/HU (#$@[\\]^_`{|}~) "   /* swedish, finnish, hungarian */
"    TR (#$@[\\]^_`{|}~)    ";  /* turkish */

const unsigned char countryconversiontable[] = { 1, 4, 11, 5, 3, 8, 0, 9 };

unsigned char restoreaudio = 0;
/* 0 Nokia, 1 Philips, 2 Sagem */
/* typ_vcr/dvb: 	v1 a1 v2 a2 v3 a3 (vcr_only: fblk) */
const int avstable_ioctl[7] =
{
	AVSIOSVSW1, AVSIOSASW1, AVSIOSVSW2, AVSIOSASW2, AVSIOSVSW3, AVSIOSASW3, AVSIOSFBLK
};
const unsigned char avstable_scart[3][7] =
{
	{ 3, 2, 1, 0, 1, 1, 2 },
	{ 3, 3, 2, 2, 3, 2, 2 },
	{ 2, 1, 0, 0, 0, 0, 0 },
};
const unsigned char avstable_dvb[3][7] =
{
	{ 5, 1, 1, 0, 1, 1, 0 },
	{ 1, 1, 1, 1, 1, 1, 0 },
	{ 0, 0, 0, 0, 0, 0, 0 },
};

/* language dependent texts */
#define MAXMENULANGUAGE 3 /* 0 deutsch, 1 englisch, 2 französisch, 3 niederländisch */

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
	22, 26, 28, 27
};
const char hotlisttextcolumn[] =
{
	24, 14, 14, 15
};
const char hotlisttext[][2*5] =
{
	{ "dazu entf." },
	{ " add rem. " },
	{ "ajoutenlev" },
	{ "toev.verw." },
};

const char configonoff[][2*3] =
{
	{ "ausein" },
	{ "offon " },
	{ "desact" },
	{ "uitaan" } 
};

const char configmenu[][2*Menu_Height*Menu_Width] =
{
	{
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
		"àááááááááááááááááááááááááááááâè««««««««««««««««««««««««««««««›"
		"ã     Konfigurationsmenue    äé«¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤«›"
		"åææææææææææææææææææææææææææææçé««««««««««««««««««««««««««««««›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã1 Favoriten: Seite 111 dazu äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãíîñò                        äéËÈÈ¨¨ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã+-?                         äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã2     Teletext-Auswahl      äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãí                          îäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã      Bildschirmformat      äéËÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ã3  Standard-Modus 16:9      äéË¤ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã4  TextBild-Modus 16:9      äéË¤ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã5        Helligkeit         äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãAnzeige 1/3 reduzieren      äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã6  nationaler Zeichensatz   äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãautomatische Erkennung      äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ãí    DE (#$@[\\]^_`{|}~)    îäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ãí Sprache/Language deutsch îäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"åææææææææææææææææææææææææææææçéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ëìììììììììììììììììììììììììììììê›››››››››››››››››››››››››››››››"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè««««««««««««««««««««««««««««««›"
		"ã     Configuration menu     äé«¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤«›"
		"åææææææææææææææææææææææææææææçé««««««««««««««««««««««««««««««›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã1 Favorites:  add page 111  äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãíîñò                        äéËÈÈ¨¨ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã+-?                         äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã2     Teletext selection    äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãí                          îäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã        Screen format       äéËÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ã3 Standard mode 16:9        äéË¤ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã4 Text/TV mode  16:9        äéË¤ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã5        Brightness         äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ã         Reduce by 1/3      äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã6   national characterset   äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ã automatic recognition      äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ãí    DE (#$@[\\]^_`{|}~)    îäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ãí Sprache/language english îäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"åææææææææææææææææææææææææææææçéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ëìììììììììììììììììììììììììììììê›››››››››››››››››››››››››››››››"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè««««««««««««««««««««««««««««««›"
		"ã    Menu de configuration   äé«¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤«›"
		"åææææææææææææææææææææææææææææçé««««««««««««««««««««««««««««««›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã1 Favorites: ajout. page 111äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãíîñò                        äéËÈÈ¨¨ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã+-?                         äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã2  Selection de teletext    äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãí                          îäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã      Format de l'#cran     äéËÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ã3 Mode standard 16:9        äéË¤ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã4 Texte/TV      16:9        äéË¤ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã5          Clarte           äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ã  R#duire de 1/3            äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã6     police nationale      äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãreconn. automatique         äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ãí    DE (#$@[\\]^_`{|}~)    îäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ãí Sprache/language francaisîäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"åææææææææææææææææææææææææææææçéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ëìììììììììììììììììììììììììììììê›››››››››››››››››››››››››››››››"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"àááááááááááááááááááááááááááááâè««««««««««««««««««««««««««««««›"
		"ã      Configuratiemenu      äé«¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤«›"
		"åææææææææææææææææææææææææææææçé««««««««««««««««««««««««««««««›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã1 Favorieten: toev. pag 111 äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãíîñò                        äéËÈÈ¨¨ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã+-?                         äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã2     Teletekst-selectie    äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãí                          îäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã     Beeldschermformaat     äéËÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ã3   Standaardmode 16:9      äéË¤ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã4   Tekst/TV mode 16:9      äéË¤ÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã5        Helderheid         äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ã  Verminderen met 1/3       äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ã                            äéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ã6    nationale tekenset     äéË¤ÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇÇË›"
		"ãautomatische erkenning      äéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ãí    DE (#$@[\\]^_`{|}~)    îäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"ãí Sprache/Language nederl. îäéËÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈÈË›"
		"åææææææææææææææææææææææææææææçéËËËËËËËËËËËËËËËËËËËËËËËËËËËËËË›"
		"ëìììììììììììììììììììììììììììììê›››››››››››››››››››››››››››››››"
	}
};

const char catchmenutext[][80] =
{
	{ "        íïðî w{hlen   ñò anzeigen       "
	  "¤¤¤¤¤¤¤¤¨¨¨¨¤¤¤¤¤¤¤¤¤¤¨¨¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤" },
	{ "        íïðî select   ñò show           "
	  "¤¤¤¤¤¤¤¤¨¨¨¨¤¤¤¤¤¤¤¤¤¤¨¨¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤" },
	{ "  íïðî selectionner   ñò montrer        "
	  "¤¤¨¨¨¨¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¨¨¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤" },
	{ "        íïðî kiezen   ñò tonen          "
	  "¤¤¤¤¤¤¤¤¨¨¨¨¤¤¤¤¤¤¤¤¤¤¨¨¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤" }
};

const char message_3[][38] =
{
	{ "ã   suche nach Teletext-Anbietern   äé" },
	{ "ã  searching for teletext Services  äé" },
	{ "ã  recherche des services teletext  äé" },
	{ "ã zoeken naar teletekst aanbieders  äé" }
};
const char message_3_blank[] = "ã                                   äé";
const char message_7[][38] =
{
	{ "ã kein Teletext auf dem Transponder äé" },
	{ "ã   no teletext on the transponder  äé" },
	{ "ã pas de teletext sur le transponderäé" },
	{ "ã geen teletekst op de transponder  äé" }
};
const char message_8[][38] =
{
/*    00000000001111111111222222222233333333334 */
/*    01234567890123456789012345678901234567890 */
	{ "ã  warte auf Empfang von Seite 100  äé" },
	{ "ã waiting for reception of page 100 äé" },
	{ "ã attentre la réception de page 100 äé" },
	{ "ãwachten op ontvangst van pagina 100äé" }
};
const char message8pagecolumn[] = /* last(!) column of page to show in each language */
{
	33, 34, 34, 35
};
const char message_9[][38] =
{
/*    0000000000111111111122222222223 */
/*    0123456789012345678901234567890 */
	{ "ã     Seite 100 existiert nicht!    äé" },
	{ "ã      Page 100 does not exist!     äé" },
	{ "ã      Page 100 n'existe pas!       äé" },
	{ "ã    Pagina 100 bestaat niet!       äé" } 
};
#define MESSAGE9PAGECOLUMN 14

/* buffers */
unsigned char  lcd_backbuffer[120*64 / 8];
unsigned char  timestring[8];
unsigned char  page_char[PAGESIZE];
unsigned short page_atrb[PAGESIZE]; /*  ?????:h:cc:bbbb:ffff -> ?=reserved, h=double height, c=charset (0:G0 / 1:G1c / 2:G1s), b=background, f=foreground */

/* cachetables */
unsigned char *cachetable[0x900][0x80];
unsigned char subpagetable[0x900];
unsigned char countrycontrolbitstable[0x900][0x80];

/* colormaps */
unsigned short rd1[] = {0x01<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0x00<<8};
unsigned short gn1[] = {0x01<<8, 0x00<<8, 0xFF<<8, 0xFF<<8, 0x00<<8, 0x00<<8, 0xFF<<8, 0xFF<<8, 0x00<<8, 0x20<<8, 0x10<<8, 0x20<<8, 0x00<<8};
unsigned short bl1[] = {0x01<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0xFF<<8, 0xFF<<8, 0xFF<<8, 0xFF<<8, 0x00<<8, 0x40<<8, 0x20<<8, 0x40<<8, 0x00<<8};
unsigned short tr1[] = {0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0xFFFF , 0x0000 , 0x0000 , 0x0A00 , 0x3000 };
struct fb_cmap colormap_1 = {1, 13, rd1, gn1, bl1, tr1};

unsigned short rd2[] = {0x01<<8, 0xA8<<8, 0x00<<8, 0xA8<<8, 0x00<<8, 0xA8<<8, 0x00<<8, 0xA8<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0x00<<8};
unsigned short gn2[] = {0x01<<8, 0x00<<8, 0xA8<<8, 0xA8<<8, 0x00<<8, 0x00<<8, 0xA8<<8, 0xA8<<8, 0x00<<8, 0x20<<8, 0x10<<8, 0x20<<8, 0x00<<8};
unsigned short bl2[] = {0x01<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0xA8<<8, 0xA8<<8, 0xA8<<8, 0xA8<<8, 0x00<<8, 0x40<<8, 0x20<<8, 0x40<<8, 0x00<<8};
unsigned short tr2[] = {0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0x0000 , 0xFFFF , 0x0000 , 0x0000 , 0x0A00 , 0x3000 };
struct fb_cmap colormap_2 = {1, 13, rd2, gn2, bl2, tr2};

/* hamming table */
const unsigned char dehamming[] =
{
	0x01, 0xFF, 0x01, 0x01, 0xFF, 0x00, 0x01, 0xFF, 0xFF, 0x02, 0x01, 0xFF, 0x0A, 0xFF, 0xFF, 0x07,
	0xFF, 0x00, 0x01, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x06, 0xFF, 0xFF, 0x0B, 0xFF, 0x00, 0x03, 0xFF,
	0xFF, 0x0C, 0x01, 0xFF, 0x04, 0xFF, 0xFF, 0x07, 0x06, 0xFF, 0xFF, 0x07, 0xFF, 0x07, 0x07, 0x07,
	0x06, 0xFF, 0xFF, 0x05, 0xFF, 0x00, 0x0D, 0xFF, 0x06, 0x06, 0x06, 0xFF, 0x06, 0xFF, 0xFF, 0x07,
	0xFF, 0x02, 0x01, 0xFF, 0x04, 0xFF, 0xFF, 0x09, 0x02, 0x02, 0xFF, 0x02, 0xFF, 0x02, 0x03, 0xFF,
	0x08, 0xFF, 0xFF, 0x05, 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0x02, 0x03, 0xFF, 0x03, 0xFF, 0x03, 0x03,
	0x04, 0xFF, 0xFF, 0x05, 0x04, 0x04, 0x04, 0xFF, 0xFF, 0x02, 0x0F, 0xFF, 0x04, 0xFF, 0xFF, 0x07,
	0xFF, 0x05, 0x05, 0x05, 0x04, 0xFF, 0xFF, 0x05, 0x06, 0xFF, 0xFF, 0x05, 0xFF, 0x0E, 0x03, 0xFF,
	0xFF, 0x0C, 0x01, 0xFF, 0x0A, 0xFF, 0xFF, 0x09, 0x0A, 0xFF, 0xFF, 0x0B, 0x0A, 0x0A, 0x0A, 0xFF,
	0x08, 0xFF, 0xFF, 0x0B, 0xFF, 0x00, 0x0D, 0xFF, 0xFF, 0x0B, 0x0B, 0x0B, 0x0A, 0xFF, 0xFF, 0x0B,
	0x0C, 0x0C, 0xFF, 0x0C, 0xFF, 0x0C, 0x0D, 0xFF, 0xFF, 0x0C, 0x0F, 0xFF, 0x0A, 0xFF, 0xFF, 0x07,
	0xFF, 0x0C, 0x0D, 0xFF, 0x0D, 0xFF, 0x0D, 0x0D, 0x06, 0xFF, 0xFF, 0x0B, 0xFF, 0x0E, 0x0D, 0xFF,
	0x08, 0xFF, 0xFF, 0x09, 0xFF, 0x09, 0x09, 0x09, 0xFF, 0x02, 0x0F, 0xFF, 0x0A, 0xFF, 0xFF, 0x09,
	0x08, 0x08, 0x08, 0xFF, 0x08, 0xFF, 0xFF, 0x09, 0x08, 0xFF, 0xFF, 0x0B, 0xFF, 0x0E, 0x03, 0xFF,
	0xFF, 0x0C, 0x0F, 0xFF, 0x04, 0xFF, 0xFF, 0x09, 0x0F, 0xFF, 0x0F, 0x0F, 0xFF, 0x0E, 0x0F, 0xFF,
	0x08, 0xFF, 0xFF, 0x05, 0xFF, 0x0E, 0x0D, 0xFF, 0xFF, 0x0E, 0x0F, 0xFF, 0x0E, 0x0E, 0xFF, 0x0E
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
/* Local Variables: */
/* indent-tabs-mode:t */
/* tab-width:3 */
/* c-basic-offset:3 */
/* comment-column:0 */
/* fill-column:120 */
/* End: */
