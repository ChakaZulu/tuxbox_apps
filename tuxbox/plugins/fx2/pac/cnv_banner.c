#include <stdio.h>
#include <string.h>

int  main( int argc, char ** argv )
{
	char	name[256];
	char	buffer[256];
	char	*p;
	FILE	*in;
	FILE	*out;
	int		height=0;
	int		n;

	strcpy(name,argv[1]);
	p = strchr(name,'.');
	if ( !p )
		return 1;
	in = fopen( name, "r" );
	height = 0;
	if ( !in )
		return 2;
	while( fgets(buffer,256,in) )
		height++;
	fclose(in);
	in = fopen( name, "r" );

	strcpy(p,".xpm");

	out=fopen(name,"w");
	if ( !out )
		return 3;

	*p = 0;

	fprintf(out,"/* XPM */\n");
	fprintf(out,"static	char	*%s[] = {\n",name);
	fprintf(out,"\"  64   %d   2   1\",\n",height);
	fprintf(out,"\". c #ffffff\",\n");
	fprintf(out,"\"# c #000000\",\n");

	while( fgets( buffer, 256, in ) )
	{
		p=strchr(buffer,'\n');
		if ( p )
			*p=0;
		fprintf(out,"\"");
		for( n=0, p=buffer; *p; p++, n++ )
			fprintf(out,"%c",*p==' '?'.':'#');
		for( ; n < 64; n++ )
			fprintf(out,".");
		height--;
		fprintf(out,"\"%s\n",height?",":"");
	}

	fprintf(out,"};\n");

	fclose(in);
	fclose(out);

	return 0;
}
