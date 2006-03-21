/******************************************************************************
 *                        <<< Keyboard 2 RemoteControl daemon >>>
 *                (c) Robert "robspr1" Spreitzer 2006 (robert.spreitzer@inode.at)
 *  
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *-----------------------------------------------------------------------------
 * $Log: kb2rcd.c,v $
 * Revision 0.16  2006/03/21 19:08:54  robspr1
 * - add time-delayed keys
 *
 * Revision 0.15  2006/03/15 22:09:56  robspr1
 * - start plugins with SCRIPTxx=Plugin:myplugin
 *
 * Revision 0.14  2006/03/09 18:50:01  robspr1
 * - add scripts
 *
 * Revision 0.13  2006/03/07 19:58:24  robspr1
 * - add timeout between keys
 *
 * Revision 0.12  2006/03/07 15:56:53  robspr1
 * - fix copy'n'paste error for MAXMOUSE
 *
 * Revision 0.11  2006/03/06 21:09:46  robspr1
 * - change to kb2rcd.conf and change mouse behaviour
 *
 * Revision 0.10  2006/03/05 22:39:03  robspr1
 * - add to cvs
 *
 * Revision 0.9  2006/03/05 22:50:00  robspr1
 * - change mouse-cursor behavior
 * - add debug-output switch
 *
 * Revision 0.8  2006/03/05 17:50:00  robspr1
 * - add delay between keystrokes
 *
 * Revision 0.7  2006/03/05 16:30:00  robspr1
 * - add debug_output with key_names
 *
 * Revision 0.6  2006/03/05 16:00:00  robspr1
 * - add config for mouse-cursor
 * - release button before sending a new button
 *
 * Revision 0.5  2006/03/05 15:00:00  robspr1
 * - add mouse-cursor
 *
 * Revision 0.4  2006/03/05 12:00:00  robspr1
 * - add lock-file for conversion
 *
 * Revision 0.3  2006/03/05 10:00:00  robspr1
 * - default .conf file is generated
 *
 * Revision 0.2  2006/03/04 22:00:00  robspr1
 * - change rc-read from NON-BLOCKING to BLOCKING
 * - fix writing ALT_ and SHIFT_ keys
 * - fix signal -HUP
 *
 * Revision 0.1  2006/03/04 20:00:00  robspr1
 * - first version
 *
 ******************************************************************************/

#include "kb2rcd.h"

/******************************************************************************
 * FindKeyName
 ******************************************************************************/
/*!
 * check the given key_name

 \param			: char* the key_name
 \return 		: the index in the keyname-table, -1 if not found
*/
int FindKeyName(char* name)
{
	int i=0;

	do
	{
		if (strcmp(name,keyname[i].name)==0) break;
	} while (keyname[++i].code!=0xFFFFFFFF);
	if (keyname[i].code==0xFFFFFFFF)	i=-1;
	return i;
}

/******************************************************************************
 * FindKeyCode
 ******************************************************************************/
/*!
 * check the given key_code

 \param			: unsigned long the key_code
 \return 		: the index in the keyname-table, -1 if not found
*/
int FindKeyCode(unsigned long code)
{
	int i=0;

	do
	{
		if (keyname[i].code==code) break;
	} while (keyname[++i].code!=0xFFFFFFFF);
	if (keyname[i].code==0xFFFFFFFF)	i=-1;
	return i;
}

/******************************************************************************
 * ReadConf
 ******************************************************************************/
/*!
 * read configuration-file 

 \param			: none
 \return 		: none
*/
void ReadConf()
{
	FILE *fd_conf;
	char* p1;
	char* p2;
	char linebuffer[256];
	unsigned long rccode;
	int i, j;
	iCount=0;
	
	memset(keyconv,0,sizeof(keyconv));
	memset(szScripts,0,sizeof(szScripts));
	
	// open config-file
	if (!(fd_conf = fopen(CFGPATH CFGFILE, "r")))
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO,"Config not found, using defaults") : printf("kb2rcD <Config not found, using defaults>\n");
		keyconv[0].in_code=KEY_MINUS; keyconv[0].out_code[0]=KEY_HELP; 
		keyconv[1].in_code=KEY_ESC; keyconv[1].out_code[0]=KEY_HOME; 
		keyconv[2].in_code=KEY_F1; keyconv[2].out_code[0]=KEY_RED; 
		keyconv[3].in_code=KEY_F2; keyconv[3].out_code[0]=KEY_GREEN; 
		keyconv[4].in_code=KEY_F3; keyconv[4].out_code[0]=KEY_YELLOW; 
		keyconv[5].in_code=KEY_F4; keyconv[5].out_code[0]=KEY_BLUE; 
		keyconv[6].in_code=BTN_LEFT; keyconv[6].out_code[0]=KEY_POWER; 
		keyconv[7].in_code=BTN_RIGHT; keyconv[7].out_code[0]=KEY_OK; 
		keyconv[8].in_code=KEY_102ND; keyconv[8].out_code[0]=KEY_VOLUMEDOWN; 
		keyconv[9].in_code=KEY_GRAVE; keyconv[9].out_code[0]=KEY_VOLUMEUP; 
		keyconv[10].in_code=KEY_PAUSE; keyconv[10].out_code[0]=KEY_MUTE; 
		keyconv[11].in_code=KEY_DELETE; keyconv[11].out_code[0]=KEY_SETUP; 
		iCount=12;
		return;
	}

	// read config-file line-by-line
	while(fgets(linebuffer, sizeof(linebuffer), fd_conf))
	{
		if ((p1 = strstr(linebuffer, "MOUSECNT=")))
		{
			sscanf(p1 + 9, "%d", &iMouseCnt);
			continue;
		}
		else if ((p1 = strstr(linebuffer, "MINMOUSE=")))
		{
			sscanf(p1 + 9, "%d", &iMinMouse);
			continue;
		}
		else if ((p1 = strstr(linebuffer, "MAXMOUSE=")))
		{
			sscanf(p1 + 9, "%d", &iMaxMouse);
			continue;
		}
		else if ((p1 = strstr(linebuffer, "DELAY=")))
		{
			sscanf(p1 + 6, "%d", &iDelay);
			continue;
		}
		else if ((p1 = strstr(linebuffer, "SMARTDELY=")))
		{
			sscanf(p1 + 10, "%d", &iSmartDelay);
			continue;
		}
		else if ((p1 = strstr(linebuffer, "INVERSE=")))
		{
			sscanf(p1 + 8, "%d", &iInverse);
			continue;
		}
		else if((p1 = strstr(linebuffer, "WEBPORT=")))
		{
			sscanf(p1 + 8, "%d", &webport);
			continue;
		}
		else if((p1 = strstr(linebuffer, "WEBPASS=")))
		{
			sscanf(p1 + 8, "%s", &webpass[0]);
			continue;
		}
		else if((p1 = strstr(linebuffer, "WEBUSER=")))
		{
			sscanf(p1 + 8, "%s", &webuser[0]);
			continue;
		}
		else if ((p1 = strstr(linebuffer, "SCRIPT")) && (*(p1+8) == '='))		// scan for scripts
		{
			int iIdx=-1;
			char index = *(p1+7);
			if((index >= '0') && (index <= '9')) iIdx=index-'0';
			index = *(p1+6);
			if((index >= '0') && (index <= '9')) iIdx+=(index-'0')*10;
			{
				p2=strchr(p1,'\n');					// remove carrige-return 
				if (p2)	*p2=0;
				sscanf(p1 + 9, "%s", szScripts[iIdx-1]);
			}
			continue;			
		}
		
		p1=linebuffer;
		rccode=0;
			
		// first check for type (ALT_, SHIFT_ or KEY_) or T1_ to T4_ prefix
		if (strncmp(p1,"ALT_",4)==0)
		{
			rccode=RC_ALT;
			p1+=4;	
		}
		else if (strncmp(p1,"SHIFT_",6)==0)
		{
			rccode=RC_SHIFT;
			p1+=6;	
		}
		else if (strncmp(p1,"T1_",3)==0)
		{
			rccode=RC_T1;
			p1+=3;	
		}
		else if (strncmp(p1,"T2_",3)==0)
		{
			rccode=RC_T2;
			p1+=3;	
		}
		else if (strncmp(p1,"T3_",3)==0)
		{
			rccode=RC_T3;
			p1+=3;	
		}
		else if (strncmp(p1,"T4_",3)==0)
		{
			rccode=RC_T4;
			p1+=3;	
		}

		// first we check if we know the key-name
		if ((p2=strchr(p1,'=')) != NULL)
		{
			*p2='\0';													// p1 should now be our key-name
			i = FindKeyName(p1);
			if (i!=-1)												// if we found a keyname
			{
				keyconv[iCount].in_code = (keyname[i].code | rccode);			
				i=0;														// index for out_codes

				do
				{
					p1=p2+1;		
					if ((p2=strchr(p1,';')) != NULL) *p2='\0';
					else
					{
						p2=strchr(p1,'\n');					// remove carrige-return 
						if (p2)	*p2=0;
					}
					j = FindKeyName(p1);
					if (j!=-1) keyconv[iCount].out_code[i++] = keyname[j].code;
					
				} while ((j!=0) && (p2!=NULL));
				iCount++;
			}
		}
	}

	slog ? syslog(LOG_DAEMON | LOG_INFO, "use %u conversions",iCount) : printf("kb2rcD <use %u conversions>\r\n",iCount);
	
	// close config-file
	fclose(fd_conf);
}

/******************************************************************************
 * WriteConf
 ******************************************************************************/
/*!
 * write the configuration back to the file
 
 \param			: none
 \return 		: 1:OK    0:FAILED    
*/
int WriteConf()
{
	FILE *fd_conf;
	int i,j,k;

	// open config-file
	if (!(fd_conf = fopen(CFGPATH CFGFILE , "w")))
	{
		return 0;
	}

	fprintf(fd_conf,"MOUSECNT=%d\n",iMouseCnt);
	fprintf(fd_conf,"MINMOUSE=%d\n",iMinMouse);
	fprintf(fd_conf,"MAXMOUSE=%d\n",iMaxMouse);
	fprintf(fd_conf,"DELAY=%d\n",iDelay);
	fprintf(fd_conf,"SMARTDELY=%d\n",iSmartDelay);
	fprintf(fd_conf,"INVERSE=%d\n\n",iInverse);
	fprintf(fd_conf,"WEBPORT=%d\n", webport);
	fprintf(fd_conf,"WEBUSER=%s\n", webuser);
	fprintf(fd_conf,"WEBPASS=%s\n\n", webpass);
	
	
	for (i=0; i<MAX_SCRIPTS; i++)
	{
		if (szScripts[i][0] != '\0')
		{
			fprintf(fd_conf,"SCRIPT%02u=%s\n",i+1,szScripts[i]);
		}
	}
	fprintf(fd_conf,"\n");
	
	for (i=0; i<iCount; i++)
	{
		if (keyconv[i].in_code & RC_ALT) fprintf(fd_conf, "ALT_");
		if (keyconv[i].in_code & RC_SHIFT) fprintf(fd_conf, "SHIFT_");
		if (keyconv[i].in_code & RC_T1) fprintf(fd_conf, "T1_");
		if (keyconv[i].in_code & RC_T2) fprintf(fd_conf, "T2_");
		if (keyconv[i].in_code & RC_T3) fprintf(fd_conf, "T3_");
		if (keyconv[i].in_code & RC_T4) fprintf(fd_conf, "T4_");
		j=FindKeyCode((keyconv[i].in_code & 0xFFFF));
		if (j!=-1) 
		{
			fprintf(fd_conf, "%s=",keyname[j].name);
			for (k=0;k<MAX_OUT;k++)
			{
				if (keyconv[i].out_code[k]==0) break;
				j=FindKeyCode(keyconv[i].out_code[k]);
				if (j==-1) break;
				fprintf(fd_conf, "%s;",keyname[j].name);
			}
		}
		fprintf(fd_conf, "\n");
	}
	
	fclose(fd_conf);
	return 1;
}

/******************************************************************************
 * OpenRC
 ******************************************************************************/
/*!
 * opens the connection to the remotecontrol

 \param			: none
 \return 		: 1:OK    0:FAILED    
*/
int OpenRC(void)
{
	// remote-control
	if ((rc = open("/dev/input/event0", O_RDWR))<0)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO,"open rc failed") : printf("kb2rcD <open rc failed>");
		return 0;
	}
//	fcntl(rc, F_SETFL, fcntl(rc, F_GETFL) | O_NONBLOCK);

	return 1;
}


/******************************************************************************
 * CloseRC
 ******************************************************************************/
/*!
 * closes the connection to the remotecontrol

 \param			: none
 \return 		: none
*/
void CloseRC(void)
{
	if (rc != 0)
		close(rc);
}

/******************************************************************************
 * EncodeBase64
 ******************************************************************************/
// from tuxmaild
void EncodeBase64(char *decodedstring, int decodedlen)
{
	char encodingtable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int src_index, dst_index;

	memset(encodedstring, 0, sizeof(encodedstring));
	for(src_index = dst_index = 0; src_index < decodedlen; src_index += 3, dst_index += 4)
	{
		encodedstring[0 + dst_index] = encodingtable[decodedstring[src_index] >> 2];
		encodedstring[1 + dst_index] = encodingtable[(decodedstring[src_index] & 3) << 4 | decodedstring[1 + src_index] >> 4];
		encodedstring[2 + dst_index] = encodingtable[(decodedstring[1 + src_index] & 15) << 2 | decodedstring[2 + src_index] >> 6];
		encodedstring[3 + dst_index] = encodingtable[decodedstring[2 + src_index] & 63];
	}

	if(decodedlen % 3)
	{
		switch(3 - decodedlen%3)
		{
			case 2:
				encodedstring[strlen(encodedstring) - 2] = '=';
			case 1:
			encodedstring[strlen(encodedstring) - 1] = '=';
		}
	}
}

/******************************************************************************
 * SendRCCode
 ******************************************************************************/
/*!
 * send a code to the event-buffer
 
 \param			: code: a key-code or a PAUSE
 \param			: count: 0 : RELEASE the key, else send count key-strokes, end with release
 \return 		: none
*/
int SendRCCode(unsigned int code, unsigned int count)
{
	struct input_event iev;
	char szScript[80];
	
	iev.type=EV_KEY;
	iev.code=code;

	// send release key
	if (count == 0) 
	{
		iev.value=KEY_RELEASED;
		if (iSmartDelay) usleep(iSmartDelay*1000);
		return write(rc,&iev,sizeof(iev));
	}
	
	// send one full keystroke or pause
	if (count == 1)
	{
		// do we have a code for pause
		if ((code & 0xFFFF0000) == PAUSE0)
		{
			if ((code&0xFFFF) >= 1000) sleep((code&0xFFFF)/1000);
			else usleep(1000*(code&0xFFFF));
		}
		// do we have a code for a script
		else if ((code & 0xFFFF0000) == SCRIPT00)
		{
			if (szScripts[(code&0xFFFF)-1][0]!='\0') 
			{
				strcpy(szScript,szScripts[(code&0xFFFF)-1]);
				// check if we should call a plugin
				if (strncasecmp(szScript, "Plugin:",7)==0)
				{
					int sock;																	//! socket		
					struct sockaddr_in SockAddr;
					char http_cmd[1024];
					char *http_cmd1 = "GET /cgi-bin/startPlugin?name=";
					strcpy(http_cmd, http_cmd1);
					strcat(http_cmd,&szScript[7]);
					strcat(http_cmd, " HTTP/1.1\n");
					if (debug) printf(" <Plugin:%s> ",&szScript[7]);
      		if(webuser[0])
      		{
      			strcat(http_cmd, "Authorization: Basic ");
      
      			strcpy(decodedstring, webuser);
      			strcat(decodedstring, ":");
      			strcat(decodedstring, webpass);
      			EncodeBase64(decodedstring, strlen(decodedstring));
      
      			strcat(http_cmd, encodedstring);
      			strcat(http_cmd, "\n\n");			
      		}
      		else
      		{
      			strcat(http_cmd, "\n");
      		}
					if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        	{
        		printf("\r\nkb2rcd <could not create Socket>\r\n");
        	}
        	else
        	{
        		SockAddr.sin_family = AF_INET;
        		SockAddr.sin_port = htons(webport);
        		inet_aton("127.0.0.1", &SockAddr.sin_addr);
        	
        		if (connect(sock, (struct sockaddr*)&SockAddr, sizeof(SockAddr)))
        		{
        			printf("\r\nkb2rcd <could not connect to WebServer>\r\n");
        		}
        		else
        		{
        			send(sock, http_cmd, strlen(http_cmd), 0);
        		}
        		close(sock);
        	}
				}
				else 
				{
					if (debug) printf(" <Script:%s> ",szScript);			
					system(szScript);
				}
			}
		}
		else
		{
			iev.value=KEY_PRESSED;
			if (iSmartDelay) usleep(iSmartDelay*1000);
			write(rc,&iev,sizeof(iev));
			iev.value=KEY_RELEASED;
			if (iSmartDelay) usleep(iSmartDelay*1000);
			write(rc,&iev,sizeof(iev));
		}
		return 1;
	}
	
	// send some autorepeat keys
	iev.value=KEY_PRESSED;
	if (iSmartDelay) usleep(iSmartDelay*1000);
	write(rc,&iev,sizeof(iev));
	iev.value=KEY_AUTOREPEAT;
	while (--count)
	{
		if (iSmartDelay) usleep(iSmartDelay*1000);
		write(rc,&iev,sizeof(iev));
	}
	iev.value=KEY_RELEASED;
	if (iSmartDelay) usleep(iSmartDelay*1000);
	return write(rc,&iev,sizeof(iev));
}

/******************************************************************************
 * GetRCCode
 ******************************************************************************/
/*!
 * read the rc-codes and convert
 
 \param			: none
 \return 		: none
*/
void GetRCCode()
{

	static unsigned long rc_last_code = KEY_RESERVED;
	static unsigned long last_time = 0;								
	static signed long cursor_h = 0;
	static signed long cursor_v = 0;
	static int cnt_h = 0;
	static int cnt_v = 0;
	
	int i;
	unsigned long	kbcode;
	char* pName;
	unsigned long act_time;								

	pName=NULL;

	// we read exactly one input-event
	if (read(rc, &ev, sizeof(ev)) == sizeof(ev))
	{
		if (ev.type==EV_KEY)
		{
			int j;
			j=FindKeyCode(ev.code);
			if (j!=-1) pName=keyname[j].name;
		}
		
		if (debug) printf("kb2rcd: s:%lu u:%lu t:%x c:%x (%s) v:%d\r\n",ev.time.tv_sec,ev.time.tv_usec,ev.type, ev.code, pName, ev.value);
		// if key is released
		if (ev.value==0) last_time=0;
		act_time = ev.time.tv_usec + (ev.time.tv_sec % 10)*1000000;
		if ((last_time) && (act_time<last_time)) act_time += 10000000;
		
		// do we have a valid input?
		
		// relativ-event, cursor
		if ((ev.type==EV_REL) && (abs(ev.value)>=iMinMouse))
		{
			if (iMouseCnt)
			{
  			if (ev.code == 0)										// horizontal axes
  			{
  				// change direction
  				if (((ev.value > 0) && (cnt_h < 0)) || ((ev.value < 0) && (cnt_h > 0)))
  				 cnt_h = 0;
  				else
  				{
  					if (ev.value > 0) 
						{
  						if (++cnt_h >= iMouseCnt)
  						{
  							SendRCCode(KEY_RIGHT,1);
  							cnt_h=0;
  						}
  					}
  					else
  					{
  						if (--cnt_h <= -iMouseCnt)
  						{
  							SendRCCode(KEY_LEFT,1);
  							cnt_h=0;
  						}
  					}					
  				}
  			}
  			else																// vertical axes
  			{
  				// change direction
  				if (((ev.value > 0) && (cnt_v < 0)) || ((ev.value < 0) && (cnt_v > 0)))
  				 cnt_v = 0;
  				else
  				{
  					if (ev.value > 0) 
						{
  						if (++cnt_v >= iMouseCnt)
  						{
  							SendRCCode(KEY_DOWN,1);
  							cnt_v=0;
  						}
  					}
  					else
  					{
  						if (--cnt_v <= -iMouseCnt)
  						{
  							SendRCCode(KEY_UP,1);
  							cnt_v=0;
  						}
  					}					
  				}
  			}		
			}
			else			// not using mouse-counts, use relative value
			{
  			if (ev.code == 0)										// horizontal axes
  			{
  				cursor_h += ev.value;
  				if (cursor_h > iMaxMouse)
  					SendRCCode(KEY_RIGHT,cursor_h/iMaxMouse );
  				else if (cursor_h < -iMaxMouse)
  					SendRCCode(KEY_LEFT,cursor_h/(-iMaxMouse));
  				cursor_h %= iMaxMouse;
  			}
  			else																// vertical axes
  			{
  				cursor_v += ev.value;
  				if (cursor_v > iMaxMouse)
  					SendRCCode(KEY_DOWN,cursor_v/iMaxMouse);
  				else if (cursor_v < -iMaxMouse)
  					SendRCCode(KEY_UP,cursor_v/(-iMaxMouse));
  				cursor_v %= iMaxMouse;
  			}
			}
			if (debug) printf("kb2rcd: cu_h:%ld cu_v:%ld c_h:%d c_v:%d\r\n",cursor_h,cursor_v,cnt_h,cnt_v);
		}
	
		if ((((iInverse) && (ev.value==0)) || ((iInverse==0) && (ev.value))) && (ev.type==EV_KEY))
		{
			kbcode = ev.code;
							
			// remember time the key is pressed
			if ((ev.value==1) && (last_time==0))
			{
				last_time = ev.time.tv_usec + (ev.time.tv_sec % 10)*1000000;
			}
	
			if (rc_last_code == RC_ALT) kbcode |= RC_ALT;
			if (rc_last_code == RC_SHIFT) kbcode |= RC_SHIFT;
			// check for minimum_pressed_time
			if (last_time)
			{
				unsigned long diff=(act_time - last_time) / 1000000;
				if (diff == 1) kbcode |= RC_T1;
				if (diff == 2) kbcode |= RC_T2;
				if (diff == 3) kbcode |= RC_T3;
				if (diff == 4) kbcode |= RC_T4;
			}
			
			FILE* pipe;
			pipe=fopen(LCKFILE,"r");
			if (pipe!=NULL)
			{
				fclose(pipe);
			}
			
			if ((active) && (pipe==NULL))
			{
  			for (i=0;i<iCount;i++)
  			{
  				if (keyconv[i].in_code == kbcode)
  				{
 						if (iInverse==0) SendRCCode(keyconv[i].in_code,KEY_RELEASED);
  					if (debug) printf("kb2rcd: convert %lx to",keyconv[i].in_code);
  					int j;
  					for (j=0;j<MAX_OUT;j++)
  					{
  						if (keyconv[i].out_code[j]==0) break;
  						usleep(((unsigned long)iDelay)*1000);
  						SendRCCode(keyconv[i].out_code[j],1);
  						if (debug) printf(" %lx",keyconv[i].out_code[j]);
  					}
  					if (debug) printf("\r\n");
  					break;
  				}
  			}
			}
			
			if ((ev.code == KEY_LEFTALT) || (ev.code == KEY_RIGHTALT)) rc_last_code = RC_ALT;
			else if ((ev.code == KEY_LEFTSHIFT) || (ev.code == KEY_RIGHTSHIFT)) rc_last_code = RC_SHIFT;
			else rc_last_code = ev.code;
			
/*
			{  						
				FILE* pipe;  	
				if ((pipe = fopen("/tmp/keys.log", "a"))!=NULL)
				{
					int i=0;
					do
					{
						if (keyname[i].code==ev.code)
						{
							fprintf(pipe,"key: %s\r\n",keyname[i].name);
							break;
						}
					} while (keyname[++i].code!=0xFFFFFFFF);
					if (keyname[i].code==0xFFFFFFFF) fprintf(pipe,"key: %04x\r\n",ev.code);
					fclose(pipe);
				}
			}
*/
		}
		else		// no valid key -> ignore and invalidate last pressed key
		{
			rc_last_code = KEY_RESERVED;
		}
//		usleep(1000000/100);
	}		
}

/******************************************************************************
 * SigHandler
 ******************************************************************************/
void SigHandler(int signal)
{
	switch(signal)
	{
		case SIGTERM:
			slog ? syslog(LOG_DAEMON | LOG_INFO,"shutdown") : printf("kb2rcD <shutdown>\n");
			run = 0;
			SendRCCode(KEY_RIGHTALT,1);																							// send key with no effect
			break;

		case SIGHUP:
		{
			slog ? syslog(LOG_DAEMON | LOG_INFO,"update") : printf("kb2rcD <update>\n");
			ReadConf();																															// load configuration
			WriteConf();																														// write configuration
		}	break;

		case SIGUSR1:
			active = 1;
			fclose(fopen(ACTFILE, "w"));
			slog ? syslog(LOG_DAEMON | LOG_INFO,"enable kb-translation") : printf("kb2rcD <enable kb-translation>\n");
			break;
			
		case SIGUSR2:
			active = 0;
			unlink(ACTFILE);
			slog ? syslog(LOG_DAEMON | LOG_INFO,"disable kb-translation") : printf("kb2rcD <disable kb-translation>\n");
			break;
	}
}

/******************************************************************************
 * MainProgram
 ******************************************************************************/
int main(int argc, char **argv)
{
	char cvs_revision[] = "$Revision: 0.16 $";
	int param = 0;
	
	sscanf(cvs_revision, "%*s %s", versioninfo_d);

	// check commandline parameter
	if(argc > 1)
	{
		for(param = 1; param < argc; param++)
		{
			if(!strcmp(argv[param], "-syslog"))
			{
				slog = 1;
				openlog("kb2rcD", LOG_ODELAY, LOG_DAEMON);
			}
			else if(!strcmp(argv[param], "-d"))
			{
				debug = 1;
			}
			else if(!strcmp(argv[param], "-show"))
			{
				int i=0;
				printf("use the following codes:\r\n");
				printf("(prefix ALT_ or SHIFT_ are possible)");
				do
				{
					if (i%4==0) printf("\r\n\t");
					printf("%s,\t",keyname[i].name);
				} while (keyname[++i].code!=0xFFFFFFFF);
				printf("\r\n");
				return 0;
			}
			else if(!strcmp(argv[param], "-v"))
			{
				printf("%s\r\n",versioninfo_d);
				return 0;
			}
			else if(!strcmp(argv[param], "-?"))
			{
				printf("kb2rcd %s << Keyboard 2 RemoteControl daemon >>\r\n",versioninfo_d);
				printf("(c) 2006 Robert [robspr1] Spreitzer\r\n");
				printf("free software, license GNU GPL V2\r\n\r\n");
				printf("kb2rcd -syslog      : log to syslog\r\n");
				printf("       -show        : show all key-codes\r\n");
				printf("       -v           : print version\r\n");
				printf("       -d           : debug outputs\r\n");
				printf("       -?           : this help\r\n");
				return 0;
			}
		}
	}

	// create daemon
	time(&tt);
	strftime(timeinfo, 22, "%d.%m.%Y - %T", localtime(&tt));

	switch(fork())
	{
		case 0:
			slog ? syslog(LOG_DAEMON | LOG_INFO, "%s started [%s]", versioninfo_d, timeinfo) : printf("kb2rcD %s started [%s]\n", versioninfo_d, timeinfo);
			setsid();
			chdir("/");
		break;

		case -1:
			slog ? syslog(LOG_DAEMON | LOG_INFO, "%s aborted!", versioninfo_d) : printf("kb2rcD %s aborted!\n", versioninfo_d);
			return -1;
		default:

		exit(0);
	}

	// check for running daemon
	if ((fd_pid = fopen(PIDFILE, "r+")))
	{
		fscanf(fd_pid, "%d", &pid);

		if(kill(pid, 0) == -1 && errno == ESRCH)
		{
			pid = getpid();
			rewind(fd_pid);
			fprintf(fd_pid, "%d", pid);
			fclose(fd_pid);
		}
		else
		{
			slog ? syslog(LOG_DAEMON | LOG_INFO, "Daemon already running with PID %d", pid) : printf("kb2rcD <Daemon already running with PID %d>\n", pid);
			fclose(fd_pid);
			return -1;
		}
	}
	else
	{
		pid = getpid();
		fd_pid = fopen(PIDFILE, "w");
		fprintf(fd_pid, "%d", pid);
		fclose(fd_pid);
	}

	// install sighandler
	if (signal(SIGTERM, SigHandler) == SIG_ERR)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Installation of Signalhandler for TERM failed") : printf("kb2rcD <Installation of Signalhandler for TERM failed>\n");
		return -1;
	}
	if (signal(SIGHUP, SigHandler) == SIG_ERR)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Installation of Signalhandler for HUP failed") : printf("kb2rcD <Installation of Signalhandler for HUP failed>\n");
		return -1;
	}

	if (signal(SIGUSR1, SigHandler) == SIG_ERR)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Installation of Signalhandler for USR1 failed") : printf("kb2rcD <Installation of Signalhandler for USR1 failed>\n");
		return -1;
	}

	if (signal(SIGUSR2, SigHandler) == SIG_ERR)
	{
		slog ? syslog(LOG_DAEMON | LOG_INFO, "Installation of Signalhandler for USR2 failed") : printf("kb2rcD <Installation of Signalhandler for USR2 failed>\n");
		return -1;
	}


	// read, update or create config
	ReadConf();																							// read actual config
	WriteConf();																						// write actual config back to file

	// remote-control
	OpenRC();
	
	// we are online now
	run=1;
	active=1;
	fclose(fopen(ACTFILE, "w"));
	
	// check events until signal to finish
	do
	{
		GetRCCode();
	}
	while (run);

	// close remote-control-addon
	CloseRC();
		
	unlink(PIDFILE);
	unlink(ACTFILE);
		
	time(&tt);
	strftime(timeinfo, 22, "%d.%m.%Y - %T", localtime(&tt));

	slog ? syslog(LOG_DAEMON | LOG_INFO, "%s closed [%s]", versioninfo_d, timeinfo) : printf("kb2rcD %s closed [%s]\n", versioninfo_d, timeinfo);

	closelog();

	return 0;
}
