#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libmd5sum/libmd5sum.h"

unsigned char md5buffer[16];


int checkFile(char* filename, char* result)
{
	int	count;
	char	md5string[40]="";
	FILE*	fi;
	char	buf[100];
	char	keystr[100];
	char	valstr[100];
	char*	tmpptr;
	char*	key;
	char*	val;
	char	keyfound;

	//get the file-md5sum and convert to string..
	if( md5_file(filename, 1, (unsigned char*) &md5buffer))
	{
		strcpy(result, "not found");
		return -2;
	}

	for(count=0;count<16;count++)
        {
		char tmp[6];
               	sprintf((char*) &tmp, "%02x", md5buffer[count] );
		strcat(md5string, tmp);
        }
	
	//search the md5-string in the ressource-file
	fi = fopen("/etc/ucodes.md5", "rt");
	if(fi==NULL)
	{
		strcpy(result, "no ressourcefile");
		return -3;
	}
	while(!feof(fi))
	{	//split md5 - description...
		if(fgets(buf,sizeof(buf)-1,fi)!=NULL)
		{
			tmpptr=buf;
			key= (char*) &keystr;
			val= (char*) &valstr;
			keyfound = 0;
			for(; (*tmpptr!=10) && (*tmpptr!=13);tmpptr++)
			{
				if((*tmpptr==' ') && (!keyfound))
				{
					keyfound=1;
				}
				else
				{
					if(!keyfound)
					{
						*key = *tmpptr;
						key++;
					}
					else
					{
						*val = *tmpptr;
						val++;
					}
				}
			}
			*key=0;
			*val=0;

			if(strcmp(keystr,md5string)==0)
			{
				strcpy(result, valstr );
				return 0;
			}
		}
	}
	fclose(fi);

	strcpy(result, "unknown");
	return -1;
}



int main(int argc, char **argv)
{
	char res[60];
	
	
	checkFile("/ucodes/avia500.ux", (char*) &res);
	printf("avia500: %s\n", res);

	checkFile("/ucodes/avia600.ux", (char*) &res);
        printf("avia600: %s\n", res);

        checkFile("/ucodes/ucode.bin", (char*) &res);
        printf("ucodes: %s\n", res);

        checkFile("/ucodes/cam-alpha.bin", (char*) &res);
        printf("cam-alpha: %s\n", res);





	return 1;
}
