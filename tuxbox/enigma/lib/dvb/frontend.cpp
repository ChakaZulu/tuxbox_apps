#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "edvb.h"
#include "esection.h"
#include "frontend.h"
#include "decoder.h"

eFrontend* eFrontend::frontend;

eFrontend::eFrontend(int type, const char *demod, const char *sec): type(type)
{
	state=stateIdle;
	timer=new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(timeout()));
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
	} else
		secfd=-1;
		
	if (eDVB::getInstance()->config.getKey("/elitedvb/frontend/freqOffset", freq_offset))
	{
		freq_offset = 0;
		eDVB::getInstance()->config.setKey("/elitedvb/frontend/freqOffset", freq_offset);
	}

	qDebug("FreqOffset = %d", freq_offset);
	
	if (type==feCable)
	{
		lnbfreq_low=lnbfreq_hi=threshold=do_sec=0;
	} else
	{
		lnbfreq_low= 9750000;
		lnbfreq_hi= 10600000;
		threshold=	11700000;
		do_sec=1;
	}
	
	lastcsw=0;
}

void eFrontend::timeout()
{
	qDebug("status %x lock %d\n", Status(), Locked());
	if (Locked())
	{
		qDebug("+");
		state=stateIdle;
		emit tunedIn(transponder, 0);
	} else
		if (--tries)
		{
			qDebug("-: %x", Status());
			timer->start(100, true);
		} else
		{
			printf("couldn't lock. (state: %x)\n", Status());
			state=stateIdle;
			emit tunedIn(transponder, -ETIMEDOUT);
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
	int32_t strength=-1;
	ioctl(fd, FE_READ_SIGNAL_STRENGTH, &strength);
	return strength;
}

int eFrontend::SNR()
{
	int32_t snr=-1;
	ioctl(fd, FE_READ_SNR, &snr);
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
		int sat,								// diseqc satellite, &1 -> SAT_A/B, &2 -> OPT_A/B
		Modulation QAM)					// Modulation, QAM_xx
{
	FrontendParameters front;
	secCmdSequence seq;
	secCommand cmd;
	secDiseqcCmd diseqc;
	int hi;
	
	Decoder::Flush();
	eSection::abortAll();
	timer->stop();
	if (state==stateTuning)
	{
		state=stateIdle;
		if (transponder)
			emit tunedIn(transponder, -ECANCELED);
	}
	
	if (state==stateTuning)
		return -EBUSY;
	transponder=trans;

	diseqc.addr=0x10;
	diseqc.cmd=0x38;
	diseqc.numParams=1;
	cmd.type=SEC_CMDTYPE_DISEQC;
	
	if (Frequency>threshold)
	{
		front.Frequency=Frequency-lnbfreq_hi;
		seq.continuousTone = SEC_TONE_ON;
		hi=1;
	} else
	{
		front.Frequency=Frequency-lnbfreq_low;
		seq.continuousTone = SEC_TONE_OFF;
		hi=0;
	}
	
	front.Frequency+=freq_offset;
	front.Inversion=Inversion;

	diseqc.params[0]=0xF0;

	if (polarisation==polVert)
	{
		seq.voltage=SEC_VOLTAGE_13;
	} else if (polarisation==polHor)
	{
		diseqc.params[0]|=2;
		seq.voltage=SEC_VOLTAGE_18;
	} else
		qFatal("BLA was ist dass denn fuer eine pol.");
	
	if (hi)
		diseqc.params[0]|=1;
	diseqc.params[0]|=sat<<2;
	
	cmd.u.diseqc=diseqc;
	seq.miniCommand=SEC_MINI_NONE;

	ioctl(fd, FE_SET_POWER_STATE, FE_POWER_ON);

	if ((diseqc.params[0]^lastcsw))		// only when changing satellites or pol.
	{
		int changelnb=(diseqc.params[0]^lastcsw)&~3;
		
		lastcsw=diseqc.params[0];
		
		if (changelnb)
		{
			if (sat==0)
				seq.miniCommand=SEC_MINI_A;
			else if (sat==1)
				seq.miniCommand=SEC_MINI_B;
		} else
			seq.miniCommand=SEC_MINI_NONE;

		seq.numCommands=changelnb?1:0;
		seq.commands=&cmd;
		if (do_sec)
			if (ioctl(secfd, SEC_SEND_SEQUENCE, &seq)<0)
			{
				perror("SEC_SEND_SEQUENCE");
				return -1;
			}
	}

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
	qDebug("IF: %d %d", front.Frequency, seq.continuousTone);

	qDebug("ok, sec etc. done");
	if (ioctl(fd, FE_SET_FRONTEND, &front)<0)
	{
		perror("FE_SET_FRONTEND");
		return -1;
	}
	qDebug("<--- FE_SET_FRONTEND");
	state=stateTuning;
	tries=10; // 1.0 second timeout
	timer->start(50, true);
	qDebug("<-- tuned");
	return 0;
}

int eFrontend::tune_qpsk(eTransponder *transponder, 
		uint32_t Frequency, 		// absolute frequency in kHz
		int polarisation, 			// polarisation (polHor, polVert, ...)
		uint32_t SymbolRate, 		// symbolrate in symbols/s (e.g. 27500000)
		uint8_t FEC_inner,			// FEC_inner (-1 for none, 0 for auto, but please don't use that)
		int Inversion,	// spectral inversion, INVERSION_OFF / _ON / _AUTO (but please...)
		int sat)								// diseqc satellite, &1 -> SAT_A/B, &2 -> OPT_A/B
{
	return tune(transponder, Frequency, polarisation, SymbolRate, getFEC(FEC_inner), Inversion?INVERSION_ON:INVERSION_OFF, sat, QPSK);
}

int eFrontend::tune_qam(eTransponder *transponder, 
		uint32_t Frequency, 		// absolute frequency in kHz
		uint32_t SymbolRate, 		// symbolrate in symbols/s (e.g. 6900000)
		uint8_t FEC_inner, 			// FEC_inner (-1 for none, 0 for auto, but please don't use that). normally -1.
		int Inversion,	// spectral inversion, INVERSION_OFF / _ON / _AUTO (but please...)
		int QAM)					// Modulation, QAM_xx
{
	return tune(transponder, Frequency, 0, SymbolRate, getFEC(FEC_inner), Inversion?INVERSION_ON:INVERSION_OFF, 0, getModulation(QAM));
}
