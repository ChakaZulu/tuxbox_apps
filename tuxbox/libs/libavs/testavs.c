#include "libavs.h"

int main()
{
	if ( avsInit(1) != 0 )
	{
		return -1;
	}

	avsSetRoute( epTV, esVCR, emNONE );

	avsDeInit();
}
