/*
 * $Id: sysinfo.c,v 1.1 2009/12/20 16:22:58 rhabarber1848 Exp $
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <tuxbox.h>
#include "sysinfo.h"
#include "text.h"
#include "color.h"


#define S_VERSION 1.76

//#define NET_DEBUG

#include "gfx.h"
#include <tuxbox/libucodes.h>

extern int FSIZE_BIG;
extern int FSIZE_MED;
extern int FSIZE_SMALL;
#define BUFSIZE 	4095
#define NETWORKFILE_VAR		"/var/etc/network/interfaces"
#define NETWORKFILE_ETC		"/etc/network/interfaces"
#define RESOLVEFILE_VAR		"/var/etc/resolv.conf"
#define RESOLVEFILE_ETC		"/etc/resolv.conf"


#define MAXLINES 500


char uptime[50]		="";
char datum[15]		="";
char zeit[15]		="";

char processor[5]	="";
char cpu[6]		="";
char tuner[16]		="";
char pc_clock[10]	="";
char bus_clock[10]	="";
char cpu_rev[30]	="";
char bogomips[10]	="";
char kernel[256]	="";

double user_perf	=0;
double nice_perf	=0;
double sys_perf		=0;
double idle_perf	=0;
double old_user_perf	=0;
double old_nice_perf	=0;
double old_sys_perf	=0;
double old_idle_perf	=0;

float memtotal		=0;
float memfree		=0;
float memused		=0;
float memactive		=0;
float meminakt		=0;

float old_memtotal	=0;
float old_memfree	=0;
float old_memused	=0;
float old_memactive	=0;
float old_meminakt	=0;


char Filesystem[15][256]={"",""};
char FS_total[15][10]	={"",""};
char FS_used[15][10]	={"",""};
char FS_free[15][10]	={"",""};
char FS_percent[15][6]	={"",""};
char FS_mount[15][64]	={"",""};
int FS_count		=0;

char mtds[10][256]	={"","","","","","","","","",""};
int mtd_count		=0;


char IP_ADRESS[25]	={ "192.168.0.1" };
char MAC_ADRESS[25]	={ "00:11:22:33:44:55" };
char BC_ADRESS[25]	={ "192.168.0.255" };
char MASK_ADRESS[25]	={ "255.255.255.0" };
char BASE_ADRESS[10]	={ "0x000" };
char GATEWAY_ADRESS[25]	={ "192.168.0.1" };
char NAMES_ADRESS[25]	={ "192.168.0.1" };

long long read_akt		=0;
long long read_old		=0;
long long write_akt		=0;
long long write_old		=0;
long long delta_read		=0;
long long delta_write		=0;
int delta_read_old		=0;
int delta_write_old		=0;
long	read_packet		=0;
long	write_packet		=0;
long 	dummy			=0;



int linie_oben=0, linie_unten=0, rahmen=0, rabs=0;
int stx=80, sty=80;
int enx=620, eny=505;
int up_place=0;
int x_pos=0;
int win_sx=0, win_ex=0,win_sy=0,win_ey=0;

char NOMEM[]="Sysinfo <Out of memory>\n";

void up_main_mem (void);
void hintergrund (void);
int Read_Neutrino_Cfg(char *entry);
void render_koord (char ver);
void up_full (char sel);
void up_net (void);

unsigned char *lfb = 0, *lbb = 0;
unsigned char title[256];
char *line_buffer=NULL;
unsigned char *trstr;
int mloop=1;

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
	printf("sysinfo Version %.2f killed\n",S_VERSION);
	exit(1);
}

int init_fb (void)
{
	int tv;

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
	
		if((stx=Read_Neutrino_Cfg("screen_StartX"))<0)
			stx=65;
		
		if((enx=Read_Neutrino_Cfg("screen_EndX"))<0)
			enx=655;

		if((sty=Read_Neutrino_Cfg("screen_StartY"))<0)
			sty=45;

		if((eny=Read_Neutrino_Cfg("screen_EndY"))<0)
			eny=530;
			
		tv=enx-stx-590;
		tv=tv/2;
		stx+=tv;
	
		tv=eny-sty-485;
		tv=tv/2;
		sty+=tv;
		
		tv=0;
	
		enx=590+stx;
		eny=485+sty;
	
		fb = open(FB_DEVICE, O_RDWR);
		rc = open(RC_DEVICE, O_RDONLY);

		if(ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
		{
			printf("Sysinfo <FBIOGET_FSCREENINFO failed>\n");
			return -1;
		}
		if(ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
		{
			printf("Sysinfo <FBIOGET_VSCREENINFO failed>\n");
			return -1;
		}
		
		if(!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
		{
			printf("Sysinfo <mapping of Framebuffer failed>\n");
			return -1;
		}


		if((error = FT_Init_FreeType(&library)))
		{
			printf("Sysinfo <FT_Init_FreeType failed with Errorcode 0x%.2X>", error);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
		{
			printf("Sysinfo <FTC_Manager_New failed with Errorcode 0x%.2X>\n", error);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_SBitCache_New(manager, &cache)))
		{
			printf("Sysinfo <FTC_SBitCache_New failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		if((error = FTC_Manager_Lookup_Face(manager, FONT, &face)))
		{
			printf("Sysinfo <FTC_Manager_Lookup_Face failed with Errorcode 0x%.2X>\n", error);
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		use_kerning = FT_HAS_KERNING(face);

		desc.font.face_id = FONT;
		desc.flags = FT_LOAD_MONOCHROME;

		if(!(lbb = malloc(var_screeninfo.xres*var_screeninfo.yres)))
		{
			printf("Sysinfo <allocating of Backbuffer failed>\n");
			FTC_Manager_Done(manager);
			FT_Done_FreeType(library);
			munmap(lfb, fix_screeninfo.smem_len);
			return -1;
		}

		memset(lbb, 0, var_screeninfo.xres*var_screeninfo.yres);


	fcntl(rc, F_SETFL, (fcntl(rc, F_GETFL) | O_EXCL) |O_NONBLOCK);
	return 0;
}


int Read_Neutrino_Cfg(char *entry)
{
	FILE *nfh;
	char tstr [512], *cfptr=NULL;
	int rv=-1;

	if((nfh=fopen(NCFFILE,"r"))!=NULL)
	{
		tstr[0]=0;

		while((!feof(nfh)) && ((strstr(tstr,entry)==NULL) || ((cfptr=strchr(tstr,'='))==NULL)))
		{
			fgets(tstr,500,nfh);
		}
		if(!feof(nfh) && cfptr)
		{
			++cfptr;
			if(sscanf(cfptr,"%d",&rv)!=1)
			{
				rv=-1;
			}
		}
		fclose(nfh);
	}
	return rv;
}

void correct_string (char *temp)
{
	int z=0;
	while (temp[z]!=0)
	{
		if((temp[z]>0)&&(temp[z]<32)) temp[z]=32;
		z++;
	}
}

void corr (char *aus, char *temp)
{
	char temp_data[256]={ "" };
	sprintf(temp_data,temp);
	int z=0;
	while (temp_data[z]!=0)
	{
		if((temp_data[z]>0)&&(temp_data[z]<=32)) temp_data[z]=0;
		z++;
	}
	sprintf(aus,"%s",temp_data);
}


void daten_auslesen(char *buffer, char *ergebnis, char symbol)
{
	int count=0, i=0;
	while ((buffer[count]!=symbol))count++;
	count++;
	while (isspace(buffer[count]))count++;
	while (!isspace(buffer[count]))
	{
		ergebnis[i]=buffer[count];
		i++;
		count++;
	}
	ergebnis[i]=0;
}	

int get_date (void)
{
	long t = time(NULL);
	struct tm *tp;
	tp = localtime(&t);
	sprintf(zeit,"%02d:%02d:%02d",tp->tm_hour,tp->tm_min,tp->tm_sec);
	sprintf(datum,"%02d.%02d.%4d",tp->tm_mday,(tp->tm_mon+1),(1900+tp->tm_year));
	return 0;
}

void update_zeit (void)
{
	
	get_date();
	RenderBox(enx-150, sty+rahmen, enx-rahmen, linie_oben-rabs, FILL, CMCST);
	RenderString(zeit, (stx+458), (sty+29), 120, LEFT, 30, CMCT);
	RenderString("Uhr", (stx+541), (sty+29), 50, LEFT, 30, CMCT);
	RenderString(datum, (stx+449), (sty+46), 130, CENTER, 25, CMCT);
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

}	

int get_perf(void)
{
       long curCPU[5]={0,0,0,0,0};
       long prevCPU[5]={0,0,0,0,0};
       double perf[5]={0,0,0,0,0};
       float faktor;
       int i = 0;
       FILE *f;
       char line[MAXLINES];

       if(prevCPU[0]==0)
       {
               f=fopen("/proc/stat","r");
               if(f)
               {
                       fgets(line,256,f); 
                       sscanf(line,"cpu %lu %lu %lu %lu",&prevCPU[1],&prevCPU[2],&prevCPU[3],&prevCPU[4]);
                       for(i=1;i<5;i++)
                               prevCPU[0]+=prevCPU[i];
               }
               fclose(f);
               sleep(1);
       }
       else
       {
               for(i=0;i<5;i++)
                               prevCPU[i]=curCPU[i];
       }

       while(((curCPU[0]-prevCPU[0]) < 100) || (curCPU[0]==0))
       {
               f=fopen("/proc/stat","r");
               if(f)
               {
                       curCPU[0]=0;
                       fgets(line,256,f); /* cpu */
                       sscanf(line,"cpu %lu %lu %lu %lu",&curCPU[1],&curCPU[2],&curCPU[3],&curCPU[4]);
                       for(i=1;i<5;i++) curCPU[0]+=curCPU[i];
               }
               fclose(f);
               if((curCPU[0]-prevCPU[0])<100)
                       sleep(1);
       }
       if(!(curCPU[0] - prevCPU[0])==0)
       {
               faktor = 100.0/(curCPU[0] - prevCPU[0]);
               for(i=0;i<4;i++)
                       perf[i]=(curCPU[i]-prevCPU[i])*faktor;

               perf[4]=100.0-perf[1]-perf[2]-perf[3];

 		user_perf=perf[1];
 		nice_perf=perf[2];
 		sys_perf=perf[3];
 		idle_perf=perf[4];
      }
       return 0;
}

int get_uptime(void)
{
 	FILE *f;
	char line[MAXLINES];
	f=fopen("/proc/uptime","r");
	if(f)
	{
		fgets(line,256,f);
		float ret[4];
		const char* strTage[2] = {"T", "T"};
		const char* strStunden[2] = {"Std", "Std"};
		const char* strMinuten[2] = {"Min", "Min"};
		sscanf(line,"%f",&ret[0]);
		ret[0]=ret[0]/60;
		ret[1]=(long)(ret[0])/60/24; 
		ret[2]=(long)(ret[0])/60-(long)(ret[1])*24; 
		ret[3]=(long)(ret[0])-(long)(ret[2])*60-(long)(ret[1])*60*24; 
		fclose(f);
		sprintf(uptime,"%.0f %s - %.0f %s - %.0f %s\n", ret[1], strTage[(int)(ret[1])==1], ret[2], strStunden[(int)(ret[2])==1], ret[3], strMinuten[(int)(ret[3])==1]);
       }
	correct_string (uptime);
	return 0;
}

int get_info_cpu(void)
{
	FILE *file=NULL;
	char *ptr;
	char line_buffer[512]="";
	if ((file = fopen("/proc/cpuinfo","r"))==NULL)
	{
		printf("kann /proc/cpuinfo nicht öffnen\n");
		return -1;
	}
	else
	{
		while(fgets(line_buffer, sizeof(line_buffer),file))
		{
			if((ptr = strstr(line_buffer, "processor"))!=NULL)
			{
				daten_auslesen(line_buffer, processor, 58);
			}
			if((ptr = strstr(line_buffer, "cpu"))!=NULL)
			{
				daten_auslesen(line_buffer, cpu, 58);
			}
			if((ptr = strstr(line_buffer, "clock"))!=NULL)
			{
				daten_auslesen(line_buffer, pc_clock, 58);
			}
			if((ptr = strstr(line_buffer, "bus"))!=NULL)
			{
				daten_auslesen(line_buffer, bus_clock, 58);
			}
			if((ptr = strstr(line_buffer, "revision"))!=NULL)
			{
				daten_auslesen(line_buffer, cpu_rev, 58);
			}
			if((ptr = strstr(line_buffer, "bogomips"))!=NULL)
			{
				daten_auslesen(line_buffer, bogomips, 58);
			}
		}
		fclose(file);
	}

	file=fopen("/proc/version","r");
	if(file)
	{
		while(fgets(line_buffer, sizeof(line_buffer),file))
		{
			if((ptr = strstr(line_buffer, "Linux"))!=NULL)
			{
				sprintf(kernel,"%s",line_buffer);
			}
		}
		fclose(file);
	}

 	correct_string (kernel);

	return 0;
}

int get_df(void)
{
	FILE *f;
	char line[512];
	int got;
	system("df -h > /tmp/systmp");
	if((f=fopen("/tmp/systmp","r"))!=NULL)
	{
		FS_count=0;
		while((fgets(line,512, f)!=NULL))
		{
			got=sscanf(line,"%s %s %s %s %s %s ", &Filesystem[FS_count], &FS_total[FS_count], &FS_used[FS_count], &FS_free[FS_count], &FS_percent[FS_count], &FS_mount[FS_count]);
			if(got==1)
				if(fgets(line+strlen(line),512-strlen(line),f)!=0)
					got=sscanf(line,"%s %s %s %s %s %s ", &Filesystem[FS_count], &FS_total[FS_count], &FS_used[FS_count], &FS_free[FS_count], &FS_percent[FS_count], &FS_mount[FS_count]);
			if (got==6 && isdigit(FS_used[FS_count][0])) FS_count++;
		}
		fclose(f);
	}
	return 0;
}

void rc_Nnull (int mu)
{
		while (ev.value!=0)
		{
			if (mu==1) up_main_mem ();
			if (mu==2) up_full (1);
			if (mu==3) up_full (2);
			if (mu==4) up_net ();
			update_zeit ();
			read(rc, &ev, sizeof(ev));
		}
	
}

void rc_null (int mu)
{
		while (ev.value==0)
		{
			if (mu==1) up_main_mem ();
			if (mu==2) up_full (1);
			if (mu==3) up_full (2);
			if (mu==4) up_net ();
			update_zeit ();
			read(rc, &ev, sizeof(ev));
		}
	
}

int show_FileS(void)
{
	int lauf=0, p_start=0, svar=0, vabs=0, z=0, size=23, end_show=0, anz=3;
	int spalte[2]={ 0,0};
	spalte[0]=stx+35;
	spalte[1]=enx-stx;
	spalte[1]=spalte[1]/2;
	spalte[1]=spalte[1]+stx;
	get_df();
	while (end_show==0)
	{
		z=0;
		vabs=32;
		hintergrund ();
		p_start=lauf;
		if ((lauf+anz)<FS_count) RenderString("[ down ]  WEITER", (stx+30), eny-12, 300, LEFT, 26, CMCT);
		if (p_start>0) RenderString("[ up ]  ZURÜCK", stx+30+(int)((enx-stx-30)/4), eny-12, 300, LEFT, 26, CMCT);
		RenderString("[ Home ]  ENDE", stx+50+((int)((enx-stx-30)/4)*3),  eny-12, 300, LEFT, 26, CMCT);
		for (svar=0;((svar<anz)&&(lauf<FS_count));svar++)
		{
			
			RenderString("Verzeichnis:", spalte[z], (linie_oben+vabs), 300, LEFT, size, CMHT);
			RenderString(Filesystem[lauf], spalte[z]+90, (linie_oben+vabs), 300, LEFT, size, CMCT);
			RenderString("Größe:", spalte[z], (linie_oben+18+vabs), 300, LEFT, size, CMHT);
			RenderString(FS_total[lauf], spalte[z]+90, (linie_oben+18+vabs), 300, LEFT, size, CMCT);
			RenderString("Genutzt:", spalte[z], (linie_oben+36+vabs), 300, LEFT, size, CMHT);
			RenderString(FS_used[lauf], spalte[z]+90, (linie_oben+36+vabs), 300, LEFT, size, CMCT);
			RenderString("Frei:", spalte[z], (linie_oben+56+vabs), 300, LEFT, size, CMHT);
			RenderString(FS_free[lauf], spalte[z]+90, (linie_oben+56+vabs), 300, LEFT, size, CMCT);
			RenderString("Belegt:", spalte[z], (linie_oben+74+vabs), 300, LEFT, size, CMHT);
			RenderString(FS_percent[lauf], spalte[z]+90, (linie_oben+74+vabs), 300, LEFT, size, CMCT);
			RenderString("Mountpunkt:", spalte[z], (linie_oben+92+vabs), 300, LEFT, size, CMHT);
			RenderString(FS_mount[lauf], spalte[z]+90, (linie_oben+92+vabs), 300, LEFT, size, CMCT);
//			z++;
			lauf++;
//			if(z>1) 
			{
//				z=0;
				vabs=vabs+125;
			}
		}
		rc_Nnull(0);
		rc_null (0);
		switch(ev.code)
		{
			case KEY_UP:	lauf=p_start-anz;
					if (lauf<0) lauf=0;
					break;
			
			case KEY_DOWN:	if(lauf>=FS_count) lauf=p_start;
					break;
			case KEY_HOME:	end_show=1;
					break;
						
			default:	lauf=p_start;
		}		
	}
	rc_Nnull(0);
	return 0;
}

int show_ps_status (int psnum)
{
	FILE *f;
	int end_show=0,i=0, vabs=21, abs=0, bstart=0, y=0, end_temp=0;
	
	char line[256];
	sprintf(line,"/proc/%d/status",psnum);
	if((f=fopen(line,"r"))!=NULL)
	{
		while (end_show==0)
		{
			abs=38;
			end_temp=0;
			hintergrund ();
			RenderBox(stx+20,linie_oben+20, enx-20, linie_unten-20, FILL, CMCST);
			RenderString("[ down ]  WEITER", (stx+30), eny-12, 300, LEFT, 26, CMCT);
			if (bstart>=17) RenderString("[ up ]  ZURÜCK", stx+30+(int)((enx-stx-30)/4), eny-12, 300, LEFT, 26, CMCT);
			RenderString("[ Home ]  ENDE", stx+50+((int)((enx-stx-30)/4)*3),  eny-12,300, LEFT, 26, CMCT);
			for (i=0;i<17;i++)
			{
				if(fgets(line,256, f)!=NULL)
				{
					correct_string (line);
					RenderString(line, stx+40, (linie_oben+abs), enx-stx-40, LEFT, 25, CMCT);
					abs=vabs+abs;
				}
				else
				{
					end_temp=1;
					RenderBox(stx+11,linie_unten+5,stx+20+(int)((enx-stx-30)/4), eny-9, FILL, CMCST);
				}
			}
			rc_Nnull(0);
			rc_null (0);
			switch(ev.code)
			{
				case KEY_UP:	fseek (f,0L,SEEK_SET);
						if (bstart<17) 
						{
							bstart=0;
						}
						else
						{
							bstart=bstart-17;
						}
						for (y=0;y<bstart;y++) fgets(line,256, f);
						break;
			
				case KEY_DOWN:	if (end_temp!=0)
						{
							fseek (f,0L,SEEK_SET);
							for (y=0;y<bstart;y++) fgets(line,256, f);
						}
						else
						{
							bstart=bstart+17;
						}
						break;
				case KEY_HOME:	end_show=1;
						break;
						
				case KEY_OK:	end_show=1;
						break;
								
				default:	fseek (f,0L,SEEK_SET);
						for (y=0;y<bstart;y++) fgets(line,256, f);

			}
		}
		fclose(f);
	}
	else
	{
		RenderBox(stx+190,sty+200, stx+410, sty+285, FILL, CMH);
		RenderBox(stx+190,sty+200, stx+410, sty+285, GRID, CMCIT);
		RenderString("Prozess beendet!", stx+190, sty+250, 410-190, CENTER, 30, CMCT);
		update_zeit();
		sleep(3);
	}
	rc_Nnull(0);
	return 0;
}

int show_ps_dmseg(char quote)
{
	FILE *f;
	int end_show=0,i=0, vabs=20, abs=0, bstart=0, y=0, z=0, end_temp=0, ps_end=0,ps_pointer=0, endf=10000;
	
	char line[256];
	char temp[10]="";
	if(quote==0) 
	{
		system("dmesg > /tmp/systmp");
	}
	else
	{
		system("ps > /tmp/systmp");
	}	
	if((f=fopen("/tmp/systmp","r"))!=NULL)
	{
		while (end_show==0)
		{
			endf=10000;
			abs=45;
			end_temp=0;
			hintergrund ();
			RenderBox(stx+20,linie_oben+20, enx-20, linie_unten-20, FILL, CMCST);
			RenderString("[ down ]  WEITER", (stx+30), eny-12, 300, LEFT, 26, CMCT);
			if (bstart>=17) RenderString("[ up ]  ZURÜCK", stx+30+(int)((enx-stx-30)/4), eny-12, 300, LEFT, 26, CMCT);
			if (quote!=0) RenderString("[ OK ]  Prozesstatus", stx+30+2*(int)((enx-stx-30)/4), eny-12, 300, LEFT, 26, CMCT);
			RenderString("[ Home ]  ENDE", stx+50+((int)((enx-stx-30)/4)*3),  eny-12,300, LEFT, 26, CMCT);
			for (i=0;i<17;i++)
			{
				if(fgets(line,256, f)!=NULL)
				{
					correct_string (line);
					if (quote==0) RenderString(line, stx+25, (linie_oben+abs), enx-stx-40, LEFT, 22, CMCT);

					else 	RenderString(line, stx+50, (linie_oben+abs), enx-stx-40, LEFT, 22, CMCT);

					abs=vabs+abs;
				}
				else
				{
					if (end_temp==0) endf=bstart+i;
					end_temp=1;
					RenderBox(stx+11,linie_unten+5,stx+20+(int)((enx-stx-30)/4), eny-9, FILL, CMCST);
				}
			}
			rc_Nnull(0);
			rc_null (0);
			abs=45;
			switch(ev.code)
			{
				case KEY_UP:	fseek (f,0L,SEEK_SET);
						if (bstart<17) 
						{
							bstart=0;
						}
						else
						{
							bstart=bstart-17;
						}
						for (y=0;y<bstart;y++) fgets(line,256, f);
						break;

				case KEY_DOWN:	if (end_temp!=0)
						{
							fseek (f,0L,SEEK_SET);
							for (y=0;y<bstart;y++) fgets(line,256, f);
						}
						else
						{
							bstart=bstart+17;
						}
						break;

				case KEY_HOME:	end_show=1;
						break;

				case KEY_OK:	if (quote!=0)
						{
//							printf("aussseeeen i= %d - ps_pointer = %d - endf= %d\n",i,ps_pointer,endf); fflush(stdout);
							RenderBox(stx+30+2*(int)((enx-stx-30)/4),linie_unten+6, stx+45+((int)((enx-stx-30)/4)*3), eny-6, FILL, CMCST);
							RenderString("[ OK ]  Prozesstatus", stx+30+2*(int)((enx-stx-30)/4), eny-12, 300, LEFT, 26, CMHT);
							ps_pointer=bstart;
							i=0;
							ps_end=0;
							while (ps_end==0)
							{ 
								if ((bstart+i)==0) 
								{
									i=1;
									abs=abs+vabs;
								}
								ps_pointer=bstart+i;
								RenderBox(stx+6,linie_unten+6, stx+290, eny-6, FILL, CMCST);
								RenderChar(62, (stx+30), (linie_oben+abs+9), enx, CMHT, 55);
								if (i<16) RenderString("[ down ]  WEITER", (stx+30), eny-12, 300, LEFT, 26, CMCT);
								if ((i>0)&&(ps_pointer>1)) RenderString("[ up ]  ZURÜCK", stx+30+(int)((enx-stx-30)/4), eny-12, 300, LEFT, 26, CMCT);
								rc_Nnull(0);
								rc_null (0);
								RenderBox(stx+21,linie_oben+21, stx+40, linie_unten-21, FILL, CMCST);

								switch(ev.code)
								{
									case KEY_UP:	if ((i>0)&&(ps_pointer>1))
											{
												abs=abs-vabs;//-vabs;
												i--;
												//i--;
												
											}
										break;

									case KEY_DOWN:	if ((i<16)&&(ps_pointer<(endf-1)))
											{
												abs=vabs+abs;
												i++;
											}
										break;

									case KEY_HOME:	ps_end=1;
										break;

									case KEY_OK:	fseek (f,0L,SEEK_SET);
											for (y=0;y<=ps_pointer;y++) fgets(line,256, f);
											y=0;
											z=0;
											while (!isdigit(line[y]))y++;
											while (!isspace(line[y]))
											{
												temp[z]=line[y];
												y++; z++;
											}
											temp[z]=0;
											sscanf(temp,"%d",&y);
											show_ps_status (y);
											ps_end=1;
										break;
								//	break;
								}
							}
							//	ps_end=0;
						}
						fseek (f,0L,SEEK_SET);
						for (y=0;y<bstart;y++) fgets(line,256, f);						break;
						ps_end=0;
						break;
							
				default:	fseek (f,0L,SEEK_SET);
						for (y=0;y<bstart;y++) fgets(line,256, f);

			}
		}
		fclose(f);
	}
	rc_Nnull(0);
	return 0;
}

int get_mem (void)
{
	FILE *file=NULL;
	char *ptr;
	char temp[256]="";
	char line_buffer[256]="";
	if ((file = fopen("/proc/meminfo","r"))==NULL)
	{
		printf("kann /proc/meminfo nicht öffnen\n");
		return -1;
	}
	else
	{
		while(fgets(line_buffer, sizeof(line_buffer),file))
		{
			if((ptr = strstr(line_buffer, "MemTotal"))!=NULL)
			{
				daten_auslesen(line_buffer, temp, 58);
				sscanf(temp,"%f",&memtotal);
			}
			if((ptr = strstr(line_buffer, "MemFree"))!=NULL)
			{
				daten_auslesen(line_buffer, temp, 58);
				sscanf(temp,"%f",&memfree);
			}
			if((ptr = strstr(line_buffer, "Active"))!=NULL)
			{
				daten_auslesen(line_buffer, temp, 58);
				sscanf(temp,"%f",&memactive);
			}
			if((ptr = strstr(line_buffer, "Inactive"))!=NULL)
			{
				daten_auslesen(line_buffer, temp, 58);
				sscanf(temp,"%f",&meminakt);
			}

		}
		memused=memtotal-memfree;
		memtotal=memtotal/1024.0;
		memused=memused/1024.0;
		memfree=memfree/1024.0;
		memactive=memactive/1024.0;
		meminakt=meminakt/1024.0;
		fclose(file);
	}
	return 0;
}

int get_mtd(void)
{
	FILE *file=NULL;
	mtd_count=0;
	if ((file = fopen("/proc/mtd","r"))==NULL)
	{
		printf("kann /proc/mtd nicht öffnen\n");
		return -1;
	}
	else
	{
		while(fgets(mtds[mtd_count], sizeof(mtds[mtd_count]),file)) 
		{
			correct_string (mtds[mtd_count]);
			mtd_count++;
		}
		fclose (file);
	}
	return 0;
}

void hintergrund (void)
{
unsigned char vstr[64];

		
	RenderBox(0, 0, var_screeninfo.xres, var_screeninfo.yres, FILL, CMCST);
	RenderBox(stx, sty, enx, eny, FILL, CMCIT);
	
	RenderBox(stx+rahmen, sty+rahmen, enx-rahmen, linie_oben-rabs, FILL, CMCST); //CMCST
	RenderBox(stx+rahmen, linie_oben+rabs, enx-rahmen, linie_unten-rabs, FILL, CMH);//CMH
	RenderBox(stx+rahmen, linie_unten+rabs, enx-rahmen, eny-rahmen, FILL, CMCST);//CMCST
	
	RenderBox(stx+rabs, sty+rabs, enx-rabs, linie_oben, GRID, CMCI);//CMCI
	RenderBox(stx+rabs, linie_oben, enx-rabs, linie_unten, GRID, CMCI);//CMCI
	RenderBox(stx+rabs, linie_unten, enx-rabs, eny-rabs, GRID, CMCI); //CMCI

	sprintf(vstr,"Neutrino Sysinfo %.2f", S_VERSION);
	RenderString(vstr, (stx+20), (sty+40), 300, LEFT, 40, GREEN);

	RenderString("  Worschter & SnowHead", (stx+260), (sty+42), 300, LEFT, 22, BLUE2);
}

void hauptseite (void)
{
	FILE *file=NULL;
	int abs_links=0,maxwidth=0, v_abs=37, v_dist=24, i=0, size=27, y=0, hoffs=0;
	char temp_string[256]="", fstring[16]=" Flash";
	abs_links=stx+160;
	maxwidth=enx-10-abs_links;
	get_info_cpu();
	get_uptime ();
	get_mtd();

	while (mtds[1][i]!=32) i++;
	i++;
	while (mtds[1][i]!=32) i++;
	i++;
	while (mtds[1][i]!=32)
	{
		temp_string[y]=mtds[1][i];
		y++;
		i++;
	}
	temp_string[y]=0;	
	sscanf(temp_string,"%d",&y);
	system("tuxinfo -t > /tmp/systmp");
	if((file=fopen("/tmp/systmp","r"))!=NULL)
	{
		fgets(tuner,sizeof(tuner)-1,file);
		fclose(file);
	}
	if(strlen(tuner))
	{
		strcpy(fstring,"I  ");
		if(strstr(tuner,"CABLE")!=NULL)
		{
			sprintf(tuner,"Kabel");
			hoffs=-15;
		}
		else
		{
			sprintf(tuner,"Sat");
		}
	}
	if (y< 10000) sprintf(temp_string,"%s  %s  2x%s %s",tuxbox_get_vendor_str (),tuxbox_get_model_str (), fstring, tuner);
	else sprintf(temp_string,"%s  %s  1x%s %s",tuxbox_get_vendor_str (),tuxbox_get_model_str (), fstring, tuner);
	
	
	RenderString("Modell:", (stx+40), (linie_oben+40), maxwidth, LEFT, size, CMHT);
	RenderString(temp_string, (abs_links+hoffs), (linie_oben+40), maxwidth, LEFT, size, CMCT);

	v_abs+=v_dist;
	sprintf(temp_string,"Prozessor  %s:",processor);
	RenderString(temp_string, (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString(cpu, (abs_links+hoffs), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	v_abs+=v_dist;
	
	RenderString("Takt:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString(pc_clock, (abs_links+hoffs), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	v_abs+=v_dist;
	
	RenderString("Bus-Takt:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString(bus_clock, (abs_links+hoffs), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	v_abs+=v_dist;
	
	RenderString("Bogomips:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString(bogomips, (abs_links+hoffs), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	v_abs+=v_dist;
	
	RenderString("Onlinezeit:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString(uptime, (abs_links+hoffs), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	v_abs+=v_dist+5;
	size++;
	
	
	RenderString("Ucodes:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size-3, CMHT);
	v_abs+=(v_dist-6);
	
	if ((file = fopen("/var/tuxbox/ucodes/avia600.ux","r"))!=NULL)
	{
		fclose (file);
		checkFile("/var/tuxbox/ucodes/avia600.ux", temp_string);
		RenderString(temp_string, (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size-6, CMCT);
	}
	else
	{
		RenderString("keine avia600.ux", (stx+35), (linie_oben+v_abs), maxwidth, LEFT, size-6, CMCT);
	}
	
	if ((file = fopen("/var/tuxbox/ucodes/cam-alpha.bin","r"))!=NULL)
	{
		fclose (file);
		checkFile("/var/tuxbox/ucodes/cam-alpha.bin", temp_string);
		RenderString(temp_string, (abs_links+40), (linie_oben+v_abs), maxwidth, LEFT, size-6, CMCT);
	}
	else 
	{
		RenderString("keine camalpha.bin", (abs_links+40), (linie_oben+v_abs), maxwidth, LEFT, size-6, CMCT);
	}
	
	v_abs+=(v_dist-6);
	if ((file = fopen("/var/tuxbox/ucodes/avia500.ux","r"))!=NULL)
	{
		fclose (file);
		checkFile("/var/tuxbox/ucodes/avia500.ux", temp_string);
		RenderString(temp_string, (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size-6, CMCT);
	}
	else 
	{
		RenderString("keine avia500.ux", (stx+35), (linie_oben+v_abs), maxwidth, LEFT, size-6, CMCT);
	}
	
	if ((file = fopen("/var/tuxbox/ucodes/ucode.bin","r"))!=NULL)
	{
		fclose (file);
		checkFile("/var/tuxbox/ucodes/ucode.bin", temp_string);
		RenderString(temp_string, (abs_links+40), (linie_oben+v_abs), maxwidth, LEFT, size-6, CMCT);
	}
	else 
	{
		RenderString("ucode 0014 (build in)", (abs_links+40), (linie_oben+v_abs), maxwidth, LEFT, size-6, CMCT);
	}
	v_abs=v_abs+v_dist+4;
	
	
	RenderString("Flash Struktur:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size-3, CMHT);
	for (i=1;i<mtd_count;i++)
	{
		v_abs+=(v_dist-6);
		RenderString((mtds[i]), (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size-8, CMCT);
	}
	v_abs+=(v_dist+10);
	up_place=v_abs;
	
	RenderString(kernel, (stx), (linie_unten-10), enx-stx, CENTER, 19, CMCT);
	PaintIcon("/share/tuxbox/neutrino/icons/rot.raw",stx+27,linie_unten-8+(int)((eny-linie_unten)/2),1);
	RenderString("FILE-SYSTEM", (stx+46), eny-12, maxwidth, LEFT, size-2, CMCT);
	PaintIcon("/share/tuxbox/neutrino/icons/gruen.raw",stx+27+(int)((enx-stx-30)/4),linie_unten-8+(int)((eny-linie_unten)/2),1);
	RenderString("PROZESSE",  stx+45+(int)((enx-stx-30)/4), eny-12, maxwidth, LEFT, size-2, CMCT);
	PaintIcon("/share/tuxbox/neutrino/icons/gelb.raw",stx+12+((int)((enx-stx-30)/4)*2),linie_unten-8+(int)((eny-linie_unten)/2),1);
	RenderString("KERNEL MESSAGE", stx+30+((int)((enx-stx-30)/4)*2),  eny-12, maxwidth, LEFT, size-2, CMCT);
	PaintIcon("/share/tuxbox/neutrino/icons/blau.raw",stx+27+((int)((enx-stx-30)/4)*3),linie_unten-8+(int)((eny-linie_unten)/2),1);
	RenderString("NETZWERK", stx+45+((int)((enx-stx-30)/4)*3),  eny-12, maxwidth, LEFT, size-2, CMCT);
}

void up_main_mem (void)
{
	char temp_string[200]="";
	int LO=0,maxwidth=200,size=28;
	LO=linie_oben+2;

	RenderBox(enx-240, LO+5, enx-10, LO+32, FILL, CMH);
	RenderBox(enx-240, LO+138, enx-10, LO+175, FILL, CMH);
	RenderBox(enx-240, LO+320, enx-10, LO+350, FILL, CMH);
	
	RenderString("Speicher total:", (stx+390), (LO+25), maxwidth, LEFT, size-4, CMHT);
	if (x_pos==enx-197) get_mem();
	if (x_pos==enx-197) get_perf();
	
	
	old_user_perf	=user_perf;
	old_nice_perf	=nice_perf;
	old_sys_perf	=sys_perf;
	old_idle_perf	=idle_perf;

	old_memtotal	=memtotal;
	old_memfree	=memfree;
	old_memused	=memused;
	old_memactive	=memactive;
	old_meminakt	=meminakt;
	
	
	if (x_pos!=enx-197) get_mem();
	if (x_pos!=enx-197) get_perf();
	
	sprintf(temp_string," %2.1f MB",memtotal);
	RenderString(temp_string, (stx+490), (LO+25), maxwidth, LEFT, size-4, CMCT);
	if(x_pos==enx-197)
	{
		RenderBox(enx-200, LO+32, enx-40, LO+136, FILL, CMCST);
		RenderBox(enx-200, LO+32, enx-40, LO+136, GRID, CMCIT);
	}
	sprintf(temp_string,"free: %2.1f MB",memfree);
	RenderString(temp_string, (stx+385), (LO+155), maxwidth, LEFT, size-6, GREEN);
	sprintf(temp_string,"used: %2.1f MB",memused);
	RenderString(temp_string, (stx+470), (LO+155), maxwidth, LEFT, size-6, BLUE2);
	sprintf(temp_string,"aktiv: %2.1f MB",memactive);
	RenderString(temp_string, (stx+385), (LO+170), maxwidth, LEFT, size-6, YELLOW);
	sprintf(temp_string,"inakt: %2.1f MB",meminakt);
	RenderString(temp_string, (stx+470), (LO+170), maxwidth, LEFT, size-6, LRED);
	RenderLine(x_pos-3,(int)LO+135-(old_meminakt*(100/memtotal)), x_pos+1, (int)LO+135-(meminakt*(100/memtotal)),2, RED);
	RenderLine(x_pos-3,(int)LO+135-(old_memtotal*(100/memtotal)), x_pos+1, (int)LO+135-(memtotal*(100/memtotal)),2, CMCT);
	RenderLine(x_pos-3,(int)LO+135-(old_memfree*(100/memtotal)), x_pos+1, (int)LO+135-(memfree*(100/memtotal)),2, GREEN);
	RenderLine(x_pos-3,(int)LO+135-(old_memused*(100/memtotal)), x_pos+1, (int)LO+135-(memused*(100/memtotal)),2, BLUE2);
	RenderLine(x_pos-3,(int)LO+135-(old_memactive*(100/memtotal)), x_pos+1, (int)LO+135-(memactive*(100/memtotal)),2, YELLOW);
	
	
		
	LO=LO+178;
	RenderString("Systemauslastung in %", (stx+390), (LO+22), maxwidth, LEFT, size-4, CMHT);
	if(x_pos==enx-197)
	{
		RenderBox(enx-200, LO+30, enx-40, LO+136, FILL, CMCST);
		RenderBox(enx-200, LO+30, enx-40, LO+136, GRID, CMCIT);
	}
	sprintf(temp_string,"user: %3.1f",user_perf);
	RenderString(temp_string, (stx+385), (LO+155), maxwidth, LEFT, size-6, GREEN);
	sprintf(temp_string,"idle: %3.1f",idle_perf);
	RenderString(temp_string, (stx+470), (LO+155), maxwidth, LEFT, size-6, BLUE2);
	sprintf(temp_string,"sytem: %3.1f",sys_perf);
	RenderString(temp_string, (stx+385), (LO+170), maxwidth, LEFT, size-6, YELLOW);
	sprintf(temp_string,"nice: %3.1f",nice_perf);
	RenderString(temp_string, (stx+470), (LO+170), maxwidth, LEFT, size-6, LRED);

	RenderLine(x_pos-3,(int)LO+133-(old_nice_perf), x_pos+1, (int)LO+133-(nice_perf),2, LRED);
	RenderLine(x_pos-3,(int)LO+133-(old_user_perf), x_pos+1, (int)LO+133-(user_perf),2, GREEN);
	RenderLine(x_pos-3,(int)LO+133-(old_idle_perf), x_pos+1, (int)LO+133-(idle_perf),2, BLUE2);
	RenderLine(x_pos-3,(int)LO+133-(old_sys_perf), x_pos+1, (int)LO+133-(sys_perf),2, YELLOW);
	
	x_pos=x_pos+3;
	if(x_pos>=enx-44) x_pos=enx-197;
	
	RenderString("( 1 )", enx-33, linie_oben+50, 30, LEFT, 23, CMCT);
	RenderChar('V',enx-25,linie_oben+72,enx-15,CMCT,18);
	RenderChar('o',enx-25,linie_oben+85,enx-15,CMCT,18);
	RenderChar('l',enx-24,linie_oben+100,enx-20,CMCT,18);
	RenderChar('l',enx-24,linie_oben+115,enx-20,CMCT,18);
	RenderString("b.",enx-25,linie_oben+130,30,LEFT, 23, CMCT);
	RenderString("( 2 )", enx-33, linie_oben+50+178, 30, LEFT, 23, CMCT);
	RenderChar('V',enx-25,linie_oben+72+178,enx-15,CMCT,18);
	RenderChar('o',enx-25,linie_oben+85+178,enx-15,CMCT,18);
	RenderChar('l',enx-24,linie_oben+100+178,enx-20,CMCT,18);
	RenderChar('l',enx-24,linie_oben+115+178,enx-20,CMCT,18);
	RenderString("b.",enx-25,linie_oben+130+178,30, LEFT,23,CMCT);

}


void get_net_traf(void)
{
	FILE 	*file=NULL;
	char 	*ptr;
	char 	line_buffer[256]	="";
	if ((file = fopen("/proc/net/dev","r"))==NULL)
	{
		printf("Open Proc-File Failure\n"); fflush(stdout);
	}
	else
	{
		while (fgets(line_buffer, sizeof(line_buffer), file))
		{
			if((ptr = strstr(line_buffer, "eth0:"))!=NULL)
			{
#if defined NET_DEBUG2
				printf("Procline=%s\n",line_buffer); fflush(stdout);
#endif
				sscanf(ptr+5,"%Ld%ld%ld%ld%ld%ld%ld%ld%Ld%ld",&read_akt,&read_packet,&dummy,&dummy,&dummy,&dummy,&dummy,&dummy,&write_akt,&write_packet);
#if defined NET_DEBUG2
				printf("Read=%Ld\n",read_akt);
				printf("Write=%Ld\n",write_akt); fflush(stdout);
				printf("Read_packet=%ld\n",read_packet);
				printf("Write_pcket=%ld\n",write_packet); fflush(stdout);
#endif
				
			}
		}
		fclose (file);
	}
}


void up_net (void)
{
	char temp_string[200]="";
	int LO=0,maxwidth=200,size=28;
	LO=linie_oben+2;

	RenderBox(enx-240, LO+138, enx-10, LO+225, FILL, CMH);
	
	RenderString("Netzauslastung:", (stx+390), (LO+25), maxwidth, LEFT, size-4, CMHT);
	if(x_pos==enx-197)
	{
		RenderBox(enx-200, LO+32, enx-40, LO+136, FILL, CMCST);
		RenderBox(enx-200, LO+32, enx-40, LO+136, GRID, CMCIT);
	}
	get_net_traf();
	read_old=read_akt;
	write_old=write_akt;
	usleep(730000);
	get_net_traf();
	delta_read=(read_akt-read_old);
	delta_write=(write_akt-write_old);
	
	RenderString("Receive:", (stx+350), (LO+155), maxwidth, LEFT, size-4, LRED);
	sprintf(temp_string,"Packet: %ld",read_packet);
	RenderString(temp_string, (stx+340), (LO+175), maxwidth, LEFT, size-6, CMCT);
	sprintf(temp_string,"Byte: %Ld",read_akt);
	RenderString(temp_string, (stx+450), (LO+175), maxwidth, LEFT, size-6, CMCT);
	
	RenderString("Transmit:", (stx+350), (LO+195), maxwidth, LEFT, size-4, GREEN);
	sprintf(temp_string,"Packet: %ld",write_packet);
	RenderString(temp_string, (stx+340), (LO+215), maxwidth, LEFT, size-6, CMCT);
	sprintf(temp_string,"Byte: %Ld",write_akt);
	RenderString(temp_string, (stx+450), (LO+215), maxwidth, LEFT, size-6, CMCT);
	
	delta_read=delta_read/10240;
	delta_write=delta_write/10240;
	if(delta_read>=100) delta_read=100;
	if(delta_write>=100) delta_write=100;

	RenderLine(x_pos,(int)LO+133-delta_read_old, x_pos+3, (int)LO+133-delta_read,3, LRED);
	RenderLine(x_pos,(int)LO+133-delta_write_old, x_pos+3, (int)LO+133-delta_write,3, GREEN);
	
	delta_read_old=delta_read;
	delta_write_old=delta_write;

	x_pos=x_pos+3;
	if(x_pos>=enx-44) x_pos=enx-197;
		
	
}


void render_koord (char ver)
{
	int i=0;
	char temptext[10]="";
	win_sx=stx+66; win_sy=linie_oben+20; win_ex=enx-24; win_ey=linie_unten-45;
	fflush(stdout);
	RenderBox(stx+10,linie_oben+10, enx-10, linie_unten-10, FILL, CMCST);
	
	RenderVLine( win_sx, win_sy, win_ey, 0, 2, 1, CMCT);
	RenderHLine( win_sx, win_ey, win_ex, 0, 2, 1, CMCT);
	RenderLine(win_sx-4,win_sy+10,win_sx,win_sy,6,CMCT);
	RenderLine(win_sx+4,win_sy+10,win_sx,win_sy,6,CMCT);
	RenderLine(win_ex-12,win_ey-4,win_ex,win_ey,4,CMCT);
	RenderLine(win_ex-12,win_ey+4,win_ex,win_ey,4,CMCT);

	for (i=win_sx+50;i<win_ex;i=i+50) RenderVLine(i, win_sy+20, win_ey,1, 1, 10, CMCS);
	if (ver==1)
	{
		get_mem();
		for (i=1;i<=6; i++) 
		{	
			RenderHLine( win_sx, (win_ey-3-(i*50)), win_ex-20, 1, 1, 10, CMCS);
			RenderHLine( win_sx-10, win_ey-3-(i*50), win_sx, 0, 2, 1, CMCT);
			sprintf(temptext,"%d MB",(i*5));
			if (memtotal>40) sprintf(temptext,"%d MB",(i*10));
			RenderString(temptext, (win_sx-50),win_ey-(i*50)+2, 38, RIGHT, 20, CMCT);
		}	
	}
	if (ver==2)
	{
		for (i=1;i<=10; i++) 
		{	
			RenderHLine( win_sx, (win_ey-3-(i*30)), win_ex-20, 1, 1, 10, CMCS);
			RenderHLine( win_sx-10, win_ey-3-(i*30), win_sx, 0, 2, 1, CMCT);
			sprintf(temptext,"%d %c",(i*10),37);
			RenderString(temptext, (win_sx-50),win_ey-(i*30)+2, 38, RIGHT, 20, CMCT);
		}	
	}
	
}

void up_full (char sel)
{
	char temp_string[20]="";
	RenderBox(stx+10,win_ey+6, enx-15, linie_unten-10, FILL, CMCST);
	if (sel==1)
	{
		if (x_pos==win_sx+6) get_mem();

		old_memtotal	=memtotal;
		old_memfree	=memfree;
		old_memused	=memused;
		old_memactive	=memactive;
		old_meminakt	=meminakt;
	
	
		if (x_pos!=win_sx+3) get_mem();
		sprintf(temp_string,"total: %2.1f MB",memtotal);
		RenderString(temp_string, (win_sx), (linie_unten -17), 100, LEFT, 26, CMCT);
		sprintf(temp_string,"free: %2.1f MB",memfree);
		RenderString(temp_string, (win_sx+100), (linie_unten -17), 100, LEFT, 26, GREEN);
		sprintf(temp_string,"used: %2.1f MB",memused);
		RenderString(temp_string, (win_sx+200), (linie_unten -17), 100, LEFT, 26, BLUE2);
		sprintf(temp_string,"aktiv: %2.1f MB",memactive);
		RenderString(temp_string, (win_sx+300), (linie_unten -17), 100, LEFT, 26, YELLOW);
		sprintf(temp_string,"inakt: %2.1f MB",meminakt);
		RenderString(temp_string, (win_sx+400), (linie_unten -17), 100, LEFT, 26, LRED);
		RenderLine(x_pos-3,(int)win_ey-3-3*(old_meminakt*(100/memtotal)), x_pos+1, (int)win_ey-3-3*(meminakt*(100/memtotal)),2, LRED);
		RenderLine(x_pos-3,(int)win_ey-3-3.3*(old_memtotal*(100/memtotal)), x_pos+1, (int)win_ey-3-3.3*(memtotal*(100/memtotal)),2, CMCT);
		RenderLine(x_pos-3,(int)win_ey-3-3*(old_memfree*(100/memtotal)), x_pos+1, (int)win_ey-3-3*(memfree*(100/memtotal)),2, GREEN);
		RenderLine(x_pos-3,(int)win_ey-3-3*(old_memused*(100/memtotal)), x_pos+1, (int)win_ey-3-3*(memused*(100/memtotal)),2, BLUE2);
		RenderLine(x_pos-3,(int)win_ey-3-3*(old_memactive*(100/memtotal)), x_pos+1, (int)win_ey-3-3*(memactive*(100/memtotal)),2, YELLOW);
	
		x_pos=x_pos+3;
		if (x_pos >=win_ex-20) 
		{
			x_pos= win_sx+6;	
			render_koord (1);
		}
               sleep(1);
	}
	if (sel==2)
	{
		if (x_pos==win_sx+6) 
		{
			get_perf();
			old_nice_perf=win_ey-3-3*(nice_perf);
			old_user_perf=win_ey-3-3*(user_perf);
			old_idle_perf=win_ey-3-3*(idle_perf);
			old_sys_perf=win_ey-3-3*(sys_perf);
		}
		else
		{
			old_user_perf	=user_perf;
			old_nice_perf	=nice_perf;
			old_sys_perf	=sys_perf;
			old_idle_perf	=idle_perf;
		}
		if (x_pos!=win_sx+6) get_perf();
		
		sprintf(temp_string,"user: %3.1f %c",user_perf,37);
		RenderString(temp_string, (win_sx), (linie_unten -17), 100, LEFT, 26, GREEN);
		sprintf(temp_string,"idle: %3.1f %c",idle_perf,37);
		RenderString(temp_string, (win_sx+100), (linie_unten -17), 100, LEFT, 26, BLUE2);
		sprintf(temp_string,"sytem: %3.1f %c",sys_perf,37);
		RenderString(temp_string, (win_sx+200), (linie_unten -17), 100, LEFT, 26, YELLOW);
		sprintf(temp_string,"nice: %3.1f %c",nice_perf,37);
		RenderString(temp_string, (win_sx+300), (linie_unten -17), 100, LEFT, 26, LRED);

		nice_perf=win_ey-3-3*(nice_perf);
		user_perf=win_ey-3-3*(user_perf);
		idle_perf=win_ey-3-3*(idle_perf);
		sys_perf=win_ey-3-3*(sys_perf);

		RenderLine(x_pos,(int)old_nice_perf, x_pos+3, (int)nice_perf,3, LRED);
		RenderLine(x_pos,(int)old_user_perf, x_pos+3, (int)user_perf,3, GREEN);
		RenderLine(x_pos,(int)old_idle_perf, x_pos+3, (int)idle_perf,3, BLUE2);
		RenderLine(x_pos,(int)old_sys_perf, x_pos+3, (int)sys_perf,3, YELLOW);

		x_pos=x_pos+3;
		if (x_pos >=win_ex-20) 
		{
			x_pos= win_sx+6;
			render_koord (2);
		}
	
	}
}

void mem_full(void)
{
	char end_show=0;
	hintergrund ();
	RenderString("[ Home ]  ENDE", stx+50+((int)((enx-stx-30)/4)*3),  eny-12,300, LEFT, 26, CMCT);
	render_koord(1);
	x_pos=win_sx+6;
	while (!end_show)
	{
		rc_Nnull(2);
		rc_null (2);
		switch(ev.code)
		{
			case KEY_HOME:	end_show=1;
					break;
			default:	end_show=0;
		}
	}

}


void perf_full(void)
{
	char end_show=0;
	hintergrund ();
	RenderString("[ Home ]  ENDE", stx+50+((int)((enx-stx-30)/4)*3),  eny-12,300, LEFT, 26, CMCT);
	render_koord(2);
	x_pos=win_sx+6;
	while (!end_show)
	{
		rc_Nnull(3);
		rc_null (3);
		switch(ev.code)
		{
			case KEY_HOME:	end_show=1;
					break;
			default:	end_show=0;
		}
	}
	
}

void get_network (void)
{
	FILE *file=NULL;
	char 	*ptr;
	char 	line_buffer[256]	="";
	char 	temp_line[256]		="";
	system ("ifconfig eth0 > /tmp/.sys_net");
	if ((file = fopen("/tmp/.sys_net","r"))==NULL)
	{
		printf("Sysinfo: Netfile konnte nicht geoeffnet werden\n"); fflush(stdout);
	}
	else
	{
		while (fgets(line_buffer, sizeof(line_buffer), file))               
		{
			sprintf(temp_line,"HWaddr ");
			if((ptr = strstr(line_buffer, temp_line))!=NULL)
			{
				corr(MAC_ADRESS,(ptr+strlen(temp_line)));
#if defined NET_DEBUG
				printf("MAC-Adress=%s\n",MAC_ADRESS); fflush(stdout);
			
#endif
			}
			sprintf(temp_line,"inet addr:");
			if((ptr = strstr(line_buffer, temp_line))!=NULL)
			{
				corr(IP_ADRESS,(ptr+strlen(temp_line)));
#if defined NET_DEBUG
				printf("IP_ADRESS=%s\n",IP_ADRESS); fflush(stdout);
#endif
			}
			sprintf(temp_line,"Bcast:");
			if((ptr = strstr(line_buffer, temp_line))!=NULL)
			{
				corr(BC_ADRESS,(ptr+strlen(temp_line)));
#if defined NET_DEBUG
				printf("BC_ADRESS=%s\n",BC_ADRESS); fflush(stdout);
#endif
			}
			sprintf(temp_line,"Mask:");
			if((ptr = strstr(line_buffer, temp_line))!=NULL)
			{
				corr(MASK_ADRESS,(ptr+strlen(temp_line)));
#if defined NET_DEBUG
				printf("MASK_ADRESS=%s\n",MASK_ADRESS); fflush(stdout);
#endif
			}
			sprintf(temp_line,"Base address:");
			if((ptr = strstr(line_buffer, temp_line))!=NULL)
			{
				corr(BASE_ADRESS,(ptr+strlen(temp_line)));
#if defined NET_DEBUG
				printf("BASE_ADRESS=%s\n",BASE_ADRESS); fflush(stdout);
#endif
			}
		}
		fclose (file);
	}
	sprintf(temp_line,"gateway ");
	if ((file = fopen(NETWORKFILE_VAR,"r"))==NULL)
	{
		if ((file = fopen(NETWORKFILE_ETC,"r"))==NULL)
		{		
			printf("Sysinfo: Interface-File konnten nicht gefunden werden\n"); fflush(stdout);
		}
		else
		{
			while (fgets(line_buffer, sizeof(line_buffer), file))
			{
				if((ptr = strstr(line_buffer, temp_line))!=NULL)
				{
					corr(GATEWAY_ADRESS,(ptr+strlen(temp_line)));
					
#if defined NET_DEBUG
					printf("GATEWAY_ADRESS ETC=%s\n",GATEWAY_ADRESS); fflush(stdout);
#endif
				}	
			}
			fclose(file);
		}
	}
	else
	{
		while (fgets(line_buffer, sizeof(line_buffer), file))
		{
			if((ptr = strstr(line_buffer, temp_line))!=NULL) 
			{
				corr(GATEWAY_ADRESS,(ptr+strlen(temp_line)));
#if defined NET_DEBUG
				printf("GATEWAY_ADRESS VAR=%s\n",GATEWAY_ADRESS); fflush(stdout);
#endif
			}
		}
		fclose(file);
	}
	sprintf(temp_line,"nameserver ");
	if ((file = fopen(RESOLVEFILE_VAR,"r"))==NULL)
	{
		if ((file = fopen(RESOLVEFILE_ETC,"r"))==NULL)
		{		
			printf("Sysinfo: Resolve-File konnten nicht gefunden werden\n"); fflush(stdout);
		}
		else
		{
			while (fgets(line_buffer, sizeof(line_buffer), file))
			{
				if((ptr = strstr(line_buffer, temp_line))!=NULL)
				{
					corr(NAMES_ADRESS,(ptr+strlen(temp_line)));
					
#if defined NET_DEBUG
					printf("NAMES_ADRESS ETC=%s\n",NAMES_ADRESS); fflush(stdout);
#endif
				}	
			}
			fclose(file);
		}
	}
	else
	{
		while (fgets(line_buffer, sizeof(line_buffer), file))
		{
			if((ptr = strstr(line_buffer, temp_line))!=NULL) 
			{
				corr(NAMES_ADRESS,(ptr+strlen(temp_line)));
#if defined NET_DEBUG
				printf("NAMES_ADRESS VAR=%s\n",NAMES_ADRESS); fflush(stdout);
#endif
			}
		}
		fclose(file);
	}

#if defined NET_DEBUG
	printf("IP_ADRESS=%s\n",IP_ADRESS); fflush(stdout);
	printf("MAC-Adress=%s\n",MAC_ADRESS); fflush(stdout);
	printf("MASK_ADRESS=%s\n",MASK_ADRESS); fflush(stdout);
	printf("BC_ADRESS=%s\n",BC_ADRESS); fflush(stdout);
	printf("GATEWAY_ADRESS=%s\n",GATEWAY_ADRESS); fflush(stdout);
	printf("NAMES_ADRESS=%s\n",NAMES_ADRESS); fflush(stdout);
#endif
	
	
	
}

void search_clients (void)
{
	char temp_IP[20]	={ "" };
	int i=0, j=3;
	char temp_line[50]		="";
	RenderBox(stx+rahmen+150, linie_oben+300, enx-rahmen-150, linie_unten-rabs-30, FILL, BLUE2);//CMH
	RenderBox(stx+rahmen+155, linie_oben+305, enx-rahmen-155, linie_unten-rabs-35, FILL, CMH);//CMH
	RenderString("Suche nach Clients", (stx+rahmen+190), (linie_oben+340), 300, LEFT, 40, CMHT);
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);

	while ((IP_ADRESS[i]>32)&&(j>0))
	{
		temp_IP[i]=IP_ADRESS[i];
		if (IP_ADRESS[i]==46) j--;
		i++;
	}
#if defined NET_DEBUG
	printf("temp IP=%s\n",temp_IP); fflush(stdout);
#endif
	for (i=0;i<255;i++)
	{
		sprintf(temp_line,"ping -c 1 %s%d > /dev/null & ",temp_IP,i);
		system(temp_line);
		usleep(1000);
#if defined NET_DEBUG
		printf("temp_line=%s\n",temp_line); fflush(stdout);
#endif
	}
	sleep (2);
	system("killall ping  > /dev/null &");

}


void show_network (void)
{
	FILE *file=NULL;
	char 	*ptr;
	char 	line_buffer[256]	="";
	char temp_line[50]		="";
	int ret				=0;	
	int i=0,j=0,old_abs=0, x_plus=0, mainloop=1;
	char temp_IP[20]		={ "" };
	char temp_MAC[20]		={ "" };
	x_pos=enx-197;
	hintergrund ();
	get_network ();
	int abs_links=0,maxwidth=0, v_abs=37, v_dist=24, size=27;
	abs_links=stx+160;
	maxwidth=enx-10-abs_links;
//	RenderCircle( stx+30,linie_unten+(int)((eny-linie_unten)/2),'R');
	PaintIcon("/share/tuxbox/neutrino/icons/rot.raw",stx+27,linie_unten-8+(int)((eny-linie_unten)/2),1);
	RenderString("CLIENTS SUCHEN", (stx+46), eny-12, maxwidth, LEFT, size-2, CMCT);
	RenderString("[ Home ]  ENDE", stx+50+((int)((enx-stx-30)/4)*3),  eny-12,300, LEFT, 26, CMCT);
	
	RenderString("Box IP:", (stx+40), (linie_oben+40), maxwidth, LEFT, size, CMHT);
	RenderString(IP_ADRESS, (abs_links), (linie_oben+40), maxwidth, LEFT, size, CMCT);
	sprintf(temp_line, "ping -c 1 %s > /dev/null",IP_ADRESS);
#if defined NET_DEBUG
	printf("%s\n",temp_line); fflush(stdout);
#endif
	ret=system(temp_line);
	if (ret==0)
	{
		RenderString("ping ok", (abs_links+130), (linie_oben+40), maxwidth, LEFT, size, GREEN);
	}
	else
	{
		RenderString("ping false", (abs_links+130), (linie_oben+40), maxwidth, LEFT, size, RED);
	}
	v_abs+=v_dist;
	
	RenderString("Gateway:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString(GATEWAY_ADRESS, (abs_links), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	sprintf(temp_line, "ping -c 1 %s > /dev/null",GATEWAY_ADRESS);
#if defined NET_DEBUG
	printf("%s\n",temp_line); fflush(stdout);
#endif
	ret=system(temp_line);
	if (ret==0)
	{
		RenderString("ping ok", (abs_links+130), (linie_oben+v_abs), maxwidth, LEFT, size, GREEN);
	}
	else
	{
		RenderString("ping false", (abs_links+130), (linie_oben+v_abs), maxwidth, LEFT, size, RED);
	}
	v_abs+=v_dist;
	
	RenderString("Nameserver:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString(NAMES_ADRESS, (abs_links), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	sprintf(temp_line, "ping -c 1 %s > /dev/null",NAMES_ADRESS);
#if defined NET_DEBUG
	printf("%s\n",temp_line); fflush(stdout);
#endif
	ret=system(temp_line);
	if (ret==0)
	{
		RenderString("ping ok", (abs_links+130), (linie_oben+v_abs), maxwidth, LEFT, size, GREEN);
	}
	else
	{
		RenderString("ping false", (abs_links+130), (linie_oben+v_abs), maxwidth, LEFT, size, RED);
	}
	v_abs+=v_dist;

	RenderString("Internet:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString("www.google.de", (abs_links), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	sprintf(temp_line, "ping -c 1 google.de > /dev/null");
#if defined NET_DEBUG
	printf("%s\n",temp_line); fflush(stdout);
#endif
	ret=system(temp_line);
	if (ret==0)
	{
		RenderString("ping ok", (abs_links+130), (linie_oben+v_abs), maxwidth, LEFT, size, GREEN);
	}
	else
	{
		RenderString("ping false", (abs_links+130), (linie_oben+v_abs), maxwidth, LEFT, size, RED);
	}
	v_abs+=v_dist;

		
	RenderString("Box MAC:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString(MAC_ADRESS, (abs_links), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	v_abs+=v_dist;
	
	RenderString("Subn. Mask:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString(MASK_ADRESS, (abs_links), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	v_abs+=v_dist;
	
	RenderString("Broadcast:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString(BC_ADRESS, (abs_links), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	v_abs+=v_dist;
	
	RenderString("Base Addr:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size, CMHT);
	RenderString(BASE_ADRESS, (abs_links), (linie_oben+v_abs), maxwidth, LEFT, size, CMCT);
	v_abs+=v_dist+10;
	
	RenderString("Bekannte Netzwerk Clients:", (stx+40), (linie_oben+v_abs), maxwidth, LEFT, size-3, CMHT);
	old_abs=v_abs;
	
	
	RenderBox(enx-200, linie_oben+34, enx-40, linie_oben+138, FILL, CMCST);
	RenderBox(enx-200, linie_oben+34, enx-40, linie_oben+138, GRID, CMCIT);
	
	
	while(mainloop!=0)
	{
		x_plus=0;
		v_abs+=v_dist;
		RenderBox(stx+rahmen+2, linie_oben+old_abs+5, enx-rahmen-2, linie_unten-rabs-2, FILL, CMH);//CMH

		ret=0;
		sprintf(temp_line,"0x2");
		if ((file = fopen("/proc/net/arp","r"))!=NULL)
		{
			while ((fgets(line_buffer, sizeof(line_buffer), file))&& ret<12)
			{
				if((ptr = strstr(line_buffer, temp_line))!=NULL)
				{
#if defined NET_DEBUG
					printf("arp %s\n",line_buffer); fflush(stdout);
					printf("ptr %s\n",ptr); fflush(stdout);
#endif
					i=0; j=0;
					while (line_buffer[i]>32) 
					{
						temp_IP[j]=line_buffer[i];
						i++;
						j++;
					}
					temp_IP[j]=0;
					j=0;
					i=0;
					while (ptr[i]>32) i++;
//					i++;
					while (ptr[i]<33) i++;
//					i++;
					while (ptr[i]>32) 
					{
						temp_MAC[j]=ptr[i];
						i++;
						j++;
					}
					temp_MAC[j]=0;	
					RenderString(temp_IP, (stx+40+x_plus), (linie_oben+v_abs), maxwidth, LEFT, size-4, CMCT);
					RenderString(temp_MAC, (stx+145+x_plus), (linie_oben+v_abs), maxwidth, LEFT, size-4, CMCT);
					ret++;
					v_abs+=(v_dist-1);
					if (ret==6)
					{
						v_abs=old_abs+v_dist;
						x_plus=enx-stx;
						x_plus=x_plus/2;
						x_plus=x_plus-30;
					}
				}
			}
			fclose (file);
		}
		rc_Nnull(4);
		rc_null (4);
		switch(ev.code)
		{
			case KEY_RED:
				search_clients();
				v_abs=old_abs;
				break;

			case KEY_HOME:
				mainloop=0;
				break;

			default:
				v_abs=old_abs;
		}
	}
}

int main (void)
{
	init_fb ();
	FILE *file=NULL;
	char mainloop=1;
	char systemp[35]="/var/plugins/tuxwet/swisstime &";
	
	signal(SIGINT, quit_signal);
	signal(SIGTERM, quit_signal);
	signal(SIGQUIT, quit_signal);

	put_instance(instance=get_instance()+1);

	linie_oben=52+sty;
	linie_unten=eny-38;
	rahmen=8;
	rabs=(int)rahmen/2;
	if ((file = fopen("/var/plugins/tuxwet/swisstime","r"))!=NULL)
	{
		fclose (file);
		system (systemp);
	}
	while (mainloop)
	{
		x_pos=enx-197;
		hintergrund ();
		hauptseite ();
		rc_Nnull(1);
		rc_null (1);
		switch(ev.code)
		{
			case KEY_1:		mem_full();
						break;
					
			case KEY_2:		perf_full();
						break;

			case KEY_RED:		show_FileS();
						break;

			case KEY_GREEN:		show_ps_dmseg(1);
						break;

			case KEY_YELLOW:	show_ps_dmseg(0);
						break;

			case KEY_BLUE:		show_network();
						break;

			case KEY_HOME:		mainloop=0;
						break;

		}
		if ((file = fopen("/var/plugins/tuxwet/swisstime","r"))!=NULL)
		{
			fclose (file);
			system (systemp);
		}
	}

	memset(lbb, 0, var_screeninfo.xres*var_screeninfo.yres);
	memcpy(lfb, lbb, var_screeninfo.xres*var_screeninfo.yres);
	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);
	free(line_buffer);
	free(trstr);
	free(lbb);
	munmap(lfb, fix_screeninfo.smem_len);
	fcntl(rc, F_SETFL, O_NONBLOCK);
	close(fb);
	close(rc);
	remove ("/tmp/systmp");
	put_instance(get_instance()-1);
	return 0;
}
