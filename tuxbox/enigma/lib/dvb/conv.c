#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	char buffer[100];
	FILE *in=fopen("iso639.txt", "rt");
	if (!in)
		perror("iso639.txt");
	
	printf("struct ISO639\n"
		"{\n"
		"	char *iso639foreign, *iso639int, *twoletterm, *description1, *description2;\n"
		"} iso639[]={\n");
	while (fgets(buffer, 100, in))
	{
		char *a1=buffer;
		char *a2=a1, *tl, *d1, *d2;
		buffer[strlen(buffer)-1]=0;
		a2--;
		while (*++a2!='\t');
		*(tl=a2++)=0;
		while (*++tl!='\t');
		*(d1=tl++)=0;
		while (*++d1!='\t');
		*(d2=d1++)=0;
		while (*++d2!='\t');
		*(d2++)=0;
		printf("		{\"%s\", \"%s\", \"%s\", \"%s\", \"%s\"}, \n", a1, a2, tl, d1, d2);
	}
	printf("		};\n");
}