#include <lib/dvb/frontend.h>

#include <config.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <cmath>

#include <lib/base/ebase.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/esection.h>
#include <lib/dvb/decoder.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
#include <lib/gui/emessage.h>

#include <dbox/fp.h>
#define FP_IOCTL_GET_LNB_CURRENT 9

eFrontend* eFrontend::frontend;

eFrontend::eFrontend(int type, const char *demod, const char *sec)
:type(type), curRotorPos( 1000 ), state(stateIdle),
	transponder(0), timer2(eApp), rotorTimer1(eApp), rotorTimer2(eApp), 
	noRotorCmd(0)
{
	timer=new eTimer(eApp);

	CONNECT(rotorTimer1.timeout, eFrontend::RotorStartLoop );
	CONNECT(rotorTimer2.timeout, eFrontend::RotorRunningLoop );

	CONNECT(timer->timeout, eFrontend::timeout);
	fd=::open(demod, O_RDWR);
	if (fd<0)
	{
		perror(demod);
		return;
	}
#if HAVE_DVB_API_VERSION < 3
	if (type==eSystemInfo::feSatellite)
	{
		secfd=::open(sec, O_RDWR);
		if (secfd<0)
		{
			perror(sec);
			return;
		}
	} else
		secfd=-1;
#else
	curContTone = curVoltage = -1;
#endif
	needreset = 2;
}

void eFrontend::InitDiSEqC()
{
	lastcsw = lastSmatvFreq = lastRotorCmd = lastucsw = lastToneBurst = -1;
	lastLNB=0;
	// DiSEqC Reset
	sendDiSEqCCmd( 0, 0 );
	// peripheral power supply on
	sendDiSEqCCmd( 0, 3 );
	usleep(150000);
}

void eFrontend::timeout()
{
	if (Locked())
	{
		eDebug("+");
		state=stateIdle;

		int updateTransponder=0;
		switch ( eSystemInfo::getInstance()->getHwType() )
		{
			case eSystemInfo::DM5600:
			case eSystemInfo::DM5620:
			case eSystemInfo::DM7000:
				updateTransponder=1;
				break;
			case eSystemInfo::dbox2Nokia:
			case eSystemInfo::dbox2Philips:
			case eSystemInfo::dbox2Sagem:
				eDebug("FE_GET_FRONTEND is stil sucking (dbox2)... "
				 "until dbox2 head drivers are not fixed we better "
				 "don't update transponder data !");
			 break;
			default:
				eDebug("unknown HWType.. don't use FE_GET_FRONTEND");
				break;
		}
		if ( transponder->satellite.valid && !eDVB::getInstance()->getScanAPI() &&
			updateTransponder )
		{
#if HAVE_DVB_API_VERSION < 3
			FrontendParameters front;
#else
			dvb_frontend_event front;
#endif
			if (ioctl(fd, FE_GET_FRONTEND, &front)<0)
				perror("FE_GET_FRONTEND");
			else
			{
				eDebug("FE_GET_FRONTEND OK");
				eSatellite * sat = eTransponderList::getInstance()->findSatellite(transponder->satellite.orbital_position);
				if (sat)
				{
					eLNB *lnb = sat->getLNB();
					if (lnb)
					{
						eDebug("oldFreq = %d", transponder->satellite.frequency );
#if HAVE_DVB_API_VERSION < 3
						transponder->satellite.frequency = transponder->satellite.frequency > lnb->getLOFThreshold() ?
								front.Frequency + lnb->getLOFHi() :
								front.Frequency + lnb->getLOFLo();
#else
						transponder->satellite.frequency = transponder->satellite.frequency > lnb->getLOFThreshold() ?
								front.parameters.frequency + lnb->getLOFHi() :
								front.parameters.frequency + lnb->getLOFLo();
#endif
						eDebug("newFreq = %d", transponder->satellite.frequency );
					}
				}
/*				transponder->satellite.fec = front.u.qpsk.FEC_inner;
				transponder->satellite.symbol_rate = front.u.qpsk.SymbolRate;*/
#if HAVE_DVB_API_VERSION < 3
				transponder->satellite.inversion=front.Inversion;
#else
				transponder->satellite.inversion=front.parameters.inversion;
#endif
//				eDebug("NEW INVERSION = %d", front.Inversion );
			}
		}

		/*emit*/ tunedIn(transponder, 0);
	}
	else
	{
		if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite
			&& eDVB::getInstance()->getScanAPI() && lastLNB )
		{
			if ( abs(lastLNB->getLOFHi() - transponder->satellite.frequency) > 2000000 &&
					 abs(lastLNB->getLOFLo() - transponder->satellite.frequency) > 2000000 )
				tries=1;
		}

		if (--tries)
		{
			eDebugNoNewLine("-: %x,", Status());
			timer->start(100, true);
		}
		else
		{
			if ( !eDVB::getInstance()->getScanAPI() )
				needreset++;
			eDebug("couldn't lock. (state: %x)", Status());
			state=stateIdle;
			/*emit*/ tunedIn(transponder, -ETIMEDOUT);
		}
	}
}

eFrontend::~eFrontend()
{
	if (fd>=0)
		::close(fd);
#if HAVE_DVB_API_VERSION < 3
	if (secfd>=0)
		::close(secfd);
#endif
	frontend=0;
}

int eFrontend::Status()
{
#if HAVE_DVB_API_VERSION < 3
	FrontendStatus status=0;
#else
	fe_status_t status;
#endif
	if ( ioctl(fd, FE_READ_STATUS, &status) < 0 )
		eDebug("FE_READ_STATUS failed (%m)");
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
	::close(fp);  
}

#if HAVE_DVB_API_VERSION < 3
int eFrontend::sendDiSEqCCmd( int addr, int Cmd, eString params, int frame )
{
	secCmdSequence seq;
	secCommand cmd;

	int cnt=0;
	for ( unsigned int i=0; i < params.length() && i < 6; i+=2 )
		cmd.u.diseqc.params[cnt++] = strtol( params.mid(i, 2).c_str(), 0, 16 );
    
	cmd.type = SEC_CMDTYPE_DISEQC_RAW;
	cmd.u.diseqc.cmdtype = frame;
	cmd.u.diseqc.addr = addr;
	cmd.u.diseqc.cmd = Cmd;
	cmd.u.diseqc.numParams = cnt;

	eString parms;
	for (int i=0; i < cnt; i++)
		parms+=eString().sprintf("0x%02x ",cmd.u.diseqc.params[i]);

	if ( transponder && lastLNB )
	{
//		eDebug("hold current voltage and continuous tone");
		// set Continuous Tone ( 22 Khz... low - high band )
		if ( transponder->satellite.frequency > lastLNB->getLOFThreshold() )
			seq.continuousTone = SEC_TONE_ON;
		else 
			seq.continuousTone = SEC_TONE_OFF;
		// set voltage
		if ( transponder->satellite.polarisation == polVert )
			seq.voltage = SEC_VOLTAGE_13;
		else
			seq.voltage = SEC_VOLTAGE_18;
	}
	else
	{
		eDebug("set continuous tone OFF and voltage to 13V");
		seq.continuousTone = SEC_TONE_OFF;
		seq.voltage = SEC_VOLTAGE_13;
	}
    
//  eDebug("cmdtype = %02x, addr = %02x, cmd = %02x, numParams = %02x, params=%s", frame, addr, Cmd, cnt, parms.c_str() );
	seq.miniCommand = SEC_MINI_NONE;
	seq.commands=&cmd;
	seq.numCommands=1;

	if ( ioctl(secfd, SEC_SEND_SEQUENCE, &seq) < 0 )
	{
		eDebug("SEC_SEND_SEQUENCE failed ( %m )");
		return -1;
	}
/*  else
		eDebug("cmd send");*/

	lastcsw = lastucsw = -1;

	return 0;
}
#else
int eFrontend::sendDiSEqCCmd( int addr, int Cmd, eString params, int frame )
{
	struct dvb_diseqc_master_cmd cmd;

	cmd.msg[0]=frame;
	cmd.msg[1]=addr;
	cmd.msg[2]=Cmd;
	cmd.msg_len=3;

	for (uint8_t i = 0; i < params.length() && i < 6; i += 2)
		cmd.msg[3 + cmd.msg_len++] = strtol( params.mid(i, 2).c_str(), 0, 16 );

	if (ioctl(fd, FE_SET_TONE, SEC_TONE_OFF) < 0)
	{
		eDebug("FE_SET_TONE failed (%m)");
		return -1;
	}
	curContTone = eSecCmdSequence::TONE_OFF;

	if (ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &cmd) < 0)
	{
		eDebug("FE_DISEQC_SEND_MASTER_CMD failed (%m)");
		return -1;
	}

	lastcsw = lastucsw = -1;

	return 0;
}
#endif

#if HAVE_DVB_API_VERSION < 3
int eFrontend::SendSequence( const eSecCmdSequence &s )
{
	secCmdSequence seq;
	memset( &seq, 0, sizeof(seq) );
	seq.commands = s.commands;
	seq.numCommands = s.numCommands;
	seq.continuousTone = s.continuousTone;
	seq.miniCommand = s.toneBurst;
	switch ( s.voltage )
	{
		case eSecCmdSequence::VOLTAGE_13:
			seq.voltage=s.increasedVoltage?SEC_VOLTAGE_13_5:SEC_VOLTAGE_13;
			break;
		case eSecCmdSequence::VOLTAGE_18:
			seq.voltage=s.increasedVoltage?SEC_VOLTAGE_18_5:SEC_VOLTAGE_18;
			break;
		case eSecCmdSequence::VOLTAGE_OFF:
		default:
			seq.voltage=SEC_VOLTAGE_OFF;
			break;
	}
	return ::ioctl(secfd, SEC_SEND_SEQUENCE, &seq);
}
#else
int eFrontend::SendSequence( const eSecCmdSequence &seq )
{
	int i=0, ret=0, wait=0;
	dvb_diseqc_master_cmd *scommands;

// set Tone
//	eDebug("curContTone = %d, newTone = %d", curContTone, seq.continuousTone );
	if ( curContTone != eSecCmdSequence::TONE_OFF &&
		( seq.continuousTone == eSecCmdSequence::TONE_OFF
	// diseqc command or minidiseqc command to sent ?
		|| seq.numCommands || seq.toneBurst != eSecCmdSequence::NONE ) )
	{
//		eDebug("disable cont Tone");
		ret = ioctl(fd, FE_SET_TONE, SEC_TONE_OFF);
		if ( ret < 0 )
		{
			eDebug("FE_SET_TONE failed (%m)");
			return ret;
		}
		curContTone = eSecCmdSequence::TONE_OFF;
		wait=1;
	}

	if (seq.increasedVoltage)
	{
//		eDebug("enable high voltage");
		ret = ioctl(fd, FE_ENABLE_HIGH_LNB_VOLTAGE, 1);
		if ( ret < 0 )
		{
			eDebug("FE_ENABLE_HIGH_LNB_VOLTAGE failed (%m)");
			return ret;
		}
	} 
//	eDebug("curVoltage = %d, new Voltage = %d", curContTone, seq.voltage);
	if ( seq.voltage != curVoltage )
	{
		ret = ioctl(fd, FE_SET_VOLTAGE, seq.voltage);
		if (  ret < 0 )
		{
			eDebug("FE_SET_VOLTAGE failed (%m)");
			return ret;
		}
		curVoltage = seq.voltage;
//		eDebug("set voltage to %s", seq.voltage==eSecCmdSequence::VOLTAGE_13?"13 V":seq.voltage==eSecCmdSequence::VOLTAGE_18?"18 V":"unknown ... bug!!");
		wait=1;
	}

	if ( wait )
	{
//		eDebug("delay 30 ms");
		usleep(30*1000);
		wait=0;
	}

	while (true && seq.numCommands)
	{
		scommands = &seq.commands[i];

		ret = ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, scommands);
		if ( ret < 0)
		{
			eDebug("FE_DISEQC_SEND_MASTER_CMD failed (%m)");
			return ret;
		}
//		eDebug("DiSEqC Cmd sent %02x %02x %02x %02x", scommands->msg[0], scommands->msg[1], scommands->msg[2], scommands->msg[3] );
/*
		if ( scommands->msg[0] & 1 )
		{
			if ( scommands->msg[2] == 0x39 )
			{
				eDebug("(%d)uncommited repeat command sent", i);
			}
			else
			{
				eDebug("(%d)commited repeat command sent", i);
			}
		}
		else
		{
			if ( scommands->msg[2] == 0x39 )
			{
				eDebug("(%d)uncommited command sent", i);
			}
			else
			{
				eDebug("(%d)commited command sent", i);
			}
		}*/

		i++;
		if ( i < seq.numCommands )  // another command is to send...
		{
			if (wait && seq.commands[i].msg[0] & 1)
			{
				usleep(wait*1000);
//				eDebug("delay %dms",wait);
			}
			if ( seq.commands[i].msg[2] == scommands->msg[2]
						&& seq.commands[i].msg[3] == scommands->msg[3] )
			// send repeat without uncommitted switch in gap... wait 120 ms
			{
				usleep(120*1000);
//				eDebug("delay 120ms");
			}
			else if ( seq.commands[i].msg[0] & 1  // repeat
				|| (i+1 < seq.numCommands &&
				seq.commands[i-1].msg[2] == seq.commands[i+1].msg[2] ))
			{
				if (!wait)
				{
					// we calc prev message length
					int mlength = (scommands->msg_len * 8 + scommands->msg_len) * 15;
//					eDebug("messageLength = %dms", mlength / 10 );
					wait = ( ( (1200 - mlength) / 20) );
					// half time we wait before next cmd
					usleep(wait*1000);
//					eDebug("delay %dms",wait);
				}
				else
					wait = 0;
			}  // MSG[1] ??? or MSG
			else if ( seq.commands[i].msg[1] == 0x58 ) // handle SMATV Cmd
			{
//				eDebug("delay 10ms");  // Standard > 6ms
				usleep( 10*1000 );
			}
			else  // we calc the delay between two cmds with difference types.. switch..rotor
			{
				usleep( 120*1000 );  // wait after msg
//				eDebug("delay 120ms");
				wait=0;
			}
		}
		else
			break;
	}

	if (seq.numCommands)  // When DiSEqC Commands was sent.. then wait..
	{
//		eDebug("delay 30ms after send last diseqc command");  // Standard > 15ms
		usleep(30*1000);
	}

  // send minidiseqc
	if (seq.toneBurst != eSecCmdSequence::NONE)
	{
		ret = ioctl(fd, FE_DISEQC_SEND_BURST, seq.toneBurst);
		if ( ret < 0)
		{
			eDebug("FE_DISEQC_SEND_BURST failed (%m)");
			return ret;
		}
//		eDebug("toneBurst sent\n");

		usleep(30*1000); // after send toneburst we wait...
//		eDebug("delay 30ms");
	}

//	eDebug("curTone = %d, newTone = %d", curContTone, seq.continuousTone );
	if ( seq.continuousTone == eSecCmdSequence::TONE_ON && curContTone != SEC_TONE_ON )
	{
//		eDebug("enable continuous Tone");
		ret = ioctl(fd, FE_SET_TONE, SEC_TONE_ON);
		if ( ret < 0 )
		{
			eDebug("FE_SET_TONE failed (%m)");
			return ret;
		}
		else
		{
			curContTone = eSecCmdSequence::TONE_ON;
			usleep(10*1000);
//			eDebug("delay 10ms");
		}
	}
/////////////////////////////////////////
	return 0;
}
#endif

int eFrontend::RotorUseTimeout(eSecCmdSequence& seq, eLNB *lnb )
{
#if HAVE_DVB_API_VERSION < 3
	secCommand *commands = seq.commands;
#else
	dvb_diseqc_master_cmd *commands = seq.commands;
#endif
	double TimePerDegree=1000/lnb->getDiSEqC().DegPerSec; // msec
	int startDelay=800;  // we use hardcoded start delay of 800msec

	// we send first the normal DiSEqC Switch Cmds
	// and then the Rotor CMD
	seq.numCommands--;

	int secTone = seq.continuousTone;
	// send DiSEqC Sequence ( normal diseqc switches )
	seq.continuousTone = eSecCmdSequence::TONE_OFF;
	if ( SendSequence(seq) < 0 )
	{
		eDebug("SendSequence failed (%m)");
		return -1;
	}
	else if ( lnb->getDiSEqC().SeqRepeat )   // Sequence Repeat selected ?
	{
		usleep( 100000 ); // between seq repeats we wait 75ms
		SendSequence(seq);  // then repeat the command
	}
	if ( lastLNB != lnb )
		usleep( 1000000 ); // wait 1sek
	else
		usleep( 100000 ); // wait 100ms

	// send DiSEqC Sequence (Rotor)
	seq.commands=&commands[seq.numCommands];  // last command is rotor cmd... see above...
	seq.numCommands=1;  // only rotor cmd
	seq.toneBurst = eSecCmdSequence::NONE;
	seq.continuousTone=secTone;

	if ( SendSequence(seq) < 0 )
	{
		eDebug("SendSequence failed (%m)");
		return -1;
	}
	else
		eDebug("Rotor Cmd is sent");

	/* emit */ s_RotorRunning(newPos);

	if ( curRotorPos != 1000 ) // uninitialized  
		usleep( (int)( abs(newPos - curRotorPos) * TimePerDegree * 100) + startDelay );

	/* emit */ s_RotorStopped();

	curRotorPos = newPos;

	return 0;
}

int eFrontend::RotorUseInputPower(eSecCmdSequence& seq, eLNB *lnb )
{
#if HAVE_DVB_API_VERSION < 3
	secCommand *commands = seq.commands;
#else
	dvb_diseqc_master_cmd *commands = seq.commands;
#endif
	idlePowerInput=0;
	runningPowerInput=0;
	int secTone = seq.continuousTone;

	// open front prozessor
	int fp=::open("/dev/dbox/fp0", O_RDWR);
	if (fp < 0)
	{
		eDebug("couldn't open fp");
		return -1;
	}

	// we send first the normal DiSEqC Switch Cmds
	// and then the Rotor CMD
	seq.numCommands--;

//	eDebug("sent normal diseqc switch cmd");
//	eDebug("0x%02x,0x%02x,0x%02x,0x%02x, numParams=%d, numcmds=%d", seq.commands[0].u.diseqc.cmdtype, seq.commands[0].u.diseqc.addr, seq.commands[0].u.diseqc.cmd, seq.commands[0].u.diseqc.params[0], seq.numCommands, seq.commands[0].u.diseqc.numParams );
	// send DiSEqC Sequence ( normal diseqc switches )

	seq.continuousTone=SEC_TONE_OFF;
	if ( SendSequence(seq) < 0 )
	{
		eDebug("SendSequence failed (%m)");
		::close(fp);
		return -2;
	}
	else if ( lnb->getDiSEqC().SeqRepeat )   // Sequence Repeat selected ?
	{
		usleep( 100000 ); // between seq repeats we wait 100ms
		SendSequence(seq);  // then repeat the cmd
	}

	if ( lastLNB != lnb )
	{
		usleep( 1000*1000 ); // wait 1sek
//		eDebug("sleep 1sek");
	}
	else
	{
		usleep( 100*1000 ); // wait 100ms
//		eDebug("sleep 100ms");
	}

	// get power input of Rotor on idle  not work on dbox yet .. only dreambox
	if (ioctl(fp, FP_IOCTL_GET_LNB_CURRENT, &idlePowerInput )<0)
	{
		eDebug("FP_IOCTL_GET_LNB_CURRENT sucks. (%m)");
		::close(fp);
		return -1;
	}
//	eDebug("idle power input = %dmA", idlePowerInput );

	// send DiSEqC Sequence (Rotor)
	seq.commands=&commands[seq.numCommands];  // last command is rotor cmd... see above...
	seq.numCommands=1;  // only rotor cmd
	seq.toneBurst = eSecCmdSequence::NONE;

//	eDebug("0x%02x,0x%02x,0x%02x,0x%02x,0x%02x", seq.commands[0].u.diseqc.cmdtype, seq.commands[0].u.diseqc.addr, seq.commands[0].u.diseqc.cmd, seq.commands[0].u.diseqc.params[0], seq.commands[0].u.diseqc.params[1]);
	seq.continuousTone=secTone;
	if ( SendSequence(seq) < 0 )
	{
		eDebug("SendSequence failed (%m)");
		::close(fp);
		return -2;
	}
	::close(fp);
	// set rotor start timeout  // 2 sek..
	gettimeofday(&rotorTimeout,0);
	rotorTimeout+=2000;
	RotorStartLoop();
	return 0;
}

void eFrontend::RotorStartLoop()
{
	// timeouted ??
	timeval now;
	gettimeofday(&now,0);
	if ( rotorTimeout < now )
	{
		eDebug("rotor has timeoutet :( ");
		/* emit */ s_RotorTimeout();
		RotorFinish();
	}
	else
	{
		// open front prozessor
		int fp=::open("/dev/dbox/fp0", O_RDWR);
		if (fp < 0)
		{
			eDebug("couldn't open fp");
			return;
		}

		if (ioctl(fp, FP_IOCTL_GET_LNB_CURRENT, &runningPowerInput)<0)
		{
			eDebug("FP_IOCTL_GET_LNB_CURRENT sucks.\n");
			::close(fp);
			return;
		}
//		eDebug("running %d mA", runningPowerInput);
//		eDebug("delta %d mA", DeltaA);
		if ( abs(runningPowerInput-idlePowerInput ) >= DeltaA ) // rotor running ?
		{
			eDebug("Rotor is Running");
			/* emit */ s_RotorRunning( newPos );

			// set rotor running timeout  // 150 sek
			gettimeofday(&rotorTimeout,0);
			rotorTimeout+=150000;
			RotorRunningLoop();
		}
		else
			rotorTimer1.start(50,true);  // restart timer
		::close(fp);
	}
}

void eFrontend::RotorRunningLoop()
{
	timeval now;
	gettimeofday(&now,0);
	if ( rotorTimeout < now )
	{
		eDebug("Rotor timeouted :-(");
		/* emit */ s_RotorTimeout();
	}
	else
	{
		// open front prozessor
		int fp=::open("/dev/dbox/fp0", O_RDWR);
		if (fp < 0)
		{
			eDebug("couldn't open fp");
			return;
		}

		if (ioctl(fp, FP_IOCTL_GET_LNB_CURRENT, &runningPowerInput)<0)
		{
			printf("FP_IOCTL_GET_LNB_CURRENT sucks.\n");
			::close(fp);
			return;
		}
//		eDebug("running %d mA", runningPowerInput);

		if ( abs( idlePowerInput-runningPowerInput ) <= DeltaA ) // rotor stoped ?
		{
			eDebug("Rotor Stopped");
			/* emit */ s_RotorStopped();
			RotorFinish();
		}
		else
			rotorTimer2.start(50,true);  // restart timer
		::close(fp);
	}
}

void eFrontend::RotorFinish(bool tune)
{
	if ( voltage != eSecCmdSequence::VOLTAGE_18 )
#if HAVE_DVB_API_VERSION < 3
{
		if (ioctl(secfd, SEC_SET_VOLTAGE, increased ? SEC_VOLTAGE_13_5 : SEC_VOLTAGE_13) < 0 )
			eDebug("SEC_SET_VOLTAGE failed (%m)");
}
#else
		if ( ioctl(fd, FE_SET_VOLTAGE, voltage) < 0 )
			eDebug("FE_SET_VOLTAGE failed (%m)");
		curVoltage = voltage;
#endif
	if ( tune )
		transponder->tune();
}

/*----------------------------------------------------------------------------*/
double factorial_div( double value, int x)
{
	if(!x)
		return 1;
	else
	{
		while( x > 1)
		{
			value = value / x--;
		}
	}
	return value;
}

/*----------------------------------------------------------------------------*/
double powerd( double x, int y)
{
	int i=0;
	double ans=1.0;

	if(!y)
		return 1.000;
	else
	{
		while( i < y)
		{
			i++;
			ans = ans * x;
		}
	}
	return ans;
}

/*----------------------------------------------------------------------------*/
double SIN( double x)
{
	int i=0;
	int j=1;
	int sign=1;
	double y1 = 0.0;
	double diff = 1000.0;

	if (x < 0.0)
	{
		x = -1 * x;
		sign = -1;
	}

	while ( x > 360.0*M_PI/180)
	{
		x = x - 360*M_PI/180;
	}

	if( x > (270.0 * M_PI / 180) )
	{
		sign = sign * -1;
		x = 360.0*M_PI/180 - x;
	}
	else if ( x > (180.0 * M_PI / 180) )
	{
		sign = sign * -1;
		x = x - 180.0 *M_PI / 180;
	}
	else if ( x > (90.0 * M_PI / 180) )
	{
		x = 180.0 *M_PI / 180 - x;
	}

	while( powerd( diff, 2) > 1.0E-16 )
	{
		i++;
		diff = j * factorial_div( powerd( x, (2*i -1)) ,(2*i -1));
		y1 = y1 + diff;
		j = -1 * j;
	}
	return ( sign * y1 );
}

double COS(double x)
{
	return SIN(90 * M_PI / 180 - x);
}

/*----------------------------------------------------------------------------*/
double ATAN( double x)
{
	int i=0; /* counter for terms in binomial series */
	int j=1; /* sign of nth term in series */
	int k=0;
	int sign = 1; /* sign of the input x */
	double y = 0.0; /* the output */
	double deltay = 1.0; /* the value of the next term in the series */
	double addangle = 0.0; /* used if arctan > 22.5 degrees */

	if (x < 0.0)
	{
		x = -1 * x;
		sign = -1;
	}

	while( x > 0.3249196962 )
	{
		k++;
		x = (x - 0.3249196962) / (1 + x * 0.3249196962);
	}
	addangle = k * 18.0 *M_PI/180;

	while( powerd( deltay, 2) > 1.0E-16 )
	{
		i++;
		deltay = j * powerd( x, (2*i -1)) / (2*i -1);
		y = y + deltay;
		j = -1 * j;
	}
	return (sign * (y + addangle) );
}

double ASIN(double x)
{
	return 2 * ATAN( x / (1 + std::sqrt(1.0 - x*x)));
}

double Radians( double number )
{
	return number*M_PI/180;
}

double Deg( double number )
{
	return number*180/M_PI;
}

double Rev( double number )
{
	return number - std::floor( number / 360.0 ) * 360;
}

double calcElevation( double SatLon, double SiteLat, double SiteLon, int Height_over_ocean = 0 )
{
	double  a0=0.58804392,
					a1=-0.17941557,
					a2=0.29906946E-1,
					a3=-0.25187400E-2,
					a4=0.82622101E-4,

					f = 1.00 / 298.257, // Earth flattning factor

					r_sat=42164.57, // Distance from earth centre to satellite

					r_eq=6378.14,  // Earth radius

					sinRadSiteLat=SIN(Radians(SiteLat)),
					cosRadSiteLat=COS(Radians(SiteLat)),

					Rstation = r_eq / ( std::sqrt( 1.00 - f*(2.00-f)*sinRadSiteLat*sinRadSiteLat ) ),

					Ra = (Rstation+Height_over_ocean)*cosRadSiteLat,
					Rz= Rstation*(1.00-f)*(1.00-f)*sinRadSiteLat,

					alfa_rx=r_sat*COS(Radians(SatLon-SiteLon)) - Ra,
					alfa_ry=r_sat*SIN(Radians(SatLon-SiteLon)),
					alfa_rz=-Rz,

					alfa_r_north=-alfa_rx*sinRadSiteLat + alfa_rz*cosRadSiteLat,
					alfa_r_zenith=alfa_rx*cosRadSiteLat + alfa_rz*sinRadSiteLat,

					El_geometric=Deg(ATAN( alfa_r_zenith/std::sqrt(alfa_r_north*alfa_r_north+alfa_ry*alfa_ry))),

					x = std::fabs(El_geometric+0.589),
					refraction=std::fabs(a0+a1*x+a2*x*x+a3*x*x*x+a4*x*x*x*x),
          El_observed = 0.00;

	if (El_geometric > 10.2)
		El_observed = El_geometric+0.01617*(COS(Radians(std::fabs(El_geometric)))/SIN(Radians(std::fabs(El_geometric))) );
	else
	{
		El_observed = El_geometric+refraction ;
	}

	if (alfa_r_zenith < -3000)
		El_observed=-99;

	return El_observed;
}

double calcAzimuth(double SatLon, double SiteLat, double SiteLon, int Height_over_ocean=0)
{
	double	f = 1.00 / 298.257, // Earth flattning factor

					r_sat=42164.57, // Distance from earth centre to satellite

					r_eq=6378.14,  // Earth radius

					sinRadSiteLat=SIN(Radians(SiteLat)),
					cosRadSiteLat=COS(Radians(SiteLat)),

					Rstation = r_eq / ( std::sqrt( 1 - f*(2-f)*sinRadSiteLat*sinRadSiteLat ) ),
					Ra = (Rstation+Height_over_ocean)*cosRadSiteLat,
					Rz = Rstation*(1-f)*(1-f)*sinRadSiteLat,

					alfa_rx = r_sat*COS(Radians(SatLon-SiteLon)) - Ra,
					alfa_ry = r_sat*SIN(Radians(SatLon-SiteLon)),
					alfa_rz = -Rz,

					alfa_r_north = -alfa_rx*sinRadSiteLat + alfa_rz*cosRadSiteLat,

					Azimuth = 0.00;

	if (alfa_r_north < 0)
		Azimuth = 180+Deg(ATAN(alfa_ry/alfa_r_north));
	else
		Azimuth = Rev(360+Deg(ATAN(alfa_ry/alfa_r_north)));

	return Azimuth;
}

double calcDeclination( double SiteLat, double Azimuth, double Elevation)
{
	return Deg( ASIN(SIN(Radians(Elevation)) *
												SIN(Radians(SiteLat)) +
												COS(Radians(Elevation)) *
												COS(Radians(SiteLat)) +
												COS(Radians(Azimuth))
												)
						);
}

double calcSatHourangle( double Azimuth, double Elevation, double Declination, double Lat )
{
	double a = - COS(Radians(Elevation)) *
							 SIN(Radians(Azimuth)),

				 b = SIN(Radians(Elevation)) *
						 COS(Radians(Lat)) -
						 COS(Radians(Elevation)) *
						 SIN(Radians(Lat)) *
						 COS(Radians(Azimuth)),

// Works for all azimuths (northern & sourhern hemisphere)
						 returnvalue = 180 + Deg(ATAN(a/b));

	(void)Declination;

	if ( Azimuth > 270 )
	{
		returnvalue = ( (returnvalue-180) + 360 );
		if (returnvalue>360)
			returnvalue = 360 - (returnvalue-360);
  }

	if ( Azimuth < 90 )
		returnvalue = ( 180 - returnvalue );

	return returnvalue;
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
//	eDebug("op = %d", trans->satellite.orbital_position );
	int finalTune=1;
#if HAVE_DVB_API_VERSION < 3
	FrontendParameters front;
#else
	struct dvb_frontend_parameters front;
#endif
	eSection::abortAll();
	timer->stop();

	if ( rotorTimer1.isActive() || rotorTimer2.isActive() )
	{
		eDebug("Switch while running rotor... send stop..");
		/* emit */ s_RotorStopped();
		rotorTimer1.stop();
		rotorTimer2.stop();
		// ROTOR STOP
		sendDiSEqCCmd( 0x31, 0x60 );
		sendDiSEqCCmd( 0x31, 0x60 );
		lastRotorCmd=-1;
	}

	if (state==stateTuning)
	{
		state=stateIdle;
		if (transponder)
			/*emit*/ tunedIn(transponder, -ECANCELED);
	}

	if (state==stateTuning)
		return -EBUSY;

#if HAVE_DVB_API_VERSION < 3
	if (needreset >= 2)
	{
		ioctl(fd, FE_SET_POWER_STATE, FE_POWER_ON);
		usleep(150000);
		// reset all diseqc devices
		if (type==eSystemInfo::feSatellite)
			InitDiSEqC();
		needreset=0;
	}
#endif
	transponder=trans;

	if ( sat )   // then we must do Satellite Stuff
	{
		eSwitchParameter &swParams = sat->getSwitchParams();
		eLNB *lnb = sat->getLNB();
		// Variables to detect if DiSEqC must sent .. or not
		int csw = lnb->getDiSEqC().DiSEqCParam,
				ucsw = (lnb->getDiSEqC().uncommitted_cmd ?
					lnb->getDiSEqC().uncommitted_cmd : lastucsw),
				ToneBurst = (lnb->getDiSEqC().MiniDiSEqCParam ?
					lnb->getDiSEqC().MiniDiSEqCParam : lastToneBurst),
				RotorCmd = lastRotorCmd;
//				SmatvFreq = -1;

		eSecCmdSequence seq;
#if HAVE_DVB_API_VERSION < 3
		secCommand *commands=0; // pointer to all sec commands
#else
		dvb_diseqc_master_cmd *commands=0;
#endif
    
		// num command counter
		int cmdCount=0;

		if (csw <= eDiSEqC::SENDNO)  // use AA/AB/BA/BB/SENDNO
		{
//			eDebug("csw=%d, csw<<2=%d", csw, csw << 2);
			if ( csw != eDiSEqC::SENDNO )
				csw = 0xF0 | ( csw << 2 );
			if ( polarisation==polHor)
			{
				csw |= 2;  // Horizontal
				eDebug("Horizontal");
			}
			else
				eDebug("Vertikal");

			if ( Frequency > lnb->getLOFThreshold() )
			{
				csw |= 1;   // 22 Khz enabled
				eDebug("Hi Band");
			}
			else
				eDebug("Low Band");
		}
		//else we sent directly the cmd 0xF0..0xFF

		if ( csw != eDiSEqC::SENDNO )
			eDebug("DiSEqC Switch cmd = %04x", csw);
		else
			eDebug("send no committed diseqc cmd !");
			
		// Rotor Support
		if ( lnb->getDiSEqC().DiSEqCMode == eDiSEqC::V1_2 && !noRotorCmd )
		{           
			bool useGotoXX=false;
#define SPECIAL_MODE
#ifdef SPECIAL_MODE
			std::map<int,int>::iterator it = lnb->getDiSEqC().RotorTable.find( sat->getOrbitalPosition() );

			if (it != lnb->getDiSEqC().RotorTable.end())  // position for selected sat found ?
				RotorCmd=it->second;
			else  // entry not in table found
#else
			if ( lnb->getDiSEqC().useGotoXX )
#endif
			{
#ifdef SPECIAL_MODE
				eDebug("Entry for %d,%d° not in Rotor Table found... i try gotoXX°", sat->getOrbitalPosition() / 10, sat->getOrbitalPosition() % 10 );
#endif
				useGotoXX=true;
				int pos = sat->getOrbitalPosition();
				int satDir = pos < 0 ? eDiSEqC::WEST : eDiSEqC::EAST;

				double SatLon = abs(pos)/10.00,
							 SiteLat = lnb->getDiSEqC().gotoXXLatitude,
							 SiteLon = lnb->getDiSEqC().gotoXXLongitude;

				if ( lnb->getDiSEqC().gotoXXLaDirection == eDiSEqC::SOUTH )
					SiteLat = -SiteLat;

				if ( lnb->getDiSEqC().gotoXXLoDirection == eDiSEqC::WEST )
					SiteLon = 360 - SiteLon;

				if (satDir == eDiSEqC::WEST )
					SatLon = 360 - SatLon;

				eDebug("siteLatitude = %lf, siteLongitude = %lf, %lf degrees", SiteLat, SiteLon, SatLon );
				double azimuth=calcAzimuth(SatLon, SiteLat, SiteLon );
				double elevation=calcElevation( SatLon, SiteLat, SiteLon );
				double declination=calcDeclination( SiteLat, azimuth, elevation );
				double satHourAngle=calcSatHourangle( azimuth, elevation, declination, SiteLat );
				eDebug("azimuth=%lf, elevation=%lf, declination=%lf, PolarmountHourAngle=%lf", azimuth, elevation, declination, satHourAngle );
				
				int tmp=(int)round( fabs( 180 - satHourAngle ) * 10.0 );
				RotorCmd = (tmp/10)*0x10 + gotoXTable[ tmp % 10 ];

				if (satHourAngle < 180)  // the east
					RotorCmd |= 0xE000;
				else                     // west
					RotorCmd |= 0xD000;

				eDebug("RotorCmd = %04x", RotorCmd);
			}
#ifndef SPECIAL_MODE
			else  // we use builtin rotor sat table
			{
				std::map<int,int>::iterator it = lnb->getDiSEqC().RotorTable.find( sat->getOrbitalPosition() );

				if (it != lnb->getDiSEqC().RotorTable.end())  // position for selected sat found ?
					RotorCmd=it->second;
				else  // entry not in table found
					eDebug("Entry for %d,%d° not in Rotor Table found... please add", sat->getOrbitalPosition() / 10, sat->getOrbitalPosition() % 10 );
			}
#endif

			if ( RotorCmd != lastRotorCmd )  // rotorCmd must sent?
			{
				cmdCount=1; // this is the RotorCmd
				if ( ucsw != lastucsw )
					cmdCount++;
				if ( csw != lastcsw && csw & 0xF0) // NOT SENDNO
					cmdCount++;
				cmdCount += (cmdCount-1)*lnb->getDiSEqC().DiSEqCRepeats;

#if HAVE_DVB_API_VERSION < 3
			// allocate memory for all DiSEqC commands
				commands = new secCommand[cmdCount];
				commands[cmdCount-1].type = SEC_CMDTYPE_DISEQC_RAW;
				commands[cmdCount-1].u.diseqc.addr=0x31;     // normal positioner
				commands[cmdCount-1].u.diseqc.cmdtype=0xE0;  // no replay... first transmission
#else
				commands = new dvb_diseqc_master_cmd[cmdCount];
				commands[cmdCount-1].msg[0]=0xE0;
				commands[cmdCount-1].msg[1]=0x31;
#endif
				if ( useGotoXX )
				{
					eDebug("Rotor DiSEqC Param = %04x (useGotoXX)", RotorCmd);
#if HAVE_DVB_API_VERSION < 3
					commands[cmdCount-1].u.diseqc.cmd=0x6E; // gotoXX Drive Motor to Angular Position
					commands[cmdCount-1].u.diseqc.numParams=2;
					commands[cmdCount-1].u.diseqc.params[0]=((RotorCmd & 0xFF00) / 0x100);
					commands[cmdCount-1].u.diseqc.params[1]=RotorCmd & 0xFF;
#else
					commands[cmdCount-1].msg[2]=0x6E;
					commands[cmdCount-1].msg[3]=((RotorCmd & 0xFF00) / 0x100);
					commands[cmdCount-1].msg[4]=RotorCmd & 0xFF;
					commands[cmdCount-1].msg_len=5;
#endif
				}
				else
				{
					eDebug("Rotor DiSEqC Param = %02x (use stored position)", RotorCmd);
#if HAVE_DVB_API_VERSION < 3
					commands[cmdCount-1].u.diseqc.cmd=0x6B;  // goto stored sat position
					commands[cmdCount-1].u.diseqc.numParams=1;
					commands[cmdCount-1].u.diseqc.params[0]=RotorCmd;
#else
					commands[cmdCount-1].msg[2]=0x6B;
					commands[cmdCount-1].msg[3]=RotorCmd;
					commands[cmdCount-1].msg_len=4;
#endif
				}
			}
		}
			
/*		else if ( lnb->getDiSEqC().DiSEqCMode == eDiSEqC::SMATV )
		{
			SmatvFreq=Frequency;
			if ( lastSmatvFreq != SmatvFreq )
			{
				if ( lnb->getDiSEqC().uncommitted_cmd && lastucsw != ucsw)
					cmdCount=3;
				else
					cmdCount=2;
				cmdCount += (cmdCount-1)*lnb->getDiSEqC().DiSEqCRepeats;

				// allocate memory for all DiSEqC commands
				commands = new secCommand[cmdCount];

				commands[cmdCount-1].type = SEC_CMDTYPE_DISEQC_RAW;
				commands[cmdCount-1].u.diseqc.addr = 0x71;	// intelligent slave interface for multi-master bus
				commands[cmdCount-1].u.diseqc.cmd = 0x58;	  // write channel frequency
				commands[cmdCount-1].u.diseqc.cmdtype = 0xE0;
				commands[cmdCount-1].u.diseqc.numParams = 3;
				commands[cmdCount-1].u.diseqc.params[0] = (((Frequency / 10000000) << 4) & 0xF0) | ((Frequency / 1000000) & 0x0F);
				commands[cmdCount-1].u.diseqc.params[1] = (((Frequency / 100000) << 4) & 0xF0) | ((Frequency / 10000) & 0x0F);
				commands[cmdCount-1].u.diseqc.params[2] = (((Frequency / 1000) << 4) & 0xF0) | ((Frequency / 100) & 0x0F);
			}
		}*/
		if ( lnb->getDiSEqC().DiSEqCMode >= eDiSEqC::V1_0 )
		{
//			eDebug("ucsw=%d lastucsw=%d csw=%d lastcsw=%d", ucsw, lastucsw, csw, lastcsw);
			if ( ucsw != lastucsw || csw != lastcsw || (ToneBurst && ToneBurst != lastToneBurst) )
			{
//				eDebug("cmdCount=%d", cmdCount);
				int loops;
				if ( cmdCount )  // Smatv or Rotor is avail...
					loops = cmdCount - 1;  // do not overwrite rotor cmd
				else // no rotor or smatv
				{
					// DiSEqC Repeats and uncommitted switches only when DiSEqC >= V1_1
					if ( lnb->getDiSEqC().DiSEqCMode >= eDiSEqC::V1_1	&& ucsw != lastucsw )
						cmdCount++;
					if ( csw != lastcsw && csw & 0xF0 )
						cmdCount++;
					cmdCount += cmdCount*lnb->getDiSEqC().DiSEqCRepeats;
					loops = cmdCount;

					if ( cmdCount )
#if HAVE_DVB_API_VERSION < 3
						commands = new secCommand[cmdCount];
#else
						commands = new dvb_diseqc_master_cmd[cmdCount];
#endif
				}

				for ( int i = 0; i < loops;)  // fill commands...
				{
					enum { UNCOMMITTED, COMMITTED } cmdbefore = COMMITTED;
#if HAVE_DVB_API_VERSION < 3
					commands[i].type = SEC_CMDTYPE_DISEQC_RAW;
					commands[i].u.diseqc.cmdtype= i ? 0xE1 : 0xE0; // repeated or not repeated transm.
					commands[i].u.diseqc.numParams=1;
					commands[i].u.diseqc.addr=0x10;
#else
					commands[i].msg[0]= i ? 0xE1 : 0xE0;
					commands[i].msg[1]=0x10;
					commands[i].msg_len=4;
#endif
					if ( ( lnb->getDiSEqC().SwapCmds
						&& lnb->getDiSEqC().DiSEqCMode > eDiSEqC::V1_0
						&& ucsw != lastucsw )
						|| !(csw & 0xF0) )
					{
						cmdbefore = UNCOMMITTED;
#if HAVE_DVB_API_VERSION < 3
						commands[i].u.diseqc.params[0] = ucsw;
						commands[i].u.diseqc.cmd=0x39;
#else
						commands[i].msg[2]=0x39;
						commands[i].msg[3]=ucsw;
#endif
					}
					else
					{
#if HAVE_DVB_API_VERSION < 3
						commands[i].u.diseqc.params[0] = csw;
						commands[i].u.diseqc.cmd=0x38;
#else
						commands[i].msg[2]=0x38;
						commands[i].msg[3]=csw;
#endif
					}
//					eDebug("normalCmd");
//					eDebug("0x%02x,0x%02x,0x%02x,0x%02x", commands[i].u.diseqc.cmdtype, commands[i].u.diseqc.addr, commands[i].u.diseqc.cmd, commands[i].u.diseqc.params[0]);
					i++;
					if ( i < loops
						&& ( ( (cmdbefore == COMMITTED) && ucsw )
							|| ( (cmdbefore == UNCOMMITTED) && csw & 0xF0 ) ) )
					{
						memcpy( &commands[i], &commands[i-1], sizeof(commands[i]) );
						if ( cmdbefore == COMMITTED )
						{
#if HAVE_DVB_API_VERSION < 3
							commands[i].u.diseqc.cmd=0x39;
							commands[i].u.diseqc.params[0]=ucsw;
#else
							commands[i].msg[2]=0x39;
							commands[i].msg[3]=ucsw;
#endif
						}
						else
						{
#if HAVE_DVB_API_VERSION < 3
							commands[i].u.diseqc.cmd=0x38;
							commands[i].u.diseqc.params[0]=csw;
#else
							commands[i].msg[2]=0x38;
							commands[i].msg[3]=csw;
#endif
						}
//						eDebug("0x%02x,0x%02x,0x%02x,0x%02x", commands[i].u.diseqc.cmdtype, commands[i].u.diseqc.addr, commands[i].u.diseqc.cmd, commands[i].u.diseqc.params[0]);
						i++;
					}
				}
			}
		}
		if ( !cmdCount)
		{
			eDebug("send no DiSEqC");
			seq.commands=0;
		}
		seq.numCommands=cmdCount;
		eDebug("%d DiSEqC cmds to send", cmdCount);

		seq.toneBurst = eSecCmdSequence::NONE;
		if ( ucsw != lastucsw || csw != lastcsw || (ToneBurst && ToneBurst != lastToneBurst) )
		{
			if ( lnb->getDiSEqC().MiniDiSEqCParam == eDiSEqC::A )
			{
				eDebug("Toneburst A");
				seq.toneBurst=eSecCmdSequence::TONEBURST_A;
			}
			else if ( lnb->getDiSEqC().MiniDiSEqCParam == eDiSEqC::B )
			{
				eDebug("Toneburst B");
				seq.toneBurst=eSecCmdSequence::TONEBURST_B;
			}
			else
				eDebug("no Toneburst (MiniDiSEqC)");
		}
     
			// no DiSEqC related Stuff

		// calc Frequency
		int local = Frequency - ( Frequency > lnb->getLOFThreshold() ? lnb->getLOFHi() : lnb->getLOFLo() );
#if HAVE_DVB_API_VERSION < 3
		front.Frequency = abs(local);
#else
		front.frequency = abs(local);
#endif
		// set Continuous Tone ( 22 Khz... low - high band )
		if ( (swParams.HiLoSignal == eSwitchParameter::ON) || ( (swParams.HiLoSignal == eSwitchParameter::HILO) && (Frequency > lnb->getLOFThreshold()) ) )
			seq.continuousTone = eSecCmdSequence::TONE_ON;
		else if ( (swParams.HiLoSignal == eSwitchParameter::OFF) || ( (swParams.HiLoSignal == eSwitchParameter::HILO) && (Frequency <= lnb->getLOFThreshold()) ) )
			seq.continuousTone = eSecCmdSequence::TONE_OFF;

		seq.increasedVoltage = lnb->getIncreasedVoltage();

		// Voltage( 0/14/18V  vertical/horizontal )
		if ( swParams.VoltageMode == eSwitchParameter::_14V || ( polarisation == polVert && swParams.VoltageMode == eSwitchParameter::HV )  )
			voltage = eSecCmdSequence::VOLTAGE_13;
		else if ( swParams.VoltageMode == eSwitchParameter::_18V || ( polarisation==polHor && swParams.VoltageMode == eSwitchParameter::HV)  )
			voltage = eSecCmdSequence::VOLTAGE_18;
		else
			voltage = eSecCmdSequence::VOLTAGE_OFF;
     
		// set cmd ptr in sequence..
		seq.commands=commands;

		// handle DiSEqC Rotor
		if ( lastRotorCmd != RotorCmd && !noRotorCmd )
		{
//			eDebug("handle DISEqC Rotor.. cmdCount = %d", cmdCount);
/*			eDebug("0x%02x,0x%02x,0x%02x,0x%02x,0x%02x",
				commands[cmdCount-1].u.diseqc.cmdtype,
				commands[cmdCount-1].u.diseqc.addr,
				commands[cmdCount-1].u.diseqc.cmd,
				commands[cmdCount-1].u.diseqc.params[0],
				commands[cmdCount-1].u.diseqc.params[1]);*/

			// drive rotor always with 18V ( is faster )
			seq.voltage = eSecCmdSequence::VOLTAGE_18;

			lastRotorCmd=RotorCmd;

			increased = seq.increasedVoltage;
			newPos = sat->getOrbitalPosition();

			if ( lnb->getDiSEqC().useRotorInPower&1 )
			{
				DeltaA=(lnb->getDiSEqC().useRotorInPower & 0x0000FF00) >> 8;
				RotorUseInputPower(seq, lnb );
				finalTune=0;
			}
			else
			{
				RotorUseTimeout(seq, lnb );
				RotorFinish(false);  // set correct voltage...
			}
		}
		else if ( lastucsw != ucsw || ( ToneBurst && lastToneBurst != ToneBurst) )
		{
send:
			seq.voltage=voltage;
			if ( SendSequence(seq) < 0 )
			{
				eDebug("SendSequence failed (%m)");
				return -1;
			}
			else if ( lnb->getDiSEqC().DiSEqCMode >= eDiSEqC::V1_1 && lnb->getDiSEqC().SeqRepeat )  // Sequence Repeat ?
			{
				usleep( 100000 ); // between seq repeats we wait 80ms
				SendSequence(seq);  // just do it *g*
			}
		}
		else if ( lastcsw != csw )
		{
			// faster zap workaround... send only diseqc when satpos changed
			if ( lnb->getDiSEqC().FastDiSEqC && csw && (csw / 4) == (lastcsw / 4) )
			{
				eDebug("Satellite has not changed.. don't send DiSEqC cmd (Fast DiSEqC)");
				seq.numCommands=0;
			}
			goto send; // jump above...
		}
		else
			eDebug("no Band or Polarisation changed .. don't send Sequence");

		lastcsw = csw;
		lastucsw = ucsw;
		lastToneBurst = ToneBurst;
		lastLNB = lnb;  /* important.. for the right timeout
											 between normal diseqc cmd and rotor cmd */

		// delete allocated memory
		delete [] commands;
	}

	if (finalTune)
	{
#if HAVE_DVB_API_VERSION < 3
		front.Inversion = Inversion;
#else
		front.inversion = Inversion;
#endif
		switch (type)
		{
			case eSystemInfo::feCable:
				eDebug("Cable Frontend detected");
#if HAVE_DVB_API_VERSION < 3
				front.Frequency = Frequency;
				front.u.qam.QAM=QAM;
				front.u.qam.FEC_inner=FEC_inner;
				front.u.qam.SymbolRate=SymbolRate;
#else
				front.frequency = Frequency * 1000;
				front.u.qam.modulation=QAM;
				front.u.qam.fec_inner=FEC_inner;
				front.u.qam.symbol_rate=SymbolRate;
#endif
				break;
			case eSystemInfo::feSatellite:
				eDebug("Sat Frontend detected");
#if HAVE_DVB_API_VERSION < 3
				front.u.qpsk.FEC_inner=FEC_inner;
				front.u.qpsk.SymbolRate=SymbolRate;
#else
				front.u.qpsk.fec_inner=FEC_inner;
				front.u.qpsk.symbol_rate=SymbolRate;
#endif
				break;
			case eSystemInfo::feTerrestrial:
				eDebug("DVB-T Frontend detected");
				break;
		}
		if (ioctl(fd, FE_SET_FRONTEND, &front)<0)
		{
			eDebug("FE_SET_FRONTEND failed (%m)");
			return -1;
		}
		eDebug("FE_SET_FRONTEND OK");

		state=stateTuning;
		tries=20;

		timer->start(50, true);
  }
	return 0;
}

int eFrontend::tune_qpsk(eTransponder *transponder, 
		uint32_t Frequency, 		// absolute frequency in kHz
		int polarisation, 			// polarisation (polHor, polVert, ...)
		uint32_t SymbolRate,		// symbolrate in symbols/s (e.g. 27500000)
		uint8_t FEC_inner,			// FEC_inner (-1 for none, 0 for auto, but please don't use that)
		int Inversion,					// spectral inversion, INVERSION_OFF / _ON / _AUTO (but please...)
		eSatellite &sat)				// Satellite Data.. LNB, DiSEqC, switch..
{
	return tune(transponder, Frequency, polarisation, SymbolRate, getFEC(FEC_inner), Inversion==2?INVERSION_AUTO:Inversion?INVERSION_ON:INVERSION_OFF, &sat, QPSK);
}

int eFrontend::tune_qam(eTransponder *transponder, 
		uint32_t Frequency,			// absolute frequency in kHz
		uint32_t SymbolRate,		// symbolrate in symbols/s (e.g. 6900000)
		uint8_t FEC_inner,			// FEC_inner (-1 for none, 0 for auto, but please don't use that). normally -1.
		int Inversion,					// spectral inversion, INVERSION_OFF / _ON / _AUTO (but please...)
		int QAM)								// Modulation, QAM_xx
{
	return tune(transponder, Frequency, 0, SymbolRate, getFEC(FEC_inner), Inversion==2?INVERSION_AUTO:Inversion?INVERSION_ON:INVERSION_OFF, 0, getModulation(QAM));
}

int eFrontend::savePower()
{
#if HAVE_DVB_API_VERSION < 3
	if ( ioctl(fd, FE_SET_POWER_STATE, FE_POWER_OFF) < 0 )
		eDebug("FE_SET_POWER_STATE failed (%m)");

	if (secfd != -1)
	{        
		eSecCmdSequence seq;

		seq.commands=0;
		seq.numCommands=0;
		seq.voltage= eSecCmdSequence::VOLTAGE_OFF;
		seq.continuousTone = eSecCmdSequence::TONE_OFF;
		seq.toneBurst = eSecCmdSequence::NONE;
		if (SendSequence(seq) < 0 )
		{
			eDebug("SendSequence failed (%m)");
			return -1;
		}
	}
	needreset = 2;
#endif
	return 0;
}
