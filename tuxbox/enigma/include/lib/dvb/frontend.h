#ifndef __FRONTEND_H
#define __FRONTEND_H

/*
	 this handles all kind of frontend activity including
	 sec etc.
*/

#include <config.h>
#include <stdlib.h>

#if HAVE_DVB_API_VERSION < 3
#include <ost/dmx.h>
#include <ost/frontend.h>
#include <ost/sec.h>
#include <ost/video.h>
#define DEMOD_DEV "/dev/dvb/card0/frontend0"
#define SEC_DEV "/dev/dvb/card0/sec0"
#else
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/video.h>
#define DEMOD_DEV "/dev/dvb/adapter0/frontend0"
#define SEC_DEV "/dev/dvb/adapter0/sec0"
#define CodeRate fe_code_rate_t
#define SpectralInversion fe_spectral_inversion_t
#define Modulation fe_modulation_t
#endif

#include <lib/base/ebase.h>
#include <lib/base/estring.h>

class eLNB;
class eTransponder;
class eSatellite;
class eSwitchParameter;

// DiSEqC Command Sequence Wrapper... for Multi API compatibility

struct eSecCmdSequence
{
	enum { TONE_OFF=SEC_TONE_OFF, TONE_ON=SEC_TONE_ON };

#if HAVE_DVB_API_VERSION < 3
	enum { VOLTAGE_OFF=SEC_VOLTAGE_OFF, VOLTAGE_13=SEC_VOLTAGE_13, VOLTAGE_18=SEC_VOLTAGE_18 };
	enum { NONE=SEC_MINI_NONE, TONEBURST_A=SEC_MINI_A, TONEBURST_B=SEC_MINI_B };
	secCommand *commands;
#else
	enum {VOLTAGE_13=SEC_VOLTAGE_13, VOLTAGE_18=SEC_VOLTAGE_18, VOLTAGE_OFF };
	enum {TONEBURST_A=SEC_MINI_A, TONEBURST_B=SEC_MINI_B, NONE };
	dvb_diseqc_master_cmd *commands;
#endif
	int numCommands;
	int toneBurst;
	int voltage;
	int continuousTone;
	bool increasedVoltage;
};

/**
 * \brief A frontend, delivering TS.
 *
 * A frontend is something like a tuner. You can tune to a transponder (or channel, as called with DVB-C).
 */
class eFrontend: public Object
{
	int type,
			fd,
#if HAVE_DVB_API_VERSION < 3
			secfd,
#else
			curContTone,
			curVoltage,
#endif
			needreset,
			lastcsw,
			lastucsw,
			lastToneBurst,
			lastRotorCmd,
			lastSmatvFreq,
			curRotorPos;    // current Orbital Position

	eLNB *lastLNB;
         
	enum { stateIdle, stateTuning, stateDiSEqC } state;
	eTransponder *transponder;
	eFrontend(int type, const char *demod=DEMOD_DEV, const char *sec=SEC_DEV);
	static eFrontend *frontend;
	eTimer *timer, timer2, rotorTimer1, rotorTimer2;
	int tries, noRotorCmd;
	int tune(eTransponder *transponder, 
			uint32_t Frequency, int polarisation,
			uint32_t SymbolRate,
			CodeRate FEC_inner,
			SpectralInversion Inversion, eSatellite* sat, Modulation QAM);
	Signal1<void, eTransponder*> tpChanged;
// ROTOR INPUTPOWER
	timeval rotorTimeout;
	int idlePowerInput;
	int runningPowerInput;
	int newPos;
// Non blocking rotor turning
	int DeltaA,
			voltage,
			increased;
///////////////////
	void timeout();
	int RotorUseTimeout(eSecCmdSequence& seq, eLNB *lnb);
	int RotorUseInputPower(eSecCmdSequence& seq, eLNB *lnb);
	void RotorStartLoop();
	void RotorRunningLoop();
	void RotorFinish(bool tune=true);
	int SendSequence( const eSecCmdSequence &seq );
	void checkLock();
	void updateTransponder( eTransponder * );
public:
	void disableRotor() { noRotorCmd = 1, lastRotorCmd=-1; } // no more rotor cmd is sent when tune
	void enableRotor() { noRotorCmd = 0, lastRotorCmd=-1; }  // rotor cmd is sent when tune
	int sendDiSEqCCmd( int addr, int cmd, eString params="", int frame=0xE0 );

	Signal1<void, int> s_RotorRunning;
	Signal0<void> s_RotorStopped, s_RotorTimeout;
	Signal2<void, eTransponder*, int> tunedIn;
	~eFrontend();

	static int open(int type)
	{
		if (!frontend)
			frontend=new eFrontend(type);
		if (frontend->fd<0)
		{
			close();
			return frontend->fd;
		}
		return 0;
	}

	static void close()	{		delete frontend;	}

	static eFrontend *getInstance() { return frontend; }

	int Type() { return type; }

	int Status();
	int Locked() { return Status()&FE_HAS_LOCK; }
	void InitDiSEqC();
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

		// switches of as much as possible.
	int savePower();
};


#endif
