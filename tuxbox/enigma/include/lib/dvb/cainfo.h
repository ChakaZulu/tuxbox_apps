#ifndef __cainof_h
#define __cainfo_h

#define NUMCASYSTEMS 8
typedef struct
	{
		unsigned char status;
		unsigned char caID;
	} t_CASystems;

class eCAInfo
{
public:
	static void saveCAIDsInfo(__u8 *);
	static t_CASystems *getPtr2CASystems(void);
	static unsigned int NumberOfCASystems(void);
};
#endif
