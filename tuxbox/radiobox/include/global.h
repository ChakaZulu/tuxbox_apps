#ifndef __global_h__
#define __global_h__

enum PlayOrder 
{
	PO_Random,
	PO_Normal
};

struct __setup
{
	PlayOrder	play_order;
	
	__setup()
	{
		play_order = PO_Random;
	};

};

#ifndef DEFGLOB
extern  __setup globals;
#endif

#endif /* __global_h__ */
