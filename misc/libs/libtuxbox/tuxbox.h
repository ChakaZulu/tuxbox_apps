/*

  $Id: tuxbox.h,v 1.2 2003/01/01 22:00:02 Jolt Exp $
 
  $Log: tuxbox.h,v $
  Revision 1.2  2003/01/01 22:00:02  Jolt
  Whooops ;)

  Revision 1.1  2003/01/01 21:30:10  Jolt
  Tuxbox info lib



*/
#ifndef __libtuxbox__
#define __libtuxbox__


#ifdef __cplusplus
extern "C" {
#endif

unsigned int tuxbox_get_capabilities(void);
unsigned int tuxbox_get_manufacturer(void);
char *tuxbox_get_manufacturer_str(void);
unsigned int tuxbox_get_model(void);
char *tuxbox_get_model_str(void);
unsigned int tuxbox_get_version(void);

#ifdef __cplusplus
}
#endif


#endif
