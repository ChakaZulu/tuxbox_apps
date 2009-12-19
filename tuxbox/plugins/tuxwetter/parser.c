/*
 * $Id: parser.c,v 1.1 2009/12/19 19:42:49 rhabarber1848 Exp $
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

/* 
*********************************************************************************************
********************************** New Tuxwetter XML-File_parser ****************************
*********************************************************************************************
*/
#include <curl/curl.h>
#include "tuxwetter.h"
#include "parser.h"
#include "http.h"

/*
Interne Variablen Bitte nicht direkt aufrufen!!!
*/
char 	data		[1000][50];
char 	conveng		[500][40]; 
char	convger		[500][40];
int	prev_count =	0;
char    null[2]=  	{0,0};
int 	tc=		0;
int 	t_actday=	0;
int	t_actmonth=	0;
int 	t_actyear=	0;
const char mnames[12][10]={"Januar","Februar","M�rz","April","Mai","Juni","Juli","August","September","Oktober","November","Dezember"};
char prstrans[512];

//**************************************** Preview Counter ********************************

extern char CONVERT_LIST[];
extern void TrimString(char *strg);

void prs_check_missing(char *entry)
{
char rstr[512];
int found=0;
FILE *fh;

	if((fh=fopen(MISS_FILE,"r"))!=NULL)
	{
		while(!feof(fh)&&!found)
		{
			if(fgets(rstr,500,fh))
			{
				TrimString(rstr);
				if(!strcmp(rstr,entry))
				{
					found=1;
				}
			}
		}
		fclose(fh);
	}
	if(!found)
	{
		if((fh=fopen(MISS_FILE,"a"))!=NULL)
		{
			fprintf(fh,"%s\n",entry);
			fclose(fh);
		}
	}
}

char  *prs_translate(char *trans, char *tfile)
{
char *sptr;
int i,found=0;
FILE *fh;

	if((fh=fopen(tfile,"r"))!=NULL)
	{
		while(!found && fgets(prstrans,511,fh))
		{
			TrimString(prstrans);
			if(strstr(prstrans,trans)==prstrans)
			{
				sptr=prstrans+strlen(trans);
				if(*sptr=='|')
				{
					++sptr;
					i=strlen(sptr);
					memmove(prstrans,sptr,i+1);
					found=1;
				}
			}
		}
		fclose(fh);
	}
	if(found && strlen(prstrans))
	{	
		if(!strcmp(prstrans,"---"))
		{
			*prstrans=0;
		}
		return prstrans;
	}
	return trans;
}

int prs_get_prev_count (void)
{
	return prev_count;
}

int prs_get_day (int i, char *out, int metric)
{
	int ret=1, set=(PRE_DAY+(i*PRE_STEP)), z=0, intdaynum=0, monthtemp=0;
	char day[15], tstr[128];
	char *pt1, *pt2;

	*out=0;
	if((pt1=strstr(data[set],"T=\""))!=NULL)
	{
		pt1+=3;
		if((pt2=strstr(pt1,"\""))!=NULL)
		{
			strncpy(day,pt1,pt2-pt1);
			day[pt2-pt1]=0;
		
			for(z=0;z<=tc;z++)
			{
				if (strcasecmp(day,conveng[z])==0) strcpy (day,convger[z]);
			}
			
			pt2++;
			if((pt1=strstr(pt2,"DT=\""))!=NULL)
			{
				pt1+=4;
				if((pt2=strstr(pt1," "))!=NULL)
				{
					pt2++;
					if(sscanf(pt2,"%d",&intdaynum)==1)
					{
						monthtemp=t_actmonth;
						if (intdaynum < t_actday) 
						{
							if((++monthtemp)>12)
							{
							monthtemp =1;
							}
						}
						sprintf(tstr,"%s",prs_translate((char*)day,CONVERT_LIST));
						if(metric)
						{
							sprintf (out,"%s,  %02d. %s", tstr, intdaynum, prs_translate((char*)mnames[monthtemp-1],CONVERT_LIST));
						}
						else
						{
							sprintf (out,"%s, %s %02d. ", tstr, prs_translate((char*)mnames[monthtemp-1],CONVERT_LIST),intdaynum);
						}
						ret=0;
					}
				}
			}
		}
	}
return ret;
}

int prs_get_val (int i, int what, int nacht, char *out)
{
int z;

	strcpy(out,data[(what & ~TRANSLATION)+(i*PRE_STEP)+(nacht*NIGHT_STEP)]);
	if(what & TRANSLATION)
	{
		for(z=0;z<=tc;z++)
		{
			if (strcasecmp(out,conveng[z])==0)
			{
				strcpy (out,convger[z]);
				return 0;
			}
		}
		if(sscanf(out,"%d",&z)!=1)
		{
			prs_check_missing(out);
		}
	}
	return (strlen(out)==0);
}

int prs_get_dbl (int i, int what, int nacht, char *out)
{
int ret=1;
double tv;
	
	*out=0;
	if(sscanf(data[(what & ~TRANSLATION)+(i*PRE_STEP)+(nacht*NIGHT_STEP)], "%lf", &tv)==1)
	{
		sprintf(out, "%05.2lf", tv);
		ret=0;
	}
	return ret;
}

int prs_get_time(int i, int what, char *out, int metric)
{
int hh,mm,ret=1;

	*out=0;
	if(sscanf(data[(what & ~TRANSLATION)+(i*PRE_STEP)],"%d:%d",&hh,&mm)==2)
	{
		if(metric)
		{
			if(hh<12)
			{
				if(strstr(data[(what & ~TRANSLATION)+(i*PRE_STEP)],"PM")!=NULL)
				{
					hh+=12;
				}
			}
			else
			{
				if(strstr(data[(what & ~TRANSLATION)+(i*PRE_STEP)],"AM")!=NULL)
				{
					hh=0;
				}
			}
			sprintf(out,"%02d:%02d",hh,mm);
		}
		else
		{
			sprintf(out,"%02d:%02d %s",hh,mm,(strstr(data[(what & ~TRANSLATION)+(i*PRE_STEP)],"PM")!=NULL)?"PM":"AM");
		}
		ret=0;
	}
	return ret;
}

int prs_get_dtime(int i, int what, char *out, int metric)
{
int hh,mm,ret=1;

	*out=0;
	if(sscanf(data[(what & ~TRANSLATION)+(i*PRE_STEP)],"%d/%d/%d %d:%d",&t_actmonth,&t_actday,&t_actyear,&hh,&mm)==5)
	{
		if(metric)
		{
			if((hh<12)&&(strstr(data[(what & ~TRANSLATION)+(i*PRE_STEP)],"PM")!=NULL))
			{
				hh+=12;
			}
			sprintf(out,"%02d.%02d.%04d %02d:%02d",t_actday,t_actmonth,t_actyear+2000,hh,mm);
		}
		else
		{
			sprintf(out,"%04d/%02d/%02d %02d:%02d %s",t_actyear+2000,t_actmonth,t_actday,hh,mm,(strstr(data[(what & ~TRANSLATION)+(i*PRE_STEP)],"PM")!=NULL)?"PM":"AM");
		}
		ret=0;
	}
	return ret;
}

//**************************************** Parser ****************************************

//*** XML File ***

int parser(char *citycode, char *trans, int metric, int inet, int ctmo)
{
	int  rec=0, flag=0;
	int cc=0, bc=1, day_data=PRE_DAY, exit_ind=-1;
	char gettemp;
	FILE *wxfile=NULL;
	char url[200];
	char debug[505];
	int previews=9;
	extern char par[], key[];

	memset(data,0,1000*50);
	memset(conveng,0,500*40); 
	memset(convger,0,500*40);
	prev_count=0;
	memset(null,0,2);
	tc=0;
	t_actday=0;
	t_actmonth=0;
	
/*	sprintf (url,"http://xoap.weather.com/weather/local/%s?cc=*&dayf=%d&prod=xoap&unit=%c&par=1005530704&key=a9c95f7636ad307b",citycode,previews,(metric)?'m':'u');
	exit_ind=HTTP_downloadFile(url, "/tmp/tuxwettr.tmp", 0, inet, ctmo, 3);
*/
	sprintf (url,"wget -q -O /tmp/tuxwettr.tmp http://xoap.weather.com/weather/local/%s?unit=%c\\&dayf=%d\\&cc=*\\&prod=xoap\\&link=xoap\\&par=%s\\&key=%s",citycode,(metric)?'m':'u',previews,par,key);
	exit_ind=system(url);
	sleep(1);
	if(exit_ind != 0)
	{
		printf("Tuxwetter <Download data from server failed. Errorcode: %d>\n",exit_ind);
		exit_ind=-1;
		return exit_ind;
	}
	exit_ind=-1;
	system("sed -i /'prmo'/,/'\\/lnks'/d /tmp/tuxwettr.tmp");
	if ((wxfile = fopen("/tmp/tuxwettr.tmp","r"))==NULL)
	{
		printf("Tuxwetter <Missing tuxwettr.tmp File>\n");
		return exit_ind;
	}
	else
	{
	bc=1;
		fgets(debug,500,wxfile);
//		printf("%s",debug);
		fgets(debug,5,wxfile);
//		printf("%s",debug);
		if((debug[0] != 60)||(debug[1] != 33)||(debug[2] != 45)||(debug[3] != 45))
		{
			fclose(wxfile);
			return exit_ind;
		}
		else {
		fclose(wxfile);
		wxfile = fopen("/tmp/tuxwettr.tmp","r");
		while (!feof(wxfile))
		{
			gettemp=fgetc(wxfile);
			if ((gettemp >=97) && (gettemp <=122)) gettemp = gettemp -32;
			if (gettemp == 13) gettemp=0; 
			if (bc == day_data)
			{
				
				if (gettemp == 62) 
				{
					rec = 0;
				}
				if (rec == 1)
				{
					data[bc][cc] = gettemp;
					cc++;
				}
				if (gettemp == 60) rec = 1;
				if (gettemp == 13) data[bc][cc+1] =0;
				if (gettemp == 10) 
				{
					bc++;
					cc = 0;
					rec = 0;
					flag=1;
					prev_count++;
				}
			}
			else
			{
				if (gettemp == 60) rec = 0;
				if (rec == 1)
				{
					data[bc][cc] = gettemp;
					cc++;
				}
				if (gettemp == 62) rec = 1;
				if (gettemp == 13) data[bc][cc] =0;
				if (gettemp == 10) 
				{
					bc++;
					cc = 0;
					rec = 0;
				}
			}
			if ((flag==1) && (gettemp == 0))
			{
				day_data=day_data+PRE_STEP;
				flag=0;
			}
		}
		}
		fclose(wxfile);
	}
	if (prev_count > 0) prev_count=prev_count-1;
	if (prev_count > 0) prev_count=prev_count-1;
	cc=0;

	exit_ind=1;
	
//*** �bersetzungs File ***
	
	if ((wxfile = fopen(trans,"r"))==NULL)
	{
		printf("Tuxwetter <File %s not found.>\n",trans);
		return exit_ind;
	}
	else
	{
		while (!feof(wxfile))
		{
			gettemp=fgetc(wxfile);
			if (gettemp == 10)
			{
				cc=0;
				tc++;
				flag=0;
			}
			else
			{
				if (gettemp == 124)
				{
					cc=0;
					flag=2;
				}
				if (gettemp == 13) gettemp = 0;
				if (flag==0) 
				{
					if ((gettemp >=97) && (gettemp <=122)) gettemp = gettemp -32;
					conveng[tc][cc]=gettemp;
				}
				if (flag==1) convger[tc][cc]=gettemp;
				cc++;
				if (flag == 2) 
				{
					flag--;
					cc=0;
				}
			}
		}
		fclose(wxfile);
	}
	prs_get_dtime (0, ACT_UPTIME, debug, metric);
return 0;
}


