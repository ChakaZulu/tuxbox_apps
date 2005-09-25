/*
	TuxCom - TuxBox-Commander Plugin

	Copyright (C) 2004 'dbluelle' (dbluelle@blau-weissoedingen.de)

	Homepage: http://www.blau-weissoedingen.de/dreambox/

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#include <config.h>
#include <errno.h>
#include <locale.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <plugin.h>

#include <dbox/avs_core.h>
#include <dbox/saa7126_core.h>

#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H


#ifndef HAVE_DREAMBOX_HARDWARE
#include <linux/input.h>
#endif

#define AVS "/dev/dbox/avs0"
#define SAA "/dev/dbox/saa0"

#define MENUROWS      10
#define MENUITEMS     10
#define MENUSIZE       59
#define MINBOX        380
#define BUTTONWIDTH   114
#define BUTTONHEIGHT  30
#define COLORBUTTONS  4

#define LEFTFRAME    0
#define RIGHTFRAME   1

#define DEFAULT_PATH "/"
#define charset " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789#!$%&?*()@\\/=<>+-_,.;:"

#define FILEBUFFER_SIZE (100 * 1024) // Edit files up to 100k
#define FTPBUFFER_SIZE  (200 * 1024) // FTP Download Buffer size

#define MSG_VERSION    "Tuxbox Commander Version 1.10\n"
#define MSG_COPYRIGHT  "© dbluelle 2004-2005"
//rc codes

//rc codes
#ifdef HAVE_DREAMBOX_HARDWARE

#if TUXCOM_DBOX_VERSION < 3

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

#else
// rc codes


#define	RC_0			'0'
#define	RC_1			'1'
#define	RC_2			'2'
#define	RC_3			'3'
#define	RC_4			'4'
#define	RC_5			'5'
#define	RC_6			'6'
#define	RC_7			'7'
#define	RC_8			'8'
#define	RC_9			'9'

#define	RC_RIGHT	0x0191
#define	RC_LEFT		0x0192
#define	RC_UP			0x0193
#define	RC_DOWN		0x0194
#define	RC_PLUS		0x0195
#define	RC_MINUS	0x0196

#define	RC_OK				0x0D
#define	RC_STANDBY	0x1C
#define RC_ESC			RC_HOME

#define	RC_HOME			0x01B1
#define	RC_MUTE			0x01B2
#define	RC_HELP			0x01B3
#define	RC_DBOX			0x01B4

#define	RC_GREEN	0x01A1
#define	RC_YELLOW	0x01A2
#define	RC_RED		0x01A3
#define	RC_BLUE		0x01A4

#define RC_PAUSE	RC_HELP
#define RC_ALTGR	0x12
#define RC_BS			0x7F
#define RC_POS1		RC_HOME
#define RC_END		0x13
#define RC_INS		0x10
#define RC_ENTF		0x11
#define RC_STRG		0x00
#define RC_LSHIFT	0x0E
#define RC_RSHIFT	0x0E
#define RC_ALT		0x0F
#define RC_NUM		RC_DBOX
#define RC_ROLLEN	0x00
#define RC_F5			RC_DBOX
#define RC_F6			RC_HELP
#define RC_F7			RC_MUTE
#define RC_F8			0x01C8
#define RC_F9			0x01C9
#define RC_F10		0x01CA
#define RC_RET		0x0D
#define RC_RET1		0x01CC
#define RC_CAPSLOCK	0x01CD
#define RC_ON			0x01CE

#define RC_F1		RC_RED
#define RC_F2		RC_GREEN
#define RC_F3		RC_YELLOW
#define RC_F4		RC_BLUE
#define RC_PAGEUP	RC_PLUS
#define RC_PAGEDOWN	RC_MINUS

int rctable[] =
{
   0x00, RC_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'ß', '´', RC_BS, 0x09,
   'q',  'w', 'e', 'r', 't', 'z', 'u', 'i', 'o', 'p', 'ü', '+', RC_RET, RC_STRG, 'a', 's',
   'd',  'f', 'g', 'h', 'j', 'k', 'l', 'ö', 'ä', '^', RC_LSHIFT, '#', 'y', 'x', 'c', 'v',
   'b',  'n', 'm', ',', '.', '-', RC_RSHIFT, 0x00, RC_ALT, 0x20, RC_CAPSLOCK,RC_F1,RC_F2,RC_F3,RC_F4,RC_F5,
   RC_F6,RC_F7,RC_F8,RC_F9,RC_F10,RC_NUM,RC_ROLLEN,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, RC_STANDBY, 0x00, 0x00, 0x00, 0x00, '<', RC_OK, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, RC_ALTGR, 0x00, RC_POS1, RC_UP, RC_PAGEUP, RC_LEFT, RC_RIGHT, RC_END, RC_DOWN,RC_PAGEDOWN,RC_INS,RC_ENTF,
   0x00, RC_MUTE, RC_MINUS, RC_PLUS, RC_STANDBY, 0x00, 0x00, RC_PAUSE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
int rcshifttable[] =
{
   0x00, RC_ESC, '!', '"', '§', '$', '%', '&', '/', '(', ')', '=', '?', '`', 0x08, 0x09,
   'Q',  'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P', 'Ü', '*', RC_RET1, RC_STRG, 'A', 'S',
   'D',  'F', 'G', 'H', 'J', 'K', 'L', 'Ö', 'Ä', '°', RC_LSHIFT, 0x27, 'Y', 'X', 'C', 'V',
   'B',  'N', 'M', ';', ':', '_', RC_RSHIFT, 0x00, RC_ALT, 0x20, RC_CAPSLOCK,RC_F1,RC_F2,RC_F3,RC_F4,RC_F5,
   RC_F6,RC_F7,RC_F8,RC_F9,RC_F10,RC_NUM,RC_ROLLEN,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, RC_STANDBY, 0x00, 0x00, 0x00, 0x00, '>'
};
int rcaltgrtable[] =
{
   0x00, RC_ESC, 0x00, '²', '³', 0x00, 0x00, 0x00, '{', '[', ']', '}', '\\', 0x00, 0x00, 0x00,
   '@',  0x00, '€', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, '~', RC_RET1, RC_STRG, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, RC_LSHIFT, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00,  0x00, 'µ', 0x00, 0x00, 0x00, RC_RSHIFT, 0x00, RC_ALT, 0x20, RC_CAPSLOCK,RC_F1,RC_F2,RC_F3,RC_F4,RC_F5,
   RC_F6,RC_F7,RC_F8,RC_F9,RC_F10,RC_NUM,RC_ROLLEN,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, RC_STANDBY, 0x00, 0x00, 0x00, 0x00, '|'
};

// kb codes

#define KBC_UP		0x01
#define KBC_DOWN	0x02
#define KBC_RIGHT	0x03
#define KBC_LEFT	0x04
#define KBC_INS		0x05
#define KBC_DEL		0x06
#define KBC_POS1	0x07
#define KBC_BACKSPACE	0x7F
#define KBC_END		0x0A
#define KBC_PAGEUP	0x0B
#define KBC_PAGEDOWN	0x0C
#define KBC_RETURN	0x0D

#endif


//freetype stuff

#define FONT FONTDIR "/pakenham.ttf"
// if font is not in usual place, we look here:
#define FONT2 "/var/tuxbox/config/enigma/fonts/pakenham.ttf"

enum {LANG_INT,LANG_DE, LANG_IT, LANG_SV};
enum {RC_NORMAL,RC_EDIT};
enum {LEFT, CENTER, RIGHT};
enum {VERY_SMALL, SMALL, BIG};

FT_Library		library;
FTC_Manager		manager;
FTC_SBitCache		cache;
FTC_SBit		sbit;
#if FREETYPE_MAJOR  == 2 && FREETYPE_MINOR == 0
FTC_Image_Desc		desc;
#else
FTC_ImageTypeRec	desc;
#endif
FT_Face			face;
FT_UInt			prev_glyphindex;
FT_Bool			use_kerning;



enum {OK, OKCANCEL, OKHIDDENCANCEL,YESNOCANCEL,NOBUTTON,OVERWRITECANCEL,OVERWRITESKIPCANCEL,CANCELRUN};
enum {YES, NO, HIDDEN,CANCEL, OVERWRITE, SKIP, OVERWRITEALL,SKIPALL,EDIT, RENAME, SEARCHRESULT, EDITOR};
enum {GZIP,BZIP2,COMPRESS,TAR,FTP};

#define FONTHEIGHT_VERY_SMALL 20
#define FONTHEIGHT_SMALL      24
#define FONTHEIGHT_BIG        32
#define FONT_OFFSET           5
#define FONT_OFFSET_BIG       6
#define BORDERSIZE            5
//framebuffer stuff

enum {FILL, GRID};
enum {TRANSP, WHITE, BLACK, BLUE1, BLUE2, ORANGE, GREEN, YELLOW, RED, GRAY,GREEN2,GRAY2, BLUE_TRANSP, GRAY_TRANSP, BLUE3};

unsigned char *lfb = 0, *lbb = 0;

struct fb_fix_screeninfo fix_screeninfo;
struct fb_var_screeninfo var_screeninfo;

unsigned short rd[] = {0xFF<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0xE8<<8, 0xFF<<8, 0xb0<<8, 0x00<<8, 0x50<<8, 0x00<<8, 0x50<<8, 0x00<<8};
unsigned short gn[] = {0xFF<<8, 0x00<<8, 0x00<<8, 0x80<<8, 0xC0<<8, 0xd0<<8, 0xE8<<8, 0x00<<8, 0xb0<<8, 0xff<<8, 0x50<<8, 0x00<<8, 0x50<<8, 0x40<<8};
unsigned short bl[] = {0xFF<<8, 0x00<<8, 0x80<<8, 0xFF<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0xb0<<8, 0x00<<8, 0x50<<8, 0x80<<8, 0x50<<8, 0xff<<8};
unsigned short tr[] = {0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000 , 0x0000 , 0x0000 , 0x80ff , 0x80ff , 0x0000 };
struct fb_cmap colormap = {1, 14, rd, gn, bl, tr};


int trans_map     [] = {BLUE1,BLUE_TRANSP,TRANSP};
int trans_map_mark[] = {GRAY2,GRAY_TRANSP,GRAY_TRANSP};

#ifndef HAVE_DREAMBOX_HARDWARE
struct input_event ev;
#endif

unsigned short rccode;
char kbcode;

//some data

int avs, saa, fnc_old, saa_old, screenmode;
int rc, fb, kb;
int sx, ex, sy, ey;
int PosX, PosY, StartX, StartY, FrameWidth, NameWidth, SizeWidth;
int curframe, cursort, curvisibility;
int tool[MENUITEMS*2];
int colortool[COLORBUTTONS];
int overwriteall, skipall;
int textuppercase;

int framerows;
int viewx;
int viewy;
int menuitemwidth;
int menuitemnumber;

char szFileBuffer[FILEBUFFER_SIZE];
char* szCommand;
char* szZipCommand;
char tmpzipdir[256];
char szClipboard[256];
char szSearchstring[FILENAME_MAX];
char szTextSearchstring[FILENAME_MAX];
char szPass[20];
long commandsize;

int fncmodes[] = {AVS_FNCOUT_EXT43, AVS_FNCOUT_EXT169};
int saamodes[] = {SAA_WSS_43F, SAA_WSS_169F};

FILE *conf;
int language, langselect, autosave;

#define ACTION_NOACTION 0
#define ACTION_PROPS    1
#define ACTION_RENAME   2
#define ACTION_VIEW     3
#define ACTION_EDIT     4
#define ACTION_COPY     5
#define ACTION_MOVE     6
#define ACTION_MKDIR    7
#define ACTION_DELETE   8
#define ACTION_MKFILE   9
#define ACTION_MKLINK   10

#define ACTION_EXEC      1
#define ACTION_MARKER    2
#define ACTION_SORT      3
#define ACTION_REFRESH   4
#define ACTION_DELLINE   5
#define ACTION_INSLINE   6
#define ACTION_CLEAR     7
#define ACTION_UPPERCASE 8
#define ACTION_LOWERCASE 9
#define ACTION_KILLPROC  10
#define ACTION_TOLINUX   11
#define ACTION_MARKTEXT  12
#define ACTION_INSTEXT   13




#define BTN_OK            0
#define BTN_CANCEL        1
#define BTN_HIDDEN        2
#define BTN_YES           3
#define BTN_NO            4
#define BTN_OVERWRITE     5
#define BTN_SKIP          6
#define BTN_OVERWRITEALL  7
#define BTN_SKIPALL       8
#define BTN_RENAME        9
#define BTN_ASK           10
#define BTN_AUTO          11
#define BTN_GERMAN        12
#define BTN_ENGLISH       13
#define BTN_ITALIAN       14
#define BTN_SWEDISH       15

#define SORT_UP    1
#define SORT_DOWN -1

#define SELECT_NOCHANGE 0
#define SELECT_UPDIR    1
#define SELECT_ROOTDIR  2

#define SHOW_NO_OUTPUT    0
#define SHOW_OUTPUT       1
#define SHOW_SEARCHRESULT 2

#define REPEAT_TIMER 3

#define INI_VERSION 1

#define NUM_LANG 4

#define MAINMENU 7

enum {MSG_EXEC              ,
      MSG_EXEC_NOT_POSSIBLE ,
      MSG_COPY              ,
      MSG_COPY_MULTI        ,
      MSG_COPY_PROGRESS     ,
      MSG_COPY_NOT_POSSIBLE ,
      MSG_MOVE              ,
      MSG_MOVE_MULTI        ,
      MSG_MOVE_PROGRESS     ,
      MSG_DELETE            ,
      MSG_DELETE_MULTI      ,
      MSG_DELETE_PROGRESS   ,
      MSG_RENAME            ,
      MSG_MKDIR             ,
      MSG_MKFILE            ,
      MSG_MKLINK            ,
      MSG_COMMAND           ,
      MSG_SAVE              ,
      MSG_FILE_EXISTS       ,
      MSG_LINE              ,
      MSG_READ_ZIP_DIR      ,
      MSG_EXTRACT           ,
      MSG_FTP_NOCONN        ,
      MSG_FTP_CONN          ,
      MSG_FTP_ERROR         ,
      MSG_FTP_READDIR       ,
      MSG_KILLPROC          ,
      MSG_PROCESSID         ,
      MSG_PROCESSUSER       ,
      MSG_PROCESSNAME       ,
      MSG_CANCELDOWNLOAD    ,
      MSG_APPENDDOWNLOAD    ,
      MSG_SEARCHFILES       ,
      MSG_SAVESETTINGS		};

enum {INFO_COPY     ,
      INFO_MOVE     ,
      INFO_EXEC     ,
      INFO_MARKER   ,
      INFO_PROC     ,
      INFO_PASS1    ,
      INFO_PASS2    ,
      INFO_PASS3    ,
      INFO_PASS4    ,
      INFO_SEARCH1  ,
      INFO_SEARCH2  ,
      INFO_SAVED    ,
      INFO_ACCESSED ,
      INFO_MODIFIED ,
      INFO_CREATED  ,
      INFO_DATETIME };


char *numberchars[] = {  "0#!$%&?*()@\\",
                 		 "1/=<>+-_,.;:" ,
                 		 "abc2",
                 		 "def3",
                 		 "ghi4",
                 		 "jkl5",
                 		 "mno6",
                 		 "pqrs7",
                 		 "tuv8",
                 		 "wxyz9" };

char *info[]   = { "(select 'hidden' to copy in background)"               ,"('versteckt' wählen zum Kopieren im Hintergrund)"              ,"(Seleziona 'nascosto' per copiare in background)"              ,"(välj 'gömd' för att kopiera i bakgrunden)"        ,
                   "(select 'hidden' to move in background)"               ,"('versteckt' wählen zum Verschieben im Hintergrund)"           ,"(Seleziona 'nascosto' per muovere in background)"              ,"(välj 'gömd' för att flytta i bakgrund)"           ,
                   "(select 'hidden' to execute in background)"            ,"('versteckt' wählen zum Ausführen im Hintergrund)"             ,"(Seleziona 'nascosto' per eseguire in background)"             ,"(välj 'hidden' för att starta i bakgrunden)"       ,
                   "selected:%d"                                           ,"markiert:%d"                                                   ,"Seleziona:%d"                                                  ,"vald:%d"                                           ,
				   "Warning: killing a process can make your box unstable!","Warnung: Prozesse beenden kann die Box instabil werden lassen!","Attenzione: fermare un processo può rendere il DB instabile!"  ,"Varning: döda en process kan göra din box ostabil!",
				   "Please enter your password"                            ,"Bitte Passwort eingeben"                                       ,"Per fovore inserire la password"                               ,"Skriv in ditt lösenord"                            ,
				   "Please enter new password"                             ,"Bitte neues Passwort eingeben"                                 ,"Per fovore inserire la nuova password"                         ,"Skriv in ditt nya lösenord"                        ,
				   "Please enter new password again"                       ,"Bitte neues Passwort wiederholen"                              ,"Per fovore inserire la nuova password di nuovo"                ,"Skriv in ditt nya lösenord igen"                   ,
				   "password has been changed"                             ,"Passwort wurde geändert"                                       ,"La password è stata cambiata"                                  ,"lösenordet har ändrats"                            ,
				   "searching..."							               ,"Suche läuft..."                                                ,"Ricerca in corso..."                                           ,"Söker..."                                          ,
				   "search result"									       ,"Suchergebnis"                                                  ,"Risultato della ricerca"                                       ,"Sökresultat"                                       ,
				   "settings saved"                                        ,"Einstellungen gespeichert"                                     ,"Impostazioni salvate"                                          ,"Inställningar sparade"                             ,
				   "last access"                                           ,"letzter Zugriff"                                               ,"last access"                                                   ,"Senast öppnad"                                     ,
				   "last modified"                                         ,"letze Änderung"                                                ,"last modified"                                                 ,"Senast modifierad"                                 ,
				   "created"                                               ,"Erstellung"                                                    ,"created"                                                       ,"skapad"                                            ,
				   "%m/%d/%Y %H:%M:%S"                                     ,"%d.%m.%Y %H:%M:%S"                                             ,"%m/%d/%Y %H:%M:%S"                                             ,"%Y-%m-%d %H:%M:%S"                                 };

char *msg[]   = { "Execute '%s' ?"                             ,"'%s' ausführen ?"                                ,"Eseguire '%s'  ?"                                ,"Starta '%s' ?"                        ,
                  "Cannot execute file '%s'"                   ,"Kann '%s' nicht ausführen"                       ,"Impossibile eseguire il file '%s' "              ,"Kan inte starta fil"                  ,
                  "Copy '%s' to '%s' ?"                        ,"'%s' nach '%s' kopieren ?"                       ,"Copiare '%s' a '%s'  ?"                          ,"Kopiera '%s' till '%s'?"              ,
                  "Copy %d file(s) to '%s' ?"                  ,"%d Datei(en) nach '%s' kopieren ?"               ,"Copiare %d file in '%s'  ?"                      ,"Kopiera %d fil(er) till '%s'?"        ,
                  "Copying file '%s' to '%s'..."               ,"kopiere '%s' nach '%s' ..."                      ,"Sto copiando file '%s' in '%s' ..."              ,"Kopierar filen '%s' till '%s'..."     ,
                  "Cannot copy to same Directory"              ,"kann nicht in das gleiche Verzeichnis kopieren"  ,"Impossibile copiare alla stessa directory"       ,"Kan inte kopiera till samma mapp"     ,
                  "Move '%s' to '%s' ?"                        ,"'%s' nach '%s' verschieben ?"                    ,"Muovere '%s' in '%s' ?"                          ,"Flytta '%s' till '%s' ?"              ,
				  "Move %d file(s) to '%s' ?"                  ,"%d Datei(en) nach '%s' verschieben ?"            ,"Muovere %d file in '%s' ?"                       ,"Flytta %d fil(er) till '%s' ?"        ,
				  "Moving file '%s' to '%s'..."                ,"verschiebe '%s' nach '%s' ..."                   ,"Sto muovendo file '%s' in '%s' ..."              ,"Flyttar filen '%s' till '%s'..."      ,
				  "Delete '%s' ?"                              ,"'%s' löschen ?"                                  ,"Cancellare '%s' ?"                               ,"Radera '%s' ?"                        ,
				  "Delete %d files ?"                          ,"%d Datei(en) löschen ?"                          ,"Cancellare i %d file ?"                          ,"Radera %d fil(er) ?"                  ,
				  "Deleting file '%s'..."                      ,"lösche Datei '%s' ..."                           ,"Sto cancellando i file '%s' ..."                 ,"Raderar filen '%s'..."                ,
				  "rename file '%s' :"                         ,"Datei '%s' umbenennen:"                          ,"Rinominare il file '%s' :"                       ,"Byt namn på filen '%s'"               ,
				  "create new directory in '%s'"               ,"neues Verzeichnis in '%s' erstellen"             ,"Creare una nuova directory nella directory '%s'" ,"Skapa ny mapp i mappen '%s'"          ,
				  "create new file in directory '%s'"          ,"neue Datei in Verzeichnis '%s' erstellen"        ,"Creare un nuovo file '%s' nella directory"       ,"Skapa ny fil i mappen '%s'"           ,
				  "create link to '%s%s\' in directory '%s'"   ,"Verweis auf '%s%s' in Verzeichnis '%s' erstellen","Creare un link a '%s%s' nella directory '%s' "   ,"Skapa länk till '%s%s\' i mappen '%s'",
				  "execute linux command"                      ,"Linux-Kommando ausführen"                        ,"Eseguire un comando linux"                       ,"Exekvera Linux kommando"              ,
				  "save changes to '%s' ?"                     ,"Änderungen an '%s' speichern ?"                  ,"Salvare i cambiamenti a '%s' ?"                  ,"Spara ändringar till '%s'?"           ,
				  "file '%s' already exists"                   ,"Datei '%s' existiert bereits"                    ,"Il file '%s' esiste già"                         ,"Filen '%s' finns redan"               ,
				  "line %d of %d%s"                            ,"Zeile %d von %d%s"                               ,"Linea %d di %d%s"                                ,"Linje %d av %d%s"                     ,
				  "reading archive directory..."               ,"Lese Archiv-Verzeichnis..."                      ,"Sto leggendo la directory dell'archivio..."      ,"Läser arkivmapp..."                   ,
				  "extracting from file '%s'..."               ,"Entpacke aus Datei '%s'"                         ,"Sto estraendo dal file '%s'"                     ,"Extraherar från filen '%s'..."        ,
				  "no connection to"                           ,"Keine Verbindung zu"                             ,"Nessuna connessione"                             ,"Ingen anslutning till"                ,
				  "connecting to"                              ,"Verbinde mit"                                    ,"Mi sto connettendo"                              ,"Ansluten till"                        ,
				  "error in ftp command '%s%s'"                ,"Fehler bei FTP-Kommando '%s%s'"                  ,"Errore nel comando ftp '%s%s'"                   ,"Fel i FTP-kommando '%s%s'"            ,
				  "reading directory"                          ,"Lese Verzeichnis"                                ,"Sto leggendo la directory"                       ,"läser mappinformation"                ,
				  "Do you really want to kill process '%s'?"   ,"Wollen sie wirklich den Prozess '%s' beenden?"   ,"Vuoi davvero fermare il processo '%s' ?"         ,"Vill du verkligen döda process '%s'?" ,
				  "process id"                                 ,"Prozess ID"                                      ,"ID processo"                                     ,"process ID"                           ,
				  "owner"                                      ,"Besitzer"                                        ,"Proprietario"                                    ,"ägare"                                ,
				  "process"                                    ,"Prozess"                                         ,"Processo"                                        ,"process"                              ,
				  "cancel download ?"                          ,"Download abbrechen ?"                            ,"Cancellare Download ?"                           ,"avbryt nedladdning ?"                 ,
				  "append to file '%s' ?"                      ,"An Datei '%s' anhängen ?"                        ,"Aggiungere al file '%s' ?"                       ,"Lägg till i fil '%s' ?"               ,
				  "search in directory %s for file:"           ,"In Verzeichnis %s suchen nach Datei:"            ,"Sto cercando il file %s:"                        ,"sök i mappen %s efter fil:"           ,
				  "save current settings ?"                    ,"Einstellungen speichern ?"                       ,"Salvare le impostazioni correnti ?"              ,"spara nuvarande inställningar ?"      };

char *menuline[]  = { ""      , ""       ,""      ,""       ,
                      "rights", "Rechte" ,"Attrib","rätti." ,
                      "rename", "umben." ,"Rinom.","byt na.",
                      "view"  , "Ansicht","Vedi"  ,"visa"   ,
                      "edit"  , "bearb." ,"Edita" ,"ändra"  ,
                      "copy"  , "kopier.","Copia" ,"kopiera",
                      "move"  , "versch.","Muovi" ,"flytta" ,
                      "mkdir" , "mkdir"  ,"mkdir" ,"mkdir"  ,
                      "delete", "löschen","Canc." ,"radera" ,
                      "touch" , "neu"    ,"Crea"  ,"touch"  ,
                      "link"  , "Verw."  ,"Link"  ,"länk"   };

char *editorline[]= { ""      , ""       ,""         ,""       ,
                      ""      , ""       ,""         ,""       ,
                      ""      , ""       ,""         ,""       ,
                      "mark"  , "mark."  ,"Seleziona","markera",
                      ""      , ""       ,""         ,""       ,
                      "copy"  , "kopier.","Copia"    ,"kopiera",
                      "move"  , "versch.","Muovi"    ,"flytta" ,
                      ""      , ""       ,""         ,""       ,
                      "delete", "löschen","Cancella" ,"radera" ,
                      ""      , ""       ,""         ,""       ,
                      ""      , ""       ,""         ,""       };

char *colorline[] = { ""               , ""                     ,""                ,""                 ,
                      "execute command", "Kommando ausführen"   ,"Esegui comando"  ,"starta kommando"  ,
                      "toggle marker"  , "Datei markieren"      ,"Seleziona"       ,"växla markering"  ,
                      "sort directory" , "Verzeichnis sortieren","Ordina directory","sortera mapp"     ,
                      "refresh view"   , "Ansicht aktualisieren","Rivisualizza"    ,"uppdatera vy"     ,
                      "delete line"    , "Zeile löschen"        ,"Cancella riga"   ,"radera linje"     ,
                      "insert line"    , "Zeile einfügen"       ,"Inserisci riga"  ,"lägg till linje"  ,
                      "clear input"    , "Eingabe löschen"      ,"Cancella ins."   ,"rensa inmatning"  ,
                      "set uppercase"  , "Grossbuchstaben"      ,"Imposta su"      ,"sätt versaler"    ,
                      "set lowercase"  , "Kleinbuchstaben"      ,"Imposta giù"     ,"sätt gemener"     ,
                      "kill process"   , "Prozess beenden"      ,"Ferma processo"  ,"döda process"     ,
                      "to linux format", "in Linux-Format"      ,"A formato linux" ,"till Linux format",
                      "mark text"      , "Text markieren"       ,"Marca testo"     ,"markera text"     ,
                      "insert text"    , "Text einfügen"        ,"Inserisci testo" ,"lägg till text"   };

char *mbox[]     = { "OK"           , "OK"                ,"OK"                ,"OK"             ,
                     "Cancel"       , "Abbrechen"         ,"Annulla"           ,"Avbryt"         ,
                     "Hidden"       , "Versteckt"         ,"Nascosto"          ,"Gömd"           ,
                     "yes"          , "ja"                ,"Si"                ,"ja"             ,
                     "no"           , "nein"              ,"No"                ,"nej"            ,
                     "overwrite"    , "überschr."         ,"Sovrascrivi"       ,"skriv över"     ,
                     "skip"         , "überspringen"      ,"Salta"             ,"hoppa över"     ,
                     "overwrite all", "alle überschreiben","Sovrascivi tutto"  ,"skriv över alla",
                     "skip all"     , "alle überspringen" ,"Salta tutto"       ,"hoppa över alla",
                     "rename"       , "umben."            ,"Rinomina"          ,"byt namn"       ,
                     "ask"          , "nachfragen"        ,"Chiedi"            ,"fråga"          ,
                     "auto"			, "automatisch"       ,"automatico"        ,"auto"           ,
                     "Deutsch"      , "Deutsch"           ,"Deutsch"           ,"Deutsch"        ,
                     "english"      , "english"           ,"english"           ,"english"        ,
                     "Italiano"     , "Italiano"          ,"Italiano"          ,"Italiano"       ,
                     "svenska"      , "svenska"           ,"svenska"           ,"svenska"        };

char *props[]    = { "read"   , "lesen"    ,"Lettura"   ,"läs"     ,
                     "write"  , "schreiben","Scrittura" ,"skriv"   ,
                     "execute", "ausführen","Esecuzione","exekvera"};

char *ftpstr[]   = { "host"     , "Adresse"    ,"Host"       ,"serveraddress",
                     "port"     , "Port"       ,"Porta"      ,"port"         ,
                     "user"     , "Nutzer"     ,"Utente"     ,"användare"    ,
                     "password" , "Passwort"   ,"Password"   ,"lösenord"     ,
                     "directory", "Verzeichnis","Directory"  ,"mapp"         };

char *mainmenu[] = { "search files"                       , "Dateien suchen"                            ,"Cerca file"                                ,"sök filer"                           ,
                     "taskmanager"                        , "Prozessübersicht"                          ,"Taskmanager"                               ,"Processöversikt"                     ,
                     "toggle 16:9 mode"                   , "16:9-Modus setzen"                         ,"Passa a modalità 16:9"                     ,"växla 16:9 läge"                     ,
                     "set password"                       , "Passwort setzen"                           ,"Imposta password"                          ,"sätt lösenord"                       ,
                     "language/Sprache/Lingua/Språk: <%s>", "Sprache/language/Lingua/Språk: <%s>"       ,"Lingua/language/Sprache/Språk: <%s>"       ,"Lingua/language/Sprache/Språk: <%s>" ,
                     "save settings on exit: <%s>"        , "Einstellungen beim Beenden speichern: <%s>","Salvare le impostazioni in uscita: <%s>"   ,"spara inställningar vid avslut: <%s>",
                     "save settings now"                  , "Einstellungen jetzt speichern"             ,"Salvare le impostazioni adesso"            ,"spara inställningar nu"              };

struct fileentry
{
    char name[256];
	struct stat   fentry;
};
struct zipfileentry
{
    char name[FILENAME_MAX];
	struct stat   fentry;
	struct zipfileentry * next;
};
struct marker
{
    char name[256];
	struct marker* next;
};


struct frameinfo
{
	char           			path[FILENAME_MAX];
	int            			writable;
	int			  			sort;
	int           			markcount;
	unsigned long long  	marksize;
	long          			first;
	long		  			selected;
	unsigned long  			count;
	unsigned long long  	size;
	struct fileentry*		flist;
	struct marker * 		mlist;
	int						ziptype;
	char           			zipfile[FILENAME_MAX];
	char           			zippath[FILENAME_MAX];
	struct zipfileentry*	allziplist;
	FILE*                   ftpconn;
	struct sockaddr_in      s_in;
	char 					ftphost[512];
	int  					ftpport;
	char 					ftpuser[100];
	char 					ftppass[100];

};



struct frameinfo finfo[2];

//functions

void				SetPassword();
void 				RenderBox(int sx, int sy, int ex, int ey, int mode, int color);
void 	          	RenderFrame(int frame);
void 	          	RenderMenuLine(int highlight, int refresh);
void 	          	FillDir(int frame, int selmode);
struct fileentry* 	GetSelected(int frame);
void 				SetSelected(int frame, const char* szFile);
void 	          	GetSizeString(char* sizeString, unsigned long long size);
int 	          	MessageBox(const char* msg1,const char* msg2, int mode);
int 	          	GetInputString(int width, int maxchars, char* str, char * msg, int pass);
void	          	ClearEntries(int frame);
void 				ClearZipEntries(int frame);
void	          	ClearMarker(int frame);
void	          	RenameMarker(int frame, const char* szOld, const char* szNew);
void	          	ToggleMarker(int frame);
int               	IsMarked(int frame, int pos);
int 			  	CheckOverwrite(const char* szFile, int mode, char* szNew);
void	          	ReadSettings();
void	          	WriteSettings();
void	          	DoExecute(char* szAction, int showoutput);
int 				DoCopy(struct fileentry* pfe, int typ, int checkmode);
void 				DoZipCopyEnd();
int 				DoMove(char* szFile, int typ, int checktype);
void	          	DoViewFile();
void	          	DoEditFile(char* szFile, char* szTitle, int writable);
void	          	DoTaskManager();
int               	DoEditString(int x, int y, int width, int maxchars, char* str, int vsize, int back, int pass);
int 	          	ShowProperties();
void 		 	  	RenderButtons(int he, int mode);
int 			  	flistcmp(struct fileentry * p1, struct fileentry * p2);
struct fileentry* 	getfileentry(int frame, int pos);
struct fileentry* 	FindFile(int frame, const char* szFile);
void 			  	sortframe(int frame, char* szSel);
void 			  	ShowFile(FILE* pipe, char* szAction);
void 			  	ReadZip(int typ);
int					CheckZip(char* szName);
FILE*				OpenPipe(char* szAction);
void 				OpenFTP();
void 				ReadFTPDir(int frame, char* seldir);
int					FTPcmd(int frame, const char *s1, const char *s2, char *buf);
void 				DoEditFTP(char* szFile,char* szTitle);
void 				DoMainMenu();
void 				DoSearchFiles();
