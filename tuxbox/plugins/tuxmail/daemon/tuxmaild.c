/******************************************************************************
 *                       <<< TuxMailD - POP3 Daemon >>>
 *                (c) Thomas "LazyT" Loewe 2003 (LazyT@gmx.net)
 *-----------------------------------------------------------------------------
 * $Log: tuxmaild.c,v $
 * Revision 1.9  2004/08/20 14:57:37  lazyt
 * add http-auth support for password protected webinterface
 *
 * Revision 1.8  2004/07/10 11:38:15  lazyt
 * use -DOLDFT for older FreeType versions
 * replaced all remove() with unlink()
 *
 * Revision 1.7  2004/06/29 16:33:10  lazyt
 * fix commandline interface
 *
 * Revision 1.6  2004/04/03 17:33:08  lazyt
 * remove curl stuff
 * fix audio
 * add new options PORT=n, SAVEDB=Y/N
 *
 * Revision 1.5  2004/03/31 13:55:36  thegoodguy
 * use UTF-8 encoding for ü (\xC3\xBC)
 *
 * Revision 1.4  2003/05/16 15:07:23  lazyt
 * skip unused accounts via "plus/minus", add mailaddress to spamlist via "blue"
 *
 * Revision 1.3  2003/05/10 08:24:35  lazyt
 * add simple spamfilter, show account details in message/popup
 *
 * Revision 1.2  2003/04/29 10:36:43  lazyt
 * enable/disable audio via .conf
 *
 * Revision 1.1  2003/04/21 09:24:52  lazyt
 * add tuxmail, todo: sync (filelocking?) between daemon and plugin
 ******************************************************************************/

#include "tuxmaild.h"

/******************************************************************************
 * ReadConf (0=fail, 1=done)
 ******************************************************************************/

int ReadConf()
{
	FILE *fd_conf;
	char *ptr;
	char encodingtable[64] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
	char line_buffer[256];
	int loop;
	int src_index, dst_index;

	//open config

		if(!(fd_conf = fopen(CFGPATH CFGFILE, "r")))
		{
			printf("TuxMailD <generate new Config, please modify and restart Daemon>\n");

			if(mkdir(CFGPATH, S_IRWXU) == -1 && errno != EEXIST)
			{
				printf("TuxMailD <could not create ConfigDir>\n");
				return 0;
			}

			if(!(fd_conf = fopen(CFGPATH CFGFILE, "w")))
			{
				printf("TuxMailD <could not create Config>\n");
				return 0;
			}

			fprintf(fd_conf, "STARTDELAY=30\n");
			fprintf(fd_conf, "INTERVALL=15\n\n");
			fprintf(fd_conf, "POP3LOG=Y\n");
			fprintf(fd_conf, "LOGMODE=S\n\n");
			fprintf(fd_conf, "SAVEDB=Y\n\n");
			fprintf(fd_conf, "AUDIO=Y\n");
			fprintf(fd_conf, "VIDEO=1\n\n");
			fprintf(fd_conf, "WEBPORT=80\n");
			fprintf(fd_conf, "WEBUSER=\n");
			fprintf(fd_conf, "WEBPASS=\n\n");
			fprintf(fd_conf, "NAME0=\n");
			fprintf(fd_conf, "HOST0=\n");
			fprintf(fd_conf, "USER0=\n");
			fprintf(fd_conf, "PASS0=\n");

			fclose(fd_conf);

			return 0;
		}

	//clear database

		memset(account_db, 0, sizeof(account_db));
		startdelay = intervall = pop3log = logmode = savedb = audio = video = webport = webuser[0] = webpass[0] = 0;

	//fill database

		while(fgets(line_buffer, sizeof(line_buffer), fd_conf))
		{
			if((ptr = strstr(line_buffer, "STARTDELAY="))) sscanf(ptr + 11, "%d", &startdelay);

			else if((ptr = strstr(line_buffer, "INTERVALL="))) sscanf(ptr + 10, "%d", &intervall);

			else if((ptr = strstr(line_buffer, "POP3LOG="))) sscanf(ptr + 8, "%c", &pop3log);

			else if((ptr = strstr(line_buffer, "LOGMODE="))) sscanf(ptr + 8, "%c", &logmode);

			else if((ptr = strstr(line_buffer, "SAVEDB="))) sscanf(ptr + 7, "%c", &savedb);

			else if((ptr = strstr(line_buffer, "AUDIO="))) sscanf(ptr + 6, "%c", &audio);

			else if((ptr = strstr(line_buffer, "VIDEO="))) sscanf(ptr + 6, "%d", &video);

			else if((ptr = strstr(line_buffer, "WEBPORT="))) sscanf(ptr + 8, "%d", &webport);
			else if((ptr = strstr(line_buffer, "WEBUSER="))) sscanf(ptr + 8, "%s", &webuser[0]);
			else if((ptr = strstr(line_buffer, "WEBPASS="))) sscanf(ptr + 8, "%s", &webpass[0]);

			else if((ptr = strstr(line_buffer, "NAME0="))) sscanf(ptr + 6, "%s", account_db[0].name);
			else if((ptr = strstr(line_buffer, "HOST0="))) sscanf(ptr + 6, "%s", account_db[0].host);
			else if((ptr = strstr(line_buffer, "USER0="))) sscanf(ptr + 6, "%s", account_db[0].user);
			else if((ptr = strstr(line_buffer, "PASS0="))) sscanf(ptr + 6, "%s", account_db[0].pass);

			else if((ptr = strstr(line_buffer, "NAME1="))) sscanf(ptr + 6, "%s", account_db[1].name);
			else if((ptr = strstr(line_buffer, "HOST1="))) sscanf(ptr + 6, "%s", account_db[1].host);
			else if((ptr = strstr(line_buffer, "USER1="))) sscanf(ptr + 6, "%s", account_db[1].user);
			else if((ptr = strstr(line_buffer, "PASS1="))) sscanf(ptr + 6, "%s", account_db[1].pass);

			else if((ptr = strstr(line_buffer, "NAME2="))) sscanf(ptr + 6, "%s", account_db[2].name);
			else if((ptr = strstr(line_buffer, "HOST2="))) sscanf(ptr + 6, "%s", account_db[2].host);
			else if((ptr = strstr(line_buffer, "USER2="))) sscanf(ptr + 6, "%s", account_db[2].user);
			else if((ptr = strstr(line_buffer, "PASS2="))) sscanf(ptr + 6, "%s", account_db[2].pass);

			else if((ptr = strstr(line_buffer, "NAME3="))) sscanf(ptr + 6, "%s", account_db[3].name);
			else if((ptr = strstr(line_buffer, "HOST3="))) sscanf(ptr + 6, "%s", account_db[3].host);
			else if((ptr = strstr(line_buffer, "USER3="))) sscanf(ptr + 6, "%s", account_db[3].user);
			else if((ptr = strstr(line_buffer, "PASS3="))) sscanf(ptr + 6, "%s", account_db[3].pass);

			else if((ptr = strstr(line_buffer, "NAME4="))) sscanf(ptr + 6, "%s", account_db[4].name);
			else if((ptr = strstr(line_buffer, "HOST4="))) sscanf(ptr + 6, "%s", account_db[4].host);
			else if((ptr = strstr(line_buffer, "USER4="))) sscanf(ptr + 6, "%s", account_db[4].user);
			else if((ptr = strstr(line_buffer, "PASS4="))) sscanf(ptr + 6, "%s", account_db[4].pass);

			else if((ptr = strstr(line_buffer, "NAME5="))) sscanf(ptr + 6, "%s", account_db[5].name);
			else if((ptr = strstr(line_buffer, "HOST5="))) sscanf(ptr + 6, "%s", account_db[5].host);
			else if((ptr = strstr(line_buffer, "USER5="))) sscanf(ptr + 6, "%s", account_db[5].user);
			else if((ptr = strstr(line_buffer, "PASS5="))) sscanf(ptr + 6, "%s", account_db[5].pass);

			else if((ptr = strstr(line_buffer, "NAME6="))) sscanf(ptr + 6, "%s", account_db[6].name);
			else if((ptr = strstr(line_buffer, "HOST6="))) sscanf(ptr + 6, "%s", account_db[6].host);
			else if((ptr = strstr(line_buffer, "USER6="))) sscanf(ptr + 6, "%s", account_db[6].user);
			else if((ptr = strstr(line_buffer, "PASS6="))) sscanf(ptr + 6, "%s", account_db[6].pass);

			else if((ptr = strstr(line_buffer, "NAME7="))) sscanf(ptr + 6, "%s", account_db[7].name);
			else if((ptr = strstr(line_buffer, "HOST7="))) sscanf(ptr + 6, "%s", account_db[7].host);
			else if((ptr = strstr(line_buffer, "USER7="))) sscanf(ptr + 6, "%s", account_db[7].user);
			else if((ptr = strstr(line_buffer, "PASS7="))) sscanf(ptr + 6, "%s", account_db[7].pass);

			else if((ptr = strstr(line_buffer, "NAME8="))) sscanf(ptr + 6, "%s", account_db[8].name);
			else if((ptr = strstr(line_buffer, "HOST8="))) sscanf(ptr + 6, "%s", account_db[8].host);
			else if((ptr = strstr(line_buffer, "USER8="))) sscanf(ptr + 6, "%s", account_db[8].user);
			else if((ptr = strstr(line_buffer, "PASS8="))) sscanf(ptr + 6, "%s", account_db[8].pass);

			else if((ptr = strstr(line_buffer, "NAME9="))) sscanf(ptr + 6, "%s", account_db[9].name);
			else if((ptr = strstr(line_buffer, "HOST9="))) sscanf(ptr + 6, "%s", account_db[9].host);
			else if((ptr = strstr(line_buffer, "USER9="))) sscanf(ptr + 6, "%s", account_db[9].user);
			else if((ptr = strstr(line_buffer, "PASS9="))) sscanf(ptr + 6, "%s", account_db[9].pass);
		}

	//check for update

		if(!startdelay || !intervall || !pop3log || !logmode || !savedb || !audio || !video || !webport)
		{
			printf("TuxMailD <missing Param(s), update Config>\n");

			fd_conf = freopen(CFGPATH CFGFILE, "w", fd_conf);

			if(!startdelay) startdelay = 30;
			if(!intervall) intervall = 15;
			if(!pop3log) pop3log = 'Y';
			if(!logmode) logmode = 'S';
			if(!savedb) savedb = 'Y';
			if(!audio) audio = 'Y';
			if(!video) video = 1;
			if(!webport) webport = 80;

			fprintf(fd_conf, "STARTDELAY=%d\n", startdelay);
			fprintf(fd_conf, "INTERVALL=%d\n\n", intervall);
			fprintf(fd_conf, "POP3LOG=%c\n", pop3log);
			fprintf(fd_conf, "LOGMODE=%c\n\n", logmode);
			fprintf(fd_conf, "SAVEDB=%c\n\n", savedb);
			fprintf(fd_conf, "AUDIO=%c\n", audio);
			fprintf(fd_conf, "VIDEO=%d\n\n", video);
			fprintf(fd_conf, "WEBPORT=%d\n", webport);
			fprintf(fd_conf, "WEBUSER=%s\n", webuser);
			fprintf(fd_conf, "WEBPASS=%s\n", webpass);

			for(loop = 0; loop < 10; loop++)
			{
				fprintf(fd_conf, "\nNAME%d=%s\n", loop, account_db[loop].name);
				fprintf(fd_conf, "HOST%d=%s\n", loop, account_db[loop].host);
				fprintf(fd_conf, "USER%d=%s\n", loop, account_db[loop].user);
				fprintf(fd_conf, "PASS%d=%s\n", loop, account_db[loop].pass);

				if(!account_db[loop + 1].name[0]) break;
			}
		}

		fclose(fd_conf);

	//check config

		if(startdelay < 15 || startdelay > 60)
		{
			printf("TuxMailD <STARTDELAY=%d out of Range, set to \"30\">\n", startdelay);
			startdelay = 30;
		}

		if(!intervall) printf("TuxMailD <INTERVALL=0, check Account(s) and Exit>\n");
		else if(intervall < 5 || intervall > 60)
		{
			printf("TuxMailD <INTERVALL=%d out of Range, set to \"15\">\n", intervall);
			intervall = 15;
		}

		if(pop3log != 'Y' && pop3log != 'N')
		{
			printf("TuxMailD <POP3LOG=%c invalid, set to \"N\">\n", pop3log);
			pop3log = 'N';
		}

		if(logmode != 'A' && logmode != 'S')
		{
			printf("TuxMailD <LOGMODE=%c invalid, set to \"S\">\n", logmode);
			logmode = 'S';
		}

		if(savedb != 'Y' && savedb != 'N')
		{
			printf("TuxMailD <SAVEDB=%c invalid, set to \"Y\">\n", savedb);
			savedb = 'Y';
		}

		if(audio != 'Y' && audio != 'N')
		{
			printf("TuxMailD <AUDIO=%c invalid, set to \"Y\">\n", audio);
			audio = 'Y';
		}

		if(video < 1 || video > 4)
		{
			printf("TuxMailD <VIDEO=%d invalid, set to \"1\">\n", video);
			video = 1;
		}

		if(webport < 1 || webport > 65535)
		{
			printf("TuxMailD <WEBPORT=%d invalid, set to \"80\">\n", webport);
			webport = 80;
		}
		
		if(webuser[0])
		{
		    //build encoded string

    			strcpy(plainstring, &webuser[0]);
    			strcat(plainstring, ":");
			strcat(plainstring, &webpass[0]);

			for(src_index = dst_index = 0; src_index < strlen(plainstring); src_index += 3, dst_index += 4)
			{
		    		encodedstring[0 + dst_index] = encodingtable[plainstring[0 + src_index] >> 2];
				encodedstring[1 + dst_index] = encodingtable[(plainstring[0 + src_index] & 3) << 4 | plainstring[1 + src_index] >> 4];
				encodedstring[2 + dst_index] = encodingtable[(plainstring[1 + src_index] & 15) << 2 | plainstring[2 + src_index] >> 6];
				encodedstring[3 + dst_index] = encodingtable[plainstring[2 + src_index] & 63];
			}

			encodedstring[dst_index] = '\0';

			if(strlen(plainstring) % 3)
			{
			    for(src_index = 0; src_index < (3 - strlen(plainstring) % 3); src_index++, dst_index--)
			    {
				    encodedstring[dst_index - 1] = '=';
			    }
			}
		}

		accounts = 0;

		for(loop = 0; loop <= 9; loop++)
		{
			if(account_db[loop].name[0] && account_db[loop].host[0] && account_db[loop].user[0] && account_db[loop].pass[0]) accounts++;
			else break;
		}

		if(accounts)
		{
			if(pop3log == 'N') printf("TuxMailD <check %d Accounts every %d min without Logging>\n", accounts, intervall);
			else if(logmode == 'A') printf("TuxMailD <check %d Accounts every %d min with Logging in Append-Mode>\n", accounts, intervall);
			else printf("TuxMailD <check %d Accounts every %d min with Logging in Single-Mode>\n", accounts, intervall);
			return 1;
		}
		else
		{
			printf("TuxMailD <no valid Accounts found>\n");
			return 0;
		}
}

/******************************************************************************
 * ReadSpamList
 ******************************************************************************/

void ReadSpamList()
{
	FILE *fd_spam;
	char line_buffer[64];

	spam_entries = use_spamfilter = 0;

	if(!(fd_spam = fopen(CFGPATH SPMFILE, "r")))
	{
		printf("TuxMailD <no Spamlist found, Filter disabled>\n");
		return;
	}
	else
	{
		memset(spamfilter, 0, sizeof(spamfilter));

		while(fgets(line_buffer, sizeof(line_buffer), fd_spam) && spam_entries < 100)
		{
			if(sscanf(line_buffer, "%s", spamfilter[spam_entries].address) == 1) spam_entries++;
		}

		if(spam_entries)
		{
			use_spamfilter = 1;
			printf("TuxMailD <Spamlist contains %d Entries, Filter enabled>\n", spam_entries);
		}
		else printf("TuxMailD <empty Spamlist, Filter disabled>\n");

		fclose(fd_spam);
	}
}

/******************************************************************************
 * InterfaceThread
 ******************************************************************************/

void *InterfaceThread(void *arg)
{
	int fd_sock, fd_conn;
	struct sockaddr_un srvaddr;
	socklen_t addrlen;
	char command;

	//setup connection

		unlink(SCKFILE);

		srvaddr.sun_family = AF_UNIX;
		strcpy(srvaddr.sun_path, SCKFILE);
		addrlen = sizeof(srvaddr.sun_family) + strlen(srvaddr.sun_path);

		if((fd_sock = socket(PF_UNIX, SOCK_STREAM, 0)) == -1)
		{
			printf("TuxMailD <Socketerror: socket failed>\n");
			return 0;
		}

		if(bind(fd_sock, (struct sockaddr*)&srvaddr, addrlen) == -1)
		{
			printf("Tux>MailD <Socketerror: bind failed>\n");
			return 0;
		}

		if(listen(fd_sock, 0) == -1)
		{
			printf("TuxMailD <Socketerror: listen failed>\n");
			return 0;
		}

	//communication loop

		while(1)
		{
			if((fd_conn = accept(fd_sock, (struct sockaddr*)&srvaddr, &addrlen)) == -1)
			{
				printf("TuxMailD <Socketerror: accept failed>\n");
				continue;
			}

			recv(fd_conn, &command, 1, 0);

			switch(command)
			{
				case 'G':	send(fd_conn, &online, 1, 0);
						break;

				case 'S':	recv(fd_conn, &online, 1, 0);
						kill(pid, SIGUSR2);
						break;

				case 'L':	ReadSpamList();
			}

			close(fd_conn);
		}

	return 0;
}

/******************************************************************************
 * ConvertHeader
 ******************************************************************************/

int ConvertHeader(char *string)
{
	char tempbuffer[1024], conversion[3];
	char *ptr;
	int value, len;

	//check for encoded string

		if((ptr = strstr(string, "?=")))
		{
			if(*(ptr - 2) == '?') ptr = strstr(ptr + 2, "?=");

			len = ptr+2 - string;
			memset(tempbuffer, 0, sizeof(tempbuffer));
			memcpy(tempbuffer, string, len);

		//convert

			strtok(tempbuffer, "?");
			strtok(NULL, "?");
			strtok(NULL, "?");
			ptr = strtok(NULL, "?");

			while(*ptr != '\0')
			{
				if(*ptr == '_') header[stringindex++] = ' ';
				else if(*ptr == '=')
				{
					memcpy(conversion, ++ptr, 2);
					ptr++;
					conversion[2] = 0;
					sscanf(conversion, "%X", &value);
					sprintf(&header[stringindex++], "%c", value);
				}
				else header[stringindex++] = *ptr;

				ptr++;
			}
		}
		else return 1;

	return len;
}

/******************************************************************************
 * SendPOPCommand (0=fail, 1=done)
 ******************************************************************************/

int SendPOPCommand(int command, char *param)
{
	struct hostent *server;
	struct sockaddr_in SockAddr;
	FILE *fd_log;
	char send_buffer[128], recv_buffer[4096], month[4];
	char *ptr, *ptr1;
	int loop, day, hour, minute;

	//build commandstring

		switch(command)
		{
			case INIT:	if(!(server = gethostbyname(param)))
					{
						printf("TuxMailD <could not resolve Host \"%s\">\n", param);
						return 0;
					}

					if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
					{
						printf("TuxMailD <could not create Socket>\n");
						return 0;
					}

					SockAddr.sin_family = AF_INET;
					SockAddr.sin_port = htons(110);
					SockAddr.sin_addr = *(struct in_addr*) server->h_addr_list[0];

					if(connect(sock, (struct sockaddr*)&SockAddr, sizeof(SockAddr)))
					{
						printf("TuxMailD <could not connect to Host \"%s\">\n", param);
						close(sock);
						return 0;
					}
					break;

			case USER:	sprintf(send_buffer, "USER %s\r\n", param);
					break;

			case PASS:	sprintf(send_buffer, "PASS %s\r\n", param);
					break;

			case STAT:	sprintf(send_buffer, "STAT\r\n");
					break;

			case UIDL:	sprintf(send_buffer, "UIDL %s\r\n", param);
					break;

			case TOP:	sprintf(send_buffer, "TOP %s 0\r\n", param);
					break;

			case DELE:	sprintf(send_buffer, "DELE %s\r\n", param);
					break;

			case QUIT:	sprintf(send_buffer, "QUIT\r\n");
		}

	//send command to server

		if(command != INIT)
		{
			send(sock, send_buffer, strlen(send_buffer), 0);

			if(pop3log == 'Y')
			{
				if((fd_log = fopen(LOGFILE, "a")))
				{
					fprintf(fd_log, "%s", send_buffer);
					fclose(fd_log);
				}
				else printf("TuxMailD <could not log POP3-Command>\n");
			}
		}

	//get server response

		stringindex = 0;

		while(recv(sock, &recv_buffer[stringindex], 1, 0) > 0)
		{
			if(command == TOP)
			{
				if(recv_buffer[stringindex] == '\n' && recv_buffer[stringindex - 3] == '\n')
				{
					recv_buffer[stringindex+1] = '\0';
					break;
				}
			}
			else if(recv_buffer[stringindex] == '\n')
			{
				recv_buffer[stringindex+1] = '\0';
				break;
			}

			if(stringindex < sizeof(recv_buffer) - 1) stringindex++;
			else
			{
				printf("TuxMailD <Buffer Overflow>\n");
				stringindex = 0;
			}
		}

		if(pop3log == 'Y')
		{
			if((fd_log = fopen(LOGFILE, "a")))
			{
				fprintf(fd_log, "%s", recv_buffer);
				fclose(fd_log);
			}
			else printf("TuxMailD <could not log POP3-Response>\n");
		}

	//check server response

		if(!strncmp(recv_buffer, "+OK", 3))
		{
			switch(command)
			{
				case STAT:	sscanf(recv_buffer, "+OK %d", &messages);
						break;

				case UIDL:	sscanf(recv_buffer, "+OK %*d %s", uid);
						break;

				case TOP:	stringindex = 0;
						memset(header, 0, sizeof(header));

						if((ptr = strstr(recv_buffer, "\nDate:")))
						{
							ptr += 6;

							while(*ptr == ' ') ptr++;

							if(*ptr < '0' || *ptr > '9') sscanf(ptr, "%*s %d %s %*d %d:%d", &day, &month[0], &hour, &minute);
							else sscanf(ptr, "%d %s %*d %d:%d", &day, &month[0], &hour, &minute);

							sprintf(header, "%.2d.%.3s|%.2d:%.2d|", day, month, hour, minute);
							stringindex += 13;
						}
						else
						{
							memcpy(header, "??.???|??:??|", 13);
							stringindex += 13;
						}
		
						if((ptr = strstr(recv_buffer, "\nFrom:")))
						{
							ptr += 6;

							while(*ptr == ' ') ptr++;

							ptr1 = &header[stringindex];

							while(*ptr != '\r')
							{
								if(*ptr == '=' && *(ptr + 1) == '?') ptr += ConvertHeader(ptr);
								else memcpy(&header[stringindex++], ptr++, 1);
							}

							if(use_spamfilter)
							{
								spam_detected = 0;

								for(loop = 0; loop < spam_entries; loop++)
								{
									if(strstr(ptr1, spamfilter[loop].address))
									{
										printf("TuxMailD <Spamfilter active, delete Mail from \"%s\">\n", ptr1);
										spam_detected = 1;
										break;
									}
								}
							}

							header[stringindex++] = '|';
						}
						else
						{
							memcpy(&header[stringindex], "-?-|", 4);
							stringindex += 4;
						}

						if((ptr = strstr(recv_buffer, "\nSubject:")))
						{
							ptr += 9;

							while(*ptr == ' ') ptr++;

							while(*ptr != '\r')
							{
								if(*ptr == '=' && *(ptr + 1) == '?') ptr += ConvertHeader(ptr);
								else memcpy(&header[stringindex++], ptr++, 1);
							}

							header[stringindex++] = '|';
						}
						else
						{
							memcpy(&header[stringindex], "-?-|", 4);
							stringindex += 4;
						}

						header[stringindex] = '\0';
						break;

				case QUIT:	close(sock);
			}
		}
		else
		{
			printf("TuxMailD <Server Error>\n");
			close(sock);
			return 0;
		}

	return 1;
}

/******************************************************************************
 * CheckAccount (0=fail, 1=done)
 ******************************************************************************/

int CheckAccount(int account)
{
	int loop;
	FILE *fd_status;
	int filesize, skip_uid_check = 0;
	char statusfile[] = "/tmp/tuxmail.?";
	char *known_uids = 0, *ptr = 0;
	char mailnumber[12];

	//timestamp

		time(&tt);
		strftime(timeinfo, 22, "%R", localtime(&tt));

	//get mail count

		if(!SendPOPCommand(INIT, account_db[account].host)) return 0;

		if(!SendPOPCommand(USER, account_db[account].user)) return 0;

		if(!SendPOPCommand(PASS, account_db[account].pass)) return 0;

		if(!SendPOPCommand(STAT, "")) return 0;

		account_db[account].mail_all = messages;
		account_db[account].mail_new = 0;
		deleted_messages = 0;

	//get mail info

		statusfile[sizeof(statusfile) - 2] = account | '0';

		if(messages)
		{
			//load last status from file

				if((fd_status = fopen(statusfile, "r")))
				{
					fseek(fd_status, 0, SEEK_END);

					if((filesize = ftell(fd_status)))
					{
						known_uids = malloc(filesize + 1);
						memset(known_uids, 0, filesize + 1);

						rewind(fd_status);
						fread(known_uids, filesize, 1, fd_status);
					}
					else
					{
						printf("TuxMailD <empty Status for Account %d>\n", account);
						skip_uid_check = 1;
					}

					fclose(fd_status);
				}
				else
				{
					printf("TuxMailD <no Status for Account %d>\n", account);
					skip_uid_check = 1;
				}

			//clear status

				if(!(fd_status = fopen(statusfile, "w"))) printf("TuxMailD <could not create Status for Account %d>\n", account);

			//generate listing

				if(fd_status) fprintf(fd_status, "- --:-- %s ---/---\n", account_db[account].name); /* reserve space */

				for(loop = messages; loop != 0; loop--)
				{
					sprintf(mailnumber, "%d", loop);

					if(!SendPOPCommand(UIDL, mailnumber))
					{
						free(known_uids);
						if(fd_status) fclose(fd_status);
						return 0;
					}

					if(skip_uid_check)
					{
						if(!SendPOPCommand(TOP, mailnumber))
						{
							free(known_uids);
							if(fd_status) fclose(fd_status);
							return 0;
						}

						if(use_spamfilter && spam_detected)
						{
							if(!SendPOPCommand(DELE, mailnumber))
							{
								free(known_uids);
								if(fd_status) fclose(fd_status);
								return 0;
							}
						}
						else
						{
							account_db[account].mail_new++;

							if(fd_status) fprintf(fd_status, "|N|%s|%s\n", uid, header);
						}
					}
					else
					{
						if((ptr = strstr(known_uids, uid)))
						{
							if(*(ptr - 2) == 'D')
							{
								if(!SendPOPCommand(DELE, mailnumber))
								{
									free(known_uids);
									if(fd_status) fclose(fd_status);
									return 0;
								}

								deleted_messages++;
							}
							else 
							{
								if(fd_status)
								{
									fprintf(fd_status, "|O|");

									while(*ptr != '\n') fprintf(fd_status, "%c", *ptr++);

									fprintf(fd_status, "\n");
								}
							}
						}
						else
						{
							if(!SendPOPCommand(TOP, mailnumber))
							{
								free(known_uids);
								if(fd_status) fclose(fd_status);
								return 0;
							}

							if(use_spamfilter && spam_detected)
							{
								if(!SendPOPCommand(DELE, mailnumber))
								{
									free(known_uids);
									if(fd_status) fclose(fd_status);
									return 0;
								}
							}
							else
							{
								account_db[account].mail_new++;
	
								if(fd_status) fprintf(fd_status, "|N|%s|%s\n", uid, header);
							}
						}
					}
				}

				if(fd_status)
				{
					rewind(fd_status);

					fprintf(fd_status, "%.1d %s %s %.3d/%.3d\n", account, timeinfo, account_db[account].name, account_db[account].mail_new, account_db[account].mail_all - deleted_messages);
				}

				free(known_uids);

				if(fd_status) fclose(fd_status);
		}
		else
		{
			account_db[account].mail_new = 0;

			if((fd_status = fopen(statusfile, "w")))
			{
				fprintf(fd_status, "%.1d %s %s 000/000\n", account, timeinfo, account_db[account].name);

				fclose(fd_status);
			}
			else printf("TuxMailD <could not create Status for Account %d>\n", account);
		}

	//close session

		if(!SendPOPCommand(QUIT, "")) return 0;

	return 1;
}

/******************************************************************************
 * PlaySound
 ******************************************************************************/

void PlaySound()
{
	int dsp, format = AFMT_S16_LE, channels = 1, speed = 12000;

	if((dsp = open(DSP, O_WRONLY)) == -1)
	{
		printf("TuxMailD <could not open DSP (%s), Sound disabled>\n", strerror(errno));
		return;
	}

	if(ioctl(dsp, SNDCTL_DSP_SETFMT, &format) == -1)
	{
		printf("TuxMailD <could not set DSP-Format (%s), Sound disabled>\n", strerror(errno));
		close(dsp);
		return;
	}

	if(ioctl(dsp, SNDCTL_DSP_CHANNELS, &channels) == -1)
	{
		printf("TuxMailD <could not set DSP-Channels (%s), Sound disabled>\n", strerror(errno));
		close(dsp);
		return;
	}

	if(ioctl(dsp, SNDCTL_DSP_SPEED, &speed) == -1)
	{
		printf("TuxMailD <could not set DSP-Samplingrate (%s), Sound disabled>\n", strerror(errno));
		close(dsp);
		return;
	}

#ifdef DREAMBOX

	write(dsp, &audiodata, sizeof(audiodata));
#else
	int count = 0;

	while(count < sizeof(audiodata))
	{
	    write(dsp, &audiodata[count += 16], 16);
	}
#endif
	ioctl(dsp, SNDCTL_DSP_SYNC);

	close(dsp);
}

/******************************************************************************
 * NotifyUser
 ******************************************************************************/

void NotifyUser()
{
	int loop;
	struct sockaddr_in SockAddr;
	char http_cmd[1024], tmp_buffer[128];
	char http_cmd1[] = "GET /cgi-bin/startPlugin?name=tuxmail.cfg HTTP/1.1\n\n";
	char http_cmd2[] = "GET /cgi-bin/xmessage?timeout=5&caption=TuxMail%20Information&body=Neue%20Nachrichten%20liegen%20auf%20dem%20Server:%0A%0A";
	char http_cmd3[] = "GET /control/message?nmsg=Neue%20Nachrichten%20liegen%20auf%20dem%20Server:%0A%0A";
	char http_cmd4[] = "GET /control/message?popup=Neue%20Nachrichten%20liegen%20auf%20dem%20Server:%0A%0A";

	//audio notify

		if(audio == 'Y') PlaySound();

	//video notify

		switch(video)
		{
			case 4:	strcpy(http_cmd, http_cmd4);
				break;

			case 3:	strcpy(http_cmd, http_cmd3);
				break;

			case 2:	strcpy(http_cmd, http_cmd2);
				break;

			default:strcpy(http_cmd, http_cmd1);
		}

		if(video > 1)
		{
			for(loop = 0; loop < 10; loop++)
			{
				if(account_db[loop].mail_new)
				{
					if(video == 2) sprintf(tmp_buffer, "%%20%%20%%20%%20%%20Konto%%20#%d:%%20%.3d%%20Mail(s)%%20f\xC3\xBCr%%20%s%%0A", loop, account_db[loop].mail_new, account_db[loop].name);
					if(video == 3 || video == 4) sprintf(tmp_buffer, "%%20%%20%%20%%20%%20Konto%%20#%d:%%20%.3d%%20Mail(s)%%20f\xC3\xBCr%%20%s%%0A", loop, account_db[loop].mail_new, account_db[loop].name);
					strcat(http_cmd, tmp_buffer);

				}
			}

			strcat(http_cmd, " HTTP/1.1\n");
			
			if(webuser[0])
			{
			    strcat(http_cmd, "Authorization: Basic ");
			    strcat(http_cmd, &encodedstring[0]);
			    strcat(http_cmd, "\n\n");			
			}
			else
			{
			    strcat(http_cmd, "\n");
			}
		}

		if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			printf("TuxMailD <could not create Socket>\n");
			return;
		}

		SockAddr.sin_family = AF_INET;
		SockAddr.sin_port = htons(webport);
		inet_aton("127.0.0.1", &SockAddr.sin_addr);

		if(connect(sock, (struct sockaddr*)&SockAddr, sizeof(SockAddr)))
		{
			printf("TuxMailD <could not connect to WebServer>\n");
			close(sock);
			return;
		}

		send(sock, http_cmd, strlen(http_cmd), 0);

		close(sock);
}

/******************************************************************************
 * SigHandler
 ******************************************************************************/

void SigHandler(int signal)
{
	switch(signal)
	{
		case SIGTERM:	printf("TuxMailD <shutdown>\n");
				intervall = 0;
				break;

		case SIGHUP:	printf("TuxMailD <update>\n");
				if(!ReadConf()) intervall = 0;
				ReadSpamList();
				break;

		case SIGUSR1:	online ^= 1;
		case SIGUSR2:	online?printf("TuxMailD <wakeup>\n"):printf("TuxMailD <sleep>\n");
	}
}

/******************************************************************************
 * MainProgram
 ******************************************************************************/

int main(int argc, char **argv)
{
	char cvs_revision[] = "$Revision: 1.9 $", versioninfo[12];
	int account, mailstatus;
	pthread_t thread_id;
	void *thread_result = 0;

	//create daemon

		sscanf(cvs_revision, "%*s %s", versioninfo);

		time(&tt);
		strftime(timeinfo, 22, "%d.%m.%Y - %T", localtime(&tt));

		switch(fork())
		{
			case 0:		printf("TuxMailD %s started [%s]\n", versioninfo, timeinfo);
					setsid();
					break;

			case -1:	printf("TuxMailD %s aborted!\n", versioninfo);
					return -1;

			default:	exit(0);
		}

	//read, update or create config

		if(!ReadConf()) return -1;

	//read spamlist

		ReadSpamList();

	//check for running daemon

		if((fd_pid = fopen(PIDFILE, "r+")))
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
				printf("TuxMailD <Daemon already running with PID %d>\n", pid);
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

	//install sighandler

		if(signal(SIGTERM, SigHandler) == SIG_ERR)
		{
			printf("TuxMailD <Installation of Signalhandler failed>\n");
			return -1;
		}

		if(signal(SIGHUP, SigHandler) == SIG_ERR)
		{
			printf("TuxMailD <Installation of Signalhandler failed>\n");
			return -1;
		}

		if(signal(SIGUSR1, SigHandler) == SIG_ERR)
		{
			printf("TuxMailD <Installation of Signalhandler failed>\n");
			return -1;
		}

		if(signal(SIGUSR2, SigHandler) == SIG_ERR)
		{
			printf("TuxMailD <Installation of Signalhandler failed>\n");
			return -1;
		}

	//install communication interface

		if(pthread_create(&thread_id, NULL, InterfaceThread, NULL))
		{
			printf("TuxMailD <Interface-Thread failed>\n");
			return -1;
		}

	//restore database

		if(savedb == 'Y')
		{
		    printf("TuxMailD <restore Mail-DB>\n");
		    system("cp /var/tuxbox/config/tuxmail/tuxmail.[0-9] /tmp 2>/dev/null");
		}
		
	//check accounts

		if(argc > 1 && !strcmp(argv[1], "-nodelay")) printf("TuxMailD <override Startdelay>\n");
		else sleep(startdelay); /* give gui some time to complete startup */

		do
		{
			if(online)
			{
				if(pop3log == 'Y' && logmode == 'S') fclose(fopen(LOGFILE, "w"));

				mailstatus = 0;

				for(account = 0; account < accounts; account++)
				{
					if(CheckAccount(account))
					{
						printf("TuxMailD <Account %d: %.3d/%.3d Mail(s) for %s>\n", account, account_db[account].mail_new, account_db[account].mail_all - deleted_messages, account_db[account].name);

						mailstatus += account_db[account].mail_new;
					}
					else printf("TuxMailD <Account %d skipped>\n", account);
				}

				if(mailstatus) NotifyUser();
			}

			sleep(intervall * 60);
		}
		while(intervall);

	//cleanup

		pthread_cancel(thread_id);
		pthread_join(thread_id, thread_result);

		if(savedb == 'Y')
		{
		    printf("TuxMailD <backup Mail-DB>\n");
		    system("cp /tmp/tuxmail.[0-9] /var/tuxbox/config/tuxmail 2>/dev/null");
		}
		
		unlink(PIDFILE);

		time(&tt);
		strftime(timeinfo, 22, "%d.%m.%Y - %T", localtime(&tt));

		printf("TuxMailD %s closed [%s]\n", versioninfo, timeinfo);

		return 0;
}
