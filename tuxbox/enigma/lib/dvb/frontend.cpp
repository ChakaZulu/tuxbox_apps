#include <lib/dvb/frontend.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <lib/base/ebase.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/esection.h>
#include <lib/dvb/decoder.h>
#include <lib/system/econfig.h>

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
		seq.numCommands=0;
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
  lastRotorCmd=-1;
  lastSmatvFreq=-1;
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
	eDebug("SNR:%d",snr);

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
		eSatellite* sat,
		Modulation QAM)					// Modulation, QAM_xx
{
  FrontendParameters front;
  Decoder::Flush();
	eSection::abortAll();
	timer->stop();
  eLNB *lnb = sat->getLNB();
  eSwitchParameter &swParams = sat->getSwitchParams();
  
  if (state==stateTuning)
	{
		state=stateIdle;
		if (transponder)
			/*emit*/ tunedIn(transponder, -ECANCELED);
	}
	
	if (state==stateTuning)
		return -EBUSY;

  transponder=trans;

	if (lnb)   // then we must do Satellite Stuff
	{
    // Variables to detect if DiSEqC must sent .. or not
    int csw = lnb->getDiSEqC().DiSEqCParam,
        RotorCmd=-1;
//        SmatvFreq=-1

    int sendSeq = 1;
    
    secCmdSequence seq;
    memset( &seq, 0, sizeof( seq ) );    

    int cmdCount=0;

    if (csw <= eDiSEqC::BA)
    {
      csw = 0xF0 | ( csw << 2 );
      if ( polarisation==polHor )
        csw |= 2;  // Horizontal
        
      if ( Frequency > lnb->getLOFThreshold() )
        csw |= 1;   // 22 Khz enabled
    }
    eDebug("csw = %04x", csw);
    //else we sent directly the cmd 0xF0..0xFF

    // Rotor Support
    if ( lnb->getDiSEqC().DiSEqCMode == eDiSEqC::V1_2 )
    {           
      if ( lnb->getDiSEqC().uncommitted_gap ) // the we add 2 * repeats + 1 + 1;
        cmdCount = ( lnb->getDiSEqC().DiSEqCRepeats << 1 ) + 2;
      else // then we add repeats + 1 + 1
        cmdCount = lnb->getDiSEqC().DiSEqCRepeats + 2;

      // allocate memory for all DiSEqC commands
      seq.commands = new secCommand[cmdCount];

      seq.commands[cmdCount-1].type = SEC_CMDTYPE_DISEQC_RAW;      
      seq.commands[cmdCount-1].u.diseqc.addr=0x31;     // normal positioner
      seq.commands[cmdCount-1].u.diseqc.cmdtype=0xE0;  // no replay... first transmission

      if ( lnb->getDiSEqC().useGotoXX )
      {
        seq.commands[cmdCount-1].u.diseqc.cmd=0x6E;      // goto xx // Drive Motor to Angular Position
     		seq.commands[cmdCount-1].u.diseqc.numParams=2;
     		seq.commands[cmdCount-1].u.diseqc.params[0]=0x01;
     		seq.commands[cmdCount-1].u.diseqc.params[1]=0x33;
        RotorCmd=( (seq.commands[cmdCount-1].u.diseqc.params[0] << 8) | (seq.commands[cmdCount-1].u.diseqc.params[1] ) );
      }
      else
      {
        
      }
      //  if ( gotoX Function )
      //  {
      //     use orbital Position *g*
      //  }
      //  else
      //  {
      //    drive to in rotor saved position
      //  }
    }
/*    if ( lnb->getDiSEqC().DiSEqCMode == eDiSEqC::SMATV )
    {
      if ( uncommitted ) // the we add 2 * repeats + 1 + 1;
        cmdCount = ( lnb->getDiSEqC().DiSEqCRepeats << 1 ) + 2;
      else // then we add repeats + 1 + 1
        cmdCount = lnb->getDiSEqC().DiSEqCRepeats + 2;

      // allocate memory for all DiSEqC commands
      seq.commands = new secCommand[cmdCount];

      seq.commands[cmdCount-1].type = SEC_CMDTYPE_DISEQC_RAW;
    	seq.commands[cmdCount-1].u.diseqc.addr = 0x71;	// intelligent slave interface for multi-master bus
    	seq.commands[cmdCount-1].u.diseqc.cmd = 0x58;	  // write channel frequency
      seq.commands[cmdCount-1].u.diseqc.cmdtype = 0xE0;
    	seq.commands[cmdCount-1].u.diseqc.numParams = 3;
    	seq.commands[cmdCount-1].u.diseqc.params[0] = (((Frequency / 10000000) << 4) & 0xF0) | ((Frequency / 1000000) & 0x0F);
    	seq.commands[cmdCount-1].u.diseqc.params[1] = (((Frequency / 100000) << 4) & 0xF0) | ((Frequency / 10000) & 0x0F);
    	seq.commands[cmdCount-1].u.diseqc.params[2] = (((Frequency / 1000) << 4) & 0xF0) | ((Frequency / 100) & 0x0F);
      SmatvFreq=Frequency;
    }*/
    if ( lnb->getDiSEqC().DiSEqCMode >= eDiSEqC::V1_0 )
    {
      int loops;
      if ( cmdCount )
        loops = cmdCount-1;  // not change always setted rotor or smatv command
      else
      {
        if ( lnb->getDiSEqC().uncommitted_gap ) // then we add 2 * repeats + 1;
          cmdCount = ( lnb->getDiSEqC().DiSEqCRepeats << 1 ) + 1;
        else // then we add repeats + 1
          cmdCount = lnb->getDiSEqC().DiSEqCRepeats + 1;

        // allocate memory for all DiSEqC commands
        seq.commands = new secCommand[cmdCount];
        loops = cmdCount;
      }
      
      for ( int i = 0; i < loops;)
      {
        seq.commands[i].type = SEC_CMDTYPE_DISEQC_RAW;
        seq.commands[i].u.diseqc.addr=0x10;
        if ( lnb->getDiSEqC().uncommitted_switch )
          seq.commands[i].u.diseqc.cmd=0x39;          // uncomitted switch
        else
          seq.commands[i].u.diseqc.cmd=0x38;          // comitted switch        
   			seq.commands[i].u.diseqc.cmdtype=i?0xE1:0xE0;
     		seq.commands[i].u.diseqc.numParams=1;
     		seq.commands[i].u.diseqc.params[0]=csw;
        i++;        
        if ( i < loops && lnb->getDiSEqC().uncommitted_gap )
        {
          seq.commands[i].type = SEC_CMDTYPE_DISEQC_RAW;
          seq.commands[i].u.diseqc.addr=0x10;
       		seq.commands[i].u.diseqc.cmd=0x39;          // uncommitted switch
   		  	seq.commands[i].u.diseqc.cmdtype=0xE1;      // repeated transmission
     		  seq.commands[i].u.diseqc.numParams=1;
       		seq.commands[i].u.diseqc.params[0]=csw;     // 22 Khz enabled
          i++;                
        }
      }
    }
    else // no DiSEqC
    {
      eDebug("no DiSEqC");
      seq.commands=0;
      sendSeq |= 2;
    }

    if ( lnb->getDiSEqC().MiniDiSEqCParam != eDiSEqC::NO )
    {
			if ( lnb->getDiSEqC().MiniDiSEqCParam == eDiSEqC::A )
				seq.miniCommand=SEC_MINI_A;
			else
        eDebug("SEND MINI DISEQC B");        
    }
    else  // do not send minidiseqc
      seq.miniCommand = SEC_MINI_NONE;

    seq.numCommands=cmdCount;
    eDebug("cmdCount = %d", cmdCount);

    if ( csw != lastcsw )
      lastcsw = csw;
    else if ( lastRotorCmd != RotorCmd )
      lastRotorCmd = RotorCmd;
/*    else if ( lastSmatvFreq != SmatvFreq )
      lastSmatvFreq = SmatvFreq;*/
    else
      sendSeq &= ~1;
      
    // no DiSEqC related Stuff
    
    // calc Frequency
    int local = Frequency - ( Frequency > lnb->getLOFThreshold() ? lnb->getLOFHi() : lnb->getLOFLo() );
    front.Frequency = local > 0 ? local : -local;
    
    // set Continuous Tone ( 22 Khz... low - high band )
    if ( (swParams.HiLoSignal == eSwitchParameter::ON)
    || ( (swParams.HiLoSignal == eSwitchParameter::HILO) && (Frequency > lnb->getLOFThreshold()) ) )
    {
  		seq.continuousTone = SEC_TONE_ON;
    }
  	else // swParams.hiloSignal == wSwitchParameter::OFF
  		seq.continuousTone = SEC_TONE_OFF;

    // set Voltage( 0/14/18V  vertical/horizontal )
    if ( swParams.VoltageMode == eSwitchParameter::_0V )
      seq.voltage = SEC_VOLTAGE_OFF;
    else if ( swParams.VoltageMode == eSwitchParameter::_14V || ( polarisation == polVert && swParams.VoltageMode == eSwitchParameter::HV )  )
      seq.voltage = (swParams.increased_voltage) ? SEC_VOLTAGE_13_5 : SEC_VOLTAGE_13;
  	else if ( swParams.VoltageMode == eSwitchParameter::_18V || ( polarisation==polHor && swParams.VoltageMode == eSwitchParameter::HV)  )
      seq.voltage =  (swParams.increased_voltage) ? SEC_VOLTAGE_18_5 : SEC_VOLTAGE_18;
      
    if ( sendSeq && ioctl(secfd, SEC_SEND_SEQUENCE, &seq) < 0 )
 		{
 			perror("SEC_SEND_SEQUENCE");
 			return -1;
  	}
    else if ( lnb->getDiSEqC().SeqRepeat )
    {
      usleep( 100000 ); // between seq repeats we wait 100ms
      ioctl(secfd, SEC_SEND_SEQUENCE, &seq);      
    }

    if (cmdCount)
      delete [] seq.commands;
 	}
  else  // we have only a cable box
    eDebug("no valid LNB... Cable Box ?");
  
 	front.Inversion=Inversion;

  switch (type)
 	{
   	case feCable:
      eDebug("Cable Frontend detected");
      front.Frequency = Frequency;
      front.u.qam.SymbolRate=SymbolRate;
  		front.u.qam.FEC_inner=FEC_inner;
  		front.u.qam.QAM=QAM;
  		break;
  	case feSatellite:
      eDebug("Sat Frontend detected");
      front.u.qpsk.SymbolRate=SymbolRate;
  		front.u.qpsk.FEC_inner=FEC_inner;
     break;
 	}
  if (ioctl(fd, FE_SET_FRONTEND, &front)<0)
 	{
 		perror("FE_SET_FRONTEND");
 		return -1;
 	}
  eDebug("FE_SET_FRONTEND OK");
	state=stateTuning;
 	tries=10; // 1.0 second timeout
  timer->start(50, true);
  
  return 0;
}

/*
			int changelnb=(DiSEqC.params[0]^lastcsw)&~3;
      // faster zap.. dont work on spaun
*/

int eFrontend::tune_qpsk(eTransponder *transponder, 
		uint32_t Frequency, 		// absolute frequency in kHz
		int polarisation, 			// polarisation (polHor, polVert, ...)
		uint32_t SymbolRate, 		// symbolrate in symbols/s (e.g. 27500000)
		uint8_t FEC_inner,			// FEC_inner (-1 for none, 0 for auto, but please don't use that)
		int Inversion,					// spectral inversion, INVERSION_OFF / _ON / _AUTO (but please...)
    eSatellite &sat)        // Satellite Data.. LNB, DiSEqC, switch..
{
	return tune(transponder, Frequency, polarisation, SymbolRate, getFEC(FEC_inner), Inversion?INVERSION_ON:INVERSION_OFF, &sat, QPSK);
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
