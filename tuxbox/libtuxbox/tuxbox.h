/*

  $Id: tuxbox.h,v 1.1 2003/01/04 23:38:46 waldi Exp $
 
  $Log: tuxbox.h,v $
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

#ifndef TUXBOX_H
#define TUXBOX_H

#ifdef __KERNEL__
#define TUXBOX_VERSION						KERNEL_VERSION(2,0,1)
#endif

#define TUXBOX_TAG_CAPABILITIES				"TUXBOX_CAPABILITIES"
#define TUXBOX_TAG_VENDOR					"TUXBOX_VENDOR"
#define TUXBOX_TAG_MODEL					"TUXBOX_MODEL"
#define TUXBOX_TAG_VERSION					"TUXBOX_VERSION"

#define TUXBOX_CAPABILITIES_IR_RC			0x00000001
#define TUXBOX_CAPABILITIES_IR_KEYBOARD		0x00000002
#define TUXBOX_CAPABILITIES_LCD				0x00000004
#define TUXBOX_CAPABILITIES_NETWORK			0x00000008
#define TUXBOX_CAPABILITIES_HDD				0x00000010
#define TUXBOX_CAPABILITIES_CAM_CI			0x00000020
#define TUXBOX_CAPABILITIES_CAM_EMBEDDED	0x00000040

#define	TUXBOX_VENDOR_UNKNOWN				0x00000000
#define	TUXBOX_VENDOR_NOKIA					0x00000001
#define	TUXBOX_VENDOR_SAGEM					0x00000002
#define	TUXBOX_VENDOR_PHILIPS				0x00000003
#define	TUXBOX_VENDOR_DREAM_MM				0x00000004

#define TUXBOX_MODEL_UNKNOWN				0x00000000
#define TUXBOX_MODEL_DBOX2					0x00000001
#define TUXBOX_MODEL_DREAMBOX_DM7000		0x00000002
#define TUXBOX_MODEL_DREAMBOX_DM5600		0x00000003

#ifndef __KERNEL__

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

#endif

#endif
