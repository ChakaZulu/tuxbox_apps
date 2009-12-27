/******************************************************************************
 *                      <<< TuxTxt - Teletext Plugin >>>                      *
 *                                                                            *
 *             (c) Thomas "LazyT" Loewe 2002-2003 (LazyT@gmx.net)             *
 *                                                                            *
 *    continued 2004-2005 by Roland Meier <RolandMeier@Siemens.com>           *
 *                       and DBLuelle <dbluelle@blau-weissoedingen.de>        *
 *	russian and arabic support by Leonid Protasov <Lprot@mail.ru>         *
 *                                                                            *
 ******************************************************************************/

#define TUXTXT_CFG_STANDALONE 0  // 1:plugin only 0:use library
#define TUXTXT_DEBUG 0

#include <config.h>

#ifdef HAVE_DBOX_HARDWARE
#include <tuxbox.h>
#endif
#if HAVE_DVB_API_VERSION >= 3
#include <linux/input.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>

#include <linux/fb.h>


#include <sys/ioctl.h>
#include <sys/mman.h>

#ifndef HAVE_TRIPLEDRAGON
#include <dbox/fp.h>
#include <dbox/lcd-ks0713.h>
#endif
#include <plugin.h>

#include "tuxtxt_def.h"


#if TUXTXT_CFG_STANDALONE
#include "tuxtxt_common.h"
#else
/* variables and functions from libtuxtxt */
extern tuxtxt_cache_struct tuxtxt_cache;
extern tstPageAttr tuxtxt_atrtable[];
extern int tuxtxt_init();
extern void tuxtxt_close();
extern int  tuxtxt_start(int tpid);  // Start caching
extern int  tuxtxt_stop(); // Stop caching
extern void tuxtxt_next_dec(int *i); /* skip to next decimal */
extern void tuxtxt_prev_dec(int *i); /* counting down */
extern int tuxtxt_is_dec(int i);
extern int tuxtxt_next_hex(int i);
extern void tuxtxt_decode_btt();
extern void tuxtxt_decode_adip(); /* additional information table */
extern void tuxtxt_compress_page(int p, int sp, unsigned char* buffer);
extern void tuxtxt_decompress_page(int p, int sp, unsigned char* buffer);
extern void tuxtxt_hex2str(char *s, unsigned int n);
extern tstPageinfo* tuxtxt_DecodePage(int showl25, unsigned char* page_char, tstPageAttr *page_atrb, int hintmode, int showflof);
extern void tuxtxt_FillRect(unsigned char *lfb, int xres, int x, int y, int w, int h, int color);
extern int tuxtxt_RenderChar(unsigned char *lfb, int xres,int Char, int *pPosX, int PosY, tstPageAttr *Attribute, int zoom, int curfontwidth, int curfontwidth2, int fontheight, int transpmode, unsigned char* axdrcs, int ascender);
extern void tuxtxt_RenderDRCS(int xres,unsigned char *s,unsigned char *d,unsigned char *ax, unsigned char fgcolor, unsigned char bgcolor);
#if TUXTXT_DEBUG
extern int tuxtxt_get_zipsize(int p, int sp);
#endif
extern void tuxtxt_RenderPage(tstRenderInfo* renderinfo);
extern void tuxtxt_SetPosX(tstRenderInfo* renderinfo, int column);
extern void tuxtxt_RenderCharFB(tstRenderInfo* renderinfo, int Char, tstPageAttr *Attribute);
extern void tuxtxt_ClearBB(tstRenderInfo* renderinfo,int color);
extern void tuxtxt_ClearFB(tstRenderInfo* renderinfo,int color);
extern void tuxtxt_setcolors(tstRenderInfo* renderinfo,unsigned short *pcolormap, int offset, int number);
extern int tuxtxt_SetRenderingDefaults(tstRenderInfo* renderinfo);
extern int tuxtxt_InitRendering(tstRenderInfo* renderinfo,int setTVFormat);
extern void tuxtxt_EndRendering(tstRenderInfo* renderinfo);
extern void tuxtxt_CopyBB2FB(tstRenderInfo* renderinfo);
extern void tuxtxt_SwitchScreenMode(tstRenderInfo* renderinfo,int newscreenmode);
#endif


#define TUXTXTCONF CONFIGDIR "/tuxtxt/tuxtxt2.conf"


#define fontwidth_small_lcd 8




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
//#define PageNotFound    1
#define ShowServiceName 2
#define NoServicesFound 3






/* national subsets */
const char countrystring[] =
"         Default          "   /*  0 no subset specified */
"       Czech/Slovak       "   /*  1 czech, slovak */
"         English          "   /*  2 english */
"         Estonian         "   /*  3 estonian */
"          French          "   /*  4 french */
"         Deutsch          "   /*  5 german */
"         Italian          "   /*  6 italian */
"    Latvian/Lithuanian    "   /*  7 latvian, lithuanian */
"          Polish          "   /*  8 polish */
"    Portuguese/Spanish    "   /*  9 portuguese, spanish */
"         Romanian         "   /* 10 romanian */
"Serbian/Croatian/Slovenian"   /* 11 serbian, croatian, slovenian */
"Swedish/Finnish/Hungarian "   /* 12 swedish, finnish, hungarian */
"          T~rk}e          "   /* 13 turkish */
"      Srpski/Hrvatski     "   /* 14 serbian, croatian */
"     Russkij/Bylgarski    "   /* 15 russian, bulgarian */
"        Ukra&nsxka        "   /* 16 ukrainian */
"         Ekkgmij\\         "   /* 17 greek */
"          zixar           "   /* 18 hebrew */
"           pHQY           "   /* 19 arabic */
;
#define COUNTRYSTRING_WIDTH 26
#define MAX_NATIONAL_SUBSET (sizeof(countrystring) / COUNTRYSTRING_WIDTH - 1)

#if (FREETYPE_MAJOR > 2 || (FREETYPE_MAJOR == 2 && (FREETYPE_MINOR > 1 || (FREETYPE_MINOR == 1 && FREETYPE_PATCH >= 8))))
#define FT_NEW_CACHE_API
#endif

/* some data */
char versioninfo[16];
int hotlist[10];
int maxhotlist;

int rc, lcd;
int lastpage;
int savedscreenmode;
char dumpl25;
int catch_row, catch_col, catched_page;
int swapupdown, menulanguage;
int pids_found, current_service, getpidsdone;
int SDT_ready;
int pc_old_row, pc_old_col;     /* for page catching */
int temp_page;	/* for page input */
char saveconfig, hotlistchanged;
tstRenderInfo renderinfo;
struct timeval tv_delay;
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
#ifndef HAVE_TRIPLEDRAGON
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
#endif

/* language dependent texts */
#define MAXMENULANGUAGE 10 /* 0 deutsch, 1 englisch, 2 franz�sisch, 3 niederl�ndisch, 4 griechisch, 5 italienisch, 6 polnisch, 7 schwedisch, 8 suomi, 9 portuguesa, 10 russian */
const int menusubset[] =   { NAT_DE   , NAT_UK    , NAT_FR       , NAT_UK          , NAT_GR      , NAT_IT       , NAT_PL    , NAT_SW      , NAT_SW ,   NAT_SP,      NAT_RB};


#define Menu_StartX (renderinfo.StartX + renderinfo.fontwidth*9/2)
#define Menu_StartY (renderinfo.StartY + renderinfo.fontheight)
#define Menu_Height 24
#define Menu_Width 31

const char MenuLine[] =
{
	3,8,11,12,15,17,19,20,21
};

enum
{
	M_HOT=0,
	M_PID,
	M_SC1,
	M_SC2,
	M_COL,
	M_TRA,
	M_AUN,
	M_NAT,
	M_LNG,
	M_Number
};

#define M_Start M_HOT
#define M_MaxDirect M_AUN

const char hotlistpagecolumn[] =	/* last(!) column of page to show in each language */
{
	22, 26, 28, 27, 28, 27, 28, 21, 20, 26, 26
};
const char hotlisttextcolumn[] =
{
	24, 14, 14, 15, 14, 15, 14, 23, 22, 14, 14
};
const char hotlisttext[][2*5] =
{
	{ "dazu entf." },
	{ " add rem. " },
	{ "ajoutenlev" },
	{ "toev.verw." },
	{ "pq|shava_q" },
	{ "agg. elim." },
	{ "dodajkasuj" },
	{ "ny   bort " },
	{ "lis{{pois " },
	{ " adi rem. " },
	{ "Dob. Udal." },	
};

const char configonoff[][2*3] =
{
	{ "ausein" },
	{ "offon " },
	{ "desact" },
	{ "uitaan" },
	{ "emeape" },
	{ "offon " },
	{ "wy}w} " },
	{ "p} av " },
	{ "EI ON " },
	{ "offon " },
	{ "w&kwkl" },
};
const char menuatr[Menu_Height*Menu_Width] =
{
	"0000000000000000000000000000002"
	"0111111111111111111111111111102"
	"0000000000000000000000000000002"
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
	"3144444444444444444444444444432"
	"3555555555555555555555555555532"
	"3144444444444444444444444444432"
	"3555555555555555555555555555532"
	"3555555555555555555555555555532"
	"3555555555555555555555555555532"
	"3334444444444444444444444443332"
	"2222222222222222222222222222222"
};
const char configmenu[][Menu_Height*Menu_Width] =
{
	{
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
		"�������������������������������"
		"�     Konfigurationsmen}     ��"
		"�������������������������������"
		"�1 Favoriten: Seite 111 dazu ��"
		"�����                        ��"
		"�+-?                         ��"
		"�                            ��"
		"�2     Teletext-Auswahl      ��"
		"��          suchen          ���"
		"�                            ��"
		"�      Bildschirmformat      ��"
		"�3  Standard-Modus 16:9      ��"
		"�4  TextBild-Modus 16:9      ��"
		"�                            ��"
		"�5        Helligkeit         ��"
		"��                          ���"
		"�6       Transparenz         ��"
		"��                          ���"
		"�7  nationaler Zeichensatz   ��"
		"�automatische Erkennung      ��"
		"��                          ���"
		"�� Sprache/Language deutsch ���"
		"��   www.tuxtxt.net  x.xx   ���"
		"�������������������������������"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"�������������������������������"
		"�     Configuration menu     ��"
		"�������������������������������"
		"�1 Favorites:  add page 111  ��"
		"�����                        ��"
		"�+-?                         ��"
		"�                            ��"
		"�2     Teletext selection    ��"
		"��          search          ���"
		"�                            ��"
		"�        Screen format       ��"
		"�3 Standard mode 16:9        ��"
		"�4 Text/TV mode  16:9        ��"
		"�                            ��"
		"�5        Brightness         ��"
		"��                          ���"
		"�6       Transparency        ��"
		"��                          ���"
		"�7   national characterset   ��"
		"� automatic recognition      ��"
		"��                          ���"
		"�� Sprache/language english ���"
		"��   www.tuxtxt.net  x.xx   ���"
		"�������������������������������"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"�������������������������������"
		"�    Menu de configuration   ��"
		"�������������������������������"
		"�1 Favorites: ajout. page 111��"
		"�����                        ��"
		"�+-?                         ��"
		"�                            ��"
		"�2  Selection de teletext    ��"
		"��        recherche         ���"
		"�                            ��"
		"�      Format de l'#cran     ��"
		"�3 Mode standard 16:9        ��"
		"�4 Texte/TV      16:9        ��"
		"�                            ��"
		"�5          Clarte           ��"
		"��                          ���"
		"�6       Transparence        ��"
		"��                          ���"
		"�7     police nationale      ��"
		"�reconn. automatique         ��"
		"��                          ���"
		"�� Sprache/language francais���"
		"��   www.tuxtxt.net  x.xx   ���"
		"�������������������������������"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"�������������������������������"
		"�      Configuratiemenu      ��"
		"�������������������������������"
		"�1 Favorieten: toev. pag 111 ��"
		"�����                        ��"
		"�+-?                         ��"
		"�                            ��"
		"�2     Teletekst-selectie    ��"
		"��          zoeken          ���"
		"�                            ��"
		"�     Beeldschermformaat     ��"
		"�3   Standaardmode 16:9      ��"
		"�4   Tekst/TV mode 16:9      ��"
		"�                            ��"
		"�5        Helderheid         ��"
		"��                          ���"
		"�6       Transparantie       ��"
		"��                          ���"
		"�7    nationale tekenset     ��"
		"�automatische herkenning     ��"
		"��                          ���"
		"�� Sprache/Language nederl. ���"
		"��   www.tuxtxt.net  x.xx   ���"
		"�������������������������������"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"�������������������������������"
		"�      Lemo} quhl_seym       ��"
		"�������������������������������"
		"�1 Vaboq_:    pqo_h. sek. 111��"
		"�����                        ��"
		"�+-?                         ��"
		"�                            ��"
		"�2     Epikoc^ Teket]nt      ��"
		"��        Amaf^tgsg         ���"
		"�                            ��"
		"�       Loqv^ oh|mgr         ��"
		"�3 Tq|por pq|tupor   16:9    ��"
		"�4 Tq|por eij. jeil. 16:9    ��"
		"�                            ��"
		"�5       Kalpq|tgta          ��"
		"��                          ���"
		"�6       Diav\\meia           ��"
		"��                          ���"
		"�7    Ehmij^ tuposeiq\\       ��"
		"�aut|latg amacm~qisg         ��"
		"��                          ���"
		"�� Ck~ssa/Language ekkgmij\\ ���"
		"��   www.tuxtxt.net  x.xx   ���"
		"�������������������������������"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"�������������������������������"
		"�   Menu di configurazione   ��"
		"�������������������������������"
		"�1  Preferiti:  agg. pag.111 ��"
		"�����                        ��"
		"�+-?                         ��"
		"�                            ��"
		"�2   Selezione televideo     ��"
		"��         ricerca          ���"
		"�                            ��"
		"�      Formato schermo       ��"
		"�3  Modo standard 16:9       ��"
		"�4  Text/Mod.TV 16:9         ��"
		"�                            ��"
		"�5        Luminosit{         ��"
		"��                          ���"
		"�6        Trasparenza        ��"
		"��                          ���"
		"�7   nazionalita'caratteri   ��"
		"� riconoscimento automatico  ��"
		"��                          ���"
		"�� Lingua/Language Italiana ���"
		"��   www.tuxtxt.net  x.xx   ���"
		"�������������������������������"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"�������������������������������"
		"�        Konfiguracja        ��"
		"�������������������������������"
		"�1 Ulubione : kasuj  str. 111��"
		"�����                        ��"
		"�+-?                         ��"
		"�                            ��"
		"�2     Wyb_r telegazety      ��"
		"��          szukaj          ���"
		"�                            ��"
		"�       Format obrazu        ��"
		"�3 Tryb standard 16:9        ��"
		"�4 Telegazeta/TV 16:9        ��"
		"�                            ��"
		"�5          Jasno|^          ��"
		"��                          ���"
		"�6      Prze~roczysto|^      ��"
		"��                          ���"
		"�7 Znaki charakterystyczne   ��"
		"� automatyczne rozpozn.      ��"
		"��                          ���"
		"��  J`zyk/Language   polski ���"
		"��   www.tuxtxt.net  x.xx   ���"
		"�������������������������������"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"�������������������������������"
		"�     Konfigurationsmeny     ��"
		"�������������������������������"
		"�1 Favoriter: sida 111 ny    ��"
		"�����                        ��"
		"�+-?                         ��"
		"�                            ��"
		"�2      TextTV v{ljaren      ��"
		"��            s|k           ���"
		"�                            ��"
		"�        TV- format          ��"
		"�3 Standard l{ge 16:9        ��"
		"�4 Text/Bild l{ge  16:9      ��"
		"�                            ��"
		"�5        Ljusstyrka         ��"
		"��                          ���"
		"�6     Genomskinlighet       ��"
		"��                          ���"
		"�7nationell teckenupps{ttning��"
		"� automatisk igenk{nning     ��"
		"��                          ���"
		"�� Sprache/language svenska ���"
		"��   www.tuxtxt.net  x.xx   ���"
		"�������������������������������"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"�������������������������������"
		"�        Asetusvalikko       ��"
		"�������������������������������"
		"�1 Suosikit: sivu 111 lis{{  ��"
		"�����                        ��"
		"�+-?                         ��"
		"�                            ��"
		"�2   Tekstikanavan valinta   ��"
		"��          search          ���"
		"�                            ��"
		"�         N{ytt|tila         ��"
		"�3 Vakiotila     16:9        ��"
		"�4 Teksti/TV     16:9        ��"
		"�                            ��"
		"�5         Kirkkaus          ��"
		"��                          ���"
		"�6       L{pin{kyvyys        ��"
		"��                          ���"
		"�7   kansallinen merkist|    ��"
		"� automaattinen tunnistus    ��"
		"��                          ���"
		"�� Kieli            suomi   ���"
		"��   www.tuxtxt.net  x.xx   ���"
		"�������������������������������"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"�������������������������������"
		"�    Menu de Configuracao    ��"
		"�������������������������������"
		"�1 Favoritos:  adi pag. 111  ��"
		"�����                        ��"
		"�+-?                         ��"
		"�                            ��"
		"�2     Seleccao Teletext     ��"
		"��         Procurar         ���"
		"�                            ��"
		"�       formato ecran        ��"
		"�3 Standard mode 16:9        ��"
		"�4 Text/TV mode  16:9        ��"
		"�                            ��"
		"�5          Brilho           ��"
		"��                          ���"
		"�6      Transparencia        ��"
		"��                          ���"
		"�7  Caracteres nacionaist    ��"
		"�reconhecimento utomatico    ��"
		"��                          ���"
		"�� Lingua      Portuguesa   ���"
		"��   www.tuxtxt.net  x.xx   ���"
		"�������������������������������"
	},
/*     0000000000111111111122222222223 */
/*     0123456789012345678901234567890 */
	{
		"�������������������������������"
		"�        Konfiguraciq        ��"
		"�������������������������������"
		"�1 Faworit&:   dob str. 111  ��"
		"�����                        ��"
		"�+-?                         ��"
		"�                            ��"
		"�2     W&bor teleteksta      ��"
		"��           Poisk          ���"
		"�                            ��"
		"�       Format kartinki      ��"
		"�3 Stand. revim  16:9        ��"
		"�4 Tekst/TW rev. 16:9        ��"
		"�                            ��"
		"�5          Qrkostx          ��"
		"��                          ���"
		"�6       Prozra~nostx        ��"
		"��                          ���"
		"�7  Ispolxzuem&j alfawit     ��"
		"�      awtoopredelenie       ��"
		"��                          ���"
		"��  Qz&k:         Russkij   ���"
		"��   www.tuxtxt.net  x.xx   ���"
		"�������������������������������"
	},
};

const char catchmenutext[][80] =
{
	{ "        ���� w{hlen   �� anzeigen       "
	  "0000000011110000000000110000000000000000" },
	{ "        ���� select   �� show           "
	  "0000000011110000000000110000000000000000" },
	{ "  ���� selectionner   �� montrer        "
	  "0011110000000000000000110000000000000000" },
	{ "        ���� kiezen   �� tonen          "
	  "0000000011110000000000110000000000000000" },
	{ "        ���� epikoc^  �� pqobok^        "
	  "0000000011110000000000110000000000000000" },
	{ "        ����seleziona �� mostra         "
	  "0000000011110000000000110000000000000000" },
	{ "        ���� wybiez   �� wyswietl       "
	  "0000000011110000000000110000000000000000" },
	{ "        ���� v{lj     �� visa           "
     "0000000011110000000000110000000000000000" },
	{ "        ���� valitse  �� n{yt{          "
	  "0000000011110000000000110000000000000000" },
	{ "        ���� seleccao �� mostrar        "
	  "0000000011110000000000110000000000000000" },
	{ "        ���� w&bratx  �� pokazatx       "
	  "0000000011110000000000110000000000000000" },
};

const char message_3[][38] =
{
	{ "�   Suche nach Teletext-Anbietern   ��" },
	{ "�  Searching for teletext services  ��" },
	{ "�  Recherche des services teletext  ��" },
	{ "� Zoeken naar teletekst aanbieders  ��" },
	{ "�     amaf^tgsg voq]ym Teket]nt     ��" },
	{ "�     attesa opzioni televideo      ��" },
	{ "�  Poszukiwanie sygna}u telegazety  ��" },
	{ "�    s|ker efter TextTV tj{nster    ��" },
	{ "�   etsit{{n Teksti-TV -palvelua    ��" },
	{ "�  Procurar servicos de teletexto   ��" },
	{ "�   W&polnqetsq poisk teleteksta    ��" },
};
const char message_3_blank[] = "�                                   ��";
const char message_7[][38] =
{
	{ "� kein Teletext auf dem Transponder ��" },
	{ "�   no teletext on the transponder  ��" },
	{ "� pas de teletext sur le transponder��" },
	{ "� geen teletekst op de transponder  ��" },
	{ "� jal]la Teket]nt ston amaletadot^  ��" },
	{ "� nessun televideo sul trasponder   ��" },
	{ "�   brak sygna}u na transponderze   ��" },
	{ "� ingen TextTV p} denna transponder ��" },
	{ "�    Ei Teksti-TV:t{ l{hettimell{   ��" },
	{ "�  nao ha teletexto no transponder  ��" },
	{ "�  Na transpondere net teleteksta   ��" },	
};
const char message_8[][38] =
{
/*    00000000001111111111222222222233333333334 */
/*    01234567890123456789012345678901234567890 */
	{ "�  warte auf Empfang von Seite 100  ��" },
	{ "� waiting for reception of page 100 ��" },
	{ "� attentre la r�ception de page 100 ��" },
	{ "�wachten op ontvangst van pagina 100��" },
	{ "�     amal]my k^xg sek_dar 100      ��" },
	{ "�   attesa ricezione pagina 100     ��" },
	{ "�     oczekiwanie na stron` 100     ��" },
	{ "�  v{ntar p} mottagning av sida 100 ��" },
	{ "�        Odotetaan sivua 100        ��" },
	{ "�   esperando recepcao na pag 100   ��" },
	{ "�   Ovidanie priema stranic& 100    ��" },	
};
const char message8pagecolumn[] = /* last(!) column of page to show in each language */
{
	33, 34, 34, 35, 29, 30, 30, 34, 34, 32, 34
};



/* buffers */
unsigned char  lcd_backbuffer[120*64 / 8];

//unsigned short page_atrb[40 * 25]; /*  ?????:h:cc:bbbb:ffff -> ?=reserved, h=double height, c=charset (0:G0 / 1:G1c / 2:G1s), b=background, f=foreground */



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
void SwitchTranspMode();
void SwitchHintMode();
void RenderCatchedPage();
void RenderCharLCD(int Digit, int XPos, int YPos);
void RenderMessage(int Message);
void UpdateLCD();
int  Init();
int  GetNationalSubset(char *country_code);
int  GetTeletextPIDs();
int  GetRCCode();
/* Local Variables: */
/* indent-tabs-mode:t */
/* tab-width:3 */
/* c-basic-offset:3 */
/* comment-column:0 */
/* fill-column:120 */
/* End: */
