/*
 * $Id: msgbox.c,v 1.1 2009/12/06 21:58:11 rhabarber1848 Exp $
 *
 * msgbox - d-box2 linux project
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
#include <time.h>
#include <signal.h>
#include "msgbox.h"
#include "text.h"
#include "io.h"
#include "gfx.h"
#include "txtform.h"
#include "color.h"
#ifdef HAVE_DBOX_HARDWARE
#include <dbox/fb.h>
#endif

#define M_VERSION 1.70

#define NCF_FILE 	"/var/tuxbox/config/neutrino.conf"
#define ECF_FILE	"/var/tuxbox/config/enigma/config"
#define HDF_FILE	"/tmp/.msgbox_hidden"

void TrimString(char *strg);

// Color table stuff

// Color table stuff
static char menucoltxt[4][25]={"Content_Selected","Content","inactive","Head"};
static int menucolval[4]={CMCS,CMC,CMCI,CMH};
unsigned short ord[256], ogn[256], obl[256], otr[256];
struct fb_cmap oldcmap = {0, 256, ord, ogn, obl, otr};

char *line_buffer=NULL, *title=NULL;
int size=36, type=0, timeout=0, refresh=3, flash=0, selection=0, tbuttons=0, buttons=0, bpline=3, echo=0, absolute=0, mute=1, header=1, cyclic=1;
char *butmsg[16];
int rbutt[16],hide=0,radius=0;

//static void ShowInfo(void);

// Misc
char NOMEM[]="msgbox <Out of memory>\n";
char TMP_FILE[64]="/tmp/msgbox.tmp";
unsigned char FONT[64]= "/share/fonts/pakenham.ttf";
unsigned char *lfb = 0, *lbb = 0, *obb = 0, *hbb = 0, *ibb = 0;
unsigned char nstr[BUFSIZE]="";
unsigned char *trstr;
unsigned rc,sc[8]={'a','o','u','A','O','U','z','d'}, tc[8]={0xE4,0xF6,0xFC,0xC4,0xD6,0xDC,0xDF,0xB0};
char INST_FILE[]="/tmp/rc.locked";
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
	}
	else
	{
		remove(INST_FILE);
	}
}

static void quit_signal(int sig)
{
	put_instance(get_instance()-1);
	printf("msgbox Version %.2f killed\n",M_VERSION);
	exit(1);
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

int GetSelection(char *sptr)
{
int rv=0,btn=0,run=1;
char *pt1=strdup(sptr),*pt2,*pt3;

	pt2=pt1;
	while(*pt2 && run && btn<MAX_BUTTONS)
	{
		if((pt3=strchr(pt2,','))!=NULL)
		{
			*pt3=0;
			++pt3;
		}
		else
		{
			run=0;
		}
		++tbuttons;
		if(strlen(pt2))
		{	
			rbutt[btn]=tbuttons;
			butmsg[btn]=strdup(pt2);
			CatchTabs(butmsg[btn++]);
		}
/*		else
		{
			rv=1;
		}
*/		if(run)
		{
			pt2=pt3;
		}
	}
	if(!btn)
	{
		rv=1;
	}
	else
	{
		buttons=btn;
	}
	free(pt1);
	return rv;
}

static int yo=80,dy;
static int psx, psy, pxw, pyw, myo=0, buttx=80, butty=30, buttdx=20, buttdy=10, buttsize=0, buttxstart=0, buttystart=0;

int show_txt(int buttonly)
{
FILE *tfh;
int i,bx,by,x1,y1,rv=-1,run=1,line=0,action=1,cut,itmp,btns=buttons,lbtns=(buttons>bpline)?bpline:buttons,blines=1+((btns-1)/lbtns);


	if(hide)
	{
		memcpy(lfb, hbb, var_screeninfo.xres*var_screeninfo.yres);
		return 0;
	}
	yo=(header)?80:35;
	if(!buttonly)
	{
		memcpy(lbb, ibb, var_screeninfo.xres*var_screeninfo.yres);
	}
	if((tfh=fopen(TMP_FILE,"r"))!=NULL)
	{
		fclose(tfh);
		if(!buttonly)
		{
			if(type!=1)
			{
				btns=0;
				myo=0;
			}	
		
			RenderString("X", 310, 270, 20, LEFT, BIG, TRANSP);
			pxw=GetStringLen(sx,title)+10;
			if(type==1)
			{
				RenderString("X", 310, 270, 20, LEFT, MED, TRANSP);
				myo=blines*(butty+buttdy);
				for(i=0; i<btns; i++)
				{
					itmp=GetStringLen(sx,butmsg[i])+10;
					if(itmp>buttx)
					{
						buttx=itmp;
					}
				}
			}
			buttsize=buttx;
			
			RenderString("X", 310, 270, 20, LEFT, size, TRANSP);
			if(fh_txt_getsize(TMP_FILE, &x1, &y1, &cut))
			{
				printf("msgbox <invalid Text-Format>\n");
				return -1;
			}
			x1+=10;

			dy=0.8*(double)size;
			if(pxw<x1)
			{
				pxw=x1;
			}
			if(pxw<(lbtns*buttx+lbtns*buttdx))
			{
				pxw=(lbtns*buttx+lbtns*buttdx);
			}
			if(pxw>((ex-sx)-2*buttdx))
			{
				pxw=ex-sx-2*buttdx;
			}
			psx=((ex-sx)/2-pxw/2);
			pyw=y1*dy/*-myo*/;
			if(pyw>((ey-sy)-yo-myo))
			{
				pyw=((ey-sy)-yo-myo);
			}
			psy=/*sy+*/((ey-sy)/2-(pyw+myo-yo)/2);
			if(btns)
			{
				buttxstart=psx+pxw/2-(((double)lbtns*(double)buttsize+(((lbtns>2)&&(lbtns&1))?((double)buttdx):0.0))/2.0);
//				buttystart=psy+pyw/2-(((double)blines*(double)butty+(((blines>2)&&(blines&1))?((double)buttdy):0.0))/2.0);
				buttystart=psy+y1*dy;
			}
		}
		
		while(run)
		{
			//frame layout
			if(action)
			{
				if(!buttonly)
				{
					RenderBox(psx-20, psy-yo, psx+pxw+20, psy+pyw+myo, radius, CMH);
					RenderBox(psx-20+2, psy-yo+2, psx+pxw+20-2, psy+pyw+myo-2, radius, CMC);
					if(header)
					{
						RenderBox(psx-18, psy-yo+2, psx+pxw+18, psy-yo+44, radius, CMH);
						RenderString(title, psx, psy-yo+36, pxw, CENTER, BIG, CMHT);
					}
				}
				if(buttonly || !(rv=fh_txt_load(TMP_FILE, psx, pxw, psy, dy, size, line, &cut)))
				{
					if(type==1)
					{
						for(i=0; i<btns; i++)
						{
							bx=i%lbtns;
							by=i/lbtns;
							RenderBox(buttxstart+bx*(buttsize+buttdx/2), buttystart+by*(butty+buttdy/2), buttxstart+(bx+1)*buttsize+bx*(buttdx/2), buttystart+by*(butty+buttdy/2)+butty, radius, YELLOW);
							RenderBox(buttxstart+bx*(buttsize+buttdx/2)+2, buttystart+by*(butty+buttdy/2)+2, buttxstart+(bx+1)*buttsize+bx*(buttdx/2)-2, buttystart+by*(butty+buttdy/2)+butty-2, radius, ((by*bpline+bx)==(selection-1))?CMCS:CMC);
							RenderString(butmsg[i], buttxstart+bx*(buttsize+buttdx/2), buttystart+by*(butty+buttdy/2)+butty-7, buttsize, CENTER, MED, (i==(selection-1))?CMCST:CMCIT);
						}
							
//						RenderBox((ex-sx)/2-buttx/2, psy+pyw+myo/2-butty/2, (ex-sx)/2+buttx/2, psy+pyw+myo/2+butty/2, FILL, CMCS);
//						RenderBox((ex-sx)/2-buttx/2, psy+pyw+myo/2-butty/2, (ex-sx)/2+buttx/2, psy+pyw+myo/2+butty/2, GRID, YELLOW);
//						RenderString("OK", (ex-sx)/2-buttx/2, psy+pyw+myo/2+butty/2-7, buttx, CENTER, MED, CMCST);
					}
					memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
				}
			}
			run=0;
		}
	}
	return (rv)?-1:0;	
}

int Transform_Msg(char *msg)
{
int rv=0;
FILE *xfh;

	if(*msg=='/')
	{
		if((xfh=fopen(msg,"r"))!=NULL)
		{
			fclose(xfh);
			strcpy(TMP_FILE,msg);
		}
		else
		{
			rv=1;
		}
	}
	else
	{
		if((xfh=fopen(TMP_FILE,"w"))!=NULL)
		{
			while(*msg)
			{
				if(*msg!='~')
				{
					fputc(*msg,xfh);
				}
				else
				{
					if(*(msg+1)=='n')
					{
						fputc(0x0A,xfh);
						++msg;
					}
					else
					{
						fputc(*msg,xfh);
					}
				}
				msg++;
/*					
					rc=*(msg+1);
					found=0;
					for(i=0; i<sizeof(sc) && !found; i++)
					{
						if(rc==sc[i])
						{
							rc=tc[i];
							found=1;
						}
					}
					if(found)
					{
						fputc(rc,xfh);
						++msg;
					}
					else
					{
						fputc(*msg,xfh);
					}
				}
				++msg;
*/

			}
			fclose(xfh);
		}
	}
	return rv;
}

void ShowUsage(void)
{
	printf("\nSyntax:\n");
	printf("    msgbox msg=\"text to show\" [Options]\n");
	printf("    msgbox msg=filename [Options]\n");
	printf("    msgbox popup=\"text to show\" [Options]\n");
	printf("    msgbox popup=filename [Options]\n\n");
	printf("Options:\n");
	printf("    title=\"Window-Title\"  : specify title of window\n");
	printf("    size=nn               : set fontsize\n");
	printf("    timeout=nn            : set autoclose-timeout\n");
	printf("    refresh=n             : n=1..3, see readme.txt\n");
	printf("    select=\"Button1,..\"   : Labels of up to 16 Buttons, see readme.txt\n");
	printf("    absolute=n            : n=0/1 return relative/absolute button number (default is 0)\n");
	printf("    order=n               : maximal buttons per line (default is 3)\n");
	printf("    default=n             : n=1..buttons, initially selected button, see readme.txt\n");
	printf("    echo=n                : n=0/1 print the button-label to console on return (default is 0)\n");
	printf("    hide=n                : n=0..2, function of mute-button, see readme.txt (default is 1)\n");
	printf("    cyclic=n              : n=0/1, cyclic screen refresh (default is 1)\n");

}
/******************************************************************************
 * MsgBox Main
 ******************************************************************************/

int main (int argc, char **argv)
{
int index,index2,tv,found=0;
int dloop=1, rcc=-1, flsh=0, cupd=0;
char rstr[BUFSIZE], *rptr, *aptr;
time_t tm1,tm2;
unsigned int alpha;
clock_t tk1=0;
FILE *fh;

		if(argc<2)
		{
			ShowUsage();
			return 0;
		}
		dloop=0;
		for(tv=1; !dloop && tv<argc; tv++)
		{
			aptr=argv[tv];
			if((rptr=strchr(aptr,'='))!=NULL)
			{
				rptr++;
				if(strstr(aptr,"size=")!=NULL)
				{
					if(sscanf(rptr,"%d",&size)!=1)
					{
						dloop=1;
					}
				}
				else
				{
					if(strstr(aptr,"title=")!=NULL)
					{
						title=strdup(rptr);
						CatchTabs(title);
						if(strcmp(title,"none")==0)
						{
							header=0;
						}
/*
						tpos=0;
						while(*rptr)
						{
							if(*rptr!='~')
							{
								nstr[tpos]=*rptr;
							}
							else
							{
								rc=*(rptr+1);
								found=0;
								for(i=0; i<sizeof(sc) && !found; i++)
								{
									if(rc==sc[i])
									{
										rc=tc[i];
										found=1;
									}
								}
								if(found)
								{
									nstr[tpos]=rc;
									++rptr;
								}
								else
								{
									nstr[tpos]=*rptr;
								}
							}
							++tpos;
							++rptr;
						}
						nstr[tpos]=0;
						title=strdup(nstr);
*/
					}
					else
					{
						if(strstr(aptr,"timeout=")!=NULL)
						{
							if(sscanf(rptr,"%d",&timeout)!=1)
							{
								dloop=1;
							}
						}
						else
						{
							if(strstr(aptr,"msg=")!=NULL)
							{
								dloop=Transform_Msg(rptr);
								if(timeout==0)
								{
										if((timeout=Read_Neutrino_Cfg("timing.epg"))<0)
											timeout=300;
								}
								type=1;
							}
							else
							{
								if(strstr(aptr,"popup=")!=NULL)
								{
									dloop=Transform_Msg(rptr);
									if(timeout==0)
									{
										if((timeout=Read_Neutrino_Cfg("timing.infobar"))<0)
											timeout=6;
									}
									type=2;
								}
								else
								{
									if(strstr(aptr,"refresh=")!=NULL)
									{
										if(sscanf(rptr,"%d",&refresh)!=1)
										{
											dloop=1;
										}
									}
									else
									{
										if(strstr(aptr,"select=")!=NULL)
										{
											dloop=GetSelection(rptr);
										}
										else
										{
											if(strstr(aptr,"default=")!=NULL)
											{
												if((sscanf(rptr,"%d",&selection)!=1) || selection<1)
												{
													dloop=1;
												}
											}
											else
											{
												if(strstr(aptr,"order=")!=NULL)
												{
													if(sscanf(rptr,"%d",&bpline)!=1)
													{
														dloop=1;
													}
												}
												else
												{
													if(strstr(aptr,"echo=")!=NULL)
													{
														if(sscanf(rptr,"%d",&echo)!=1)
														{
															dloop=1;
														}
													}
													else
													{
														if(strstr(aptr,"absolute=")!=NULL)
														{
															if(sscanf(rptr,"%d",&absolute)!=1)
															{
																dloop=1;
															}
														}
														else
														{
															if(strstr(aptr,"hide=")!=NULL)
															{
																if(sscanf(rptr,"%d",&mute)!=1)
																{
																	dloop=1;
																}
															}
															else
															{
																if(strstr(aptr,"cyclic=")!=NULL)
																{
																	if(sscanf(rptr,"%d",&cyclic)!=1)
																	{
																		dloop=1;
																	}
																}
																else
																{
																dloop=2;
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			switch (dloop)
			{
				case 1:
					printf("msgbox <param error: %s>\n",aptr);
					return 0;
					break;
				
				case 2:
					printf("msgbox <unknown command: %s>\n\n",aptr);
					ShowUsage();
					return 0;
					break;
			}
		}
/*for(tv=0; tv<buttons; tv++)
{
	printf("%cButton %d: %s\n",(tv==selection-1)?'>':' ',tv+1,butmsg[tv]);
}
return 0;*/

		if(!echo)
		{
			printf("\nmsgbox  Message-Box Version %.2f\n",M_VERSION);
		}
		if(!buttons)
		{
			butmsg[0]=strdup("OK");
			buttons=1;
		}
/*		
		if(selection>buttons)
		{
			printf("msgbox <param error: default=%d>\n",selection);
			return 0;
		}
*/	
		if(!absolute)
		{
			for(tv=0; tv<buttons; tv++)
			{
				rbutt[tv]=tv+1;
			}
		}
		if(selection)
		{	
			for(tv=0; tv<buttons && !found; tv++)		
			{
				if(rbutt[tv]==selection)
				{
					selection=tv+1;
					found=1;
				}
			}
			if(!found)
			{
				printf("msgbox <param error: default=%d>\n",selection);
				return 0;
			}
		}
		else
		{
			for(tv=0; tv<buttons && !selection; tv++)
			{
				if(strlen(butmsg[tv]))
				{
					selection=tv+1;
				}
			}
		}
/*		for(tv=0; selection!=rbutt[tv] && tv<buttons; tv++);
		
		if(tv>=buttons)
		{
			selection=1;
		}
*/			
		if(!title)
		{
			title=strdup("Information");
		}
		if((line_buffer=calloc(BUFSIZE+1, sizeof(char)))==NULL)
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
	
		if((radius=Read_Neutrino_Cfg("menu_kw_smooth"))<0)
		{
			if(Read_Neutrino_Cfg("rounded_corners")>0)
				radius=9;
			else
				radius=0;
		}

		if((trstr=malloc(BUFSIZE))==NULL)
		{
			printf(NOMEM);
			return -1;
		}

		fb = open(FB_DEVICE, O_RDWR);
#ifdef HAVE_DBOX_HARDWARE
		ioctl(fb, AVIA_GT_GV_GET_BLEV, &alpha);
#endif
		rc = open(RC_DEVICE, O_RDONLY);
		fcntl(rc, F_SETFL, (fcntl(rc, F_GETFL) | O_EXCL) | O_NONBLOCK);
		
	//init framebuffer

		if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			printf("msgbox <FBIOGET_FSCREENINFO failed>\n");
			return -1;
		}
		if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			printf("msgbox <FBIOGET_VSCREENINFO failed>\n");
			return -1;
		}
		
		if(ioctl(fb, FBIOGETCMAP, &oldcmap) == -1)
		{
			printf("msgbox <FBIOGETCMAP failed>\n");
			return -1;
		}
/*
		fh=fopen("/tmp/cmap.log","w");
		fprintf(fh,"Start: %d, LEN: %d\n",oldcmap.start,oldcmap.len);
		for(tv=0; tv<256; tv++)
		{
			fprintf(fh,"%02d %04x %04x %04x %04x\n",tv,oldcmap.red[tv],oldcmap.green[tv],oldcmap.blue[tv],oldcmap.transp[tv]);
		}
		fclose(fh);
*/		
		for(index=0; index<8; index++)
		{
			sprintf(rstr,"menu_%s_alpha",menucoltxt[index]);
			if((tv=Read_Neutrino_Cfg(rstr))>=0)
				for(index2=0; index2<7; index2++)
					otr[menucolval[index]+index2]=(tv<<8);
		}

		ord[FLASH]=ord[CMCT];
		ogn[FLASH]=ogn[CMCT];
		obl[FLASH]=obl[CMCT];
		otr[FLASH]=otr[CMCT];
/*
		fh=fopen("/tmp/cmap2.log","w");
		fprintf(fh,"Start: %d, LEN: %d\n",oldcmap.start,oldcmap.len);
		for(tv=0; tv<256; tv++)
		{
			fprintf(fh,"%02d %04x %04x %04x %04x\n",tv,oldcmap.red[tv],oldcmap.green[tv],oldcmap.blue[tv],oldcmap.transp[tv]);
		}
		fclose(fh);
		
*/
/*
{
int i;
printf("unsigned short rd[] = {");
for(i=CMCST; i<=CMH;i++)
printf("0x%02x<<8, ",(ord[i]>>8)&0xFF);
printf("\nunsigned short gn[] = {");
for(i=CMCST; i<=CMH;i++)
printf("0x%02x<<8, ",(ogn[i]>>8)&0xFF);
printf("\nunsigned short bl[] = {");
for(i=CMCST; i<=CMH;i++)
printf("0x%02x<<8, ",(obl[i]>>8)&0xFF);
printf("\nunsigned short tr[] = {");
for(i=CMCST; i<=CMH;i++)
printf("0x%02x<<8, ",(otr[i]>>8)&0xFF);
printf("\n");
}		
return 0;
*/
		
		if(ioctl(fb, FBIOPUTCMAP, &oldcmap) == -1)
		{
			printf("msgbox <FBIOPUTCMAP failed>\n");
			return -1;
		}

		if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
		{
			printf("msgbox <mapping of Framebuffer failed>\n");
			return -1;
		}
		
	//init fontlibrary

		if((error = FT_Init_FreeType(&library)))
		{
			printf("msgbox <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
		{
			printf("msgbox <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_SBitCache_New(manager, &cache)))
		{
			printf("msgbox <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_Lookup_Face(manager, FONT, &face)))
		{
			printf("msgbox <FTC_Manager_Lookup_Face failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		use_kerning = FT_HAS_KERNING(face);

		desc.font.face_id = FONT;

#if FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 0
		desc.image_type = ftc_image_mono;
#else
		desc.flags = FT_LOAD_MONOCHROME;
#endif

	//init backbuffer

		if(!(lbb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("msgbox <allocating of Backbuffer failed>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if(!(obb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("msgbox <allocating of Backbuffer failed>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			free(lbb);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if(!(hbb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("msgbox <allocating of Backbuffer failed>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			free(lbb);
			free(obb);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if(!(ibb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("msgbox <allocating of Backbuffer failed>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			free(lbb);
			free(obb);
			free(hbb);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if(refresh & 1)
		{
			memcpy(ibb, lfb, var_screeninfo.xres*var_screeninfo.yres);
		}
		else
		{
			memset(ibb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
		}
		if(mute==2)
		{
			memcpy(hbb, lfb, var_screeninfo.xres*var_screeninfo.yres);
		}
		else
		{
			memset(hbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
		}
		if(refresh & 2)
		{
			memcpy(obb, lfb, var_screeninfo.xres*var_screeninfo.yres);
		}
		else
		{
			memset(obb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
		}

		startx = sx /*+ (((ex-sx) - 620)/2)*/;
		starty = sy /* + (((ey-sy) - 505)/2)*/;


	/* Set up signal handlers. */
	signal(SIGINT, quit_signal);
	signal(SIGTERM, quit_signal);
	signal(SIGQUIT, quit_signal);

	put_instance(instance=get_instance()+1);

  	show_txt(0);	
//FBEnterWord( 100, 100, 100,20,CMCT);	
//rc=RC_HOME;
	
	time(&tm1);
	tm2=tm1;
	tk1=clock()/(CLOCKS_PER_SEC/1000);
#ifdef HAVE_DREAMBOX_HARDWARE
	ClearKeys();
#endif	
	
	//main loop
	
	while((rcc!=RC_HOME) && (rcc!=RC_OK) && ((timeout==-1)||((tm2-tm1)<timeout)))
	{
		rcc=GetRCCode();
		if(rcc!=-1)
		{
			time(&tm1);
		}
		else
		{
			if(++cupd>10)
			{
				if(cyclic)
				{
					show_txt(0);
					cupd=0;
				}
			}
			usleep(100000L);
		}
		if(mute && rcc==RC_MUTE)
		{
			hide^=1;
			show_txt(0);
			usleep(500000L);
			while(GetRCCode()!=-1);
			if(hide)
			{
				if((fh=fopen(HDF_FILE,"w"))!=NULL)
				{
					fprintf(fh,"hidden");
					fclose(fh);
				}
			}
			else
			{
				remove(HDF_FILE);
			}
		}
		if((!hide) && (rcc!=RC_HOME) && (rcc!=RC_OK))
		{
			switch(rcc)
			{
				case RC_LEFT:
					if(!hide && (--selection<1))
					{
						selection=buttons;
					}
					show_txt(1);
				break;
				
				case RC_RIGHT:
					if(!hide && (++selection>buttons))
					{
						selection=1;
					}
					show_txt(1);
				break;
				
				case RC_UP:
					if(!hide && ((selection-=bpline)<1))
					{
						selection=1;
					}
					show_txt(1);
				break;
				
				case RC_DOWN:
					if(!hide && ((selection+=bpline)>buttons))
					{
						selection=buttons;
					}
					show_txt(1);
				break;
				
				default:
					if(++flsh==7)
					{
						flsh=0;
						flash^=1;

						ord[FLASH]=(flash)?ord[CMC]:ord[CMCT];
						ogn[FLASH]=(flash)?ogn[CMC]:ogn[CMCT];
						obl[FLASH]=(flash)?obl[CMC]:obl[CMCT];
						otr[FLASH]=(flash)?otr[CMC]:otr[CMCT];
						ioctl(fb, FBIOPUTCMAP, &oldcmap);
					}
				break;
			}
		}
		time(&tm2);
		if(hide)
		{
			rcc=-1;
		}
	}
	if((type!=1) || (rcc!=RC_OK))
	{
		selection=0;
	}
	
	
	//cleanup

	// clear Display
//	memset(lbb, TRANSP, var_screeninfo.xres*var_screeninfo.yres);
//	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
	
	memcpy(lfb, obb, var_screeninfo.xres*var_screeninfo.yres);
	munmap(lfb, fix_screeninfo.smem_len);
#ifdef HAVE_DBOX_HARDWARE
	ioctl(fb, AVIA_GT_GV_SET_BLEV, alpha);
#endif
	close(fb);
	free(lbb);

	put_instance(get_instance()-1);

	if(echo && selection>0)
	{
		printf("%s\n",butmsg[selection-1]);
	}

	for(tv=0; tv<buttons; tv++)
	{
		free(butmsg[tv]);
	}
	free(trstr);
	free(line_buffer);
	free(title);

	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);

	free(obb);
	free(hbb);
	free(ibb);

	close(rc);

	remove("/tmp/msgbox.tmp");

	if(selection)
	{
		return rbutt[selection-1];
	}
	return 0;
}

