/******************************************************************************
 *                       <<< TuxMailD - POP3 Daemon >>>
 *                (c) Thomas "LazyT" Loewe 2003 (LazyT@gmx.net)
 *-----------------------------------------------------------------------------
 * $Log: tuxmaild.h,v $
 * Revision 1.7  2005/02/26 10:23:49  lazyt
 * workaround for corrupt mail-db
 * add ADMIN=Y/N to conf (N to disable mail deletion via plugin)
 * show versioninfo via "?" button
 * limit display to last 100 mails (increase MAXMAIL if you need more)
 *
 * Revision 1.6  2004/08/20 14:57:37  lazyt
 * add http-auth support for password protected webinterface
 *
 * Revision 1.5  2004/04/03 17:33:08  lazyt
 * remove curl stuff
 * fix audio
 * add new options PORT=n, SAVEDB=Y/N
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

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include "audio.h"

#define DSP "/dev/sound/dsp"

#define CFGPATH "/var/tuxbox/config/tuxmail/"
#define CFGFILE "tuxmail.conf"
#define SPMFILE "spamlist"
#define SCKFILE "/tmp/tuxmaild.socket"
#define LOGFILE "/tmp/tuxmaild.log"
#define PIDFILE "/tmp/tuxmaild.pid"

//pop3 commands

enum {INIT, USER, PASS, STAT, UIDL, TOP, DELE, QUIT};

//account database

struct
{
	char name[32];
	char host[64];
	char user[64];
	char pass[64];
	int  mail_all;
	int  mail_new;

}account_db[10];

//spam database

struct
{
	char address[64];

}spamfilter[100];

//some data

FILE *fd_pid;
int pid;
int webport;
char webuser[32], webpass[32];
char plainstring[64], encodedstring[64];
int startdelay, intervall;
char pop3log, logmode, audio, savedb;
int video;
char online = 1;
int accounts;
int sock;
int messages, deleted_messages;
int stringindex;
int use_spamfilter, spam_entries, spam_detected;
char uid[128];
char header[1024];
char timeinfo[22];
time_t tt;
