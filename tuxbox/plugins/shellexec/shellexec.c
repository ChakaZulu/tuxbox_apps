/*
 * $Id: shellexec.c,v 1.3 2009/12/27 12:08:02 rhabarber1848 Exp $
 *
 * shellexec - d-box2 linux project
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
#include "shellexec.h"
#include "text.h"
#include "io.h"
#include "gfx.h"
#include "lcd.h"
#include "color.h"
#ifdef HAVE_DBOX_HARDWARE
#include <dbox/fb.h>
#endif
extern int FSIZE_BIG;
extern int FSIZE_MED;
extern int FSIZE_SMALL;

unsigned char FONT[64]= "/share/fonts/pakenham.ttf";
static char CFG_FILE[128]="/var/tuxbox/config/shellexec.conf";

#define LIST_STEP 	10
#define BUFSIZE 	4095
#define LCD_CPL 	12
#define LCD_RDIST 	10

#define SH_VERSION  2.50
typedef struct {int fnum; FILE *fh[16];} FSTRUCT, *PFSTRUCT;

static int direct[32];
int MAX_FUNCS=10;
static int STYP=1;

typedef struct {char *entry; char *message; int headerpos; int type; int underline; int stay; int showalways;} LISTENTRY;
typedef LISTENTRY *PLISTENTRY;
typedef PLISTENTRY	*LIST;
typedef struct {int num_headers; int act_header; int max_header; int *headerwait; int *headermed; char **headertxt; char **icon; int *headerlevels; int *lastheaderentrys; int num_entrys; int act_entry; int max_entrys; int num_active; char *menact; char *menactdep; LIST list;} MENU;
enum {TYP_MENU, TYP_MENUDON, TYP_MENUDOFF, TYP_MENUFON, TYP_MENUFOFF, TYP_MENUSON, TYP_MENUSOFF, TYP_EXECUTE, TYP_COMMENT, TYP_DEPENDON, TYP_DEPENDOFF, TYP_FILCONTON, TYP_FILCONTOFF, TYP_SHELLRESON, TYP_SHELLRESOFF, TYP_ENDMENU, TYP_INACTIVE};
static char TYPESTR[TYP_ENDMENU+1][13]={"MENU=","MENUDON=","MENUDOFF=","MENUFON=","MENUFOFF=","MENUSON=","MENUSOFF=","ACTION=","COMMENT=","DEPENDON=","DEPENDOFF=","FILCONTON=","FILCONTOFF=","SHELLRESON=","SHELLRESOFF=","ENDMENU"};
char NOMEM[]="ShellExec <Out of memory>\n";

MENU menu;

int Check_Config(void);
int Clear_List(MENU *m, int mode);
int Get_Selection(MENU *m);
int AddListEntry(MENU *m, char *line, int pos);
int Get_Menu(int showwait);
static void ShowInfo(MENU *m, int knew);


unsigned char *lfb = 0, *lbb = 0;
unsigned char title[256];
char url[256]="time.fu-berlin.de";
char *line_buffer=NULL;
unsigned char *trstr;
int mloop=1, paging=1, noepg=0, mtmo=120, radius=0;
int ixw=400, iyw=380, xoffs=13;
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
	printf("shellexec Version %.2f killed\n",SH_VERSION);
	exit(1);
}

char *strxchr(char *xstr, char srch)
{
int quota=0;
char *resptr=xstr;

	if(resptr)
	{
		while(*resptr)
		{
			if(!quota && (*resptr==srch))
			{
				return resptr;
			}
			if(*resptr=='\'')
			{
				quota^=1;
			}
			++resptr;
		}
	}
	return NULL;
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

int GetLine(char *buffer, int size, PFSTRUCT fstruct)
{
int rv=0;
char *pt1;

	if(fstruct->fnum<0)
	{
		return rv;
	}
	rv=(fgets(buffer, size, fstruct->fh[fstruct->fnum])!=NULL);
	if(!rv)
	{
		while(!rv)
		{
			if(!fstruct->fnum)
			{
				return rv;
			}
			else
			{
				fclose(fstruct->fh[fstruct->fnum]);
				--fstruct->fnum;
				rv=(fgets(buffer, size, fstruct->fh[fstruct->fnum])!=NULL);
			}
		}
	}
	if(rv)
	{
		TrimString(buffer);
		if(strstr(buffer,"INCLUDE=") && (fstruct->fnum<15) && ((pt1=strchr(buffer,'='))!=NULL))
		{
			if(((fstruct->fh[fstruct->fnum+1]=fopen(++pt1,"r"))!=NULL) && (fgets(buffer, BUFSIZE, fstruct->fh[fstruct->fnum+1])))
			{
				fstruct->fnum++;
				TrimString(buffer);
			}
		}
		TranslateString(buffer);
	}
	return rv;	
}

int ExistFile(char *fname)
{
FILE *efh;

	if((efh=fopen(fname,"r"))==NULL)
	{
		return 0;
	}
	fclose(efh);
	return 1;
}

int FileContainText(char *line)
{
int rv=0;
long flength;
char *pt1,*tline=strdup(line),*fbuf=NULL;
FILE *ffh;

	if((pt1=strchr(tline,' '))!=NULL)
	{
		*pt1=0;
		++pt1;
		if((ffh=fopen(tline,"r"))!=NULL)
		{
			fseek(ffh,0,SEEK_END);
			flength=ftell(ffh);
			rewind(ffh);
			if((fbuf=calloc(flength+1,sizeof(char)))!=NULL)
			{
				if(fread(fbuf,(size_t)flength,1,ffh)>0)
				{
					*(fbuf+flength)=0;
					rv=strstr(fbuf,pt1)!=NULL;
				}
				free(fbuf);
			}
			fclose(ffh);
		}
	}
	free(tline);
	return rv;
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
					{
						rv=1;
					}
					else
					{
						if(strstr(cfptr,"false")!=NULL)
						{
							rv=0;
						}
						else
						{
							rv=-1;
						}
					}
				}
			}
			if(styp==2)
			{
				if(sscanf(cfptr,"%x",&rv)!=1)
				{
					if(strstr(cfptr,"true")!=NULL)
					{
						rv=1;
					}
					else
					{
						if(strstr(cfptr,"false")!=NULL)
						{
							rv=0;
						}
						else
						{
							rv=-1;
						}
					}
				}
			}
//			printf("%s\n%s=%s -> %d\n",tstr,entry,cfptr,rv);
		}
		fclose(nfh);
	}
	return rv;
}

int IsMenu(char *buf)
{
int i, res=0;

	for(i=TYP_MENU; !res && i<=TYP_MENUSOFF; i++)
	{
		if(strstr(buf,TYPESTR[i])==buf)
		{
			res=1;
		}
	}
	return res;
}

void OnMenuClose(char *cmd, char *dep)
{
int res=1;

	if(dep)
	{
		res=!system(dep);	
		ClearKeys();
//		res=ExistFile(dep);
	}
	if(cmd && res)
	{
		ShowMessage("System-Aktualisierung", "Bitte warten", 0);
		system(cmd);
		ClearKeys();
	}
}

int Check_Config(void)
{
int rv=-1, level=0;
char *pt1,*pt2;
FSTRUCT fstr;

	if((fstr.fh[0]=fopen(CFG_FILE,"r"))!=NULL)
	{
		fstr.fnum=0;
		while(GetLine(line_buffer, BUFSIZE, &fstr))
		{
			if(IsMenu(line_buffer))
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
					if((menu.icon=realloc(menu.icon,(menu.max_header+LIST_STEP)*sizeof(char*)))==NULL)
					{
						printf(NOMEM);
						Clear_List(&menu,0);
						return rv;
					}
					memset(&menu.icon[menu.max_header],0,LIST_STEP*sizeof(char*));
					if((menu.headerlevels=realloc(menu.headerlevels,(menu.max_header+LIST_STEP)*sizeof(int)))==NULL)
					{
						printf(NOMEM);
						Clear_List(&menu,0);
						return rv;
					}
					memset(&menu.headerlevels[menu.max_header],0,LIST_STEP*sizeof(int));
					if((menu.headerwait=realloc(menu.headerwait,(menu.max_header+LIST_STEP)*sizeof(int)))==NULL)
					{
						printf(NOMEM);
						Clear_List(&menu,0);
						return rv;
					}
					memset(&menu.headerwait[menu.max_header],0,LIST_STEP*sizeof(int));
					if((menu.headermed=realloc(menu.headermed,(menu.max_header+LIST_STEP)*sizeof(int)))==NULL)
					{
						printf(NOMEM);
						Clear_List(&menu,0);
						return rv;
					}
					memset(&menu.headermed[menu.max_header],0,LIST_STEP*sizeof(int));
					if((menu.lastheaderentrys=realloc(menu.lastheaderentrys,(menu.max_header+LIST_STEP)*sizeof(int)))==NULL)
					{
						printf(NOMEM);
						Clear_List(&menu,0);
						return rv;
					}
					memset(&menu.lastheaderentrys[menu.max_header],0,LIST_STEP*sizeof(int));
					menu.max_header+=LIST_STEP;
				}
				pt1=strchr(line_buffer,'=');
				if(!pt1)
				{
					pt1=line_buffer;
				}
				else
				{
					++pt1;
				}
				pt2=pt1;
				while(*pt2 && ((*pt2=='*') || (*pt2=='&') || (*pt2=='�') || (*pt2=='+') || (*pt2=='-') || (*pt2=='!') || (*pt2=='_')))
				{
					if(*pt2=='_')
					{
						menu.headermed[menu.num_headers]=1;
					}
					while(*(++pt2))
					{
						*(pt2-1)=*pt2;
					}
					*(pt2-1)=0;
					pt2=pt1;
				}
				
				if(menu.icon[menu.num_headers])
				{
					free(menu.icon[menu.num_headers]);
					menu.icon[menu.num_headers]=NULL;
				}
				if((pt2=strstr(pt1,",ICON="))!=NULL)
				{
					*pt2=0;
					menu.icon[menu.num_headers]=strdup(pt2+6);
				}
				if(menu.headertxt[menu.num_headers])
				{
					free(menu.headertxt[menu.num_headers]);
				}
				menu.headerlevels[menu.num_headers]=level++;
				if((pt2=strxchr(pt1,','))!=NULL)
				{
					*pt2=0;
				}
				menu.headertxt[menu.num_headers++]=strdup(pt1);
			}
			else
			{
				if(strstr(line_buffer,TYPESTR[TYP_ENDMENU])==line_buffer)
				{
					--level;
				}
				else
				{
					if(strstr(line_buffer,"FONT=")==line_buffer)
					{
						strcpy(FONT,strchr(line_buffer,'=')+1);
					}
					if(strstr(line_buffer,"FONTSIZE=")==line_buffer)
					{
						sscanf(strchr(line_buffer,'=')+1,"%d",&FSIZE_MED);
						FSIZE_BIG=(FSIZE_MED*5)/4;
						FSIZE_SMALL=(FSIZE_MED*4)/5;
					}
					if(strstr(line_buffer,"PAGING=")==line_buffer)
					{
						sscanf(strchr(line_buffer,'=')+1,"%d",&paging);
					}
					if(strstr(line_buffer,"LINESPP=")==line_buffer)
					{
						sscanf(strchr(line_buffer,'=')+1,"%d",&MAX_FUNCS);
					}
					if(strstr(line_buffer,"WIDTH=")==line_buffer)
					{
						sscanf(strchr(line_buffer,'=')+1,"%d",&ixw);
					}
					if(strstr(line_buffer,"HIGHT=")==line_buffer)
					{
						sscanf(strchr(line_buffer,'=')+1,"%d",&iyw);
					}
					if(strstr(line_buffer,"KILLEPG=")==line_buffer)
					{
						sscanf(strchr(line_buffer,'=')+1,"%d",&noepg);
					}
					if(strstr(line_buffer,"TIMESERVICE=")==line_buffer)
					{
						strcpy(url,strchr(line_buffer,'=')+1);
						if(strstr(url,"NONE") || strlen(url)<4)
						{
							*url=0;
						}
					}
				}
			}
//printf("Check_Config: Level: %d -> %s\n",level,line_buffer);			
		}
		rv=0;
		fclose(fstr.fh[fstr.fnum]);
	}
	ixw=(ixw>(ex-sx))?(ex-sx):((ixw<400)?400:ixw);
	iyw=(iyw>(ey-sy))?(ey-sy):((iyw<380)?380:iyw);
	return rv;
}

int Clear_List(MENU *m, int mode)
{
int i;
PLISTENTRY entr;

	if(m->menact)
	{
		free(m->menact);
		m->menact=NULL;
	}
	if(m->menactdep)
	{
		free(m->menactdep);
		m->menactdep=NULL;
	}
	if(m->max_entrys)
	{
		for(i=0; i<m->num_entrys; i++)
		{
			if(m->list[i]->entry) free(m->list[i]->entry);
			if(m->list[i]->message) free(m->list[i]->message);
			free(m->list[i]);
		}
		m->num_entrys=0;
		m->max_entrys=0;
		m->num_active=0;
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
int rv=1,rccode, mloop=1,/*lrow,lpos,*/i,j,first,last,active,knew=1;
time_t tm1,tm2;
//char dstr[BUFSIZE],*lcptr,*lcstr,*lcdptr;

	if(m->num_active)
	{
//		LCD_Init();
		i=m->act_entry;
		while((i>=0) && ((m->list[i]->type==TYP_COMMENT) || (m->list[i]->type==TYP_INACTIVE)))
		{
			++i;
			if(i>=m->num_entrys)
			{
				i=-1;
			}
		}
		if(i==-1)
		{
			i=0;
		}
		m->act_entry=i;
	}
	time(&tm1);
	do{
//		usleep(100000L);
		first=(paging)?0:(MAX_FUNCS*(int)(m->act_entry/MAX_FUNCS));
		last=(paging)?(m->num_entrys-1):(MAX_FUNCS*(int)(m->act_entry/MAX_FUNCS)+MAX_FUNCS-1);
		if(last>=m->num_entrys)
		{
			last=m->num_entrys-1;
		}
		
		active=0;
		for(i=first; i<=last && !active; i++)
		{
			active |= ((m->list[i]->type != TYP_COMMENT) && (m->list[i]->type != TYP_INACTIVE));
		}
		
		rccode=-1;
		if(knew)
		{
			ShowInfo(m, knew);
		}
/*
		if(m->num_active && knew)
		{
			knew=0;
			if(m->list[m->act_entry]->entry)
			{
				sprintf(trstr,"%s%s",(m->list[m->act_entry]->type<=TYP_MENUSOFF)?"> ":"",m->list[m->act_entry]->entry);
				if((lcptr=strxchr(trstr,','))!=NULL)
				{
					*lcptr=0;
				}
			}
			else
			{
				sprintf(trstr,"Kein Eintrag");
			}
			lcstr=strdup(trstr);
			lcdptr=lcstr;
			lcptr=trstr;
			while(*lcptr)
			{
				if(*lcptr=='~')
				{
					++lcptr;
					if(*lcptr)
					{
						if(*lcptr=='t' || *lcptr=='T')
						{
							*(lcdptr++)=' ';
						}
						++lcptr;
					}
				}
				else
				{
					*(lcdptr++)=*(lcptr++);
				}
			}
			*lcdptr=0;
			LCD_Clear();
//			LCD_draw_rectangle (0,0,119,59, LCD_PIXEL_ON,LCD_PIXEL_OFF);
//			LCD_draw_rectangle (3,3,116,56, LCD_PIXEL_ON,LCD_PIXEL_OFF);
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
			free(lcstr);
		}
*/		
		knew=1;
		switch(rccode = GetRCCode())
		{
			case RC_RED:
				if(active && direct[0]>=0)
				{	
					m->act_entry=direct[0];
					rv=1;
					mloop=0;
				}
				break;

			case RC_GREEN:	
				if(active && direct[1]>=0)
				{	
					m->act_entry=direct[1];
					rv=1;
					mloop=0;
				}
				break;

			case RC_YELLOW:	
				if(active && direct[2]>=0)
				{	
					m->act_entry=direct[2];
					rv=1;
					mloop=0;
				}
				break;

			case RC_BLUE:	
				if(active && direct[3]>=0)
				{	
					m->act_entry=direct[3];
					rv=1;
					mloop=0;
				}
				break;

			case RC_1:
				if(active && direct[4]>=0)
				{	
					m->act_entry=direct[4];
					rv=1;
					mloop=0;
				}
				break;

			case RC_2:
				if(active && direct[5]>=0)
				{	
					m->act_entry=direct[5];
					rv=1;
					mloop=0;
				}
				break;

			case RC_3:
				if(active && direct[6]>=0)
				{	
					m->act_entry=direct[6];
					rv=1;
					mloop=0;
				}
				break;

			case RC_4:
				if(active && direct[7]>=0)
				{	
					m->act_entry=direct[7];
					rv=1;
					mloop=0;
				}
				break;

			case RC_5:
				if(active && direct[8]>=0)
				{	
					m->act_entry=direct[8];
					rv=1;
					mloop=0;
				}
				break;

			case RC_6:
				if(active && direct[9]>=0)
				{	
					m->act_entry=direct[9];
					rv=1;
					mloop=0;
				}
				break;

			case RC_7:
				if(active && direct[10]>=0)
				{	
					m->act_entry=direct[10];
					rv=1;
					mloop=0;
				}
				break;

			case RC_8:
				if(active && direct[11]>=0)
				{	
					m->act_entry=direct[11];
					rv=1;
					mloop=0;
				}
				break;

			case RC_9:
				if(active && direct[12]>=0)
				{	
					m->act_entry=direct[12];
					rv=1;
					mloop=0;
				}
				break;

			case RC_0:
				if(active && direct[13]>=0)
				{	
					m->act_entry=direct[13];
					rv=1;
					mloop=0;
				}
				break;

			case RC_UP:
			case RC_MINUS:
				if(m->num_active)
				{
					i=m->act_entry-1;
					if(i<first)
					{
						i=last;
					}
					while(active && ((m->list[i]->type==TYP_COMMENT) || (m->list[i]->type==TYP_INACTIVE)))
					{
						--i;
						if(i<first)
						{
							i=last;
						}
					}
					m->act_entry=i;
				}
//				knew=1;
				break;

			case RC_DOWN:	
			case RC_PLUS:
				if(m->num_active)
				{
					i=m->act_entry+1;
					if(i>last)
					{
						i=first;
					}
					while(active && ((m->list[i]->type==TYP_COMMENT) || (m->list[i]->type==TYP_INACTIVE)))
					{
						++i;
						if(i>last)
						{
							i=first;
						}
					}
					m->act_entry=i;
				}
//				knew=1;
				break;

			case RC_LEFT:
				i=MAX_FUNCS*(m->act_entry/MAX_FUNCS)-MAX_FUNCS;
				if(i<0)
				{
					i=MAX_FUNCS*((m->num_entrys-1)/MAX_FUNCS);
				}
				j=0;
				while((m->list[i+j]->type==TYP_COMMENT || m->list[i+j]->type==TYP_INACTIVE) && active && (i+j)<=(last+MAX_FUNCS) && (i+j)<m->num_entrys)
				{
					++j;
				}
				if((i+j)<=(i+j)<=(last+MAX_FUNCS) && (i+j)<m->num_entrys)
				{
					i+=j;
				}
				m->act_entry=i;
//				knew=1;
				break;

			case RC_RIGHT:
				i=MAX_FUNCS*(m->act_entry/MAX_FUNCS)+MAX_FUNCS;
				if(i>=m->num_entrys)
				{
					i=0;
				}
				j=0;
				while((m->list[i+j]->type==TYP_COMMENT || m->list[i+j]->type==TYP_INACTIVE) && active && (i+j)<=(last+MAX_FUNCS) && (i+j)<m->num_entrys)
				{
					++j;
				}
				if((i+j)<=(last+MAX_FUNCS) && (i+j)<m->num_entrys)
				{
					i+=j;
				}
				m->act_entry=i;
//				knew=1;
				break;

			case RC_OK:	
				if(m->num_active)
				{
					rv=1;
					mloop=0;
				}
				break;

			case -1:
				knew=0;
				time(&tm2);
//printf("TLeft: %3d\r",mtmo-(tm2-tm1));				
				if((tm2-tm1)<mtmo)
				{
					break;
				}
				rv=RC_HOME;
			case RC_HOME:	
				rv=0;
				mloop=0;
				break;

			case RC_MUTE:	
				memset(lfb, TRANSP, 720*576);
				usleep(500000L);
				while(GetRCCode()!=-1)
				{
					usleep(100000L);
				}
				while(GetRCCode()!=RC_MUTE)
				{
					usleep(500000L);
				}
				while((rccode=GetRCCode())!=-1)
				{
					usleep(100000L);
				}
//				knew=1;
			break;

			case RC_STANDBY:
				rv=-1;
				mloop=0;
			break;
			
			default: knew=0; break;
		}
		if(rccode!=-1)
		{
			time(&tm1);
		}
	} while(mloop);

	ShowInfo(m, knew);

return rv;
}

int AddListEntry(MENU *m, char *line, int pos)
{
int i,found=0,pfound=0;
PLISTENTRY entr;
char *ptr1,*ptr2,*ptr3,*ptr4, *wstr;


	if(!strlen(line))
	{
		return 1;
	}
//printf("AddListEntry: %s\n",line);	
	wstr=strdup(line);
	
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
	entr->underline=entr->stay=entr->showalways=0;

	for(i=TYP_MENU; !found && i<=TYP_SHELLRESOFF; i++)
	{
		ptr4=NULL;
		if((ptr1=strstr(wstr,TYPESTR[i]))==wstr)
		{
			ptr1=strchr(wstr,'=');
			ptr1++;
			ptr2=ptr1;
			while(*ptr2 && ((*ptr2=='*') || (*ptr2=='&') || (*ptr2=='�') || (*ptr2=='+') || (*ptr2=='-') || (*ptr2=='!') || (*ptr2=='_')))
			{
				switch(*ptr2)
				{
					case '*': entr->underline=1; break;
					case '!': entr->underline=2; break;
					case '+': entr->showalways=1; break;
					case '-': entr->showalways=2; break;
					case '&': entr->stay=1; break;
					case '�': entr->stay=2; break;
				}
				while(*(++ptr2))
				{
					*(ptr2-1)=*ptr2;
				}
				*(ptr2-1)=0;
				ptr2=ptr1;
			}
			switch (i)
			{
				case TYP_EXECUTE:
				case TYP_MENUDON:
				case TYP_MENUDOFF:
				case TYP_MENUFON:
				case TYP_MENUFOFF:
					if((ptr2=strxchr(ptr1,','))!=NULL)
					{
						if((ptr4=strstr(ptr1,",ICON="))!=NULL)
						{
							*ptr4=0;
						}
						if((ptr4=strxchr(ptr2+1,','))!=NULL)
						{
							*ptr4=0;
							entr->message=strdup(ptr4+1);
						}
					}
				break;
				
				case TYP_MENU:
					if((ptr2=strstr(ptr1,",ICON="))!=NULL)
					{
						*ptr2=0;
					}
					if((ptr2=strxchr(ptr1,','))!=NULL)
					{
						*ptr2=0;
						entr->message=strdup(ptr2+1);
					}
				break;
			}
			switch (i)
			{
				case TYP_EXECUTE:
				case TYP_MENU:
				case TYP_COMMENT:
					entr->type=i;
					entr->entry=strdup(ptr1);
					entr->headerpos=pos;
					m->num_entrys++;
					found=1;
					break;
					
				case TYP_DEPENDON:
				case TYP_DEPENDOFF:
				case TYP_MENUDON:
				case TYP_MENUDOFF:
				case TYP_FILCONTON:
				case TYP_FILCONTOFF:
				case TYP_MENUFON:
				case TYP_MENUFOFF:
					if((ptr2=strstr(ptr1,",ICON="))!=NULL)
					{
						*ptr2=0;
					}
					if((ptr2=strxchr(ptr1,','))!=NULL)
					{
						if(i<TYP_EXECUTE)
						{
							ptr3=ptr2;
						}
						else
						{
							ptr2++;
							ptr3=strxchr(ptr2,',');
							ptr4=strxchr(ptr3+1,',');
						}
						if(ptr3!=NULL)
						{
							*ptr3=0;
							ptr3++;
							found=1;
							if(ptr4)
							{
								*ptr4=0;
							}
							if((i==TYP_FILCONTON) || (i==TYP_FILCONTOFF) || (i==TYP_MENUFON) || (i==TYP_MENUFOFF))
							{
								pfound=FileContainText(ptr3);
							}
							else
							{
								pfound=ExistFile(ptr3);
							}
							if((((i==TYP_DEPENDON)||(i==TYP_MENUDON)||(i==TYP_FILCONTON)||(i==TYP_MENUFON)) && pfound) || (((i==TYP_DEPENDOFF)||(i==TYP_MENUDOFF)||(i==TYP_FILCONTOFF)||(i==TYP_MENUFOFF)) && !pfound))
							{							
								entr->type=(i<TYP_EXECUTE)?TYP_MENU:((strlen(ptr2))?TYP_EXECUTE:TYP_INACTIVE);
								entr->entry=strdup(ptr1);
								if(ptr4)
								{
									entr->message=strdup(ptr4+1);
								}
								entr->headerpos=pos;
								m->num_entrys++;
							}
							else
							{
								if(entr->showalways)
								{							
									entr->type=TYP_INACTIVE;
									entr->entry=strdup(ptr1);
									entr->headerpos=pos;
									m->num_entrys++;
								}
							}
						}
					}
					break;

				case TYP_SHELLRESON:
				case TYP_SHELLRESOFF:
				case TYP_MENUSON:
				case TYP_MENUSOFF:
					if((ptr2=strstr(ptr1,",ICON="))!=NULL)
					{
						*ptr2=0;
					}
					if((ptr2=strxchr(ptr1,','))!=NULL)
					{
						if(i<TYP_EXECUTE)
						{
							ptr3=ptr2;
						}
						else
						{
							ptr2++;
							ptr3=strxchr(ptr2,',');
							ptr4=strxchr(ptr3+1,',');
						}
						if(ptr3!=NULL)
						{
							*ptr3=0;
							ptr3++;
							found=1;
							if(ptr4)
							{
								*ptr4=0;
							}
							pfound=system(ptr3);
							if((((i==TYP_SHELLRESON)||(i==TYP_MENUSON)) && !pfound) || (((i==TYP_SHELLRESOFF)||(i==TYP_MENUSOFF)) && pfound))
							{							
								entr->type=(i<TYP_EXECUTE)?TYP_MENU:((strlen(ptr2))?TYP_EXECUTE:TYP_INACTIVE);
								entr->entry=strdup(ptr1);
								if(ptr4)
								{
									entr->message=strdup(ptr4+1);
								}
								entr->headerpos=pos;
								m->num_entrys++;
							}
							else
							{
								if(entr->showalways)
								{							
									entr->type=TYP_INACTIVE;
									entr->entry=strdup(ptr1);
									entr->headerpos=pos;
									m->num_entrys++;
								}
							}
						}
					}
					break;
			}		
			if(found && (i != TYP_COMMENT) && (i != TYP_INACTIVE))
			{
				m->num_active++;
			}
		}
	}
	free(wstr);
	return !found;

}

int Get_Menu(int showwait)
{
int rv=-1, loop=1, mlevel=0, clevel=0, pos=0;
char *pt1,*pt2;
FSTRUCT fstr;

	if(showwait && menu.headerwait[menu.act_header] && menu.headertxt[menu.act_header])
	{
		ShowMessage(menu.headertxt[menu.act_header],"Bitte warten ...",0);
	}
	Clear_List(&menu,1);
	if((fstr.fh[0]=fopen(CFG_FILE,"r"))!=NULL)
	{
		loop=1;
		menu.num_active=0;
		fstr.fnum=0;
		while((loop==1) && GetLine(line_buffer, BUFSIZE, &fstr))
		{
			if(IsMenu(line_buffer))
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
//printf("Get_Menu: loop: %d, mlevel: %d, pos: %d -> %s\n",loop,mlevel,pos,line_buffer);			
		}
		if(loop)
		{
			return rv;
		}
		
		--pos;
		--mlevel;
		loop=1;
		while((loop==1) && GetLine(line_buffer, BUFSIZE, &fstr))
		{
			if(IsMenu(line_buffer))
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
				if(mlevel==clevel)
				{
					if((pt1=strchr(line_buffer,'='))!=NULL)
					{
						pt1++;
						if((pt2=strxchr(pt1,','))!=NULL)
						{
							*(pt2++)=0;
							menu.menactdep=strdup(pt2);
						}
						menu.menact=strdup(pt1);
					}
				
				}
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
	fclose(fstr.fh[fstr.fnum]);
	}

	return rv;
}

void clean_string(char *trstr, char *lcstr)
{
int i;
char *lcdptr,*lcptr,*tptr;

	lcdptr=lcstr;
	lcptr=trstr;
	while(*lcptr)
	{
		if(*lcptr=='~')
		{
			++lcptr;
			if(*lcptr)
			{
				if(*lcptr=='t')
				{
					*(lcdptr++)=' ';
				}
				else
				{
					if(*lcptr=='T')
					{
						*(lcdptr++)=' ';
						lcptr++;
						if (*lcptr && sscanf(lcptr,"%3d",&i)==1)
						{
							i=2;
							while(i-- && *(lcptr++));
						}
					}
				}
				++lcptr;
			}
		}
		else
		{
			*(lcdptr++)=*(lcptr++);
		}
	}
	*lcdptr=0;
	lcptr=tptr=lcstr;
	while(*tptr)
	{
		if(*tptr==0x27)
		{
			memmove(tptr,tptr+1,strlen(tptr));
		}
		++tptr;
	}
}
/******************************************************************************
 * ShowInfo
 ******************************************************************************/

#define XX 0x6C
#define XL 58

static void ShowInfo(MENU *m, int knew )
{
	int loop, dloop, lrow,lpos,ldy,stlen;
	double scrollbar_len, scrollbar_ofs, scrollbar_cor;
	int index=m->act_entry,tind=m->act_entry, sbw=(m->num_entrys>MAX_FUNCS)?12:0;
	char tstr[BUFSIZE], *tptr;
	char dstr[BUFSIZE],*lcptr,*lcstr;
	int dy, my, moffs, mh, toffs, soffs=4, oldx=startx, oldy=starty, sbar=0, nosel;
	PLISTENTRY pl;
	
	moffs=iyw/(MAX_FUNCS+1);
	mh=iyw-moffs;
	dy=mh/(MAX_FUNCS+1);
	toffs=dy/2;
	my=moffs+dy+toffs;
	
	startx = sx + (((ex-sx) - ixw)/2);
	starty = sy + (((ey-sy) - iyw)/2);

	tind=index;
	
	//frame layout
	RenderBox(0, 0, ixw, iyw, radius, CMC);
//	RenderBox(0, 0, ixw, iyw, GRID, CMCS);

	// titlebar
	RenderBox(2, 2, ixw-2, moffs+5, radius, CMH);
	
	for(loop=MAX_FUNCS*(index/MAX_FUNCS); loop<MAX_FUNCS*(index/MAX_FUNCS+1) && loop<m->num_entrys && !sbar; loop++)
	{
		pl=m->list[loop];
		sbar |= ((pl->type!=TYP_COMMENT) && (pl->type!=TYP_INACTIVE));
	}
	--loop;
	if(loop>index)
	{
		m->act_entry=index=loop;
	}
	//selectbar
/*	if(m->num_active && sbar)
	{
		RenderBox(2, moffs+toffs+soffs+(index%MAX_FUNCS)*dy, ixw-sbw, moffs+toffs+soffs+(index%MAX_FUNCS+1)*dy, radius, CMCS);
	}
*/

	if(sbw)
	{
		//sliderframe
		RenderBox(ixw-sbw, moffs, ixw, iyw, radius, COL_MENUCONTENT_PLUS_1);
		//slider
		
/*		Alte Sliderdarstellung
		scrollbar_len = (double)mh / (double)m->num_entrys;
		scrollbar_ofs = scrollbar_len*(double)index;
		scrollbar_cor = (double)mh - (((double)mh/scrollbar_len)*scrollbar_len);
		if(scrollbar_cor<5)
		{
			scrollbar_cor=5;
		}
		RenderBox(ixw-sbw, moffs + scrollbar_ofs, ixw, moffs + scrollbar_ofs + scrollbar_len +scrollbar_cor , radius, CMCIT);
*/
		scrollbar_len = (double)mh / (double)((m->num_entrys/MAX_FUNCS+1)*MAX_FUNCS);
		scrollbar_ofs = scrollbar_len*(double)((index/MAX_FUNCS)*MAX_FUNCS);
		scrollbar_cor = scrollbar_len*(double)MAX_FUNCS;
		RenderBox(ixw-sbw, moffs + scrollbar_ofs, ixw, moffs + scrollbar_ofs + scrollbar_cor , radius, COL_MENUCONTENT_PLUS_3);		
	}

	// Title text
	lcstr=strdup(m->headertxt[m->act_header]);
	clean_string(m->headertxt[m->act_header],lcstr);
	RenderString(lcstr, (m->headermed[m->act_header]==1)?0:45, dy-soffs+FSIZE_BIG/10, ixw-sbw, (m->headermed[m->act_header]==1)?CENTER:LEFT, FSIZE_BIG, CMHT);
	free(lcstr);

	if(m->icon[m->act_header])
	{
		PaintIcon(m->icon[m->act_header],xoffs-6,soffs+2,1);
	}
	
	index /= MAX_FUNCS;
	dloop=0;
	ldy=dy;
	//Show table of commands
	for(loop = index*MAX_FUNCS; (loop < (index+1)*MAX_FUNCS) && (loop < m->num_entrys); ++loop)
	{
		dy=ldy;
		pl=m->list[loop];
		strcpy(dstr,pl->entry);
		if((tptr=strxchr(dstr,','))!=NULL)
		{
			if(pl->type != TYP_COMMENT)
			{
				*tptr=0;
			}
		}
		lcptr=tptr=dstr;
		while(*tptr)
		{
			if(*tptr==0x27)
			{
				memmove(tptr,tptr+1,strlen(tptr));
			}
			++tptr;
		}

		if(m->num_active && sbar && (loop==m->act_entry))
		{
			RenderBox(2, my+soffs-dy, ixw-sbw, my+soffs, radius, CMCS);
		}	
		nosel=(pl->type==TYP_COMMENT) || (pl->type==TYP_INACTIVE);
		if(!(pl->type==TYP_COMMENT && pl->underline==2))
		{		
			RenderString(dstr, 45, my, ixw-sbw-65, LEFT, (pl->type==TYP_COMMENT)?SMALL:MED, (((loop%MAX_FUNCS) == (tind%MAX_FUNCS)) && (sbar) && (!nosel))?CMCST:(nosel)?CMCIT:CMCT);
		}
		if(pl->type==TYP_MENU)
		{
			RenderString(">", 30, my, 65, LEFT, MED, (((loop%MAX_FUNCS) == (tind%MAX_FUNCS)) && (sbar) && (!nosel))?CMCST:CMCT);
		}
		if(pl->underline)
		{
			int cloffs=0,ccenter=0;
			if(pl->type==TYP_COMMENT)
			{ 
				if(strlen(dstr)==0)
				{
					if(pl->underline==2)
					{
						dy/=2;
						cloffs=4*dy/3;
					}
					else
					{
						cloffs=dy/3;
					}
				}
				else			
				{
					if(pl->underline==2)
					{
						cloffs=dy/3;
						ccenter=1;
					}
				}
			}
			else
			{
				if(pl->underline==2)
				{
					dy+=dy/2;
					cloffs=-dy/4;
				}
			}
			if(ccenter)
			{
				RenderBox(xoffs, my+soffs-cloffs-2, ixw-10-sbw, my+soffs-cloffs, 0, COL_MENUCONTENT_PLUS_3);
				RenderBox(xoffs, my+soffs-cloffs-2+1, ixw-10-sbw, my+soffs-cloffs+1, 0, COL_MENUCONTENT_PLUS_1);
				RenderString("X", xoffs, my, ixw-sbw, CENTER, MED, COL_MENUCONTENT_PLUS_0);
				stlen=GetStringLen(xoffs, dstr);
				RenderBox(xoffs+(ixw-xoffs-sbw)/2-stlen/2, my+soffs-ldy, xoffs+(ixw-xoffs-sbw)/2+stlen/2+15, my+soffs, FILL, CMC);
				RenderString(dstr, xoffs, my, ixw-sbw, CENTER, MED, CMCIT);
			}
			else
			{
				RenderBox(xoffs, my+soffs-cloffs, ixw-xoffs-sbw, my+soffs-cloffs, 0, COL_MENUCONTENT_PLUS_3);
				RenderBox(xoffs, my+soffs-cloffs+1, ixw-xoffs-sbw, my+soffs-cloffs+1, 0, COL_MENUCONTENT_PLUS_1);
			}
		}
		
		if((pl->type!=TYP_COMMENT) && ((pl->type!=TYP_INACTIVE) || (pl->showalways==2)))
		{
			direct[dloop++]=(pl->type!=TYP_INACTIVE)?loop:-1;
			switch(dloop)
			{
/*
				case 1: RenderCircle(xoffs+1,my-15,'R'); break;
				case 2: RenderCircle(xoffs+1,my-15,'G'); break;
				case 3: RenderCircle(xoffs+1,my-15,'Y'); break;
				case 4: RenderCircle(xoffs+1,my-15,'B'); break;
*/
				case 1: PaintIcon("/share/tuxbox/neutrino/icons/rot.raw",xoffs-2,my-15,1); break;
				case 2: PaintIcon("/share/tuxbox/neutrino/icons/gruen.raw",xoffs-2,my-15,1); break;
				case 3: PaintIcon("/share/tuxbox/neutrino/icons/gelb.raw",xoffs-2,my-15,1); break;
				case 4: PaintIcon("/share/tuxbox/neutrino/icons/blau.raw",xoffs-2,my-15,1); break;

				default:
					if(dloop<15)
					{
						sprintf(tstr,"%1d",(dloop-4)%10);
						RenderString(tstr, xoffs, my-1, 15, CENTER, SMALL, ((loop%MAX_FUNCS) == (tind%MAX_FUNCS))?CMCST:((pl->type==TYP_INACTIVE)?CMCIT:CMCT));
					}
				break;
			}
		}
		my += dy;
	}
	dy=ldy;
	for(; dloop<MAX_FUNCS; dloop++)
	{
		direct[dloop]=-1;
	}

	//copy backbuffer to framebuffer
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

	if(m->num_active && knew)
		{
			if(m->list[m->act_entry]->entry)
			{
				sprintf(trstr,"%s%s",(m->list[m->act_entry]->type<=TYP_MENUSOFF)?"> ":"",m->list[m->act_entry]->entry);
				if((lcptr=strxchr(trstr,','))!=NULL)
				{
					*lcptr=0;
				}
			}
			else
			{
				sprintf(trstr,"Kein Eintrag");
			}
			lcstr=strdup(trstr);
			clean_string(trstr,lcstr);
			LCD_Clear();
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
			free(lcstr);
		}

	startx=oldx;
	starty=oldy;
}


int Menu_Up(MENU *m)
{
int llev=m->headerlevels[m->act_header], lmen=m->act_header, lentr=m->lastheaderentrys[m->act_header];
	
	if(m->menact)
	{
		OnMenuClose(m->menact,m->menactdep);
	}
	while((lmen>=0) && (m->headerlevels[lmen]>=llev))
	{
		--lmen;
	}
	if(lmen<0)
	{
		return 0;
	}
	m->act_header=lmen;
	Get_Menu(1);
	m->act_entry=lentr;
	
	return 1;	
}


/******************************************************************************
 * shellexec Main
 ******************************************************************************/

#define LCD_CPL 12
#define LCD_RDIST 10


int main (int argc, char **argv)
{
int index=0,cindex=0,mainloop=1,dloop=1,tv;
char tstr[BUFSIZE], *rptr;
PLISTENTRY pl;
unsigned int alpha;

		printf("shellexec Version %.2f\n",SH_VERSION);
		for(tv=1; tv<argc; tv++)
		{
			if(*argv[tv]=='/')
			{
				strcpy(CFG_FILE,argv[tv]);
			}
		}

		if((line_buffer=calloc(BUFSIZE+1, sizeof(char)))==NULL)
		{
			printf(NOMEM);
			return -1;
		}
	
		if((trstr=calloc(BUFSIZE+1, sizeof(char)))==NULL)
		{
			printf(NOMEM);
			return -1;
		}
	
		if((debounce=Read_Neutrino_Cfg("repeat_genericblocker"))<0)
			debounce=200;
			
		if((rblock=Read_Neutrino_Cfg("repeat_blocker"))<0)
			rblock=50;
			
		if(((sx=Read_Neutrino_Cfg("screen_StartX"))<0)&&((sx=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/left"))<0))
			sx=80;
		
		if(((ex=Read_Neutrino_Cfg("screen_EndX"))<0)&&((ex=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/right"))<0))
			ex=620;

		if(((sy=Read_Neutrino_Cfg("screen_StartY"))<0)&&((sy=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/top"))<0))
			sy=80;

		if(((ey=Read_Neutrino_Cfg("screen_EndY"))<0)&&((ey=Read_Neutrino_Cfg("/enigma/plugins/needoffsets/bottom"))<0))
			ey=505;
			
		if((mtmo=Read_Neutrino_Cfg("timing_menu"))<0)
			mtmo=120;
			
		if(Read_Neutrino_Cfg("rounded_corners")>0)
			radius=9;
		else
			radius=0;
			
	//init framebuffer

		fb = open(FB_DEVICE, O_RDWR);

#ifdef HAVE_DBOX_HARDWARE
		ioctl(fb, AVIA_GT_GV_GET_BLEV, &alpha);
#endif

		if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			printf("shellexec <FBIOGET_FSCREENINFO failed>\n");
			return -1;
		}
		if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			printf("shellexec <FBIOGET_VSCREENINFO failed>\n");
			return -1;
		}
		
		sprintf(line_buffer,"repeat_blocker");
		if((tv=Read_Neutrino_Cfg(line_buffer))>=0)
				debounce=tv;

		if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
		{
			printf("shellexec <mapping of Framebuffer failed>\n");
			return 0;
		}

		memset(&menu,0,sizeof(MENU));

		if(Check_Config())
		{
			printf("<ShellExec> Unable to read Config %s\n",CFG_FILE);
			Clear_List(&menu,-1);
			free(line_buffer);
			return -1;
		}
		
		rc = open(RC_DEVICE, O_RDONLY);

		LCD_Init();
		
	//init fontlibrary

		if((error = FT_Init_FreeType(&library)))
		{
			printf("shellexec <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
		{
			printf("shellexec <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_SBitCache_New(manager, &cache)))
		{
			printf("shellexec <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_Lookup_Face(manager, FONT, &face)))
		{
			printf("shellexec <FTC_Manager_Lookup_Face failed with Errorcode 0x%.2X>\n", error);
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

		if(!(lbb = malloc(720*576)))
		{
			printf("shellexec <allocating of Backbuffer failed>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		switch(noepg)
		{
			case 1: system("touch /tmp/.daemon_h;killall sectionsd"); break;
			case 2: if(STYP==1) system("[ -x /bin/sectionsdcontrol -o -x /var/bin/sectionsdcontrol ] && sectionsdcontrol --pause || wget -Y off -q -O /dev/null http://localhost/control/zapto?stopsectionsd &"); break;
		}
		
//		lbb=lfb;
		memset(lbb, TRANSP, 720*576);
		memcpy(lfb, lbb, 720*576);
/*	
		if(!(lbb = realloc(&lbb,var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("shellexec <allocating of Backbuffer failed>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
*/
		startx = sx + (((ex-sx) - 620)/2);
		starty = sy + (((ey-sy) - 505)/2);


	// if problem with config file return from plugin

	index=0;	
	ShowInfo(&menu, 1);

	//remove last key & set rc to blocking mode
/*
#if HAVE_DVB_API_VERSION == 3
	read(rc, &ev, sizeof(ev));
#else
	read(rc, &rccode, sizeof(rccode));
#endif
*/
//	fcntl(rc, F_SETFL, (fcntl(rc, F_GETFL) | O_EXCL) & ~O_NONBLOCK);
	fcntl(rc, F_SETFL, (fcntl(rc, F_GETFL) | O_EXCL) | O_NONBLOCK);

	//main loop

	menu.act_entry=0;
	if(Get_Menu(1))
	{
		printf("ShellExec <unable to create menu>\n");
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		return -1;
	}
	cindex=0;
#ifdef HAVE_DREAMBOX_HARDWARE
	ClearKeys();
#endif
	signal(SIGINT, quit_signal);
	signal(SIGTERM, quit_signal);
	signal(SIGQUIT, quit_signal);

	put_instance(instance=get_instance()+1);

	while(mainloop)
	{
		cindex=Get_Selection(&menu);
		dloop=1;
		switch(cindex)
		{
			case -1:
				mainloop=0;
				break;
				
			case 0:
				mainloop=Menu_Up(&menu);
				break;
				
			case 1:
				pl=menu.list[menu.act_entry];
				switch (pl->type)
				{
					case TYP_MENU:
						menu.act_header=pl->headerpos;
						menu.lastheaderentrys[menu.act_header]=menu.act_entry;
						menu.headerwait[menu.act_header]=pl->message!=NULL;
						if(menu.headerwait[menu.act_header])
							{
								strcpy(tstr,pl->entry);
								if((rptr=strxchr(tstr,','))!=NULL)
								{
									*rptr=0;
								}
								ShowMessage(tstr, pl->message, 0);
							}
						Get_Menu(0);
						menu.act_entry=0;
						break;
						
					case TYP_EXECUTE:
						if((rptr=strxchr(pl->entry,','))!=NULL)
						{
							strcpy(tstr,pl->entry);
							rptr=strxchr(tstr,',');
							*rptr=0;
							rptr=strxchr(pl->entry,',');
							rptr++;
							if(pl->stay)
							{
								if(pl->stay==1)
								{
									if(pl->message)
									{
										ShowMessage(tstr, pl->message, 0);
									}
									else
									{
										ShowMessage(tstr, "Bitte warten", 0);
									}
								}
								else
								{
									memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
									memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
								}
								
								if(*(rptr+strlen(rptr)-1)=='&')
								{
									*(rptr+strlen(rptr)-1)=0;
								}
							}
							else
							{
								if(*(rptr+strlen(rptr)-1)!='&')
								{
									sprintf(tstr,"%s &",rptr);
									rptr=tstr;
								}
							}
//printf("shellexec cmdline: %s\n",rptr);							
							system(rptr);
							
							ClearKeys();
							
							mainloop= pl->stay==1;
							if(pl->stay==1)
							{
								ClearKeys();
								Get_Menu(1);
							}
						}
						break;
				}
		}
	}

	//cleanup

	Clear_List(&menu,-1);

	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);
	if(strlen(url))
	{
		sprintf(line_buffer,"/sbin/rdate -s %s > /dev/null &",url);
		system(line_buffer);
	}
	
	fcntl(rc, F_SETFL, O_NONBLOCK);
	
	LCD_Close();
	
	close(rc);

	switch(noepg)
		{
			case 1: 
				{
				int slev=5,tlev;
				FILE *nfh;
				char tstr [512], *cfptr=NULL;

					if((nfh=fopen("/var/tuxbox/config/sections.conf","r"))!=NULL)
					{
						tstr[0]=0;

						do
						{
							fgets(tstr,500,nfh);
						}
						while(((cfptr=strstr(tstr,"PROZESS_PRIO="))==NULL) && (!feof(nfh)));
						if(cfptr)
						{
							cfptr+=13;
							if(sscanf(cfptr,"%d",&tlev)==1)
							{
								slev=tlev;
							}
						}
					}
				sprintf(line_buffer,"rm -f /tmp/.daemon_h;(sectionsd;sleep 1;(PIDS=`pidof sectionsd`; [ -n \"$PIDS\" ] && renice %d $PIDS > /dev/null);[ -x /bin/sectionsdcontrol -o -x /var/bin/sectionsdcontrol ] && sectionsdcontrol --restart)&",slev); 
				system(line_buffer); 
				}
			break;
			
			case 2: if(STYP==1) system("[ -x /bin/sectionsdcontrol -o -x /var/bin/sectionsdcontrol ] && sectionsdcontrol --nopause || wget -Y off -q -O /dev/null http://localhost/control/zapto?startsectionsd &"); break;
		}

	free(line_buffer);
	free(trstr);

	// clear Display
	memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
//	memset(lfb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
	munmap(lfb, fix_screeninfo.smem_len);

#ifdef HAVE_DBOX_HARDWARE
	ioctl(fb, AVIA_GT_GV_SET_BLEV, alpha);
#endif
	close(fb);
	free(lbb);

	put_instance(get_instance()-1);

	return 0;
}

