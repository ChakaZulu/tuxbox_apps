#ifndef __SRC_RDS_TEXT_H_
#define __SRC_RDS_TEXT_H_

#include <lib/base/ebase.h>
#include <lib/base/estring.h>

class RDSTextDecoder : public Object
{
	int bytesread, ptr, p1, p2;
	unsigned char buf[128], message[66], leninfo, text_len, m_ptr, state;
	unsigned short crc16, crc;
	eSocketNotifier *sn;
	void process_data(int);
public:
	RDSTextDecoder();
	~RDSTextDecoder();
	Signal1<void, eString> textReady;
};

#endif
