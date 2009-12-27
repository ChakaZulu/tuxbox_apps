/*
 * $Id: tuxwetter.c,v 1.3 2009/12/27 12:08:02 rhabarber1848 Exp $
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

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include "tuxwetter.h"
#include "parser.h"
#include "bmps.h"
#include "text.h"
#include "io.h"
#include "gfx.h"
#include "jpeg.h"
#include "gif.h"
#include "pngw.h"
#include "php.h"
#include "gifdecomp.h"
#ifdef HAVE_DBOX_HARDWARE
#include <dbox/fb.h>
#endif
#include "fb_display.h"
#include "resize.h"
#include "http.h"
#include "lcd.h"
#include "color.h"

#define P_VERSION 3.56

char CONVERT_LIST[]="/var/tuxbox/config/tuxwetter//convert.list";
#define CFG_FILE 	"/var/tuxbox/config/tuxwetter/tuxwetter.conf"
#define MCF_FILE 	"/var/tuxbox/config/tuxwetter/tuxwetter.mcfg"
#define TIME_FILE	"/var/tuxbox/config/tuxwetter/swisstime"
#define MISS_FILE	"/var/tuxbox/config/tuxwetter/missing_translations.txt"
#define NCF_FILE 	"/var/tuxbox/config/neutrino.conf"
#define ECF_FILE    "/var/tuxbox/config/enigma/config"
#define BMP_FILE 	"tuxwettr.bmp"
#define JPG_FILE	"/tmp/picture.jpg"
#define GIF_FILE	"/tmp/picture.gif"
#define GIF_MFILE	"/tmp/gpic"
#define PNG_FILE	"/tmp/picture.png"
#define PHP_FILE	"/tmp/php.htm"
#define TMP_FILE	"/tmp/tuxwettr.tmp"
#define ICON_FILE	"/tmp/icon.gif"
#define TRANS_FILE	"/tmp/picture.html"
static char TCF_FILE[128]="";

#define LIST_STEP 	10
#define MAX_FUNCS 	 7
#define LCD_CPL 	12
#define LCD_RDIST 	10

// Forward defines
int gif_on_data(char *name, int xstart, int ystart, int xsize, int ysize, int wait, int single, int center, int rahmen);
char par[32]="1005530704", key[32]="a9c95f7636ad307b";
void TrimString(char *strg);

// Color table stuff
static char menucoltxt[CMH][24]={"Content_Selected_Text","Content_Selected","Content_Text","Content","Content_inactive_Text","inactive","Head_Text","Head"};

#ifdef HAVE_DBOX_HARDWARE

unsigned short rd[] = {0x00<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 
					   0xFF<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0xFF<<8};
unsigned short gn[] = {0x00<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0xC0<<8, 0x00<<8, 
					   0xFF<<8, 0x80<<8, 0x00<<8, 0x80<<8, 0xC0<<8, 0xFF<<8, 0xFF<<8, 0x00<<8};
unsigned short bl[] = {0x00<<8, 0x00<<8, 0xFF<<8, 0x80<<8, 0xFF<<8, 0x80<<8, 0x00<<8, 0x80<<8, 
					   0xFF<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0x00<<8};
unsigned short tr[] = {0xFFFF,  0xFFFF,  0x0000,  0x0A00,  0x0000,  0x0A00,  0x0000,  0x0000, 
					   0x0000,  0x0A00,  0xFFFF,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000 };

#else

// alternativ colortable with "enigma-style"

//  		         CMCST,   CMCS,    CMCT,    CMC,     CMCIT,   CMCI,    CMHT,    CMH	
//					   WHITE,   BLUE0,   TRANSP,   CMS,   ORANGE,  GREEN,   YELLOW,  RED
unsigned short rd[] = {0xFF<<8, 0x10<<8, 0xFF<<8, 0x29<<8, 0xFF<<8, 0x33<<8, 0xFF<<8, 0x10<<8, 
					   0xFF<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0xff<<8, 0xFF<<8};
unsigned short gn[] = {0xFF<<8, 0x12<<8, 0xFF<<8, 0x4a<<8, 0xFF<<8, 0x29<<8, 0xFF<<8, 0x12<<8, 
					   0xFF<<8, 0x80<<8, 0x00<<8, 0x80<<8, 0xC0<<8, 0xff<<8, 0xff<<8, 0x00<<8};
unsigned short bl[] = {0xFF<<8, 0x58<<8, 0xFF<<8, 0x6b<<8, 0xFF<<8, 0x4a<<8, 0xFF<<8, 0x58<<8, 
					   0xFF<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0x00<<8};
unsigned short tr[] = {0x0000,  0x0000,  0x0000,  0x3300,  0x0000,  0x6b00,  0x0000,  0x0000, 
					   0x0000,  0x0A00,  0xFFFF,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000 };

#endif

unsigned short ord[256], ogn[256], obl[256], otr[256], rtr[256];

struct fb_cmap /*colormap = {1, 16, rd, gn, bl, tr},*/ oldcmap = {0, 256, ord, ogn, obl, otr}, spcmap = {1, 16, rd, gn, bl, tr};

// Menu structure stuff
enum {TYP_MENU, TYP_CITY, TYP_PICTURE, TYP_PICHTML, TYP_TXTHTML, TYP_TEXTPAGE, TYP_TXTPLAIN, TYP_EXECUTE, TYP_ENDMENU, TYP_WEATH};
static char TYPESTR[TYP_WEATH+1][10]={"MENU=","Stadt=","PICTURE=","PICHTML=","TXTHTML=","TEXTPAGE=","TXTPLAIN=","EXECUTE=","ENDMENU"};
enum {PTYP_ASK, PTYP_JPG, PTYP_GIF, PTYP_PNG};
static char PTYPESTR[PTYP_PNG+1][5]={"","JPG","GIF","PNG"};
char *cmdline=NULL;
char *line_buffer=NULL;

typedef struct {char *entry; int headerpos; int type; int pictype; int repeat; int underline; int absolute;} LISTENTRY;
typedef LISTENTRY *PLISTENTRY;
typedef PLISTENTRY	*LIST;
typedef struct {int num_headers; int act_header; int max_header; char **headertxt; int *headerlevels; int *lastheaderentrys; int num_entrys; int act_entry; int max_entrys; LIST list;} MENU;

MENU menu;
MENU funcs;

int Check_Config(void);
int Clear_List(MENU *m, int mode);
int Get_Selection(MENU *m);
int AddListEntry(MENU *m, char *line, int pos);
int Get_Menu();
void ShowInfo(MENU *m);

// Misc
char NOMEM[]="Tuxwetter <Out of memory>\n";
unsigned char FONT[64]= "/share/fonts/pakenham.ttf";
unsigned char *lfb = 0, *lbb = 0;
int intype=0, show_icons=0, gmodeon=0, ctmo=0, metric=1, loadalways=0, radius=0;
char city_code[15] = "";
char city_name[50] = "";
unsigned int alpha=0x0202;
int show_splash=1;
unsigned char lastpicture[BUFSIZE]="";
unsigned char nstr[BUFSIZE]="";
unsigned char *trstr;
unsigned char *htmstr;
unsigned char *proxyadress=NULL, *proxyuserpwd=NULL;
char INST_FILE[]="/tmp/rc.locked";
char LCDL_FILE[]="/tmp/lcd.locked";
int instance=0;

int get_instance(void)
{
FILE *fh;
int rval=0;

	if((fh=fopen(INST_FILE,"r"))!=NULL)
	{
		rval=fgetc(fh);
		fclose(fh);
	}
	return rval;
}

void put_instance(int pval)
{
FILE *fh;

	if(pval)
	{
		if((fh=fopen(INST_FILE,"w"))!=NULL)
		{
			fputc(pval,fh);
			fclose(fh);
		}
		if(pval==1)
		{
			if((fh=fopen(LCDL_FILE,"w"))!=NULL)
			{
				fputc(0,fh);
				fclose(fh);
			}
		}
	}
	else
	{
		remove(INST_FILE);
		remove(LCDL_FILE);
	}
}

static void quit_signal(int sig)
{
	put_instance(get_instance()-1);
	printf("tuxwetter Version %.2f killed\n",P_VERSION);
	exit(1);
}

void xremove(char *fname)
{
FILE *fh;

	if((fh=fopen(fname,"r"))!=NULL)
		{
		fclose(fh);
		remove(fname);
		}
}

int Read_Neutrino_Cfg(char *entry)
{
FILE *nfh;
char tstr [512], *cfptr=NULL;
int rv=-1,styp=0;

	if((((nfh=fopen(NCF_FILE,"r"))!=NULL)&&(styp=1))||(((nfh=fopen(ECF_FILE,"r"))!=NULL))&&(styp=2))
	{
		tstr[0]=0;

		while((!feof(nfh)) && ((strstr(tstr,entry)==NULL) || ((cfptr=strchr(tstr,'='))==NULL)))
		{
			fgets(tstr,500,nfh);
		}
		if(!feof(nfh) && cfptr)
		{
			++cfptr;
			if(styp==1)
			{
				if(sscanf(cfptr,"%d",&rv)!=1)
				{
					if(strstr(cfptr,"true")!=NULL)
						rv=1;
					else if(strstr(cfptr,"false")!=NULL)
						rv=0;
					else
						rv=-1;
				}
			}
			if(styp==2)
			{
				if(sscanf(cfptr,"%x",&rv)!=1)
				{
					rv=-1;
				}
			}
//			printf("%s\n%s=%s -> %d\n",tstr,entry,cfptr,rv);
		}
		fclose(nfh);
	}
	return rv;
}

/******************************************************************************
 * ReadConf (0=fail, 1=done)
 ******************************************************************************/

int ReadConf(char *iscmd)
{
	FILE *fd_conf;
	char *cptr;

	//open config

	if((!strlen(TCF_FILE)) ||  (strlen(TCF_FILE) && !(fd_conf = fopen(TCF_FILE, "r"))))
	{
		if(!(fd_conf = fopen(CFG_FILE, "r")))
		{
			if(iscmd==NULL)
			{
				printf("Tuxwetter <unable to open Config-File>\n");
				return 0;
			}
		}
		else
		{
			strcpy(TCF_FILE,CFG_FILE);
		}
	}
	if(fd_conf)
	{
		fclose(fd_conf);
	}
	if(!(fd_conf = fopen(MCF_FILE, "r")))
	{
		fd_conf = fopen(TCF_FILE, "r");
	}

	while(fgets(line_buffer, BUFSIZE, fd_conf))
	{
		TrimString(line_buffer);

		if((line_buffer[0]) && (line_buffer[0]!='#') && (!isspace(line_buffer[0])) && ((cptr=strchr(line_buffer,'='))!=NULL))
		{
			if(strstr(line_buffer,"SplashScreen") == line_buffer)
				{
					sscanf(cptr+1,"%d",&show_splash);
				}
			if(strstr(line_buffer,"ShowIcons") == line_buffer)
				{
					sscanf(cptr+1,"%d",&show_icons);
				}
			if(strstr(line_buffer,"ProxyAdressPort") == line_buffer)
				{
					proxyadress=strdup(cptr+1);
				}
			if(strstr(line_buffer,"ProxyUserPwd") == line_buffer)
				{
					proxyuserpwd=strdup(cptr+1);
				}
			if(strstr(line_buffer,"ConnectTimeout") == line_buffer)
				{
					sscanf(cptr+1,"%d",&ctmo);
				}
			if(strstr(line_buffer,"Metric") == line_buffer)
				{
					sscanf(cptr+1,"%d",&metric);
				}
			if(strstr(line_buffer,"LoadAlways") == line_buffer)
				{
					sscanf(cptr+1,"%d",&loadalways);
				}
			if(strstr(line_buffer,"PartnerID") == line_buffer)
				{
					strncpy(par,cptr+1,sizeof(par)-1);
				}
			if(strstr(line_buffer,"LicenseKey") == line_buffer)
				{
					strncpy(key,cptr+1,sizeof(key)-1);
				}
			if(strstr(line_buffer,"InetConnection") == line_buffer)
				{
					if(strstr(cptr+1,"ISDN")!=NULL)
						{
							intype=1;
						}
					if(strstr(cptr+1,"ANALOG")!=NULL)
						{
							intype=2;
						}
				}
		}
	}

	return 1;
}

int Transform_Entry(char *src, char *trg)
{
int type=0,tret=-1,fcnt,fpos,tval,ferr,tsub;	
int noprint,rndval,loctime=0;
char /*pstr[512],nstr[512],dstr[50],*/fstr[5],*cptr,*tptr,*aptr;
time_t stime;
struct tm *ltime;

	type=0;
	tsub=0;
	*trg=0;
	if((cptr=strchr(src,','))==NULL)
	{
		return tret;
	}
	cptr++;
	aptr=strchr(cptr,'|');
	if(aptr)
	{
		while(aptr && ((aptr=strchr(aptr,'|'))!=NULL))
		{
			++aptr;
			rndval=0;
			if(*aptr=='L')
			{
				++aptr;
				loctime=1;
			}
			if(*aptr=='R')
			{
				++aptr;
				rndval=1;
			}
			if(*aptr=='N')
			{
				++aptr;
			}
			if(sscanf(aptr,"%d",&tval)==1)
			{
				while((aptr<(cptr+strlen(cptr))) && (*aptr=='-' || ((*aptr>='0') && (*aptr<='9'))))
				{ 
					++aptr;
				}
				if(!rndval)
				{
					switch(*aptr)
					{
						case 'Y': tsub+=tval*365*24*3600; break;
						case 'M': tsub+=tval*31*24*3600; break;
						case 'D': tsub+=tval*24*3600; break;
						case 'h': tsub+=tval*3600; break;
						case 'm': tsub+=tval*60; break;
						case 's': tsub+=tval; break;
					}
				}
			}
		}
		time(&stime);	
		stime-=tsub;
		if(loctime)
		{
			ltime=localtime(&stime);
		}
		else
		{
			ltime=gmtime(&stime);
		}	
		fpos=0;
		ferr=0;
		tval=0;
		while(*cptr>' ' && !ferr)
		{
			if(*cptr=='|')
			{
				noprint=0;
				rndval=0;
				++cptr;
				if(*cptr=='L')
				{
					++cptr;
				}
				if(*cptr=='N')
				{
					noprint=1;
					++cptr;
				}
				if(*cptr=='R')
				{
					++cptr;
					sscanf(cptr,"%d",&rndval);
					rndval=abs(rndval);
				}
				while(*cptr && ((*cptr=='-') || ((*cptr>='0') && (*cptr<='9'))))
				{
					cptr++;
				}
				tptr=cptr+1;
				fcnt=1;
				while(*tptr && (*tptr==*cptr))
				{
					fcnt++;
					tptr++;
				}
				sprintf(fstr,"%%0%dd",fcnt);
				switch(*cptr)
				{
					case 'Y':
						if(fcnt==4)
						{
							tval=1900+ltime->tm_year;
						}
						else
						{
							tval=ltime->tm_year-100;
						}
						break;
									
					case 'M':
						tval=ltime->tm_mon+1;
						break;
								
					case 'D':
						tval=ltime->tm_mday;
						break;
										
					case 'h':
						tval=ltime->tm_hour;
						break;
										
					case 'm':
						tval=ltime->tm_min;
						break;
										
					case 's':
						tval=ltime->tm_sec;
						break;
										
					default:
						ferr=1;
						break;
				}
				if(!ferr && !noprint)
				{
					if(rndval)
					{
						tval=((int)(tval/rndval))*rndval;
					}
					sprintf(trg+fpos,fstr,tval);
					fpos+=fcnt;
				}
				cptr=tptr;
			}
			else
			{
				*(trg+(fpos++))=*cptr;
				++cptr;
			}
		}
		*(trg+fpos)=0;
		tret=ferr;
	}
	else
	{
		ferr=0;
		tret=0;
		strcpy(trg,cptr);
		strcpy(nstr,src);
		if((cptr=strchr(nstr,','))!=NULL)
		{
			*cptr=0;
		}
	}
	return tret;
}


void TrimString(char *strg)
{
char *pt1=strg, *pt2=strg;

	while(*pt2 && *pt2<=' ')
	{
		++pt2;
	}
	if(pt1 != pt2)
	{
		do
		{
			*pt1=*pt2;
			++pt1;
			++pt2;
		}
		while(*pt2);
		*pt1=0;
	}
	while(strlen(strg) && strg[strlen(strg)-1]<=' ')
	{
		strg[strlen(strg)-1]=0;
	}
}


int Check_Config(void)
{
int rv=-1, level=0;
char *pt1;
FILE *fh;

	if((fh=fopen(TCF_FILE,"r"))!=NULL)
	{
		while(fgets(line_buffer, BUFSIZE, fh))
		{
			TrimString(line_buffer);
			if(strstr(line_buffer,TYPESTR[TYP_MENU])==line_buffer)
			{
				if(menu.num_headers>=menu.max_header)
				{
					if((menu.headertxt=realloc(menu.headertxt,(menu.max_header+LIST_STEP)*sizeof(char*)))==NULL)
					{
						printf(NOMEM);
						Clear_List(&menu,0);
						return rv;
					}
					memset(&menu.headertxt[menu.max_header],0,LIST_STEP*sizeof(char*));
					if((menu.headerlevels=realloc(menu.headerlevels,(menu.max_header+LIST_STEP)*sizeof(int)))==NULL)
					{
						printf(NOMEM);
						Clear_List(&menu,0);
						return rv;
					}
					if((menu.lastheaderentrys=realloc(menu.lastheaderentrys,(menu.max_header+LIST_STEP)*sizeof(int)))==NULL)
					{
						printf(NOMEM);
						Clear_List(&menu,0);
						return rv;
					}
				menu.max_header+=LIST_STEP;
				}
				pt1=strchr(line_buffer,'=');
				if(*(++pt1)=='*')
				{
					++pt1;
				}
				if(menu.headertxt[menu.num_headers])
				{
					free(menu.headertxt[menu.num_headers]);
					menu.headertxt[menu.num_headers]=NULL;
				}
				menu.headerlevels[menu.num_headers]=level++;
				menu.headertxt[menu.num_headers++]=strdup(pt1);
			}
			else
			{
				if(strstr(line_buffer,TYPESTR[TYP_ENDMENU])==line_buffer)
				{
				--level;
				}
			}
		}
		rv=0;
		fclose(fh);
	}
	return rv;
}

int Clear_List(MENU *m, int mode)
{
int i;
PLISTENTRY entr;

	if(m->max_entrys)
	{
		for(i=0; i<m->num_entrys; i++)
		{
			if(m->list[i]->entry) free(m->list[i]->entry);
			free(m->list[i]);
		}
		m->num_entrys=0;
		m->max_entrys=0;
		m->list=NULL;
	}
	switch(mode)
	{
		case 0: return 0;
		
		case 1:
	
			if((m->list=calloc(LIST_STEP,sizeof(PLISTENTRY)))==NULL)
			{
				printf(NOMEM);
				return -1;
			}

			for(i=0; i<LIST_STEP; i++)
			{
				if((entr=calloc(1,sizeof(LISTENTRY)))==NULL)
					{
					printf(NOMEM);
					Clear_List(m,0);
					return -1;
					}
				m->list[i]=entr;
			}
			m->max_entrys=LIST_STEP;
			break;
			
		case -1:
			if(m->num_headers && m->headertxt)
			{
				for(i=0; i<m->num_headers; i++)
				{
					free(m->headertxt[i]);
				}
				m->num_headers=0;
				m->list=NULL;
			}
			if(m->headertxt)
			{
				free(m->headertxt);
				m->headertxt=NULL;
			}
			break;
	}
	return 0;
}

int Get_Selection(MENU *m)
{
int rv=1,rccode, mloop=1,lrow,lpos;
char dstr[128],*lcptr,*lcstr,*lcdptr;

	LCD_Init();
	do{
		rccode=-1;
		ShowInfo(m);
		if(m->list[m->act_entry]->entry)
		{
			sprintf(trstr,"%s%s",(m->list[m->act_entry]->type==TYP_MENU)?"> ":"",m->list[m->act_entry]->entry);
			if((lcptr=strchr(trstr,','))!=NULL)
			{
				*lcptr=0;
			}
		}
		else
		{
			sprintf(trstr,"%s",prs_translate("Kein Eintrag",CONVERT_LIST));
		}
		lcstr=strdup(trstr);
		lcptr=lcdptr=lcstr;
		while(*lcptr)
		{
			if(*lcptr=='~')
			{
				++lcptr;
				if(*lcptr)
				{
					++lcptr;
				}
			}
			*(lcdptr++)=*(lcptr++);
		}
		*lcptr=0;
		LCD_Clear();
		LCD_draw_rectangle (0,0,119,59, LCD_PIXEL_ON,LCD_PIXEL_OFF);
		LCD_draw_rectangle (3,3,116,56, LCD_PIXEL_ON,LCD_PIXEL_OFF);
		lpos=strlen(lcstr);
		lrow=0;
		while(lpos>0)
		{
			strncpy(dstr,lcstr+LCD_CPL*lrow,LCD_CPL);
			dstr[LCD_CPL]=0;
			lpos-=LCD_CPL;
			LCD_draw_string(13, (lrow+2)*LCD_RDIST, dstr);
			lrow++;
		}
		LCD_update();
		switch((rccode = GetRCCode()))
		{
			case RC_RED:	
				m->act_entry=(m->act_entry/10)*10;
				rv=1;
				mloop=0;
				break;

			case RC_GREEN:	
				m->act_entry=(m->act_entry/10)*10+1;
				rv=1;
				mloop=0;
				break;

			case RC_YELLOW:	
				m->act_entry=(m->act_entry/10)*10+2;
				rv=1;
				mloop=0;
				break;

			case RC_BLUE:	
				m->act_entry=(m->act_entry/10)*10+3;
				rv=1;
				mloop=0;
				break;

			case RC_1:
				m->act_entry=(m->act_entry/10)*10+4;
				rv=1;
				mloop=0;
				break;

			case RC_2:
				m->act_entry=(m->act_entry/10)*10+5;
				rv=1;
				mloop=0;
				break;

			case RC_3:
				m->act_entry=(m->act_entry/10)*10+6;
				rv=1;
				mloop=0;
				break;

			case RC_4:
				m->act_entry=(m->act_entry/10)*10+7;
				rv=1;
				mloop=0;
				break;

			case RC_5:
				m->act_entry=(m->act_entry/10)*10+8;
				rv=1;
				mloop=0;
				break;

			case RC_6:
				m->act_entry=(m->act_entry/10)*10+9;
				rv=1;
				mloop=0;
				break;

			case RC_UP:
			case RC_MINUS:	--m->act_entry;
					break;

			case RC_DOWN:	
			case RC_PLUS:	++m->act_entry;
					break;

			case RC_LEFT:	m->act_entry-=10;
					break;

			case RC_RIGHT:	m->act_entry+=10;
					break;

			case RC_OK:	
				rv=1;
				mloop=0;
				break;

			case RC_HOME:	
				rv=0;
				mloop=0;
				break;

			case RC_MUTE:	break;

			case RC_HELP:
				rv=-99;
				mloop=0;
				break;

			case RC_STANDBY:
				rv=-1;
				mloop=0;
				break;

			case RC_DBOX:
				rv=-98;
				mloop=0;
				break;
				
			default:	continue;
		}

		if (m->act_entry>=m->num_entrys)
		{
			m->act_entry=0;
		}
		if(m->act_entry<0)
		{
			m->act_entry=(m->num_entrys)?m->num_entrys-1:0;
		}
	} while(mloop);

	ShowInfo(m);

return rv;
}

int AddListEntry(MENU *m, char *line, int pos)
{
int i,j,found=0,pfound=1;
PLISTENTRY entr;
char *ptr1,*ptr2,*ptr3;


	if(!strlen(line))
	{
		return 1;
	}
	
	if(m->num_entrys>=m->max_entrys)
	{
		if((m->list=realloc(m->list,(m->max_entrys+LIST_STEP)*sizeof(PLISTENTRY)))==NULL)
		{
			printf(NOMEM);
			Clear_List(m,0);
			return 0;
		}
		for(i=m->num_entrys; i<m->num_entrys+LIST_STEP; i++)
		{
			if((entr=calloc(1,sizeof(LISTENTRY)))==NULL)
				{
				printf(NOMEM);
				Clear_List(m,0);
				return -1;
				}
			m->list[i]=entr;
		}
		m->max_entrys+=LIST_STEP;
	}
	
	entr=m->list[m->num_entrys];

	if(m == &funcs)
	{
		entr->type=TYP_WEATH;
		entr->entry=strdup(line);
		entr->headerpos=pos;
		m->num_entrys++;
		found=1;
	}
	else
	{
		for(i=TYP_MENU; !found && i<=TYP_EXECUTE; i++)
		{
			if((ptr1=strstr(line,TYPESTR[i]))==line)
			{
				ptr2=strchr(ptr1,'=');
				ptr2++;
				if(*ptr2=='*')
				{
					entr->underline=1;
					while(*(++ptr2))
					{
						*(ptr2-1)=*ptr2;
					}
					*(ptr2-1)=0;
					ptr2=strchr(ptr1,'=')+1;
				}
				if((i==TYP_MENU) || ((ptr1=strchr(ptr2,','))!=NULL))
				{
					if(i!=TYP_MENU)
					{
						++ptr1;
						if((ptr3=strstr(ptr1,"abs://"))!=NULL)
						{
							memmove(ptr3,ptr3+3,strlen(ptr3));
							entr->absolute=1;
						}
						else
						{
							entr->absolute=0;
						}
						if((i==TYP_PICTURE) || (i==TYP_PICHTML))
						{
							if(*ptr1=='|')
							{
								pfound=0;
								ptr2=ptr1;
								++ptr1;
								for(j=PTYP_JPG; !pfound && j<=PTYP_PNG; j++)
								{
									if(strncasecmp(ptr1,PTYPESTR[j],3)==0)
									{
										pfound=1;
										entr->pictype=j;
										ptr1+=3;
										if(sscanf(ptr1,"%d",&(entr->repeat))!=1)
										{
											entr->repeat=0;
										}
										ptr1=strchr(ptr1,'|');
										while(ptr1 && (*(++ptr1)))
										{
											*(ptr2++)=*ptr1;
										}
										*(ptr2)=0;
									}
								}	
							}
						}
					}
					ptr2=strchr(line,'=');
					ptr2++;
					entr->type=i;
					if((i==TYP_TXTHTML) || (i==TYP_TEXTPAGE) || (i==TYP_TXTPLAIN))
					{
						entr->pictype=i;
					}
					entr->entry=strdup(ptr2);
					entr->headerpos=pos;
					m->num_entrys++;
					found=1;
				}
			}
		}
	}
	return !found || (found && pfound);
}

int Get_Menu(void)
{
int rv=-1, loop=1, mlevel=0, clevel=0, pos=0;
char *pt1;
FILE *fh;

	Clear_List(&menu,1);
	if((fh=fopen(TCF_FILE,"r"))!=NULL)
	{
		loop=1;
		while((loop==1) && fgets(line_buffer, BUFSIZE, fh))
		{
			TrimString(line_buffer);
			pt1=strstr(line_buffer,TYPESTR[TYP_MENU]);
			if(pt1 && (pt1==line_buffer))
			{
				if(pos==menu.act_header)
				{
					clevel=menu.headerlevels[pos];
					loop=0;
				}
				mlevel++;
				pos++;
			}
			else
			{
				pt1=strstr(line_buffer,TYPESTR[TYP_ENDMENU]);
				if(pt1 && (pt1==line_buffer))
				{
					mlevel--;
				}
			}
		}
		if(loop)
		{
			return rv;
		}
		
		--pos;
		--mlevel;
		loop=1;
		while((loop==1) && fgets(line_buffer, BUFSIZE, fh))
		{
			TrimString(line_buffer);
			pt1=strstr(line_buffer,TYPESTR[TYP_MENU]);
			if(pt1 && (pt1==line_buffer))
			{
				pos++;
				if(mlevel==clevel)
				{
					AddListEntry(&menu, line_buffer, pos);
					rv=0;
				}
				mlevel++;
			}
			pt1=strstr(line_buffer,TYPESTR[TYP_ENDMENU]);
			if(pt1 && (pt1==line_buffer))
			{
				mlevel--;
			}
			else
			{
				if(mlevel==clevel)
				{
					AddListEntry(&menu, line_buffer, pos);
					rv=0;
				}
			}
			if(mlevel<clevel)
			{
				loop=0;
			}
		}
	fclose(fh);
	}

	return rv;
}

/******************************************************************************
 * ShowInfo
 ******************************************************************************/

#define XX 0xA7
#define XL 58

void ShowInfo(MENU *m)
{
	int scrollbar_len, scrollbar_ofs, scrollbar_cor, loop;
	int index=m->act_entry,tind=m->act_entry, sbw=(m->num_entrys>10)?12:0;
	char tstr[BUFSIZE], *tptr;
	int moffs=35, ixw=400, iyw=(m->num_entrys<10)?((m->num_entrys+1)*30+moffs):380, dy, my, mh=iyw-moffs, toffs, soffs=4, oldx=startx, oldy=starty;
	dy=(m->num_entrys<10)?30:(mh/11);
	toffs=dy/2;
	my=moffs+dy+toffs;
	
	startx = sx + (((ex-sx) - ixw)/2);
	starty = sy + (((ey-sy) - iyw)/2);

	tind=index;
	
	//frame layout
	RenderBox(0, 0, ixw, iyw, radius, CMC);

	// titlebar
	RenderBox(2, 2, ixw-2, moffs+5, radius, CMH);

	//selectbar
	RenderBox(2, moffs+toffs+soffs+(index%10)*dy+2, ixw-sbw, moffs+toffs+soffs+(index%10+1)*dy+2, radius, CMCS);


	if(sbw)
	{
		//sliderframe
		RenderBox(ixw-sbw, moffs+3, ixw, iyw, radius, CMS);
		//slider
		scrollbar_len = mh / m->num_entrys;
		scrollbar_ofs = scrollbar_len*index;
		scrollbar_cor = mh - ((mh/scrollbar_len)*scrollbar_len)-3;
		RenderBox(ixw-sbw, moffs + scrollbar_ofs+3, ixw, moffs + scrollbar_ofs + scrollbar_len +scrollbar_cor -1, radius, CMCIT);
	}

	// Title text
	RenderString(m->headertxt[m->act_header], 45, dy-soffs+3, ixw-sbw, LEFT, BIG, CMHT);

	index /= 10;
	//Show table of commands
	for(loop = index*10; (loop < (index+1)*10) && (loop < m->num_entrys); ++loop)
	{
		strcpy(tstr,m->list[loop]->entry);
		if((tptr=strchr(tstr,','))!=NULL)
		{
			*tptr=0;
		}
		RenderString(tstr, 45, my, ixw-sbw-65, LEFT, MED, ((loop%10) == (tind%10))?CMCST:CMCT);
		if(m->list[loop]->type==TYP_MENU)
		{
			RenderString(">", 30, my, 65, LEFT, MED, ((loop%10) == (tind%10))?CMCST:CMCT);
		}
		if(m->list[loop]->underline)
		{
			RenderBox(10, my+soffs+2, ixw-10-sbw, my+soffs+2, 0, COL_MENUCONTENT_PLUS_3);
			RenderBox(10, my+soffs+3, ixw-10-sbw, my+soffs+3, 0, COL_MENUCONTENT_PLUS_1);
		}

		switch(loop % 10)
		{
			case 0: PaintIcon("/share/tuxbox/neutrino/icons/rot.raw",9,my-17,1); break;
			case 1: PaintIcon("/share/tuxbox/neutrino/icons/gruen.raw",9,my-17,1); break;
			case 2: PaintIcon("/share/tuxbox/neutrino/icons/gelb.raw",9,my-17,1); break;
			case 3: PaintIcon("/share/tuxbox/neutrino/icons/blau.raw",9,my-17,1); break;
			default:
				sprintf(tstr,"%1d",(loop % 10)-3);
				RenderString(tstr, 10, my-1, 15, CENTER, SMALL, ((loop%10) == (tind%10))?CMCST:CMCT);
			break;

		}
		my += dy;
	}
	//copy backbuffer to framebuffer
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
	
	startx=oldx;
	starty=oldy;
}


/* Parse City-Code from Entry
 */

static int prs_get_city(void)
{
char *tptr;
int cpos;
int res;

	if ((tptr=strchr(menu.list[menu.act_entry]->entry,','))!=NULL)
	{
		ShowInfo(&menu);
		if(!cmdline)
		{
			ShowMessage(prs_translate("Bitte warten",CONVERT_LIST),0);
		}
		cpos=(tptr-menu.list[menu.act_entry]->entry);
		strncpy(city_code,++tptr,8);
		strncpy(city_name,menu.list[menu.act_entry]->entry,cpos);
		city_name[cpos]=0;
		city_code[8]=0;
		printf("Tuxwetter <Citycode %s selected>\n",city_code);
		if((res=parser(city_code,CONVERT_LIST,metric,intype,ctmo))!=0)
		{
			ShowMessage((res==-1)?prs_translate("keine Daten vom Wetterserver erhalten!",CONVERT_LIST):prs_translate("Datei convert.list nicht gefunden",CONVERT_LIST),1);
			city_code[0]=0;
			return 1;
		}
	}
	else
	{
		ShowMessage(prs_translate("Ungültige Daten aus tuxwetter.conf",CONVERT_LIST),1);
		city_code[0]=0;
		return 1;
	}
	
	return 0;
}

void clear_screen(void)
{
	memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
}

	

void show_data(int index)
{
char vstr[512],v2str[512],rstr[512],tstr[512],icon[50],*pt1;
int col1=60,col2=300,sy=70,dy=26, itmp, tret=0;
int gxs=30,gxw=560,gys=70,gyw=250,gicw=112,vxs=0;
int prelate=0;
int rcd;
int HMED=27;
time_t atime;
struct tm *ltime;
char tun[2]="C",sun[5]="km/h",dun[3]="km",pun[5]="mbar",cun[20];

	clear_screen();
	//frame layout
	if(index!=1)
	{
		RenderBox(0, 0, 619, 510, radius, CMC);
		RenderBox(2, 2, 617, 44, radius, CMH);
	}
	else
	{
		if(!cmdline)
		{
			ShowMessage(prs_translate("Bitte warten",CONVERT_LIST),0);
		}
	}

	strcpy(cun,prs_translate("Uhr",CONVERT_LIST));
	if(!metric)
	{
		sprintf(tun,"F");
		sprintf(sun,"mph");
		sprintf(dun,"mi");
		sprintf(pun,"in");
		*cun=0;
	}
	if(index==-99)
	{
		int i;

		col1=40; col2=340; dy=22;
		
		sprintf(rstr,"New-Tuxwetter    Version %.2f",P_VERSION);
		RenderString(rstr, 0, 34, 619, CENTER, BIG, CMHT);
	
		sprintf(rstr,"%s",prs_translate("Steuertasten in den Menüs",CONVERT_LIST));
		RenderString(rstr, 0, sy, 619, CENTER, HMED, GREEN);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Farbtasten Rot, Grün, Gelb, Blau",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("Direktanwahl Funktionen 1-4",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Zifferntasten 1-6",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("Direktanwahl Funktionen 5-10",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Hoch",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("vorheriger Menüeintrag",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Runter",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("nächster Menüeintrag",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Rechts (bei mehrseitigen Menüs)",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("eine Seite vorblättern",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Links (bei mehrseitigen Menüs)",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("eine Seite zurückblättern",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("OK",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("Menüpunkt ausführen",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Home",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("zurück zum vorigen Menü",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("DBox-Taste (im Hauptmenü)",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("fehlende Übersetzungen anzeigen",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Standby-Taste",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("New-Tuxwetter beenden",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=(1.5*(double)dy);
		
		sprintf(rstr,"%s",prs_translate("Steuertasten in Daten- und Grafikanzeige",CONVERT_LIST));
		RenderString(rstr, 0, sy, 619, CENTER, HMED, GREEN);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Hoch",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("vorherigen Eintrag anzeigen",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Runter",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("nächsten Eintrag anzeigen",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Links (in Bildanzeige)",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("neu downloaden (für WebCams)",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Rechts (bei Ani-GIF's)",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("Animation wiederholen",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("Rot (in fehlenden Übersetzungen)",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("Fehlliste löschen",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=dy;

		sprintf(rstr,"%s",prs_translate("OK / Home",CONVERT_LIST));
		RenderString(rstr, col1, sy, col2-col1, LEFT, HMED, CMCT);
		sprintf(rstr,"%s",prs_translate("Aktuelle Anzeige schließen",CONVERT_LIST));
		RenderString(rstr, col2, sy, 619-col2, LEFT, HMED, CMCT);
		sy+=(1.5*(double)dy);

		memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
		rcd=GetRCCode();
		while((rcd != RC_OK) && (rcd != RC_HOME))
		{
			rcd=GetRCCode();
		}
	}
	else
	{
		if(index==1)
		{			
			int i, tmax[7], tmin[7], mint=100, maxt=-100, j, pmin, pmax;
			double tstep=1, garr[70], tv1, tv2, tv3;
						
			RenderBox(0, 0, 619, 510, radius, CMC);
			RenderBox(2, 2, 617, 44, radius, CMH);
			sprintf(rstr,"%s",prs_translate("Trend für die kommende Woche",CONVERT_LIST));
			RenderString(rstr, 0, 34, 619, CENTER, BIG, CMHT);
			RenderLine(gxs,gys,gxs,gys+gyw+gicw-12,CMCIT);
			RenderLine(gxs+1,gys,gxs+1,gys+gyw+gicw-12,CMCIT);
			for(i=0; i<5; i++)
			{
				prs_get_val(i, PRE_TEMPH,0,vstr);
				if(sscanf(vstr,"%d",&tmax[i])!=1)
				{
					if(!i)
					{
						vxs=1;
						tmax[i]=0;
					}
					else
					{
						tmax[i]=tmax[i-1];
					}
				}
				prs_get_val(i, PRE_TEMPL,0,vstr);
				if(sscanf(vstr,"%d",&tmin[i])!=1)
				{
					tmin[i]=(i)?tmin[i-1]:0;
				}
				if(tmin[i]<mint)
				{
					mint=tmin[i];
				}
				if((i || !vxs)  && tmax[i]<mint)
				{
					mint=tmax[i];
				}
				if((i || !vxs)  && tmax[i]>maxt)
				{
					maxt=tmax[i];
				}
				if(tmin[i]>maxt)
				{
					maxt=tmin[i];
				}
				prs_get_day(i, vstr, metric);
				if((pt1=strchr(vstr,','))!=NULL)
				{
					strcpy(pt1,"_SIG");
				}
				strcpy(rstr,prs_translate(vstr,CONVERT_LIST));
				RenderString(rstr, 30+i*gicw, 370, gicw, CENTER, BIG, CMCT);
				RenderLine(gxs+(i+1)*gicw,gys,gxs+(i+1)*gicw,gys+gyw+gicw-10,CMCIT);
			}
			RenderLine(gxs+i*gicw+1,gys,gxs+i*gicw+1,gys+gyw+gicw-12,CMCIT);
			tstep=(5*(1+(int)((maxt-mint)/5))+1);
			tstep=(double)(gyw-5)/tstep;

			RenderLine(gxs,gys,gxs,gys+gyw,CMCIT);
			RenderLine(gxs+1,gys,gxs+1,gys+gyw,CMCIT);
			RenderLine(gxs+2,gys,gxs+2,gys+gyw,CMCIT);
			RenderLine(gxs,gys+gyw,gxs+gxw,gys+gyw,CMCIT);
			RenderLine(gxs,gys+gyw+1,gxs+gxw,gys+gyw+1,CMCIT);
			RenderLine(gxs,gys+gyw+2,gxs+gxw,gys+gyw+2,CMCIT);
			RenderString(prs_translate("Höchstwerte",CONVERT_LIST), gxs, gys, gxw/2, CENTER, SMALL, YELLOW);
			RenderString(prs_translate("Tiefstwerte",CONVERT_LIST), gxs+(gxw/2), gys, gxw/2, CENTER, SMALL, GREEN);

			for(i=1; i<=(5*(1+(int)((maxt-mint)/5))+1); i++)
			{
				if(i)
				{
					RenderLine(gxs,gys+gyw-(i*tstep)-2,gxs+gxw,gys+gyw-(i*tstep)-2,((!(mint+i-1)))?CMCT:CMCIT);
					if(!((mint+i-1)%5))
					{
						RenderLine(gxs,gys+gyw-(i*tstep)-3,gxs+gxw,gys+gyw-(i*tstep)-3,((!(mint+i-1)))?CMCT:CMCIT);
					}
					RenderLine(gxs,gys+gyw-(i*tstep)-1,gxs+gxw,gys+gyw-(i*tstep)-1,COL_MENUCONTENT_PLUS_3);
				}
				sprintf(vstr,"%d",mint+i-1);
				RenderString(vstr,gxs-22,gys+gyw-(i*tstep)+7, 20, RIGHT, SMALL, CMCT);
				RenderString(vstr,gxs+gxw+2,gys+gyw-(i*tstep)+7, 20, RIGHT, SMALL, CMCT);
			}
			RenderLine(gxs,gys+gyw-((i-1)*tstep)-3,gxs+gxw,gys+gyw-((i-1)*tstep)-3,((!(mint+i-1)))?CMCT:CMCIT);

// Geglättete Kurven

			for(i=0; i<5; i++)
			{
				tv1=tmin[i];
				tv2=tmin[i+1];
				for(j=0; j<10; j++)
				{
					tv3=j-2;
					if(j<2)
					{
						garr[i*10+j]=tv1;
					}
					else
					{
						if(j>7)
						{
							garr[i*10+j]=tv2;
						}
						else
						{
							garr[i*10+j]=((tv1*(6.0-tv3))+(tv2*tv3))/6.0;
						}
					}
				}
			}
			for(i=2; i<38; i++)
			{
				garr[i]=(garr[i-2]+garr[i-1]+garr[i]+garr[i+1]+garr[i+2])/5.0;
			}
			for(i=1; i<=40; i++)
			{
				pmin=(gys+gyw)-(garr[i-1]-mint+1)*tstep-1;
				pmax=(gys+gyw)-(garr[i]-mint+1)*tstep-1;
				RenderLine(gxs+gicw/2+((i-1)*(gicw/10)),pmin,gxs+gicw/2+i*(gicw/10),pmax,GREEN);
				RenderLine(gxs+gicw/2+((i-1)*(gicw/10)),pmin+1,gxs+gicw/2+i*(gicw/10),pmax+1,GREEN);
				RenderLine(gxs+gicw/2+((i-1)*(gicw/10)),pmin+2,gxs+gicw/2+i*(gicw/10),pmax+2,GREEN);
			}
			for(i=vxs; i<5; i++)
			{
				tv1=tmax[i];
				tv2=tmax[i+1];
				for(j=0; j<10; j++)
				{
					tv3=j-2;
					if(j<2)
					{
						garr[i*10+j]=tv1;
					}
					else
					{
						if(j>7)
						{
							garr[i*10+j]=tv2;
						}
						else
						{
							garr[i*10+j]=((tv1*(6.0-tv3))+(tv2*tv3))/6.0;
						}
					}
				}
			}
			for(i=2+10*vxs; i<38; i++)
			{
				garr[i]=(garr[i-2]+garr[i-1]+garr[i]+garr[i+1]+garr[i+2])/5.0;
			}
			for(i=1+10*vxs; i<=40; i++)
			{
				pmin=(gys+gyw)-(garr[i-1]-mint+1)*tstep-1;
				pmax=(gys+gyw)-(garr[i]-mint+1)*tstep-1;
				RenderLine(gxs+gicw/2+((i-1)*(gicw/10)),pmin,gxs+gicw/2+i*(gicw/10),pmax,YELLOW);
				RenderLine(gxs+gicw/2+((i-1)*(gicw/10)),pmin+1,gxs+gicw/2+i*(gicw/10),pmax+1,YELLOW);
				RenderLine(gxs+gicw/2+((i-1)*(gicw/10)),pmin+2,gxs+gicw/2+i*(gicw/10),pmax+2,YELLOW);
			}

			if(show_icons)
			{
				for(i=0; i<5; i++)
				{
					prs_get_val(i,PRE_ICON,prelate,vstr);
					sprintf  (icon,"http://image.weather.com/web/common/intlwxicons/52/%s.gif",vstr);
					if (HTTP_downloadFile(icon, ICON_FILE, 0, intype, ctmo, 2) == 0) 
					{
						tret=gif_on_data(icon, 72+i*gicw, 430, gicw, gicw, 5, (i)?((i==4)?1:0):2, 0, 0);
					}
				}
			}
			memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
		}
		else
		{
			if(index==0)
			{
				dy=24;
				sy-=2;
				// show icon
				prs_get_val(0, ACT_ICON, 0, vstr);
				sprintf (rstr,"%s.bmp",vstr);
				bmp2lcd (rstr);

				if(show_icons)
				{
					xremove(ICON_FILE);
					sprintf  (icon,"http://image.weather.com/web/common/intlwxicons/52/%s.gif",vstr);
					if (HTTP_downloadFile(icon, ICON_FILE, 0, intype, ctmo, 2) != 0) 
					{
						printf("Tuxwetter <unable to get icon>\n");
					}
				}	
				
				sprintf(rstr,"%s",prs_translate("Aktuelles Wetter",CONVERT_LIST));
				RenderString(rstr, 0, 34, 619, CENTER, BIG, CMHT);
		
				sprintf(rstr,"%s",prs_translate("Standort:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, GREEN);
				sprintf(rstr,"%s",city_name);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
	
				sprintf(rstr,"%s",prs_translate("Längengrad:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_dbl(0, ACT_LON, 0, vstr);
				sprintf(rstr,"%s",vstr);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
	
				sprintf(rstr,"%s",prs_translate("Breitengrad:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_dbl(0, ACT_LAT, 0, vstr);
				sprintf(rstr,"%s",vstr);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
	
				sprintf(rstr,"%s",prs_translate("Ortszeit:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_time(0, ACT_TIME, vstr, metric);
				sprintf(rstr,"%s %s",vstr,cun);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
		
				sprintf(rstr,"%s",prs_translate("Aktuelle Uhrzeit:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				time(&atime);
				ltime=localtime(&atime);
				if(metric)
				{
					sprintf(rstr,"%02d:%02d %s",ltime->tm_hour,ltime->tm_min,cun);
				}
				else
				{
					sprintf(rstr,"%02d:%02d %s",(ltime->tm_hour)?((ltime->tm_hour>12)?ltime->tm_hour-12:ltime->tm_hour):12,ltime->tm_min,(ltime->tm_hour>=12)?"PM":"AM");
				}
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
			
				sprintf(rstr,"%s",prs_translate("Zeitpunkt der Messung:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_dtime(0, ACT_UPTIME, vstr, metric);
				sprintf(rstr,"%s %s",vstr,cun);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=(1.5*(double)dy);
		
				sprintf(rstr,"%s",prs_translate("Bedingung:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, GREEN);
				prs_get_val(0, ACT_COND, 0, vstr);
				sprintf(rstr,"%s",vstr);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
		
				sprintf(rstr,"%s",prs_translate("Temperatur:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(0, ACT_TEMP, 0, vstr);
				prs_get_val(0, ACT_FTEMP, 0, v2str);
				sprintf(rstr,"%s °%s  %s %s °%s",vstr,tun,prs_translate("gefühlt:",CONVERT_LIST),v2str,tun);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;

				sprintf(rstr,"%s",prs_translate("Luftfeuchtigkeit:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(0, ACT_HMID, 0, vstr);
				sprintf(rstr,"%s %%",vstr);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
	
				sprintf(rstr,"%s",prs_translate("Taupunkt:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(0, ACT_DEWP, 0, vstr);
				sprintf(rstr,"%s °%s",vstr,tun);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
		
				sprintf(rstr,"%s",prs_translate("Luftdruck:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(0, ACT_PRESS, 0, vstr);
				prs_get_val(0, ACT_PRTEND, 0, v2str);
				sprintf(rstr,"%s %s  %s",vstr,pun,v2str);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;

				sprintf(rstr,"%s",prs_translate("Wind:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(0, ACT_WINDD, 0, vstr);
				prs_get_val(0, ACT_WSPEED, 0, v2str);
				if((strstr(vstr,"windstill")!=NULL) || (strstr(v2str,"CALM")!=NULL))
				{
					sprintf(rstr,"%s",prs_translate("windstill",CONVERT_LIST));
				}
				else
				{
					sprintf(tstr,"%s",prs_translate("von",CONVERT_LIST));
					sprintf(rstr,"%s %s %s %s %s",tstr,vstr,prs_translate("mit",CONVERT_LIST),v2str,sun);
				}
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=(1.5*(double)dy);
	
				sprintf(rstr,"%s",prs_translate("Sonnenaufgang:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_time(0, ACT_SUNR, vstr,metric);
				sprintf(rstr,"%s %s",vstr,cun);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
			
				sprintf(rstr,"%s",prs_translate("Sonnenuntergang:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_time(0, ACT_SUNS, vstr,metric);
				sprintf(rstr,"%s %s",vstr,cun);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
		
				sprintf(rstr,"%s",prs_translate("Mondphase:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(0, ACT_MOON, 0, v2str);
				sprintf(rstr,"%s",v2str);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;			
			
				sprintf(rstr,"%s",prs_translate("Fernsicht:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(0, ACT_VIS, 0, vstr);
				if(sscanf(vstr,"%d",&itmp)==1)
				{
					sprintf(rstr,"%s %s",vstr,dun);
				}
				else
				{
					strcpy(rstr,vstr);
				}
		
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
			
				sprintf(rstr,"%s",prs_translate("UV-Index:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(0, ACT_UVIND, 0, vstr);
				prs_get_val(0, ACT_UVTEXT, 0, v2str);
				sprintf(rstr,"%s  %s",vstr,v2str);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;

				sprintf(rstr,"%s",prs_translate("Regenrisiko:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(0, PRE_PPCP, 0, vstr);
				sprintf(rstr,"%s %%",vstr);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
				if(show_icons)
				{
					tret=gif_on_data(icon, 540, 115, 100, 100, 5, 3, 0, 0);
				}
				memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
			}
			else
			{
				--index;
				if(index==1)
				{
					prs_get_val(index-1, PRE_TEMPH, 0, vstr);
					if(strstr(vstr,"N/A")!=NULL)
					{
						prelate=1;
					}
				}
			
				// show icon
				prs_get_val(index-1,PRE_ICON,prelate,vstr);
				sprintf (rstr,"%s.bmp",vstr);
				bmp2lcd (rstr);

				if(show_icons)
				{
					xremove(ICON_FILE);
					sprintf  (icon,"http://image.weather.com/web/common/intlwxicons/52/%s.gif",vstr);
					if (HTTP_downloadFile(icon, ICON_FILE,0,intype,ctmo,2) != 0) 
					{
						printf("Tuxwetter <unable to get icon file \n");
					}
				}
			
				if(index==1)
				{
					sprintf(vstr,"%s",prs_translate("Heute",CONVERT_LIST));
				}
				else
				{
					prs_get_day(index-1, vstr, metric);
				}
				sprintf(rstr,"%s %s",prs_translate("Vorschau für",CONVERT_LIST),vstr);
				RenderString(rstr, 0, 34, 619, CENTER, BIG, CMHT);
		
				sprintf(rstr,"%s",prs_translate("Standort:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, GREEN);
				sprintf(rstr,"%s",city_name);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=(1.5*(double)dy);
		
				sprintf(rstr,"%s",prs_translate("Höchste Temperatur:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(index-1,PRE_TEMPH,0,vstr);
				sprintf(rstr,"%s °%s",vstr,tun);
				RenderString((prelate)?"---":rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;

				sprintf(rstr,"%s",prs_translate("Tiefste Temperatur:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(index-1,PRE_TEMPL,0,vstr);
				sprintf(rstr,"%s °%s",vstr,tun);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;

				sprintf(rstr,"%s",prs_translate("Sonnenaufgang:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_time(index-1, PRE_SUNR,vstr,metric);
				sprintf(rstr,"%s %s",vstr,cun);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
		
				sprintf(rstr,"%s",prs_translate("Sonnenuntergang:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_time(index-1, PRE_SUNS,vstr,metric);
				sprintf(rstr,"%s %s",vstr,cun);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=(1.5*(double)dy);
		
				RenderString(prs_translate("Tageswerte",CONVERT_LIST), col1, sy, col2-col1, LEFT, MED, GREEN);
				sy+=dy;

				sprintf(rstr,"%s",prs_translate("Bedingung:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(index-1, PRE_COND, 0, vstr);
				sprintf(rstr,"%s",vstr);
				RenderString((prelate)?"---":rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
		
				sprintf(rstr,"%s",prs_translate("Wind:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(index-1, PRE_WINDD, 0, vstr);
				prs_get_val(index-1, PRE_WSPEED, 0, v2str);
				sprintf(tstr,"%s",prs_translate("von",CONVERT_LIST));
				sprintf(rstr,"%s %s %s %s %s",tstr,vstr,prs_translate("mit",CONVERT_LIST),v2str,sun);
				RenderString((prelate)?"---":rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;

				sprintf(rstr,"%s",prs_translate("Luftfeuchtigkeit:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(index-1, PRE_HMID, 0, vstr);
				sprintf(rstr,"%s %%",vstr);
				RenderString((prelate)?"---":rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;

				sprintf(rstr,"%s",prs_translate("Regenrisiko:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(index-1, PRE_PPCP, 0, vstr);
				sprintf(rstr,"%s %%",vstr);
				RenderString((prelate)?"---":rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=(1.5*(double)dy);
			
				RenderString(prs_translate("Nachtwerte",CONVERT_LIST), col1, sy, col2-col1, LEFT, MED, GREEN);
				sy+=dy;
	
				sprintf(rstr,"%s",prs_translate("Bedingung:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(index-1, PRE_COND, 1, vstr);
				sprintf(rstr,"%s",vstr);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
			
				sprintf(rstr,"%s",prs_translate("Wind:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(index-1, PRE_WINDD, 1, vstr);
				prs_get_val(index-1, PRE_WSPEED, 1, v2str);
				if((strstr(vstr,"windstill")!=NULL) || (strstr(v2str,"CALM")!=NULL))
				{
					sprintf(rstr,"%s",prs_translate("windstill",CONVERT_LIST));
				}	
				else
				{
					sprintf(tstr,"%s",prs_translate("von",CONVERT_LIST));
					sprintf(rstr,"%s %s %s %s %s",tstr,vstr,prs_translate("mit",CONVERT_LIST),v2str,sun);
				}
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;

				sprintf(rstr,"%s",prs_translate("Luftfeuchtigkeit:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(index-1, PRE_HMID, 1, vstr);
				sprintf(rstr,"%s %%",vstr);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;
	
				sprintf(rstr,"%s",prs_translate("Regenrisiko:",CONVERT_LIST));
				RenderString(rstr, col1, sy, col2-col1, LEFT, MED, CMCT);
				prs_get_val(index-1, PRE_PPCP, 1, vstr);
				sprintf(rstr,"%s %%",vstr);
				RenderString(rstr, col2, sy, 619-col2, LEFT, MED, CMCT);
				sy+=dy;

				if(show_icons)
				{
					tret=gif_on_data(icon, 540, 115, 100, 100, 5, 3, 0, 0);
				}
				memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
			}
		}
	}
}

void scale_pic(unsigned char **buffer, int x1, int y1, int xstart, int ystart, int xsize, int ysize,
			   int *imx, int *imy, int *dxp, int *dyp, int *dxo, int *dyo, int center)
{
	float xfact=0, yfact=0;
	int txsize=0, tysize=0;
	int tempx =0, tempy=0;
	int txstart =xstart, tystart= ystart;
	
	if (xsize > (ex-xstart)) txsize= (ex-xstart);
	else  txsize= xsize; 
	if (ysize > (ey-ystart)) tysize= (ey-ystart);
	else tysize=ysize;
	xfact= 1000*txsize/x1;
	xfact= xfact/1000;
	yfact= 1000*tysize/y1;
	yfact= yfact/1000;
	
	if ( xfact <= yfact)
	{
		*imx=(int)x1*xfact;
		*imy=(int)y1*xfact;
		if (center !=0) 
		{
			tystart=(ey-sy)-*imy;
			tystart=tystart/2;
			tystart=tystart+ystart;
		}
	}
	else
	{
		*imx=(int)x1*yfact;
		*imy=(int)y1*yfact;
		if (center !=0) 
		{
			txstart=(ex-sx)-*imx;
			txstart=txstart/2;
			txstart=txstart+xstart;
		}
	}
	tempx=*imx;
	tempy=*imy;
	*buffer=(char*)color_average_resize(*buffer,x1,y1,*imx,*imy);

	*dxp=0;
	*dyp=0;
	*dxo=txstart;
	*dyo=tystart;
}

void close_jpg_gif_png(void)
{
	// clear Display	
	fb_set_gmode(0);
	ioctl(fb, FBIOPUTCMAP, oldcmap);
	memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
	gmodeon=0; 
}

int wait_image(int repeat, int first)
{
time_t t1,t2;
int rv;

	if(!repeat)
	{
		rv=GetRCCode();
		return rv;
	}
	time(&t1);
	t2=t1;
	while((t2-t1)<repeat)
	{
		rv=GetRCCodeNW();
		if(rv==-1)
		{
			usleep(200000L);
			time(&t2);
		}
		else
		{
			return rv;
		}
	}
	
	return RCTranslate(KEY_LEFT);
}

int show_jpg(char *name, int xstart, int ystart, int xsize, int ysize, int wait, int repeat, int single, int center)
{
FILE *tfh;
int x1,y1,rcj,rv=-1;
int imx,imy,dxo,dyo,dxp,dyp;
unsigned char *buffer=NULL;

	if((tfh=fopen(name,"r"))!=NULL)
	{
		fclose(tfh);
		if(fh_jpeg_getsize(name, &x1, &y1, xsize, ysize))
		{
			printf("Tuxwetter <invalid JPG-Format>\n");
			return -1;
		}
		if((buffer=(unsigned char *) malloc(x1*y1*4))==NULL)
		{
			printf(NOMEM);
			return -1;
		}
		if(!(rv=fh_jpeg_load(name, buffer, x1, y1)))
		{
			scale_pic(&buffer,x1,y1,xstart,ystart,xsize,ysize,&imx,&imy,&dxp,&dyp,&dxo,&dyo,center);
			fb_set_gmode(1);
			fb_display(buffer, imx, imy, dxp, dyp, dxo, dyo, 1, 1);
			gmodeon=1;
		}
		free(buffer);
		
		if(!rv && wait)
		{
			rcj=wait_image(repeat, 1);
			while((rcj != RC_OK) && (rcj != RC_HOME) && (rcj != RC_DOWN) && (rcj != RC_LEFT) && (rcj != RC_UP) && (rcj != RC_PLUS) && (rcj != RC_MINUS))
			{
				rcj=wait_image(repeat, 0);
			}
			if(single || (rcj==RC_OK) || (rcj==RC_HOME))
			{
				close_jpg_gif_png();
			}
			else
			{
				showBusy(startx+3,starty+3,10,0xff,00,00);
				if(rcj==RC_HOME)
				{
					rcj=RC_OK;
				}
				return rcj;
			}
		}
	}
	
	return (rv)?-1:0;	
}

int show_png(char *name, int xstart, int ystart, int xsize, int ysize, int wait, int repeat, int single, int center)
{
FILE *tfh;
int x1,y1,rcn,rv=-1;
int imx,imy,dxo,dyo,dxp,dyp;
unsigned char *buffer=NULL;

	if((tfh=fopen(name,"r"))!=NULL)
	{
		fclose(tfh);
		if(fh_png_getsize(name, &x1, &y1, xsize, ysize))
		{
			printf("Tuxwetter <invalid PNG-Format>\n");
			return -1;
		}
		if((buffer=(unsigned char *) malloc(x1*y1*4))==NULL)
		{
			printf(NOMEM);
			return -1;
		}
		if(!(rv=fh_png_load(name, buffer, x1, y1)))
		{
			scale_pic(&buffer,x1,y1,xstart,ystart,xsize,ysize,&imx,&imy,&dxp,&dyp,&dxo,&dyo,center);
			fb_set_gmode(1);
			fb_display(buffer, imx, imy, dxp, dyp, dxo, dyo, 1, 1);
			gmodeon=1;
		}
		free(buffer);
		
		if(!rv && wait)
		{
			rcn=wait_image(repeat, 1);
			while((rcn != RC_OK) && (rcn != RC_HOME) && (rcn != RC_DOWN) && (rcn != RC_LEFT) && (rcn != RC_UP) && (rcn != RC_PLUS) && (rcn != RC_MINUS))
			{
				rcn=wait_image(repeat, 0);
			}
			if(single || (rcn==RC_OK) || (rcn==RC_HOME))
			{
				close_jpg_gif_png();
			}
			else
			{
				showBusy(startx+3,starty+3,10,0xff,00,00);
				if(rcn==RC_HOME)
				{
					rcn=RC_OK;
				}
				return rcn;
			}
		}
	}
	
	return (rv)?-1:0;	
}

static int gifs=0;

int show_gif(char *name, int xstart, int ystart, int xsize, int ysize, int wait, int repeat, int single, int center, int nodecomp)
{
FILE *tfh;
int x1,y1,rcg,count,cloop,rv=-1;
int imx,imy,dxo,dyo,dxp,dyp;
unsigned char *buffer=NULL, fname[512];

	if((tfh=fopen(name,"r"))!=NULL)
	{
		fclose(tfh);
		if(nodecomp)
		{
			count=gifs;
		}
		else
		{		
			xremove("/tmp/tempgif.gif");
			gifs=count=gifdecomp(GIF_FILE, GIF_MFILE);
		}
		if(count<1)
		{
			printf("Tuxwetter <invalid GIF-Format>\n");
			return -1;
		}
		cloop=0;
		while(count--)
		{
			sprintf(fname,"%s%02d.gif",GIF_MFILE,cloop++);
			if(fh_gif_getsize(fname, &x1, &y1, xsize, ysize))
			{
				printf("Tuxwetter <invalid GIF-Format>\n");
				return -1;
			}
			if((buffer=(unsigned char *) malloc(x1*y1*4))==NULL)
			{
				printf(NOMEM);
				return -1;
			}
			if(!(rv=fh_gif_load(fname, buffer, x1, y1)))
			{
				scale_pic(&buffer,x1,y1,xstart,ystart,xsize,ysize,&imx,&imy,&dxp,&dyp,&dxo,&dyo,center);
				fb_set_gmode(1);
				fb_display(buffer, imx, imy, dxp, dyp, dxo, dyo, 1, 1);
				gmodeon=1;

				if(gifs>1)
				{
					sprintf(fname,"%s %2d / %d", prs_translate("Bild",CONVERT_LIST),cloop, gifs);
					LCD_draw_string(13, 9, fname);
					LCD_update();
				}
			}
			free(buffer);
		}
		
		if(!rv && wait)
		{
			rcg=wait_image(repeat, 1);
			while((rcg != RC_OK) && (rcg != RC_HOME) && (rcg != RC_DOWN) && (rcg != RC_UP) && (rcg != RC_PLUS) && (rcg != RC_MINUS)&& (rcg != RC_LEFT) && (rcg != RC_RIGHT))
			{
				rcg=wait_image(repeat, 0);
			}
			if(single || (rcg==RC_OK) || (rcg==RC_HOME))
			{
				close_jpg_gif_png();
			}
			else
			{
				showBusy(startx+3,starty+3,10,0xff,00,00);
				if(rcg==RC_HOME)
				{
					rcg=RC_OK;
				}
				return rcg;
			}
		}
	}
	
	return (rv)?-1:0;	
}

int gif_on_data(char *url, int xstart, int ystart, int xsize, int ysize, int wait, int single, int center, int rahmen)
{
FILE *tfh;
int i,x1,y1,rv=-1;
int imx,imy,dxo,dyo,dxp,dyp;
unsigned char *buffer=NULL,*gbuf;
unsigned char *tbuf=lfb;

	if((tfh=fopen(ICON_FILE,"r"))!=NULL)
	{
		lfb=lbb;
		if(fh_gif_getsize(ICON_FILE, &x1, &y1, xsize, ysize))
		{
			printf("Tuxwetter <invalid GIF-Format>\n");
			return -1;
		}
		if((buffer=(unsigned char *) malloc(x1*y1*4))==NULL)
		{
			printf(NOMEM);
			return -1;
		}
		if(!(rv=fh_gif_load(ICON_FILE, buffer, x1, y1)))
		{
			scale_pic(&buffer,x1,y1,xstart,ystart,xsize,ysize,&imx,&imy,&dxp,&dyp,&dxo,&dyo,center);
			if (rahmen >0)
			{
				RenderBox(xstart+1-sx-rahmen, ystart-6-sy-rahmen,xstart+xsize+2-sx+rahmen,ystart+ysize-sy-6+rahmen, 0, CMCS);
			}

			if(single & 2)
			{
				single &= ~2;
#ifdef HAVE_DBOX_HARDWARE
				i=var_screeninfo.xres*var_screeninfo.yres;
				gbuf=lfb;
				while(i--)
					if(*gbuf >=127)
						*(gbuf++)-=127;
					else
						*(gbuf++)=11;
#endif
			}
			fb_display(buffer, imx, imy, dxp, dyp, dxo, dyo, 0, single);
			if(single & 1)
			{
				ioctl(fb, FBIOPUTCMAP, &spcmap);
			}
			gmodeon=1;
		}
		free(buffer);
		lfb=tbuf;
	}
	return (rv)?-1:0;	
}

int show_php(char *name, char *title, int plain, int highlite)
{
FILE *tfh;
int x1,y1,cs,rcp,rv=-1,run=1,line=0,action=1,cut;
int col1=40,sy=70,dy=26;

	if((tfh=fopen(name,"r"))!=NULL)
	{
		fclose(tfh);

		RenderString("X", 0, 34, 619, LEFT, 32, CMCT);
		if(fh_php_getsize(name, plain, &x1, &y1))
		{
			printf("Tuxwetter <invalid PHP-Format>\n");
			return -1;
		}
		cs=32.0*((double)(619-1.5*(double)col1)/(double)x1);
		if(cs>32)
		{
			cs=32;
		}		

		dy=0.8*(double)cs;
		
		while(run)
		{
			//frame layout
			if(action)
			{
				RenderBox(0, 0, 619, 504, radius, CMC);
				RenderBox(2, 2, 617, 44, radius, CMH);
				RenderString(title, 0, 34, 619, CENTER, BIG, CMHT);

				if(!(rv=fh_php_load(name, col1, sy, dy, cs, line, highlite, plain, &cut)))
				{
					memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
				}
			}
	
			if(!rv)
			{
				rcp=GetRCCode();
				while((rcp != RC_OK) && (rcp != RC_HOME) && (rcp != RC_LEFT) && (rcp != RC_RIGHT) && (rcp != RC_DOWN) && (rcp != RC_UP) && (rcp != RC_PLUS) && (rcp != RC_MINUS)&& (rcp != RC_RED))
				{
					rcp=GetRCCode();
				}
				if(rcp==RC_HOME)
				{
					rcp=RC_OK;
				}
				if((rcp != RC_LEFT) && (rcp != RC_RIGHT))
				{
					return rcp;
				}
				switch(rcp)
				{
					case RC_RIGHT:
						if((action=cut)!=0)
						{
							line+=5;
						}
						break;
						
					case RC_LEFT:
						if(line)
						{				
							if((line-=5)<0)
							{
								line=0;
							}
							action=1;
						}
						else
						{
							action=0;
						}
						break;
				}
			}
			rv=0;
		}
	}
	return (rv)?-1:0;	
}

char *translate_url(char *cmd, int ftype, int absolute)
{
unsigned char *rptr=NULL,*pt1,*pt2,*pt3,*pt4,*pt5;
unsigned char *sstr=strdup(cmd),*estr=strdup(cmd);
FILE *fh,*fh2;
int txttype=(ftype==TYP_TXTHTML),crlf=0;
long flength;


	strcpy(trstr,cmd);
	pt1=trstr;
//	++pt1;
	if((pt2=strchr(pt1,'|'))!=NULL)
	{
		*pt2=0;
		++pt2;
		if((pt3=strchr(pt2,'|'))!=NULL)
		{
			*pt3=0;
			++pt3;
			printf("Tuxwetter <Downloading %s>\n",pt1);
			if(!HTTP_downloadFile(pt1, TRANS_FILE, 1, intype, ctmo, 2))
			{
				if((fh=fopen(TRANS_FILE,"r"))!=NULL)
				{
					fseek(fh,0,SEEK_END);
					flength=ftell(fh);
					rewind(fh);
					if((htmstr=calloc(flength+1,sizeof(char)))!=NULL)
					{
						if(fread(htmstr,(size_t)flength,1,fh)>0)
						{
							*(htmstr+flength)=0;
							if((pt4=strchr(htmstr,13))!=NULL)
								{
									crlf=(*(pt4+1)==10);
								}
							pt4=sstr;
							while(*pt2)
							{
								if((*pt2=='\\') && (*(pt2+1)=='n'))
								{
									if(crlf)
									{
										*pt4=13;
										pt4++;
									}
									pt2++;
									*pt4=10;
								}
								else
								{
									*pt4=*pt2;
								}
								++pt4;		
								++pt2;
							}
							*pt4=0;
					
							pt4=estr;
							while(*pt3)
							{
								if((*pt3=='\\') && (*(pt3+1)=='n'))
								{
									if(crlf)
									{
										*pt4=13;
										pt4++;
									}
									pt3++;
									*pt4=10;
								}
								else
								{
									*pt4=*pt3;
								}	
								++pt4;		
								++pt3;
							}
							*pt4=0;
							if((pt3=strstr(htmstr,sstr))!=NULL)
							{
								if((pt5=strstr(pt3+strlen(sstr)+1,estr))!=NULL)
								{
									do
									{
										pt4=pt3;	
										pt3++;
										pt3=strstr(pt3,sstr);
									}
									while(pt3 && (pt3<pt5));
									*pt5=0;
									pt4+=strlen(sstr);
									if(!txttype)
									{
										if(strstr(pt4,"://")!=NULL)
										{
											sprintf(trstr+strlen(trstr)+1,"\"%s",pt4);
										}
										else
										{
									 		if((pt5=(absolute)?(strrchr(pt1+8,'/')):(strchr(pt1+8,'/')))!=NULL)
									 		{
									 			sprintf(trstr+strlen(trstr)+1,"\"%s",pt1);
									 			sprintf(trstr+strlen(trstr)+1+(pt5-pt1)+((*pt4=='/')?1:2),"%s",pt4);
								 			}
									 	}
										rptr=trstr+strlen(trstr)+1;
									}
									else
									{
										xremove(PHP_FILE);
										if((fh2=fopen(PHP_FILE,"w"))!=NULL)
										{
										int dontsave=0, newline=1;
										
											flength=0;
											fprintf(fh2,"<br>");
												
											while(*pt4)	
											{
												if(*pt4=='<')
												{
													dontsave=1;
												}
												if(*pt4=='>')
												{
													dontsave=2;
												}
												if(!dontsave)
												{
													if((*pt4==' ') && (flength>60))
													{
														fprintf(fh2,"\n<br>");
														flength=0;
														newline=1;
													}
													else
													{
														if(*pt4>' ' || newline<2)
														{
															if(*pt4>' ')
															{
																newline=0;
															}
															fputc(*pt4,fh2);
														}
													}
													if(*pt4==10)
													{
														if(newline<2)
														{
															fprintf(fh2,"<br>");
														}
														flength=0;
														++newline;
													}
													flength++;
												}
												if(dontsave==2)
												{
													dontsave=0;
												}
												pt4++;
											}
											fprintf(fh2,"\n<br><br>");
											fclose(fh2);
											rptr=pt1;
										}
									}
								}
							}
						}
						free(htmstr);
					}
				fclose(fh);
				}
			}
		}
	}

	free(sstr);
	free(estr);

	if(rptr)
	{
		pt1=rptr;
		while(*pt1)
		{
			if(*pt1==' ')
			{
				*(pt1+strlen(pt1)+2)=0;
				memmove(pt1+2,pt1,strlen(pt1));
				*pt1++='%';
				*pt1++='2';
				*pt1='0';
			}
			pt1++;
		}
	}
			
	return rptr;
}

int Menu_Up(MENU *m)
{
int llev=m->headerlevels[m->act_header], lmen=m->act_header, lentr=m->lastheaderentrys[m->act_header];
	
	while((lmen>=0) && (m->headerlevels[lmen]>=llev))
	{
		--lmen;
	}
	if(lmen<0)
	{
		return 0;
	}
	m->act_header=lmen;
	Get_Menu();
	m->act_entry=lentr;
	
	return 1;	
}

/******************************************************************************
 * Tuxwetter Main
 ******************************************************************************/

int main (int argc, char **argv)
{
int index=0,cindex=0,tv,rcm,rce,lpos,lrow,ferr,tret=-1;
int mainloop=1,wloop=1, dloop=1;
char rstr[BUFSIZE], *rptr;
char dstr[BUFSIZE];
char tstr[BUFSIZE];
FILE *tfh;
LISTENTRY epl={NULL, 0, TYP_TXTPLAIN, TYP_TXTPLAIN, 0, 0, 0};
PLISTENTRY pl=&epl;

		// if problem with config file return from plugin

		for(tv=1; tv<argc; tv++)
		{
			if((strstr(argv[tv],"-v")==argv[tv])||(strstr(argv[tv],"--Version")==argv[tv]))
			{
				printf("New-Tuxwetter Version %.2f\n",P_VERSION);
				return 0;
			}
			if(*argv[tv]=='/')
			{
				strcpy(TCF_FILE,argv[tv]);
			}
			if(strchr(argv[tv],'='))
			{
				cmdline=strdup(argv[tv]);
				TrimString(cmdline);
				TranslateString(cmdline);
			}
		}

//		system("ping -c 2 google.com &");

		if((line_buffer=calloc(BUFSIZE+1, sizeof(char)))==NULL)
		{
			printf(NOMEM);
			return -1;
		}
	
		if (!ReadConf(cmdline))
		{
			printf("Tuxwetter <Configuration failed>\n");
			return -1;
		}
	
		if(((sx=Read_Neutrino_Cfg("screen_StartX"))<0)&&((sx=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/left"))<0))
			sx=80;
		
		if(((ex=Read_Neutrino_Cfg("screen_EndX"))<0)&&((ex=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/right"))<0))
			ex=620;

		if(((sy=Read_Neutrino_Cfg("screen_StartY"))<0)&&((sy=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/top"))<0))
			sy=80;

		if(((ey=Read_Neutrino_Cfg("screen_EndY"))<0)&&((ey=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/bottom"))<0))
			ey=505;

		if(Read_Neutrino_Cfg("rounded_corners")>0)
			radius=9;
		else
			radius=0;

		if((trstr=malloc(BUFSIZE))==NULL)
		{
			printf(NOMEM);
			return -1;
		}

		memset(&menu,0,sizeof(MENU));
		memset(&funcs,0,sizeof(MENU));

		if(!cmdline)
		{
			if(Check_Config())
			{
				printf("<tuxwetter> Unable to read tuxwetter.conf\n");
				Clear_List(&menu,-1);
				free(line_buffer);
				return -1;
			}
		}

		if((funcs.headertxt=calloc(1, sizeof(char*)))==NULL)
		{
			printf(NOMEM);
			free(line_buffer);
			Clear_List(&menu,-1);
			Clear_List(&funcs,-1);
			return -1;
		}

		fb = open(FB_DEVICE, O_RDWR);
#ifdef HAVE_DBOX_HARDWARE
		rc = open(RC_DEVICE, O_RDONLY);
//		fcntl(rc, F_SETFL, (fcntl(rc, F_GETFL) | O_EXCL) & ~O_NONBLOCK);
		fcntl(rc, F_SETFL, fcntl(rc, F_GETFL) | O_EXCL | O_NONBLOCK);
#else
		rc = open(RC_DEVICE, O_RDONLY|O_NONBLOCK);
#endif

#if HAVE_DVB_API_VERSION < 3
		// RC Buffer leeren
		char buf[32];
		read(rc,buf,32);
#endif

	//init framebuffer

		if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			printf("Tuxwetter <FBIOGET_FSCREENINFO failed>\n");
			return -1;
		}
		if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			printf("Tuxwetter <FBIOGET_VSCREENINFO failed>\n");
			return -1;
		}

		if(ioctl(fb, FBIOGETCMAP, &oldcmap) == -1)
		{
			printf("Tuxwetter <FBIOGETCMAP failed>\n");
			return -1;
		}

		for(index=CMCST; index<=CMH; index++)
		{
			sprintf(rstr,"menu_%s_alpha",menucoltxt[index-128]);
			if((tv=Read_Neutrino_Cfg(rstr))>=0)
				tr[index-128]=otr[index]=(tv<<8);

			sprintf(rstr,"menu_%s_blue",menucoltxt[index-128]);
			if((tv=Read_Neutrino_Cfg(rstr))>=0)
				bl[index-128]=obl[index]=(tv<<9)+(tv<<7);

			sprintf(rstr,"menu_%s_green",menucoltxt[index-128]);
			if((tv=Read_Neutrino_Cfg(rstr))>=0)
				gn[index-128]=ogn[index]=(tv<<9)+(tv<<7);

			sprintf(rstr,"menu_%s_red",menucoltxt[index-128]);
			if((tv=Read_Neutrino_Cfg(rstr))>=0)
				rd[index-128]=ord[index]=(tv<<9)+(tv<<7);
		}
		
		memcpy(&ord[128],rd,32);
		memcpy(&ogn[128],gn,32);
		memcpy(&obl[128],bl,32);
		memcpy(&otr[128],tr,32);

		obl[CMS]=obl[CMCS]>>1;
		ogn[CMS]=ogn[CMCS]>>1;
		ord[CMS]=ord[CMCS]>>1;
		otr[CMS]=otr[CMCS];

		otr[COL_MENUHEAD_PLUS_0]=otr[CMH];			
		otr[COL_MENUCONTENT_PLUS_0]=otr[CMC];			
		otr[COL_MENUCONTENTSELECTED_PLUS_0]=otr[CMCS];
		otr[COL_MENUCONTENTINACTIVE_PLUS_0]=otr[CMCI];

		otr[TRANSP]=0xFFFF;
		ord[TRANSP]=ogn[TRANSP]=obl[TRANSP]=0;

		if(ioctl(fb, FBIOPUTCMAP, &oldcmap) == -1)
		{
			printf("Tuxwetter <FBIOPUTCMAP failed>\n");
			return 0;
		}
		if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
		{
			printf("Tuxwetter <mapping of Framebuffer failed>\n");
			return 0;
		}
		
#ifdef HAVE_DBOX_HARDWARE
		ioctl(fb, AVIA_GT_GV_GET_BLEV, &alpha);
#endif

	//init fontlibrary

		if((error = FT_Init_FreeType(&library)))
		{
			printf("Tuxwetter <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
		{
			printf("Tuxwetter <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_SBitCache_New(manager, &cache)))
		{
			printf("Tuxwetter <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_Lookup_Face(manager, FONT, &face)))
		{
			printf("Tuxwetter <FTC_Manager_Lookup_Face failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		use_kerning = FT_HAS_KERNING(face);

#ifdef FT_NEW_CACHE_API
		desc.face_id = FONT;
#else
		desc.font.face_id = FONT;
#endif

#if FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 0
		desc.image_type = ftc_image_mono;
#else
		desc.flags = FT_LOAD_MONOCHROME;
#endif
	//init backbuffer

		if(!(lbb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("Tuxwetter <allocating of Backbuffer failed>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);

		startx = sx + (((ex-sx) - 620)/2);
		starty = sy + (((ey-sy) - 505)/2);

	bmp2lcd (BMP_FILE);

//  Startbildschirm

	put_instance(instance=get_instance()+1);

	if(show_splash && !cmdline)
	{
		show_jpg("/share/tuxbox/tuxwetter/startbild.jpg", sx, sy, ex-sx, ey-sy, 5, 0, 1, 1);
	}

	//main loop
	
	menu.act_entry=0;
	if(cmdline)
	{
		AddListEntry(&menu, cmdline, 0);
		cindex=99;
	}
	else
	{
		if(Get_Menu())
		{
			printf("Tuxwetter <unable to read tuxwetter.conf>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			put_instance(instance=get_instance()-1);
			return -1;
		}
		cindex=0;
	}

	while(mainloop)
	{
		if(cindex!=99)
		{
			cindex=Get_Selection(&menu);
		}
		else
		{
			cindex=1;
		}
		dloop=1;
		switch(cindex)
		{
			case -99:
				show_data(-99);
				break;
			
			case -1:
				mainloop=0;
				break;
				
			case 0:
				mainloop=Menu_Up(&menu);
				break;
				
			case -98:
				if((tfh=fopen(MISS_FILE,"r"))!=NULL)
				{
					fclose(tfh);
					pl=&epl;
					sprintf(tstr,"%s,http://localhost/../../../..%s",prs_translate("Fehlende Übersetzungen",CONVERT_LIST),MISS_FILE);
					pl->entry=strdup(tstr);
				}
				else
				{	
					ShowMessage(prs_translate("Keine fehlenden Übersetzungen",CONVERT_LIST),1);
					break;
				}
			case 1:
				if(cindex==1)
				{
					pl=menu.list[menu.act_entry];
				}
				switch (pl->type)
				{
					case TYP_MENU:
						menu.act_header=pl->headerpos;
						menu.lastheaderentrys[menu.act_header]=menu.act_entry;
						Get_Menu();
						menu.act_entry=0;
						break;
						
					case TYP_EXECUTE:
						if((rptr=strchr(pl->entry,','))!=NULL)
						{
							rptr++;
							system(rptr);
							close(rc);
#ifdef HAVE_DBOX_HARDWARE
							rc = open(RC_DEVICE, O_RDONLY);
							fcntl(rc, F_SETFL, fcntl(rc, F_GETFL) | O_EXCL | O_NONBLOCK);
#else
							rc = open(RC_DEVICE, O_RDONLY|O_NONBLOCK);
#endif
#if HAVE_DVB_API_VERSION < 3
		// RC Buffer leeren
							char buf[32];
							read(rc,buf,32);
#endif
						close_jpg_gif_png();
						}
						break;
						
					case TYP_PICTURE:
					case TYP_PICHTML:
					case TYP_TXTHTML:
					case TYP_TEXTPAGE:
					case TYP_TXTPLAIN:
						dloop=1;
						*line_buffer=0;
						LCD_Init();
						do
						{
							if((pl->type==TYP_TXTHTML) || (pl->type==TYP_PICHTML))
							{
							char *pt1=pl->entry, *pt2=nstr;
							
								strcpy(line_buffer,strchr(pt1,',')+1);
								while(*pt1 && (*pt1 != ','))
									{
										*pt2++=*pt1++;
									}
								*pt2=0;	
								tret=0;
							}
							else
							{
								tret=Transform_Entry(pl->entry, line_buffer);
							}
							if(!tret)
							{
								if((pl->type==TYP_PICHTML) || (pl->type==TYP_TXTHTML))
								{
									if((rptr=translate_url(line_buffer, pl->type, pl->absolute))!=NULL)
									{
										strcpy(line_buffer,rptr);
									}
									else
									{
										close_jpg_gif_png();
										ShowMessage(prs_translate("Formatfehler der URL in der tuxwetter.conf",CONVERT_LIST),1);
										dloop=-1;
										break;
									}
								}
								
								if((pl->type==TYP_PICHTML) || (pl->type==TYP_PICTURE))
								{
									if(pl->pictype==PTYP_ASK)
									{
										if((rptr=strrchr(line_buffer,'.'))!=NULL)
										{
											++rptr;
											if(strncasecmp(rptr,"JPG",3)==0)
											{
												pl->pictype=PTYP_JPG;
											}
											else
											{
												if(strncasecmp(rptr,"GIF",3)==0)
												{
													pl->pictype=PTYP_GIF;
												}
												else
												{
													if(strncasecmp(rptr,"PNG",3)==0)
													{
														pl->pictype=PTYP_PNG;
													}
												}
											}
										}
									}
									if(pl->pictype==PTYP_ASK)
									{
										close_jpg_gif_png();
										ShowMessage(prs_translate("Nicht unterstütztes Dateiformat",CONVERT_LIST),1);
										dloop=-1;
										break;
									}
								}
							
							
							
								if((pl->type==TYP_TXTHTML) || (pl->type==TYP_TEXTPAGE) || (pl->type==TYP_TXTPLAIN))
								{
									if(gmodeon)
									{
										close_jpg_gif_png();
										ShowInfo(&menu);
									}
									if(!cmdline)
									{
										ShowMessage(prs_translate("Bitte warten",CONVERT_LIST),0);
									}
								}
								else
								{
									if(!gmodeon && !cmdline)
									{
										ShowMessage(prs_translate("Bitte warten",CONVERT_LIST),0);
									}
								}
				
								LCD_Clear();
								LCD_draw_rectangle (0,0,119,59, LCD_PIXEL_ON,LCD_PIXEL_OFF);
								LCD_draw_rectangle (3,3,116,56, LCD_PIXEL_ON,LCD_PIXEL_OFF);
								lpos=strlen(nstr);
								lrow=0;
								while(lpos>0)
								{
									strncpy(dstr,nstr+LCD_CPL*lrow,LCD_CPL);
									lpos-=LCD_CPL;
									LCD_draw_string(13, (lrow+2)*LCD_RDIST, dstr);
									lrow++;
								}
								LCD_update();
								ferr=(strlen(line_buffer))?0:-1;
					
								if((!ferr) && ((strcmp(line_buffer,lastpicture)!=0)||(loadalways)) && (pl->pictype!=TYP_TXTHTML))
								{
									rptr=line_buffer;
									if(*rptr=='\"')
									{
										++rptr;
									}
									printf("Tuxwetter <Downloading %s>\n",rptr);
									ferr=HTTP_downloadFile(rptr, (pl->pictype==PTYP_JPG)?JPG_FILE:(pl->pictype==PTYP_PNG)?PNG_FILE:(pl->pictype==PTYP_GIF)?GIF_FILE:PHP_FILE, 1, intype, ctmo, 2);
								}
					
								if(!ferr)
								{
									switch(pl->pictype)
									{
										case PTYP_JPG:
											tret=show_jpg(JPG_FILE, sx, sy, ex-sx, ey-sy, 5, pl->repeat, 0, 1);
										break;

										case PTYP_PNG:
											tret=show_png(PNG_FILE, sx, sy, ex-sx, ey-sy, 5, pl->repeat, 0, 1);
										break;

										case PTYP_GIF:
											tret=show_gif(GIF_FILE, sx, sy, ex-sx, ey-sy, 5, pl->repeat, 0, 1, (strcmp(line_buffer,lastpicture)==0));
										break;
			
										case TYP_TEXTPAGE:
											tret=show_php(PHP_FILE, nstr, 0, 1);
										break;
										
										case TYP_TXTHTML:
											tret=show_php(PHP_FILE, nstr, 0, 0);
										break;

										case TYP_TXTPLAIN:
										{
											FILE *fh1,*fh2;
											int cnt=0;
											char *pt1;
											
											tret=-1;
											if((fh1=fopen(PHP_FILE,"r"))!=NULL)
											{
												if((fh2=fopen(TMP_FILE,"w"))!=NULL)
												{
													while(fgets(rstr, BUFSIZE-1,fh1))
													{
														TrimString(rstr);
														pt1=rstr;
														cnt=0;
														fprintf(fh2,"<br>");
														while(*pt1)
														{	
															if(*pt1==' ' && cnt>40)
															{
																fprintf(fh2,"\n<br>");
																cnt=0;
															}
															else
															{
																fputc(*pt1,fh2);
																++cnt;
															}
														++pt1;
														}
														fprintf(fh2,"\n");
													}
													fclose(fh2);
													tret=show_php(TMP_FILE, nstr, 1, 0);
													if(cindex==-98 && tret==RC_RED)
													{
														xremove(MISS_FILE);
													}
												}
												fclose(fh1);
											}
										}
										break;
									}
									
									if(cindex!=-98)
									{
										strcpy(lastpicture,line_buffer);
	
										index=menu.act_entry;						
										switch(tret)
										{
											case -1:
												close_jpg_gif_png();
												ShowMessage(prs_translate("Datei kann nicht angezeigt werden.",CONVERT_LIST),1);
												dloop=-1;
												break;
									
											case RC_UP:
											case RC_MINUS:
												if(--index < 0)
												{
													index=menu.num_entrys-1;
												}
											break;
								
											case RC_DOWN:
											case RC_PLUS:
												if(++index>=menu.num_entrys)
												{
													index=0;
												}
											break;
								
											case RC_LEFT:
												*lastpicture=0;
											case RC_RIGHT:
											break;
															
											default:
												dloop=0;
											break;
										}
										menu.act_entry=index;
										pl=menu.list[menu.act_entry];
										if((pl->type!=TYP_PICTURE) && (pl->type!=TYP_PICHTML) && (pl->type!=TYP_TXTHTML) && (pl->type!=TYP_TXTPLAIN) && (pl->type!=TYP_TEXTPAGE))
										{
											dloop=0;
											cindex=99;
										}
									}
									else
									{
										dloop=0;
									}
								}
								else
								{
									close_jpg_gif_png();
									sprintf(tstr,"%s",prs_translate("Fehler",CONVERT_LIST));
									sprintf(tstr,"%s %d %s.",tstr,ferr,prs_translate("beim Download",CONVERT_LIST));
									ShowMessage(tstr,1);
									dloop=-1;
								}
							
							
							
							}
							else
							{
								close_jpg_gif_png();
								ShowMessage(prs_translate("Formatfehler der URL in der tuxwetter.conf",CONVERT_LIST),1);
								dloop=-1;
								break;
							}
						}
						while(dloop>0);
						close_jpg_gif_png();
						break;
												
					case TYP_CITY:
						if(!prs_get_city())
						{
		
							rcm = -1;
							sprintf(tstr," ");
							
							Clear_List(&funcs, 1);
							funcs.act_entry=0;
							
							sprintf(tstr,"%s %s",prs_translate("Wetterdaten für",CONVERT_LIST),city_name);
							if(funcs.headertxt[0])
							{
								free(funcs.headertxt[0]);
							}
							funcs.headertxt[0]=strdup(tstr);

							for(index=0; index<MAX_FUNCS; index++)
							{
								if(index==2)
								{
									sprintf(rstr,"%s",prs_translate("Heute",CONVERT_LIST));
								}
								else
								{
									prs_get_day(index-2, rstr, metric);
								}
								if((rptr=strchr(rstr,','))!=NULL)
								{
									*rptr=0;
								}		
								if(index>1)
								{
									sprintf(tstr,"%s %s",prs_translate("Vorschau für",CONVERT_LIST),rstr);
								}
								else
								{
									if(index==1)
									{
										sprintf(tstr,"%s",prs_translate("Wochentrend",CONVERT_LIST));
									}
									else
									{
										sprintf(tstr,"%s",prs_translate("Aktuelles Wetter",CONVERT_LIST));
									}
								}
								AddListEntry(&funcs,tstr, 0);
							}
							wloop=1;
							while(wloop)
							{
								clear_screen();
								switch(Get_Selection(&funcs))
								{
									case -99:
										show_data(-99);
										break;
				
									case -1:
										mainloop=0;
										wloop=0;
										break;
										
									case 0:
										wloop=0;
										break;
										
									case 1:
										dloop=1;
										while(dloop>0)
										{
											if(!cmdline)
											{
												ShowMessage(prs_translate("Bitte warten",CONVERT_LIST),0);
											}
											show_data(funcs.act_entry);
											rce=GetRCCode();
											while((rce != RC_OK) && (rce != RC_HOME) && (rce != RC_DOWN) && (rce != RC_UP) && (rce != RC_PLUS) && (rce != RC_MINUS))
											{
												rce=GetRCCode();
											}
											index=funcs.act_entry;
											if(gmodeon)
											{
												close_jpg_gif_png();
											}
											switch(rce)
											{
												case RC_UP:
												case RC_MINUS:
													if(--index < 0)
													{							
														index=MAX_FUNCS-1;
													}
													break;
						
												case RC_DOWN:
												case RC_PLUS:
													if(++index>=MAX_FUNCS)
													{
													index=0;
													}
													break;

												case RC_OK:
												case RC_HOME:
													dloop=0;
													break;	
											}
											funcs.act_entry=index;
										}
								}
							}
						}
						break;
				}	
				break;
		}
		clear_screen();
		if(cindex==-98 && epl.entry)
		{
			free(epl.entry);
			epl.entry=NULL;
		}
		
		if(cmdline)
		{
			mainloop=0;
		}
	}

	//cleanup

	// clear Display
	free(proxyadress);
	free(proxyuserpwd);
	
	Clear_List(&menu,-1);
	Clear_List(&funcs,-1);

	free(trstr);
	if(cmdline)
	{
		free(cmdline);
	}

	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);

	close_jpg_gif_png();
	if(ioctl(fb, FBIOPUTCMAP, &oldcmap) == -1)
	{
		printf("Tuxwetter <FBIOPUTCMAP failed>\n");
	}
	fcntl(rc, F_SETFL, O_NONBLOCK);
	
	clear_lcd();
	
	close(rc);

    for(index=0; index<32; index++)
    {
    	sprintf(tstr,"%s%02d.gif",GIF_MFILE,index);
    	xremove(tstr);
    }
	sprintf(tstr,"[ -e /tmp/picture* ] && rm /tmp/picture*");
	system(tstr);
	xremove("/tmp/tuxwettr.tmp");
	xremove("/tmp/bmps.tar");
	xremove("/tmp/icon.gif");
	xremove("/tmp/tempgif.gif");
	xremove(PHP_FILE);
	put_instance(get_instance()-1);
	
	if((tfh=fopen(TIME_FILE,"r"))!=NULL)
	{
		fclose(tfh);
		sprintf(line_buffer,"%s &",TIME_FILE);
		system(line_buffer);
	}
	free(line_buffer);

	// clear Display
	memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
	munmap(lfb, fix_screeninfo.smem_len);
	close(fb);
	free(lbb);
	return 0;
}
