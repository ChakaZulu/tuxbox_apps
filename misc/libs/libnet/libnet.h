#ifndef __libnet__
#define __libnet__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


extern  int  netSetIP( char *dev, char *ip, char *mask, char *brdcast );
extern  void netGetIP( char *dev, char *ip, char *mask, char *brdcast );
extern  void netSetDefaultRoute( char *gw );



#ifdef __cplusplus
}
#endif


#endif
