/*

  $Id: tuxbox.h,v 1.3 2003/01/01 22:14:51 Jolt Exp $
 
  $Log: tuxbox.h,v $
  Revision 1.3  2003/01/01 22:14:51  Jolt
  - Renamed files
  - Merged in tuxbox.h

  Revision 1.2  2003/01/01 22:00:02  Jolt
  Whooops ;)

  Revision 1.1  2003/01/01 21:30:10  Jolt
  Tuxbox info lib

*/

#ifndef TUXBOX_H
#define TUXBOX_H

#ifdef __KERNEL__
#define TUXBOX_VERSION						KERNEL_VERSION(1,0,1)
#endif

#define TUXBOX_CAPABILITIES_IR_RC			0x00000001
#define TUXBOX_CAPABILITIES_IR_KEYBOARD		0x00000002
#define TUXBOX_CAPABILITIES_LCD				0x00000004
#define TUXBOX_CAPABILITIES_NETWORK			0x00000008
#define TUXBOX_CAPABILITIES_HDD				0x00000010

#define	TUXBOX_MANUFACTURER_UNKNOWN			0x0000
#define	TUXBOX_MANUFACTURER_NOKIA			0x0001
#define	TUXBOX_MANUFACTURER_SAGEM			0x0002
#define	TUXBOX_MANUFACTURER_PHILIPS			0x0003
#define	TUXBOX_MANUFACTURER_DREAM_MM		0x0004

#define TUXBOX_MODEL_UNKNOWN				0x0000
#define TUXBOX_MODEL_DBOX2					0x0001
#define TUXBOX_MODEL_DREAMBOX_DM7000		0x0002
#define TUXBOX_MODEL_DREAMBOX_DM5600		0x0003

#ifndef __KERNEL__

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

#endif
