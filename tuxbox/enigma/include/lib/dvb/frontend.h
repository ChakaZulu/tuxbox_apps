#ifndef __FRONTEND_H
#define __FRNOTEND_H

/*
	 this handles all kind of frontend activity including
	 sec etc.
*/

#include <stdlib.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/video.h>

#include <lib/base/ebase.h>
#include <lib/base/estring.h>

class eTransponder;
class eSatellite;
class eSwitchParameter;

/**
 * \brief A frontend, delivering TS.
 *
 * A frontend is something like a tuner. You can tune to a transponder (or channel, as called with DVB-C).
 */
class eFrontend: public Object
{
	int type;
	int fd;

	int lastcsw,
			lastToneBurst,
			lastRotorCmd,
			lastSmatvFreq,
			curRotorPos;    // current Orbital Position
      
	enum { stateIdle, stateTuning };
	int state;
	eTransponder *transponder;
	eFrontend(int type, const char *demod="/dev/dvb/adapter0/frontend0");
	static eFrontend *frontend;
	eTimer *timer, timer2;
	int tries;
	int tune(eTransponder *transponder, 
			uint32_t Frequency, int polarisation,
			uint32_t SymbolRate, fe_code_rate_t FEC_inner,
			fe_spectral_inversion_t Inversion, eSatellite* sat, fe_modulation_t QAM);

	void timeout();
/*
	int RotorUseTimeout(secCmdSequence& seq, int newPos, double DegPerSec );
	int RotorUseInputPower(secCmdSequence& seq, void *commands, int seqRepeat, int DeltaA, int newPos );
	*/
	int noRotorCmd;
public:
//	double calcAzimuth( double Longitude, double Latitude, int OrbitalPos );
	void disableRotor() { noRotorCmd = 1, lastcsw=0, lastRotorCmd=0; }  // no more rotor cmd is sent when tune
	void enableRotor() { noRotorCmd = 0, lastcsw=0, lastRotorCmd=0; }  // rotor cmd is sent when tune
	int sendDiSEqCCmd( int addr, int cmd, eString params="", int frame=0xE0 );

	Signal1<void, int> rotorRunning;
	Signal0<void> rotorStopped, rotorTimeout;
	Signal2<void, eTransponder*, int> tunedIn;
	~eFrontend();

	enum
	{
		feSatellite=0, feCable, feTerrestrical
	};

	static int open(int type) { if (!frontend) frontend=new eFrontend(type); if (frontend->fd<0) { close(); return frontend->fd; } return 0; }
	static void close() { delete frontend; }
	static eFrontend *getInstance() { return frontend; }

	int Type() { return type; }
	
	int Status();
	int Locked() { return Status()&FE_HAS_LOCK; }
	void Reset();
	void readInputPower();
     	
	uint32_t BER();
	/**
	 * \brief Returns the signal strength (or AGC).
	 *
	 * Range is 0..65535 in linear scale.
	 */
	int SignalStrength();
	/**
	 * \brief Returns the signal-to-noise ratio.
	 *
	 * Range is 0..65535 in linear scale.
	 */
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
			eSatellite &sat);       // complete satellite data... diseqc.. lnb ..switch

	int tune_qam(eTransponder *transponder,
			uint32_t Frequency, 		// absolute frequency in kHz
			uint32_t SymbolRate, 		// symbolrate in symbols/s (e.g. 6900000)
			uint8_t FEC_inner, 			// FEC_inner according to ETSI (-1 for none, 0 for auto, but please don't use that). normally -1.
			int inversion,					// spectral inversion on(1)/off(0)
			int QAM);								// Modulation according to etsi (1=QAM16, ...)

	int freq_offset;
};


#endif
