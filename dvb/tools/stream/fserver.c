/*
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

program: fserver by Axel Buehning <mail at diemade.de>

$Id: fserver.c,v 1.2 2004/04/26 10:02:11 diemade Exp $

*/

#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "fserver.h"

#define BUFLEN 10240
#define LISTENPORT 4000

int AnalyzeXMLRequest(char *szXML, RecordingData   *rdata);

void clean_exit(int signal);

int pid = 0;
	
int main(int argc, char * argv[])
{
	RecordingData recdata;
	struct sockaddr_in  servaddr;
	int ListenSocket, ConnectionSocket;
	u_short Port = LISTENPORT;
	char buf[BUFLEN];
	char * p_act;
	int rc;
	time_t t;
	char * a_arg[50];
	char a_grabname[256];
	char a_vpid[20];
	char a_apid[20];
	char a_filename[256];
	char a_path[256]="";
	char a_host[256];
	int	i,n;
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);

	printf("[fserver.c] fserver version $Id: fserver.c,v 1.2 2004/04/26 10:02:11 diemade Exp $\n");
	
	// set signal handler for clean termination
	signal(SIGTERM, clean_exit);
	
	a_arg[0] = a_grabname;
	a_arg[1] = "-s";
	a_arg[2] = a_filename;
	a_arg[3] = a_vpid;
	a_arg[4] = a_apid;
	n = 5;

	strcpy (a_grabname,argv[0]);
	if (strrchr(a_grabname,'/')){
		strcpy (strrchr(a_grabname,'/') + 1, "streamfile");
	}
	else {
		strcpy(a_grabname,"streamfile");
	}
	
	for (i = 1; i < argc; i++) {
		if (!strcmp("-sport",argv[i])) {
			i++; if (i >= argc) { fprintf(stderr, "[fserver.c] need port for -sport\n"); return -1; }
			Port = atoi(argv[i]);
		}
		else if (!strcmp("-o",argv[i])) {
			i++; if (i >= argc) { fprintf(stderr, "[fserver.c] need path for -o\n"); return -1; }
			strcpy (a_path, argv[i]);
		}
		else {
			a_arg[n++]=argv[i];
		}
	}
	a_arg[n] = 0;

	//network-setup
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(Port);

	i = 0;
	while ((rc = bind(ListenSocket, (struct sockaddr_in *)&servaddr, sizeof(struct sockaddr_in))))
	{
		fprintf(stderr, "[fserver.c] bind to port %d failed, RC=%d...\n",Port, rc);
		if (i == 10) {
			fprintf(stderr, "[fserver.c] Giving up\n");
			exit(1);
		}
		fprintf(stderr, "[fserver.c] %d. try, wait for 2 s\n",i++);
		sleep(2);
	}

	if (listen(ListenSocket, 5))
	{
		fprintf(stderr,"[fserver.c] listen failed\n");
		exit(1);
	}
	printf("[fserver.c] fserver successfully started\n");

	do
	{
		memset(&cliaddr, 0, sizeof(cliaddr));
		if((ConnectionSocket = accept(ListenSocket, (struct sockaddr *) &cliaddr, (socklen_t *) &clilen)) == -1){
			fprintf(stderr,"[fserver.c] accept failed\n");
			exit(1);
		}
		strcpy(a_host,inet_ntoa(cliaddr.sin_addr));
		fprintf(stderr,"[fserver.c] got request from dbox ip :%s\n",a_host);

		do
		{
			rc = recv(ConnectionSocket, buf, BUFLEN, 0);
			if ((rc > 0))
			{
				memset((void *)&recdata, 0, sizeof(recdata));
				AnalyzeXMLRequest(buf, &recdata);

				switch (recdata.cmd) 
				{
					case CMD_VCR_UNKNOWN:
						fprintf(stderr, "[fserver.c] VCR_UNKNOWN NOT HANDLED\n");
						break;
					case CMD_VCR_RECORD:
						fprintf(stderr, "[fserver.c] ********************** START RECORDING **********************\n");
						fprintf(stderr, "[fserver.c] APID		: %x\n", recdata.apid);
						fprintf(stderr, "[fserver.c] VPID		: %x\n", recdata.vpid);
						fprintf(stderr, "[fserver.c] CHANNELNAME : %s\n", recdata.channelname);
						fprintf(stderr, "[fserver.c] EPG TITLE   : %s\n", recdata.epgtitle);
						fprintf(stderr, "[fserver.c] ***********************************************************\n");
						sprintf(a_vpid,"%03x",recdata.vpid);	
						sprintf(a_apid,"%03x",recdata.apid);

						// Create filename for recording
						strcpy (a_filename,a_path);
							
						if (strlen(a_filename)) {
							strcat(a_filename,"/");
						}

						if (strlen(recdata.channelname) > 0)
						{
							p_act = recdata.channelname;
							do {
								p_act +=  strcspn(p_act, "/ \"%&-\t`'´!,:;");
								if (*p_act) {
									*p_act++ = '_';
								}
							} while (*p_act);
								
							strcat(a_filename, recdata.channelname);
							strcat(a_filename, "_");
						}

						if (strlen(recdata.epgtitle) > 0)
						{
							p_act = recdata.epgtitle;
							do {
								p_act +=  strcspn(p_act, "/ \"%&-\t`'~<>!,:;?^°$\\=*#@¤|µöäüÖÄÜß");
								if (*p_act) {
									*p_act++ = '_';
								}
							} while (*p_act);

							p_act = recdata.epgtitle;
							do {
								if ((unsigned char) (*p_act) > 128) {
									*p_act = '_';
								}
							} while (*p_act++);
							
							strcat(a_filename, recdata.epgtitle);
							strcat(a_filename, "_");
						}

						t = time (&t);
						strftime (buf, sizeof(a_filename)-1, "%Y%m%d_%H%M%S", localtime(&t));
						strcat(a_filename, buf);

						// start streaming process
						pid = fork();
						if (pid == -1) {
							fprintf(stderr, "[fserver.c] fork process failed\n");
							break;
						}
						if (pid == 0) {
							execvp(a_arg[0], a_arg);
							fprintf(stderr,"[fserver.c] execv of %s failed", a_arg[0]);
							perror("");
						}
						break;
					case CMD_VCR_STOP:
						if (pid > 0) {
							if(kill(pid,SIGTERM)) {
								printf ("[fserver.c] streamfile process not killed\n");
							}
							waitpid(pid,0,0);
							fprintf(stderr,"\n[fserver.c] Stop recording\n");
						}
						break;
					case CMD_VCR_PAUSE:
						fprintf(stderr, "[fserver.c] VCR_PAUSE NOT HANDLED\n");
						break;
					case CMD_VCR_RESUME:
						fprintf(stderr, "[fserver.c] VCR_RESUME NOT HANDLED\n");
						break;
					case CMD_VCR_AVAILABLE:
						fprintf(stderr, "[fserver.c] VCR_AVAIABLE NOT HANDLED\n");
						break;
					default:
						fprintf(stderr, "[fserver.c] unknown VCR command\n");
						break;
				}
			}
		} while((rc > 0));
	} while (1);

	return 0;
}

void clean_exit(int signal)
{
	fprintf(stderr,"[fserver.c] Received signal. Terminating.\n");
	if (pid > 0) {
		if(kill(pid,SIGTERM)) {
			printf ("[fserver.c] streamfile process not killed\n");
		}
		waitpid(pid,0,0);
		fprintf(stderr,"[fserver.c] Stopped recording\n");
	}
	fprintf(stderr,"[fserver.c] Received signal. Terminated.\n");
	exit(0);
}

/* Shameless stolen from TuxVision */
char* ParseForString(char *szStr, char *szSearch, int ptrToEnd)
{
	char *p=NULL;
	p=strstr(szStr, szSearch);
	if (p==NULL)
		return(NULL);
	if (!ptrToEnd)
		return(p);
	else
		return(p+strlen(szSearch));
	
}

char* ExtractQuotedString(char *szStr, char *szResult, int ptrToEnd)
{
	int i=0;
	int len=strlen(szStr);
	strcpy(szResult,"");
	for(i=0;i<len;i++)
		{
		if (szStr[i]=='\"')
			{
			char *pe=NULL;
			char *ps=szStr+i+1;	  
			pe=ParseForString(ps, "\"", 1);
			if (pe==NULL)
				return(NULL);

			memcpy(szResult,ps, pe-ps-1);
			szResult[pe-ps-1]=0;					   
			if (ptrToEnd)
				return(pe);
			else
				return(ps);
			}
		}
	return(NULL);
}

int AnalyzeXMLRequest(char *szXML, RecordingData   *rdata)
{
	char *p1=NULL;
	char *p2=NULL;
	char *p3=NULL;
	char szcommand[264]="";
	char szapid[264]="";
	char szvpid[264]="";
	char szmode[3]="";
	char szchannelname[264]="";
	char szepgtitle[264]="";
	int hr=0;

	if ( (szXML==NULL) || (rdata==NULL) )
		return(0);

	rdata->apid=0;
	rdata->vpid=0;
	rdata->cmd=CMD_VCR_UNKNOWN;
	strcpy(rdata->channelname,"");

	p1=ParseForString(szXML,"<record ", 1);
	if (p1!=NULL)
		{
		p2=ParseForString(p1,"command=", 1);
		p1=NULL;
		if (p2!=NULL)
			p3=ExtractQuotedString(p2, szcommand, 1);
		if (p3!=NULL)
			p1=ParseForString(p3,">", 1);
		}

	if (p1!=NULL)
		{
		p2=ParseForString(p1,"<channelname>", 1);
		p3=p2;
		p1=NULL;
		if (p2!=NULL)
			{
			p1=ParseForString(p2,"</channelname>", 0);
			if (p1!=NULL)
				{
				memcpy(szchannelname,p3,p1-p3);
				szchannelname[p1-p3]=0;
				hr=1;
				}
			}
		}
	if (p1!=NULL)
		{
		p2=ParseForString(p1,"<epgtitle>", 1);
		p3=p2;
		p1=NULL;
		if (p2!=NULL)
			{
			p1=ParseForString(p2,"</epgtitle>", 0);
			if (p1!=NULL)
				{
				memcpy(szepgtitle,p3,p1-p3);
				szepgtitle[p1-p3]=0;
				hr=1;
				}
			}
		}
	if (p1!=NULL)
		{
		p2=ParseForString(p1,"<mode>", 1);
		p3=p2;
		p1=NULL;
		if (p2!=NULL)
			{
			p1=ParseForString(p2,"</mode>", 0);
			if (p1!=NULL)
				{
				memcpy(szmode,p3,p1-p3);
				szmode[p1-p3]=0;
				hr=1;
				}
			}
		}
	
	p2=ParseForString(szXML,"<videopid>", 1);
	if (p2!=NULL)
		{
		p3=p2;
		p1=NULL;
		p1=ParseForString(p2,"</videopid>", 0);
		if (p1!=NULL)
			{
			memcpy(szvpid,p3,p1-p3);
			szvpid[p1-p3]=0;
			hr=1;
			}
		}

	p2=ParseForString(szXML,"<audiopids selected=", 1);
	if (p2!=NULL)
		{
		p3=ExtractQuotedString(p2, szapid, 1);
		if (p3!=NULL)
			p1=ParseForString(p3,">", 1);
		}

	if (!hr)
		return(hr);

	strcpy(rdata->channelname, szchannelname);
	strcpy(rdata->epgtitle, szepgtitle);

	if (strlen(szvpid)>0)
		rdata->vpid=atoi(szvpid);

	if (strlen(szapid)>0)
		rdata->apid=atoi(szapid);

	if (!strcmp(szcommand,"record"))
		rdata->cmd=CMD_VCR_RECORD;
	if (!strcmp(szcommand,"stop"))
		rdata->cmd=CMD_VCR_STOP;
	if (!strcmp(szcommand,"pause"))
		rdata->cmd=CMD_VCR_PAUSE;
	if (!strcmp(szcommand,"resume"))
		rdata->cmd=CMD_VCR_RESUME;
	if (!strcmp(szcommand,"available"))
		rdata->cmd=CMD_VCR_AVAILABLE;

	return(hr);
}
