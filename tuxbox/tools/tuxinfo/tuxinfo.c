#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libtuxbox.h>

int main(int argc, char **argv)
{

	unsigned char show_capabilities = 0;
	unsigned char show_vendor = 0;
	unsigned char show_vendor_str = 0;
	unsigned char show_model = 0;
	unsigned char show_model_str = 0;
	unsigned char show_version = 0;
	char *digit_format_str = "0x%08X\n";

	while (1) {
	
		int c;
		
		if ((c = getopt(argc, argv, "ciImMvd")) < 0)
			break;
			
		switch (c) {
		
			case 'c':
			
				show_capabilities++;
				
				break;

			case 'i':
			
				show_vendor++;
				
				break;

			case 'm':
			
				show_vendor_str++;
				
				break;

			case 'I':
			
				show_model++;
				
				break;

			case 'M':
			
				show_model_str++;
				
				break;

			case 'v':
			
				show_version++;
				
				break;
				
			case 'd':
			
				digit_format_str = "%d\n";
				
				break;

		}
		
	}

	if (show_capabilities)
		printf(digit_format_str, tuxbox_get_capabilities());

	if (show_vendor)
		printf(digit_format_str, tuxbox_get_vendor());

	if (show_vendor_str)
		printf("%s\n", tuxbox_get_vendor_str());

	if (show_model)
		printf(digit_format_str, tuxbox_get_model());

	if (show_model_str)
		printf("%s\n", tuxbox_get_model_str());

	if (show_version)
		printf(digit_format_str, tuxbox_get_version());

	return 0;
}
