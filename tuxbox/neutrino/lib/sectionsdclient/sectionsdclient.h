
#ifndef __sectionsdclient__
#define __sectionsdclient__

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>


#include <string>

using namespace std;

class CSectionsdClient
{

		int sectionsd_connect();
		bool send(int fd, char* data, int size);
		bool receive(int fd, char* data, int size);
		bool sectionsd_close(int fd);

	public:

		static const char ACTVERSION = 2;

		enum commands
		{
		};

		//command structures
		struct commandHead
		{
			unsigned char  version;
			unsigned char  cmd;
		};

		struct commandGetBouquets
		{
			bool emptyBouquetsToo;
		};


		/* construktor */
		CSectionsdClient();


};

#endif
