#include "frontend.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <core/base/ebase.h>
#include <core/dvb/edvb.h>
#include <core/dvb/esection.h>
#include <core/dvb/decoder.h>
#include <core/system/econfig.h>

eFrontend* eFrontend::frontend;

eFrontend::eFrontend(int type, const char *demod, const char *sec): type(type)
{
	state=stateIdle;
	timer=new eTimer(eApp);

	CONNECT(timer->timeout, eFrontend::timeout);
	fd=::open(demod, O_RDWR);
	if (fd<0)
	{
		perror(demod);
		return;
	}
	if (type==feSatellite)
	{
		secfd=::open(sec, O_RDWR);
		if (secfd<0)
		{
			perror(sec);
			return;
		}

		ioctl(fd, FE_SET_POWER_STATE, FE_POWER_ON);

		secCmdSequence seq;
		secCommand cmd;
		secDiseqcCmd DiSEqC;
		DiSEqC.addr=0;
		DiSEqC.cmd=0;
		DiSEqC.numParams=0;
 		cmd.type = SEC_CMDTYPE_DISEQC;
		cmd.u.diseqc=DiSEqC;
		seq.miniCommand=SEC_MINI_NONE;
		seq.numCommands=1;
		seq.commands=&cmd;
		seq.voltage=SEC_VOLTAGE_13;
		seq.continuousTone=SEC_TONE_OFF;
		if (ioctl(secfd, SEC_SEND_SEQUENCE, &seq)<0)
		{
			perror("SEC_SEND_SEQUENCE");
			exit(0);
		}
	} else
		secfd=-1;
		
	lastcsw=0;
}

void eFrontend::timeout()
{
	if (Locked())
	{
	  eDebug("+");
		state=stateIdle;
		/*emit*/ tunedIn(transponder, 0);
	} else
		if (--tries)
		{
			eDebug("-: %x", Status());
			timer->start(100, true);
		} else
		{
			eDebug("couldn't lock. (state: %x)", Status());
			state=stateIdle;
			/*emit*/ tunedIn(transponder, -ETIMEDOUT);
		}
}

eFrontend::~eFrontend()
{
	if (fd>=0)
		::close(fd);
	if (secfd>=0)
		::close(secfd);
	frontend=0;
}

int eFrontend::Status()
{
	if (fd<0)
		return fd;
	if ((type!=feCable) && (secfd<0))
		return secfd;
	FrontendStatus status=0;
	ioctl(fd, FE_READ_STATUS, &status);
	return status;
}
 
uint32_t eFrontend::BER()
{
	uint32_t ber=0;
	ioctl(fd, FE_READ_BER, &ber);
	return ber;
}

int eFrontend::SignalStrength()
{
	uint16_t strength=0;
	ioctl(fd, FE_READ_SIGNAL_STRENGTH, &strength);
#if 0
	if ((strength<0) || (strength>65535))
	{
		eWarning("buggy SignalStrength driver (or old version) (%08x)", strength);
		strength=0;
	}
#endif
	return strength;
}

int eFrontend::SNR()
{
	uint16_t snr=0;
	ioctl(fd, FE_READ_SNR, &snr);
#if 0
	if ((snr<0) || (snr>65535))
	{
		eWarning("buggy SNR driver (or old version) (%08x)", snr);
		snr=0;
	}
#endif
	return snr;
}

uint32_t eFrontend::UncorrectedBlocks()
{
	uint32_t ublocks=0;
	ioctl(fd, FE_READ_UNCORRECTED_BLOCKS, &ublocks);
	return ublocks;
}

uint32_t eFrontend::NextFrequency()
{
	uint32_t ublocks=0;
	ioctl(fd, FE_GET_NEXT_FREQUENCY, &ublocks);
	return ublocks;
}

static CodeRate getFEC(int fec)		// etsi -> api
{
	switch (fec)
	{
	case -1:
	case 15:
		return FEC_NONE;
	case 0:
		return FEC_AUTO;
	case 1:
		return FEC_1_2;
	case 2:
		return FEC_2_3;
	case 3:
		return FEC_3_4;
	case 4:
		return FEC_5_6;
	case 5:
		return FEC_7_8;
	default:
		return FEC_AUTO;
	}
}

static Modulation getModulation(int mod)
{
	switch (mod)
	{
	case 1:
		return QAM_16;
	case 2:
		return QAM_32;
	case 3:
		return QAM_64;
	case 4:
		return QAM_128;
	case 5:
		return QAM_256;
	default:
		return QAM_64;
	}
}

int eFrontend::tune(eTransponder *trans,
		uint32_t Frequency, 		// absolute frequency in kHz
		int polarisation, 			// polarisation (polHor, polVert, ...)
		uint32_t SymbolRate, 		// symbolrate in symbols/s (e.g. 27500000)
		CodeRate FEC_inner,			// FEC_inner api
		SpectralInversion Inversion,	// spectral inversion, INVERSION_OFF / _ON / _AUTO (but please...)
		eLNB *lnb,
		eSwitchParameter *swParams,
		Modulation QAM)					// Modulation, QAM_xx
{
	FrontendParameters front;
	secCmdSequence seq;
	secCommand cmd;
	secDiseqcCmd DiSEqC;
	int hi;
	
	Decoder::Flush();
	eSection::abortAll();
	timer->stop();
	if (state==stateTuning)
	{
		state=stateIdle;
		if (transponder)
			/*emit*/ tunedIn(transponder, -ECANCELED);
	}
	
	if (state==stateTuning)
		return -EBUSY;
	transponder=trans;

	if (lnb)
	{
		if ( swParams->HiLoSignal == eSwitchParameter::ON || ( swParams->HiLoSignal == eSwitchParameter::HILO && Frequency > lnb->getLOFThreshold() ) )
	 	{
			front.Frequency=Frequency-lnb->getLOFHi();
			seq.continuousTone = SEC_TONE_ON;
			hi=1;
		} else // swParams->hiloSignal == wSwitchParameter::OFF
		{
			front.Frequency=Frequency-lnb->getLOFLo();
			seq.continuousTone = SEC_TONE_OFF;
			hi=0;
		}
		DiSEqC.addr=0x10;
		DiSEqC.cmd=0x38;
		DiSEqC.numParams=1;
		DiSEqC.params[0]=0xF0;

		if ( lnb->getDiSEqC().DiSEqCMode == eDiSEqC::MINI )
			cmd.type = SEC_MINI_NONE;
		else
			cmd.type = SEC_CMDTYPE_DISEQC;

		if ( swParams->VoltageMode == eSwitchParameter::_14V || ( polarisation == polVert && swParams->VoltageMode == eSwitchParameter::HV )  )
		{
			seq.voltage=SEC_VOLTAGE_13;
		} else if ( swParams->VoltageMode == eSwitchParameter::_18V || ( polarisation==polHor && swParams->VoltageMode == eSwitchParameter::HV)  )
		{
			DiSEqC.params[0] |= 2;
			seq.voltage=SEC_VOLTAGE_18;
		} else
			eDebug("BLA was ist dass denn fuer eine pol.");

		if (hi)
			DiSEqC.params[0] |= 1;

		DiSEqC.params[0] |= lnb->getDiSEqC().DiSEqCParam<<2;

		cmd.u.diseqc=DiSEqC;

		if ((DiSEqC.params[0] ^ lastcsw))		// only when changing satellites or pol.
		{
#ifdef SPAUN_NOT_WORKING_BUT_FASTER_ZAP
			int changelnb=(DiSEqC.params[0]^lastcsw)&~3;
#else
			int changelnb=1;
#endif
			lastcsw=DiSEqC.params[0];
		
			if (changelnb && lnb->getDiSEqC().DiSEqCMode == eDiSEqC::MINI )  // Mini DiSEqC
			{
				if ( lnb->getDiSEqC().DiSEqCParam == eDiSEqC::AA )
					seq.miniCommand=SEC_MINI_A;
				else
					seq.miniCommand=SEC_MINI_B;
				seq.commands=0;
				seq.numCommands=0;
			}
			else
			{
				seq.miniCommand=SEC_MINI_NONE;
				seq.numCommands=changelnb ? 1 : 0;
				seq.commands=&cmd;
			}

			if (ioctl(secfd, SEC_SEND_SEQUENCE, &seq)<0)
			{
				perror("SEC_SEND_SEQUENCE");
				return -1;
			}
		}
	} else
	{
		eDebug("no lnb");
		front.Frequency=Frequency;
	}

	front.Inversion=Inversion;

	switch (type)
	{
	case feCable:
		front.u.qam.SymbolRate=SymbolRate;
		front.u.qam.FEC_inner=FEC_inner;
		front.u.qam.QAM=QAM;
		break;
	case feSatellite:
		front.u.qpsk.SymbolRate=SymbolRate;
		front.u.qpsk.FEC_inner=FEC_inner;
		break;
	}
	if (ioctl(fd, FE_SET_FRONTEND, &front)<0)
	{
		perror("FE_SET_FRONTEND");
		return -1;
	}
	state=stateTuning;
	tries=10; // 1.0 second timeout
	timer->start(50, true);
	return 0;
}

int eFrontend::tune_qpsk(eTransponder *transponder, 
		uint32_t Frequency, 		// absolute frequency in kHz
		int polarisation, 			// polarisation (polHor, polVert, ...)
		uint32_t SymbolRate, 		// symbolrate in symbols/s (e.g. 27500000)
		uint8_t FEC_inner,			// FEC_inner (-1 for none, 0 for auto, but please don't use that)
		int Inversion,					// spectral inversion, INVERSION_OFF / _ON / _AUTO (but please...)
		eLNB &lnb,							// DiSEqC satellite, &1 -> SAT_A/B, &2 -> OPT_A/B
		eSwitchParameter &swParams) // 22Khz(hi/lo, on, off), minidiseq(true,false), DiSEqCParameter(A/A, A/B, B/A, B/B), Voltage ( h/v, 14, 18 )
{
	return tune(transponder, Frequency, polarisation, SymbolRate, getFEC(FEC_inner), Inversion?INVERSION_ON:INVERSION_OFF, &lnb, &swParams, QPSK);
}

int eFrontend::tune_qam(eTransponder *transponder, 
		uint32_t Frequency, 		// absolute frequency in kHz
		uint32_t SymbolRate, 		// symbolrate in symbols/s (e.g. 6900000)
		uint8_t FEC_inner, 			// FEC_inner (-1 for none, 0 for auto, but please don't use that). normally -1.
		int Inversion,	// spectral inversion, INVERSION_OFF / _ON / _AUTO (but please...)
		int QAM)					// Modulation, QAM_xx
{
	return tune(transponder, Frequency, 0, SymbolRate, getFEC(FEC_inner), Inversion?INVERSION_ON:INVERSION_OFF, 0, 0, getModulation(QAM));
}
