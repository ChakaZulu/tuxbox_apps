//
// $Id: remotecontrol.cpp,v 1.27 2001/10/25 12:26:09 field Exp $
//
// $Log: remotecontrol.cpp,v $
// Revision 1.27  2001/10/25 12:26:09  field
// NVOD-Zeiten im Infoviewer stimmen
//
// Revision 1.26  2001/10/21 13:06:17  field
// nvod-zeiten funktionieren!
//
// Revision 1.25  2001/10/18 21:03:14  field
// EPG Previous/Next
//
// Revision 1.24  2001/10/16 21:22:44  field
// NVODs besser
//
// Revision 1.23  2001/10/16 19:21:30  field
// NVODs! Zeitanzeige geht noch nicht
//
// Revision 1.20  2001/10/16 17:00:13  faralla
// nvod nearly ready
//
// Revision 1.19  2001/10/15 17:27:19  field
// nvods (fast) implementiert (umschalten funkt noch nicht)
//
// Revision 1.17  2001/10/13 00:46:48  McClean
// nstreamzapd-support broken - repaired
//
// Revision 1.16  2001/10/10 17:17:13  field
// zappen auf onid_sid umgestellt
//
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
static void getNVODs(unsigned onidSid, st_nvod_info *nvods )
{
    char rip[]="127.0.0.1";

    bool got_times= false;
    int rep_cnt= 0;
    nvod_info   n_nvods[10];
    int         count= 0;
    do
    {
        rep_cnt++;
        if (rep_cnt> 1 )
        {
            usleep(200000);
            printf("getNVODs - try #%d\n", rep_cnt);
        }

        int sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        SAI servaddr;
        memset(&servaddr,0,sizeof(servaddr));
        servaddr.sin_family=AF_INET;
        servaddr.sin_port=htons(sectionsd::portNumber);
        inet_pton(AF_INET, rip, &servaddr.sin_addr);

        if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
        {
            perror("CRemoteControl - getNVODs - couldn't connect to sectionsd!\n");
        }
        else
        {
            sectionsd::msgRequestHeader req;
            req.version = 2;
            req.command = sectionsd::timesNVODservice;
            req.dataLength = 4;
            write(sock_fd, &req, sizeof(req));
            write(sock_fd, &onidSid, req.dataLength);
            sectionsd::msgResponseHeader resp;
            memset(&resp, 0, sizeof(resp));
            if(read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader))<=0)
            {
                close(sock_fd);
                return;
            }

            if(resp.dataLength)
            {
                char* pData = new char[resp.dataLength] ;
                if(read(sock_fd, pData, resp.dataLength)>0)
                {
                    //printf("dataLength: %u\n", resp.dataLength);
                    char *p=pData;

                    count = 0;
                    while(p<pData+resp.dataLength)
                    {
                        count+= 1;
                        unsigned onidsid2=*(unsigned *)p;
                        // printf("onid_sid: 0x%x\n", onidsid2);
                        n_nvods[count- 1].onid_sid = onidsid2;

                        p+=4;
                        unsigned short tsid=*(unsigned short *)p;
                        // printf("tsid: 0x%x\n", tsid);
                        n_nvods[count- 1].tsid = tsid;

                        p+=2;
                        time_t zeit=*(time_t *)p;
                        n_nvods[count- 1].startzeit = zeit;
                        p+=4;
                        //printf("%s", ctime(&zeit));

                        n_nvods[count- 1].dauer = *(unsigned *)p;
                        p+=4;

                        if (n_nvods[count- 1].dauer!= 0)
                            got_times= true;
                    }
                }
                delete[] pData;
            }
            close(sock_fd);
        }
    } while ( ( ( count== 0 ) || ( !got_times ) ) && ( rep_cnt< 20) );

    if ( count> 0 )
    {
        //sortieren
        time_t  min_zeit;
        int     min_index;
        nvods->count_nvods= 0;
        for (int j=0;j<count;j++)
        {
            min_zeit= 0x7FFFFFFF;
            min_index= -1;

            for (int i=0;i<count;i++)
            {
                if ( (n_nvods[i].dauer!= 0) && (n_nvods[i].startzeit< min_zeit) )
                {
                    min_index= i;
                    min_zeit= n_nvods[i].startzeit;
                }
            }
            if ( min_index!= -1)
            {
                memcpy(&nvods->nvods[nvods->count_nvods], &n_nvods[min_index], sizeof(nvod_info));
                n_nvods[min_index].dauer= 0;
                //printf("%s - %x\n", ctime(&nvods->nvods[nvods->count_nvods].startzeit), (unsigned int)nvods->nvods[nvods->count_nvods].startzeit);
                nvods->count_nvods++;
            }
        }
    }
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
//                printf("sending %d\n", r_msg.cmd);
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
	
                char ZapStatus = return_buf[1];

                do_immediatly = false;

                if ( return_buf[0] == '-' )
                {
                    printf("zapit failed for function >%s<\n", &return_buf[2]);
                }
                else
                {
                    switch ( return_buf[2] )
                    {
                        case '0':   printf("Unknown error reported from zapper\n");
                                    break;
                        case '1':   {
                                        printf("Zapping by number returned successful\n");
                                        break;
                                    }
                        case '2':   printf("zapit should be killed now.\n");
                                    break;
                        case '3':   {
                                        // printf("Zapping by name returned successful\n");

                                        // ueberpruefen, ob wir die Audio-PIDs holen sollen...
                                        // printf("Checking for Audio-PIDs %s - %s - %d\n", RemoteControl->remotemsg.param3, r_msg.param3, RemoteControl->remotemsg.cmd);
                                        pthread_mutex_trylock( &RemoteControl->send_mutex );
                                        if ( ( RemoteControl->remotemsg.cmd== 3 ) &&
                                             ( strcmp(RemoteControl->remotemsg.param3, r_msg.param3 )== 0 ) )
                                        {
                                            // noch immer der gleiche Kanal, Abfrage 8 starten
                                            RemoteControl->remotemsg.cmd= 8;

                                            strcpy( RemoteControl->audio_chans_int.name, r_msg.param3 );
                                            do_immediatly = true;
                                            // printf("Audio-PIDs holen for %s\n", RemoteControl->apids.name);
                                        }
                                        else
                                            pthread_mutex_unlock( &RemoteControl->send_mutex );

                                        break;
                                    }
                                    break;
                        case '4':   printf("Shutdown Box returned successful\n");
                                    break;
                        case '5':   printf("get Channellist returned successful\n");
                                    printf("Should not be received in remotecontrol.cpp. Exiting\n");
                                    break;
                        case '6':   printf("Changed to radio-mode\n");
                                    break;
                        case '7':   printf("Changed to TV-mode\n");
                                    break;
                        case '8':
                        case 'd':
                        case 'e':   {
                                        struct  pids    apid_return_buf;
                                        memset(&apid_return_buf, 0, sizeof(apid_return_buf));

                                        if ( read(sock_fd, &apid_return_buf, sizeof(apid_return_buf)) > 0 )
                                        {
                                            // PIDs emfangen...

                                            pthread_mutex_trylock( &RemoteControl->send_mutex );
                                            if ( ( strlen( RemoteControl->audio_chans_int.name )!= 0 ) ||
                                                 ( ( strcmp(RemoteControl->remotemsg.param3, r_msg.param3 )== 0 ) && (return_buf[2] == 'd') ) ||
                                                 (return_buf[2] == 'e') )
                                            {
                                                // noch immer der gleiche Kanal

                                                if ( (return_buf[2] == 'd') && ( ZapStatus & 0x80 ) )
                                                {
                                                    unsigned int onid_sid;

                                                    sscanf( r_msg.param3, "%x", &onid_sid );
                                                    getNVODs( onid_sid, &RemoteControl->nvods_int );
                                                    printf("NVOD-Basechannel - got %d nvods!\n", RemoteControl->nvods_int.count_nvods);

                                                    strcpy( RemoteControl->nvods_int.name, r_msg.param3 );

                                                    if ( RemoteControl->nvods_int.count_nvods> 0 )
                                                    {
                                                        // übertragen der ids an zapit initialisieren

                                                        // !!! AUSKOMMENTIERT, weil das tut noch nicht... !!!

                                                        RemoteControl->remotemsg.cmd= 'i';
                                                        do_immediatly = true;
                                                    }
                                                }
                                                else
                                                {
                                                    if (return_buf[2] == 'd')
                                                        strcpy( RemoteControl->audio_chans_int.name, r_msg.param3 );
                                                    if (return_buf[2] == 'e')
                                                        strcpy( RemoteControl->audio_chans_int.name, RemoteControl->nvods_int.name );

                                                    // Nur dann die Audio-Channels abholen, wenn nicht NVOD-Basechannel

                                                    RemoteControl->audio_chans_int.count_apids = apid_return_buf.count_apids;
                                                    printf("got apids for: %s - %d apids!\n", RemoteControl->audio_chans_int.name, RemoteControl->audio_chans_int.count_apids);
                                                    // printf("%d - %d - %d - %d - %d\n", apid_return_buf.apid[0], apid_return_buf.apid[1], apid_return_buf.apid[2], apid_return_buf.apid[3], apid_return_buf.apid[4] );
                                                    for(int count=0;count<apid_return_buf.count_apids;count++)
                                                    {
                                                        // printf("%s \n", apid_return_buf.apids[count].desc);
                                                        strcpy(RemoteControl->audio_chans_int.apids[count].name, apid_return_buf.apids[count].desc);
                                                        RemoteControl->audio_chans_int.apids[count].ctag= apid_return_buf.apids[count].component_tag;
                                                        RemoteControl->audio_chans_int.apids[count].is_ac3= apid_return_buf.apids[count].is_ac3;
                                                    }
                                                    RemoteControl->ecm_pid= apid_return_buf.ecmpid;
                                                }

                                                pthread_cond_signal( &g_InfoViewer->lang_cond );
                                            }
                                            if (!do_immediatly)
                                                pthread_mutex_unlock( &RemoteControl->send_mutex );
                                        }
                                        else
                                            printf("pid-description fetch failed!\n");
                                        break;
                                    }
                        case 'i':   {
                                        pthread_mutex_trylock( &RemoteControl->send_mutex );
                                        //printf("Telling zapit the number of nvod-chans: %d\n",RemoteControl->nvods_int.count_nvods);
                                        write(sock_fd, &RemoteControl->nvods_int.count_nvods, 2);

                                        //printf("Sending NVODs to zapit\n");
                                        for(int count=0;count<RemoteControl->nvods_int.count_nvods;count++)
                                        {
                                            // printf("Sending NVOD %d - %x - %x\n", count, RemoteControl->nvods_int.nvods[count].onid_sid, RemoteControl->nvods_int.nvods[count].tsid);
                                            write(sock_fd, &RemoteControl->nvods_int.nvods[count].onid_sid, 4);
                                            write(sock_fd, &RemoteControl->nvods_int.nvods[count].tsid, 2);
                                        }

                                        // immediately change to nvod #0...
                                        RemoteControl->remotemsg.cmd= 'e';
                                        RemoteControl->nvods_int.selected= RemoteControl->nvods_int.count_nvods- 1;
                                        snprintf( (char*) &RemoteControl->remotemsg.param3, 10, "%x", RemoteControl->nvods_int.nvods[RemoteControl->nvods_int.selected].onid_sid);

                                        do_immediatly = true;

                                        // pthread_mutex_unlock( &RemoteControl->send_mutex );
                                        break;
                                    }
                        case '9':   printf("Changed apid\n");
                                    break;

                        default: printf("Unknown return-code >%s<, %d\n", return_buf, return_buf[2]);
                    }
                }
                if ( !do_immediatly )
                    usleep(100000);
                    //usleep(100);
            }
            else
            {
                servaddr.sin_port=htons(1500);
                if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))!=-1)
                {
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

void CRemoteControl::CopyNVODs()
{
    pthread_mutex_lock( &send_mutex );
    memcpy(&nvods, &nvods_int, sizeof(nvods));
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

void CRemoteControl::setNVOD(int NVOD)
{
    pthread_mutex_lock( &send_mutex );

    memset(&audio_chans_int, 0, sizeof(audio_chans_int));

    remotemsg.version=1;
    remotemsg.cmd='e';
    snprintf( (char*) &remotemsg.param3, 10, "%x", nvods_int.nvods[NVOD].onid_sid);
    nvods_int.selected = NVOD;
    printf("changing NVOD# to %d\n", nvods_int.selected);

    pthread_mutex_unlock( &send_mutex );
	send();
}


void CRemoteControl::zapTo_onid_sid( unsigned int onid_sid )
{
    pthread_mutex_lock( &send_mutex );
    remotemsg.version=1;
    remotemsg.cmd= 'd';
    snprintf( (char*) &remotemsg.param3, 10, "%x", onid_sid);

    memset(&audio_chans_int, 0, sizeof(audio_chans_int));
    memset(&nvods_int, 0, sizeof(nvods_int));

    pthread_mutex_unlock( &send_mutex );

	send();
}

void CRemoteControl::zapTo(string chnlname )
{
    pthread_mutex_lock( &send_mutex );
//    getNVODs(0x850001); // Cinedom 1 fest zum testen
    remotemsg.version=1;
    remotemsg.cmd=3;
    remotemsg.param=0x0100;
    strcpy( remotemsg.param3, chnlname.c_str() );

    memset(&audio_chans_int, 0, sizeof(audio_chans_int));
    memset(&nvods_int, 0, sizeof(nvods_int));

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


