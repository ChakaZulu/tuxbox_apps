#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libmd5sum/libmd5sum.h"


struct Smd5sum
{
	char	name[20];
	char	md5data[16];
};

void setMD5Data(struct Smd5sum* mds, char* Name, char md1, char md2,  char md3,  char md4,  char md5, char md6
		char md7, char md8, char md9, char md10, char md11, char md12, char md13, char md14, char md15, char md16 )
{
	strcpy(mds->name, Name);
	
}



#define COUNT_AVIA500 2
struct Smd5sum md5avia500[COUNT_AVIA500];

#define COUNT_AVIA600 2
struct Smd5sum md5avia600[COUNT_AVIA600];

#define COUNT_UCODE 2
struct Smd5sum md5ucodes[COUNT_UCODE];

#define COUNT_CAMALPHA 4
struct Smd5sum md5cam[COUNT_CAMALPHA];

 

int main(int argc, char **argv)
{
	//setup avia 500-files
	int pos = 0;
	//avia500v093
	setMD5Data(md5avia500[pos++], "ok, V0.93", 0xfe, 0xce, 0x1d, 0x33, 0x24, 0xe0, 0x91, 0x7b, 0x92, 0x1d,
							0x81, 0x44, 0x90, 0xd8, 0xa8, 0x24 ); 





	return 1;
}
