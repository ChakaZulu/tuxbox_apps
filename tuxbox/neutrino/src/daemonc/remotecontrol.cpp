//
// $Id: remotecontrol.cpp,v 1.15 2001/10/10 14:58:09 fnbrd Exp $
//
// $Log: remotecontrol.cpp,v $
// Revision 1.15  2001/10/10 14:58:09  fnbrd
// Angepasst an neuen sectionsd
//
// Revision 1.14  2001/10/10 02:56:34  fnbrd
// nvod vorbereitet
//
// Revision 1.13  2001/10/09 20:10:08  fnbrd
// Ein paar fehlende Initialisierungen implementiert.
//
//

#include "remotecontrol.h"
#include "../global.h"

CRemoteControl::CRemoteControl()
{
	memset(&remotemsg, 0, sizeof(remotemsg) );
    memset(&audio_chans, 0, sizeof(audio_chans));
    memset(&audio_chans_int, 0, sizeof(audio_chans_int));
    ecm_pid=0;
    zapit_mode=false;

    pthread_cond_init( &send_cond, NULL );
    pthread_mutex_init( &send_mutex, NULL );

    if (pthread_create (&thrSender, NULL, RemoteControlThread, (void *) this) != 0 )
	{
		perror("CRemoteControl: Create RemoteControlThread failed\n");
	}
}

void CRemoteControl::setZapper(bool zapper)
{
	zapit_mode = zapper;
}


void CRemoteControl::send()
{
    pthread_cond_signal( &send_cond );
    usleep(10);
//    printf("CRemoteControl: after pthread_cond_signal (with %s)\n", remotemsg.param3);
}

// quick'n dirty, damit der Rest was zum arbeiten hat ;)
static void getNVODs(unsigned onidSid)
{
  char rip[]="127.0.0.1";

  int sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  SAI servaddr;
  memset(&servaddr,0,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  servaddr.sin_port=htons(sectionsd::portNumber);
  inet_pton(AF_INET, rip, &servaddr.sin_addr);

  if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1) {
    perror("Couldn't connect to sectionsd!");
    return;
  }
  sectionsd::msgRequestHeader req;
  req.version = 2;
  req.command = sectionsd::timesNVODservice;
  req.dataLength = 4;
  write(sock_fd, &req, sizeof(req));
  write(sock_fd, &onidSid, req.dataLength);
  sectionsd::msgResponseHeader resp;
  memset(&resp, 0, sizeof(resp));
  if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0) {
    close(sock_fd);
    return;
  }
  if(resp.dataLength) {
    char* pData = new char[resp.dataLength] ;
    if(read(sock_fd, pData, resp.dataLength)>0) {
      printf("dataLength: %u\n", resp.dataLength);
      char *p=pData;
      while(p<pData+resp.dataLength) {
	unsigned onidsid2=*(unsigned *)p;
	printf("onid_sid: 0x%x\n", onidsid2);
	p+=4;
	unsigned short tsid=*(unsigned short *)p;
	printf("tsid: 0x%x\n", tsid);
	p+=2;
	unsigned char numberOfTimes=*p;
	p++;
	for(int i=0; i<numberOfTimes; i++) {
	  time_t zeit=*(time_t *)p;
	  p+=4;
	  printf("%s", ctime(&zeit));
	}
      }
    }
    delete[] pData;
  }
  close(sock_fd);
}

void * CRemoteControl::RemoteControlThread (void *arg)
{
	CRemoteControl* RemoteControl = (CRemoteControl*) arg;

    int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";
    bool redo, do_immediatly;

	while(1)
	{
//        printf("CRemoteControl: before pthread_cond_wait\n");

        pthread_mutex_trylock( &RemoteControl->send_mutex );
        pthread_cond_wait( &RemoteControl->send_cond, &RemoteControl->send_mutex );
      
//        printf("CRemoteControl: after pthread_cond_wait for %s\n", RemoteControl->remotemsg.param3);

        st_rmsg r_msg;

        do
        {
            pthread_mutex_trylock( &RemoteControl->send_mutex );
            memcpy( &r_msg, &RemoteControl->remotemsg, sizeof(r_msg) );
            pthread_mutex_unlock( &RemoteControl->send_mutex );

            memset(&servaddr,0,sizeof(servaddr));
            servaddr.sin_family=AF_INET;

            #ifdef HAS_SIN_LEN
            servaddr.sin_len = sizeof(servaddr); // needed ???
           	#endif
            inet_pton(AF_INET, rip, &servaddr.sin_addr);
            sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            if ( RemoteControl->zapit_mode )
            {
                char *return_buf;
                int bytes_recvd = 0;
	
                servaddr.sin_port=htons(1505);
            	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
            	{
              		perror("CRemoteControl::RemoteControlThread - Couldn't connect to serverd zapit!");
//            		exit(-1);
            	}

                write(sock_fd, &r_msg, sizeof(r_msg));
	
                return_buf = (char*) malloc(4);
                memset(return_buf,0,sizeof(return_buf));
                bytes_recvd = recv(sock_fd, return_buf, 3,0);
                if (bytes_recvd <= 0 )
                {
                    perror("CRemoteControl::RemoteControlThread - Nothing could be received from serverd zapit\n");
//                    exit(-1);
                }
//                printf("Received %d bytes\n", bytes_recvd);
//                printf("That was returned: %s\n", return_buf);
	
                do_immediatly = false;
                switch (atoi(return_buf))
                {
                    case 0: printf("Unknown error reported from zapper\n");
//                            exit(-1);
                            break;
                    case 1: {
                                printf("Zapping by number returned successful\n");
                                break;
                            }
                    case -1: printf("Zapping by number returned UNsuccessful\n");
                            break;
                    case 2: printf("zapit should be killed now.\n");
                            break;
                    case -2: printf("zapit could not be killed\n");
                            break;
                    case 3: {
                                // printf("Zapping by name returned successful\n");

                                // ueberpruefen, ob wir die Audio-PIDs holen sollen...
//                                printf("Checking for Audio-PIDs %s - %s - %d\n", RemoteControl->remotemsg.param3, r_msg.param3, RemoteControl->remotemsg.cmd);
                                pthread_mutex_trylock( &RemoteControl->send_mutex );
                                if ( ( RemoteControl->remotemsg.cmd== 3 ) &&
                                     ( strcmp(RemoteControl->remotemsg.param3, r_msg.param3 )== 0 ) )
                                {
                                    // noch immer der gleiche Kanal, Abfrage 8 starten
                                    RemoteControl->remotemsg.cmd= 8;

                                    strcpy( RemoteControl->audio_chans_int.name, r_msg.param3 );
                                    do_immediatly = true;
//                                    printf("Audio-PIDs holen for %s\n", RemoteControl->apids.name);
                                }
                                else
                                    pthread_mutex_unlock( &RemoteControl->send_mutex );

                                break;
                            }
                            break;
                    case -3: printf("\n\nHier waere Platz fuer ne Fehlerbild-funktion\n\n");
                            break;
                    case 4: printf("Shutdown Box returned successful\n");
                            break;
                    case -4: printf("Shutdown Box was not succesful\n");
                            break;
                    case 5: printf("get Channellist returned successful\n");
                            printf("Should not be received in remotecontrol.cpp. Exiting\n");
//                            exit(-1);
                            break;
                    case -5: printf("get Channellist returned UNsuccessful\n");
                            printf("Should not be received in remotecontrol.cpp. Exiting\n");
//                            exit(-1);
                            break;
                    case 6: printf("Changed to radio-mode\n");
                            break;
                    case -6: printf("Could not change to radio-mode\n");
                            break;
                    case 7: printf("Changed to TV-mode\n");
                            break;
                    case -7: printf("Could not change to TV-Mode\n");
                            break;
                    case 8: {
//                                printf("Got a pid-description\n");
                                // printf("should not be done in remotecontrol.cpp.\n");

                                struct  pids    apid_return_buf;
                                memset(&apid_return_buf, 0, sizeof(apid_return_buf));

                                if ( read(sock_fd, &apid_return_buf, sizeof(apid_return_buf)) > 0 )
                                {
                                // PIDs emfangen, ueberpruefen, ob wir die Audio-PIDs uebernehmen sollen...

                                    pthread_mutex_trylock( &RemoteControl->send_mutex );
                                    if ( //( remotemsg.cmd== 8 ) &&
                                         ( strlen( RemoteControl->audio_chans_int.name )!= 0 ) )
                                    {
                                        // noch immer der gleiche Kanal

                                        RemoteControl->audio_chans_int.count_apids = apid_return_buf.count_apids;
                                        printf("got apids for: %s - %d apids!\n", RemoteControl->audio_chans_int.name, RemoteControl->audio_chans_int.count_apids);
                                        // printf("%d - %d - %d - %d - %d\n", apid_return_buf.apid[0], apid_return_buf.apid[1], apid_return_buf.apid[2], apid_return_buf.apid[3], apid_return_buf.apid[4] );
                                        for(int count=0;count<apid_return_buf.count_apids;count++)
                                        {
//                                           printf("%s \n", apid_return_buf.apids[count].desc);
                                            strcpy(RemoteControl->audio_chans_int.apids[count].name, apid_return_buf.apids[count].desc);
                                            RemoteControl->audio_chans_int.apids[count].ctag= apid_return_buf.apids[count].component_tag;
                                            RemoteControl->audio_chans_int.apids[count].is_ac3= apid_return_buf.apids[count].is_ac3;
                                        }
                                        RemoteControl->ecm_pid= apid_return_buf.ecmpid;
                                        // printf("ECM_PID %x\n", RemoteControl->ecm_pid);

                                        pthread_cond_signal( &g_InfoViewer->lang_cond );
                                    }
                                    pthread_mutex_unlock( &RemoteControl->send_mutex );
                                }
                                else
                                    printf("pid-description fetch failed!\n");
                                break;
                            }
                    case -8: printf("Could not get a pid-description\n");
                            printf("should not be done in remotecontrol.cpp.\n");
                            break;
                    case 9: printf("Changed apid\n");
                            break;
                    case -9: printf("Could not change apid\n");
                            break;
                    default: printf("Unknown return-code\n");
//                            exit(-1);
                }
                if ( !do_immediatly )
                    usleep(100000);
            }
            else
            {
                servaddr.sin_port=htons(1500);
                if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))!=-1)
                {
//                    printf("CRemoteControl: before write to socket %s\n", r_msg.param3);
                    write(sock_fd, &r_msg, sizeof(r_msg) );

                    usleep(1500000);
                };
            }

            close(sock_fd);

            pthread_mutex_trylock( &RemoteControl->send_mutex );
            redo= memcmp(&r_msg, &RemoteControl->remotemsg, sizeof(r_msg)) != 0;

        } while ( redo );
	}
	return NULL;
}

unsigned int CRemoteControl::GetECMPID()
{
    pthread_mutex_lock( &send_mutex );
    int ep = ecm_pid;
    pthread_mutex_unlock( &send_mutex );
    return ep;
}

void CRemoteControl::CopyAPIDs()
{
    pthread_mutex_lock( &send_mutex );
    memcpy(&audio_chans, &audio_chans_int, sizeof(audio_chans));
    pthread_mutex_unlock( &send_mutex );
}

void CRemoteControl::queryAPIDs()
{
    pthread_mutex_lock( &send_mutex );

    remotemsg.version=1;
    remotemsg.cmd=8;

    pthread_mutex_unlock( &send_mutex );
	send();
}

void CRemoteControl::setAPID(int APID)
{
    pthread_mutex_lock( &send_mutex );

    remotemsg.version=1;
    remotemsg.cmd=9;
    snprintf( (char*) &remotemsg.param, 2, "%.1d", APID);
    audio_chans_int.selected = APID;
    printf("changing APID to %d\n", audio_chans_int.selected);

    pthread_mutex_unlock( &send_mutex );
	send();
}


void CRemoteControl::zapTo(int, string chnlname )
{
    pthread_mutex_lock( &send_mutex );
//    getNVODs(0x850001); // Cinedom 1 fest zum testen
    remotemsg.version=1;
    remotemsg.cmd=3;
    remotemsg.param=0x0100;
    strcpy( remotemsg.param3, chnlname.c_str() );

    memset(&audio_chans_int, 0, sizeof(audio_chans_int));

    pthread_mutex_unlock( &send_mutex );

	send();
}

void CRemoteControl::radioMode()
{
    pthread_mutex_lock( &send_mutex );

	remotemsg.version=1;
	remotemsg.cmd=6;
	
    pthread_mutex_unlock( &send_mutex );

	send();
}

void CRemoteControl::tvMode()
{
    pthread_mutex_lock( &send_mutex );

	remotemsg.version=1;
	remotemsg.cmd=7;

    pthread_mutex_unlock( &send_mutex );	

	send();
}


void  CRemoteControl::shutdown()
{
    pthread_mutex_lock( &send_mutex );

    remotemsg.version=1;
    remotemsg.cmd=4;

    pthread_mutex_unlock( &send_mutex );

    send();
}









