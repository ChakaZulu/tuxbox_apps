/*

  $Id: tuxbox.h,v 1.2 2003/02/18 18:42:13 obi Exp $
 
  $Log: tuxbox.h,v $
  Revision 1.2  2003/02/18 18:42:13  obi
  #include <tuxbox/tuxbox.h>

  Revision 1.1  2003/01/04 23:38:46  waldi
  move libtuxbox

  Revision 1.4  2003/01/03 11:13:09  Jolt
  - Added tag defines
  - Renamed *manufacturer* to *vendor*
  - Added some caps

  Revision 1.3  2003/01/01 22:14:51  Jolt
  - Renamed files
  - Merged in tuxbox.h

  Revision 1.2  2003/01/01 22:00:02  Jolt
  Whooops ;)

  Revision 1.1  2003/01/01 21:30:10  Jolt
  Tuxbox info lib

*/

#ifndef LIBTUXBOX_H
#define LIBTUXBOX_H

#include <tuxbox/tuxbox.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned int tuxbox_get_capabilities(void);
unsigned int tuxbox_get_vendor(void);
char *tuxbox_get_vendor_str(void);
unsigned int tuxbox_get_model(void);
char *tuxbox_get_model_str(void);
unsigned int tuxbox_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBTUXBOX_H */
