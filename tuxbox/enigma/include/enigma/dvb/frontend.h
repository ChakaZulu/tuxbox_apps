#ifndef __FRONTEND_H
#define __FRNOTEND_H

/*
	 this handles all kind of frontend activity including
	 sec etc.
*/

//#include <qobject.h>
#include <qtimer.h>
#include <stdlib.h>

#include <ost/dmx.h>
#include <ost/frontend.h>
#include <ost/sec.h>
#include <ost/video.h>

class eTransponder;

class eFrontend: public /*Q*/Object
{
//	Q_OBJECT
	uint32_t lnbfreq_low, lnbfreq_hi, threshold;
	int do_sec, type;
	int fd, secfd;
	int lastcsw;
	enum { stateIdle, stateTuning };
	int state;
	eTransponder *transponder;
	eFrontend(int type, const char *demod="/dev/ost/frontend0", const char *sec="/dev/ost/sec0");
	static eFrontend *frontend;
	QTimer *timer;
	int tries;
	int tune(eTransponder *transponder, 
			uint32_t Frequency, int polarisation,
			uint32_t SymbolRate, CodeRate FEC_inner,
			SpectralInversion Inversion, int sat,
			Modulation QAM);
private:/* slots:*/
	void timeout();
/*signals:
	void tunedIn(eTransponder *trans, int error);*/
public:
	Signal2<void, eTransponder*, int> tunedIn;
	~eFrontend();

	enum
	{
		feSatellite=0, feCable
	};

	static int open(int type) { if (!frontend) frontend=new eFrontend(type); if (frontend->fd<0) { close(); return frontend->fd; } return 0; }
	static void close() { delete frontend; }
	static eFrontend *fe() { return frontend; }
	
	int Type() { return type; }
	
	int Status();
	int Locked() { return Status()&FE_HAS_LOCK; }
	
	uint32_t BER();
	int SignalStrength();
	int SNR();
	uint32_t UncorrectedBlocks();
	uint32_t NextFrequency();
	enum
	{
		polHor=0, polVert, polLeft, polRight
	};
	/** begins the tune operation and emits a "tunedIn"-signal */
	int tune_qpsk(eTransponder *transponder, 
			uint32_t Frequency, 		// absolute frequency in kHz
			int polarisation, 			// polarisation (polHor, polVert, ...)
			uint32_t SymbolRate, 		// symbolrate in symbols/s (e.g. 27500000)
			uint8_t FEC_inner,			// FEC_inner according to ETSI (-1 for none, 0 for auto, but please don't use that)
			int Inversion,					// spectral invesion on(1)/off(0)
			int sat);								// diseqc satellite, &1 -> SAT_A/B, &2 -> OPT_A/B
	int tune_qam(eTransponder *transponder, 
			uint32_t Frequency, 		// absolute frequency in kHz
			uint32_t SymbolRate, 		// symbolrate in symbols/s (e.g. 6900000)
			uint8_t FEC_inner, 			// FEC_inner according to ETSI (-1 for none, 0 for auto, but please don't use that). normally -1.
			int inversion,					// spectral inversion on(1)/off(0)
			int QAM);								// Modulation according to etsi (1=QAM16, ...)

	int freq_offset;
};


#endif
