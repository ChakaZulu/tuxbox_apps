#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libmd5sum/libmd5sum.h"


struct Sucode
{
	char md5[35];
	char name[20];
};


struct Sucode ucodes[] = 
{
"fece1d3324e0917b921d814490d8a824","avia500v093",
"7373f3934263b3c3ea1d0f500f0044a5","avia500v110",
"da492146ba7e177883feadaa0cf89aa5","avia600vb017",
"6a748fb28000738cafeb9e27443ac623","avia600vb022",
"d4c12df0d4ce8ba9eb858509d832df65","ucode",
"ff6fafbd2aa1f29afe232a72fac57870","cam_01_01_001D",
"c42d6753794dd95146eac31f2a65b516","cam_01_01_004D",
"be4b0f38557c416ce04e7fa3fa634f95","cam_01_01_005D",
"7b9b72786623e3750335c79af044e718","cam_01_01_001E",
"be7f1beb1bb437b7f7c99f8e5a968882","cam_01_01_002E",
"a8689d88e0d2df12a15732c98f865b11","cam_01_01_003E",
"4336e7d3fed43c9e063210bcdc95d23e","cam_01_01_004E",
"997b1f858f1efee525e6842558edbe3c","cam_01_01_005E",
"7f56e693a916b39a6e2734dc9b5aab7a","cam_01_02_002E",
"1905390636e70c966574a3298a1b89c3","cam_01_02_002D",
"c734207ddea7b8ceafa2505f1360f3bf","cam_01_01_004F",
"a5984825ff554ea530efc4733ffd7473","cam_01_01_005F",
"16c5e1eba0cfe63f2bbc648e1644c883","cam_NOKIA_PRODTEST2",
"420baf447bfd529a794bb36ef80f1652","cam_STREAMHACK"
};



int checkFile(char* filename, char* result)
{
	int		count;
	int		anz;
	unsigned char	md5buffer[16];
	char		md5string[40]="";

	//get the file-md5sum and convert to string..
	if( md5_file(filename, 1, (unsigned char*) &md5buffer))
 	{
		strcpy(result, "not found");
		return -1;
        }

	for(count=0;count<16;count++)
	{
		char tmp[6];
		sprintf((char*) &tmp, "%02x", md5buffer[count] );
		strcat(md5string, tmp);
	}


	anz = sizeof(ucodes) / sizeof(*ucodes); 

	for(count=0;count<anz;count++)
	{
		if( strcmp(ucodes[count].md5, md5string)==0)
		{
			strcpy(result, ucodes[count].name);
			return 1;
		}
	}

	strcpy(result, "unknown");
	return 0;
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
