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
#include <lib/gui/emessage.h>

#include <dbox/fp.h>

#define FP_IOCTL_GET_LNB_CURRENT 9

eFrontend* eFrontend::frontend;

eFrontend::eFrontend(int type, const char *demod, const char *sec)
: type(type), timer2(eApp)

{
	state=stateIdle;
	timer=new eTimer(eApp);

/*  CONNECT(timer2.timeout, eFrontend::readInputPower);
  timer2.start(250);*/
  
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

    // reset all diseqc devices

    if ( sendDiSEqCCmd( 0, 0 ) )
      exit(0);

/*    
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
		}*/
	} else
		secfd=-1;
		
	lastcsw=0;

  lastRotorCmd=-1;

  lastSmatvFreq=-1;
}

void eFrontend::Reset()
{
  lastcsw = lastSmatvFreq = lastRotorCmd = -1;
  sendDiSEqCCmd( 0, 0 );
  usleep(50000);
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

int gotoXTable[10] = { 0x00, 0x02, 0x03, 0x05, 0x06, 0x08, 0x0A, 0x0B, 0x0D, 0x0E };

void eFrontend::readInputPower()
{
      int tmp=0;
      // open front prozessor
      int fp=::open("/dev/dbox/fp0", O_RDWR);
      if (fp < 0)
      {
        eDebug("couldn't open fp");
        return;
      }

      // get power input of Rotor in idle
      if (ioctl(fp, FP_IOCTL_GET_LNB_CURRENT, &tmp )<0)
     	{
   		  eDebug("FP_IOCTL_GET_LNB_CURRENT sucks.\n");
 	    	return;
     	}
      eDebug("InputPower = %d", tmp);
      ::close(fp);  
}

int eFrontend::sendDiSEqCCmd( int addr, int Cmd, eString params, int frame )
{
  secCmdSequence seq;
  secCommand cmd;

  int cnt=0;
  for ( unsigned int i=0; i < params.length() && i < 16; i+=4 )
    cmd.u.diseqc.params[++cnt] = strtol( params.mid(i, 2).c_str(), 0, 16 );
    
	cmd.type = SEC_CMDTYPE_DISEQC_RAW;
  cmd.u.diseqc.cmdtype = frame;
  cmd.u.diseqc.addr = addr;
	cmd.u.diseqc.cmd = Cmd;
  cmd.u.diseqc.numParams = cnt;

  eString parms;
  for (int i=0; i < cnt; i++)
    parms+=eString().sprintf("0x%02x ",cmd.u.diseqc.params[i]);
  
//  eDebug("cmdtype = %02x, addr = %02x, cmd = %02x, numParams = %02x, params=%s", frame, addr, Cmd, cnt, parms.c_str() );

  seq.miniCommand = SEC_MINI_NONE;
	seq.continuousTone = SEC_TONE_OFF;
	seq.voltage = SEC_VOLTAGE_13;
  seq.commands=&cmd;
  seq.numCommands=1;

  if ( ioctl(secfd, SEC_SEND_SEQUENCE, &seq) < 0 )
 	{
 		perror("SEC_SEND_SEQUENCE");
 		return -1;
  }
/*  else
    eDebug("cmd send");*/

  return 0;
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

  if (state==stateTuning)
	{
		state=stateIdle;
		if (transponder)
			/*emit*/ tunedIn(transponder, -ECANCELED);
	}
	
	if (state==stateTuning)
		return -EBUSY;

  transponder=trans;

	if (sat)   // then we must do Satellite Stuff
	{
    eSwitchParameter &swParams = sat->getSwitchParams();
    eLNB *lnb = sat->getLNB();
    // Variables to detect if DiSEqC must sent .. or not
    int csw = lnb->getDiSEqC().DiSEqCParam,
        RotorCmd=-1;
//        SmatvFreq=-1

    int sendSeq = 1;
    
    secCmdSequence seq;
    secCommand *commands=0; // pointer to all sec commands
    
    // empty secCmdSequence struct
    memset( &seq, 0, sizeof( seq ) );    

    // num command counter
    int cmdCount=0;

    if (csw <= eDiSEqC::BB)  // use AA/AB/BA/BB ?
    {
      csw = 0xF0 | ( csw << 2 );
      if ( polarisation==polHor )
        csw |= 2;  // Horizontal
        
      if ( Frequency > lnb->getLOFThreshold() )
        csw |= 1;   // 22 Khz enabled
    }
    //else we sent directly the cmd 0xF0..0xFF
    
    eDebug("DiSEqC Switch cmd = %04x", csw);

    // Rotor Support
    if ( lnb->getDiSEqC().DiSEqCMode == eDiSEqC::V1_2 )
    {           
      if ( lnb->getDiSEqC().uncommitted_gap ) // the we add 2 * repeats + 1 + 1;
        cmdCount = ( lnb->getDiSEqC().DiSEqCRepeats << 1 ) + 2;
      else // then we add repeats + 1 + 1
        cmdCount = lnb->getDiSEqC().DiSEqCRepeats + 2;
          
      // allocate memory for all DiSEqC commands
      commands = new secCommand[cmdCount];

      commands[cmdCount-1].type = SEC_CMDTYPE_DISEQC_RAW;      
      commands[cmdCount-1].u.diseqc.addr=0x31;     // normal positioner
      commands[cmdCount-1].u.diseqc.cmdtype=0xE0;  // no replay... first transmission

      if ( lnb->getDiSEqC().useGotoXX )
      {
        int pos = sat->getOrbitalPosition() + lnb->getDiSEqC().rotorOffset;
        int absPosition = abs(pos);
        RotorCmd = ( absPosition / 10 * 0x10) + gotoXTable[ absPosition % 10 ];

        // Drive to East ?
        if ( absPosition == pos )
          RotorCmd |= 0xE000;  // then add 0xE0

        eDebug("Rotor DiSEqC Param = %04x (useGotoXX)", RotorCmd);
        commands[cmdCount-1].u.diseqc.cmd=0x6E; // gotoXX Drive Motor to Angular Position
     		commands[cmdCount-1].u.diseqc.numParams=2;
     		commands[cmdCount-1].u.diseqc.params[0]=((RotorCmd & 0xFF00) / 0x100);
     		commands[cmdCount-1].u.diseqc.params[1]=RotorCmd & 0xFF;
      }
      else  // we use builtin rotor sat table
      {
        std::map<int,int>::iterator it = lnb->getDiSEqC().RotorTable.find( sat->getOrbitalPosition() );

        if (it != lnb->getDiSEqC().RotorTable.end())  // position for selected sat found ?
        {
          commands[cmdCount-1].u.diseqc.cmd=0x6B;  // goto stored sat position
       		commands[cmdCount-1].u.diseqc.numParams=1;
       		commands[cmdCount-1].u.diseqc.params[0]=it->second;
          RotorCmd=it->second;
          eDebug("Rotor DiSEqC Param = %02x (use stored position)", RotorCmd);          
        }
        else  // entry not in table found
        {
          eDebug("add satellites to RotorTable...");
        }
      }
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

      if ( cmdCount )  // Smatv or Rotor is avail...
      {
        loops = cmdCount - 1;  // do not overwrite rotor cmd
      }
      else // no rotor or smatv
      {
        // DiSEqC Repeats and uncommitted switches only when DiSEqC >= V1_1
        if ( lnb->getDiSEqC().DiSEqCMode >= eDiSEqC::V1_1 )  
        {
          if ( lnb->getDiSEqC().uncommitted_gap ) // then we add 2 * repeats + 1;
            loops = cmdCount = ( lnb->getDiSEqC().DiSEqCRepeats << 1 ) + 1;
          else // then we add repeats + 1
            loops = cmdCount = lnb->getDiSEqC().DiSEqCRepeats + 1;
        }
        else // send only one DiSEqC Command
        {
          loops = cmdCount = 1;
        }
          
        // allocate memory for all DiSEqC commands
        commands = new secCommand[cmdCount];
      }

      for ( int i = 0; i < loops;)  // fill commands...
      {
        commands[i].type = SEC_CMDTYPE_DISEQC_RAW;
        commands[i].u.diseqc.addr=0x10;

        // when DiSEqC V1.0 is avail then loops is always 1 .. see above
        
        if ( loops > 1 && lnb->getDiSEqC().uncommitted_switch )
          commands[i].u.diseqc.cmd=0x39;          // uncomitted switch
        else // DiSEqC < V1.1 do not support repeats
          commands[i].u.diseqc.cmd=0x38;          // comitted switch
          
   			commands[i].u.diseqc.cmdtype= i ? 0xE1 : 0xE0; // repeated or not repeated transm.
     		commands[i].u.diseqc.numParams=1;
     		commands[i].u.diseqc.params[0]=csw;

        i++;        
        if ( i < loops && lnb->getDiSEqC().uncommitted_gap )
        {
          commands[i]=commands[i-1];
          commands[i].u.diseqc.cmdtype=0xE1;
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

		if ( lnb->getDiSEqC().MiniDiSEqCParam == eDiSEqC::A )
    {
      eDebug("MiniDiSEqC A");
      seq.miniCommand=SEC_MINI_A;
    }
    else if ( lnb->getDiSEqC().MiniDiSEqCParam == eDiSEqC::B )
    {
      eDebug("MiniDiSEqC B");
      seq.miniCommand=SEC_MINI_B;
    }
    else
    {
      eDebug("no Toneburst (MiniDiSEqC)");
      seq.miniCommand = SEC_MINI_NONE;
    }

    eDebug("Commands to send = %d", cmdCount);

    if ( csw != lastcsw )
    {
      lastcsw = csw;
      sendSeq = -1;
    }

/*    if ( lastSmatvFreq != SmatvFreq )
    {
      lastSmatvFreq = SmatvFreq;
      sendSeq = -2;
    }*/

    if (lastRotorCmd != RotorCmd )
    {
      lastRotorCmd = RotorCmd;
      sendSeq = -3;
    }
    
    if ( sendSeq > 0 )
     sendSeq &= ~1;
      
    // no DiSEqC related Stuff
    
    // calc Frequency
    int local = Frequency - ( Frequency > lnb->getLOFThreshold() ? lnb->getLOFHi() : lnb->getLOFLo() );
    front.Frequency = local > 0 ? local : -local;
    
    // set Continuous Tone ( 22 Khz... low - high band )
    if ( (swParams.HiLoSignal == eSwitchParameter::ON) || ( (swParams.HiLoSignal == eSwitchParameter::HILO) && (Frequency > lnb->getLOFThreshold()) ) )
    {
  		seq.continuousTone = SEC_TONE_ON;
    }
  	else if ( (swParams.HiLoSignal == eSwitchParameter::OFF) || ( (swParams.HiLoSignal == eSwitchParameter::HILO) && (Frequency <= lnb->getLOFThreshold()) ) )
    {
  		seq.continuousTone = SEC_TONE_OFF;
    }

    // set Voltage( 0/14/18V  vertical/horizontal )
    if ( swParams.VoltageMode == eSwitchParameter::_0V )
    {
      seq.voltage = SEC_VOLTAGE_OFF;
    }
    else if ( swParams.VoltageMode == eSwitchParameter::_14V || ( polarisation == polVert && swParams.VoltageMode == eSwitchParameter::HV )  )
    {
      seq.voltage = lnb->getIncreasedVoltage() ? SEC_VOLTAGE_13_5 : SEC_VOLTAGE_13;
    }
  	else if ( swParams.VoltageMode == eSwitchParameter::_18V || ( polarisation==polHor && swParams.VoltageMode == eSwitchParameter::HV)  )
    {
      seq.voltage = lnb->getIncreasedVoltage() ? SEC_VOLTAGE_18_5 : SEC_VOLTAGE_18;
    }

    // set cmd ptr in sequence..
    seq.commands=commands;
    
    // handle DiSEqC Rotor
    if ( lnb->getDiSEqC().DiSEqCMode == eDiSEqC::V1_2 && sendSeq == -3 )
    {
      int idlePowerInput=0;
      int runningPowerInput=0;
//      int cnt=0;
      
      // open front prozessor
      int fp=::open("/dev/dbox/fp0", O_RDWR);
      if (fp < 0)
      {
        eDebug("couldn't open fp");
        return -1;
      }

      // when a DiSEqC Rotor is avail.. we send first the normal DiSEqC Switch Cmd
      // and the the Rotor CMD
      seq.numCommands=cmdCount-1;
                  
      // send DiSEqC Sequence ( normal diseqc switches )
      if ( sendSeq && ioctl(secfd, SEC_SEND_SEQUENCE, &seq) < 0 )
   		{
   			perror("SEC_SEND_SEQUENCE");
   			return -1;
    	}
      else if ( lnb->getDiSEqC().SeqRepeat )   // Sequence Repeat selected ?
      {
        usleep( 80000 ); // between seq repeats we wait 80ms
        ioctl(secfd, SEC_SEND_SEQUENCE, &seq);  // then repeat the cmd
      }
      usleep( 80000 ); // between seq repeats we wait 80ms
     
      // get power input of Rotor on idle  not work on dbox yet .. only dreambox
      if (ioctl(fp, FP_IOCTL_GET_LNB_CURRENT, &idlePowerInput )<0)
     	{
   		  eDebug("FP_IOCTL_GET_LNB_CURRENT sucks.\n");
 	    	return -1;
     	}
//      eDebug("idle power input = %dmA", idlePowerInput );

      // send DiSEqC Sequence (Rotor)
      seq.numCommands=1;  // only rotor cmd
      seq.commands=&commands[cmdCount-1];  // last command is rotor cmd... see above...
      
      if ( sendSeq && ioctl(secfd, SEC_SEND_SEQUENCE, &seq) < 0 )
   		{
   			perror("SEC_SEND_SEQUENCE");
   			return -1;
    	}
      else if ( lnb->getDiSEqC().SeqRepeat )  // Sequence repeat ?
      {
        usleep( 80000 ); // between seq repeats we wait 80ms
        ioctl(secfd, SEC_SEND_SEQUENCE, &seq);
      }
      
      // set rotor start timeout      
      time_t timeout=time(0)+5;

      // now wait for rotor start
      while(true)
      {
        if (ioctl(fp, FP_IOCTL_GET_LNB_CURRENT, &runningPowerInput)<0)
       	{
     		  eDebug("FP_IOCTL_GET_LNB_CURRENT sucks.\n");
   	    	return 2;
       	}
//       	eDebug("(%d) %d mA\n", cnt, runningPowerInput);
//        cnt++;

        if ( abs(runningPowerInput-idlePowerInput ) < 70 ) // rotor running ?
        {
          usleep(50000);  // not... then wait 50ms
        }
        else  // rotor is running
        {
          timeout=0;
          break;  // leave endless loop
        }

        if ( timeout <= time(0) )   // timeout
          break;
      }

      if ( !timeout )  // then the Rotor is Running... we wait if it stops..
      {
        // set rotor timeout to 20sec's...
        timeout = time(0) + 20;
//        cnt=0;
        while(true)
        {
          if (ioctl(fp, FP_IOCTL_GET_LNB_CURRENT, &runningPowerInput)<0)
         	{
         		printf("FP_IOCTL_GET_LNB_CURRENT sucks.\n");
     	    	return 2;
         	}
//         	eDebug("(%d) %d mA", cnt, runningPowerInput);
//          cnt++;

          if ( abs( idlePowerInput-runningPowerInput ) > 70 ) // rotor stoped ?
          {
            usleep(50000);  // not... then wait 50ms
          }
          else  // rotor has stopped
            break;

          if ( timeout <= time(0) ) // Rotor has timouted
            break;
        }
      }
      ::close(fp);
    }
    else  // no Rotor avail... we send the complete cmd...
    {
      seq.numCommands=cmdCount;
      if ( sendSeq && ioctl(secfd, SEC_SEND_SEQUENCE, &seq) < 0 )
   		{
   			perror("SEC_SEND_SEQUENCE");
   			return -1;
    	}
      else if ( lnb->getDiSEqC().SeqRepeat )  // Sequence Repeat ?
      {
        usleep( 80000 ); // between seq repeats we wait 80ms
        ioctl(secfd, SEC_SEND_SEQUENCE, &seq);  // just do it *g*
      }
    }

    // delete allocated memory...
    if (cmdCount)
      delete [] commands;
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
