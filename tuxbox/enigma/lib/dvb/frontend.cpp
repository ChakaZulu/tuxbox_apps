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

eFrontend::eFrontend(int type, const char *demod)
:type(type), curRotorPos( 1000 ), timer2(eApp), noRotorCmd(0)
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
		// reset all diseqc devices
		if ( sendDiSEqCCmd( 0, 0 ) )
			exit(0);
	}
		
	lastRotorCmd=-1;
}

void eFrontend::InitDiSEqC()
{
	lastRotorCmd = -1;
	sendDiSEqCCmd( 0, 0 );
	usleep(50000);
}

void eFrontend::timeout()
{
	if (Locked())
	{
		eDebug("+");
		state=stateIdle;

		if ( transponder->satellite.valid )
		{
			dvb_frontend_event front;
			if (ioctl(fd, FE_GET_FRONTEND, &front)<0)
				perror("FE_GET_FRONTEND");
			else
			{
				eDebug("FE_GET_FRONTEND OK");
/*				eSatellite * sat = eTransponderList::getInstance()->findSatellite(transponder->satellite.orbital_position);
				if (sat)
				{
					eLNB *lnb = sat->getLNB();
					if (lnb)
					{
						transponder->satellite.frequency = transponder->satellite.frequency > lnb->getLOFThreshold() ?
								front.Frequency + lnb->getLOFHi() :
								front.Frequency + lnb->getLOFLo();
					}
				}
				transponder->satellite.fec = front.u.qpsk.FEC_inner;
				transponder->satellite.symbol_rate = front.u.qpsk.SymbolRate;*/
				transponder->satellite.inversion = front.parameters.inversion;
			}
		}

		/*emit*/ tunedIn(transponder, 0);
	}
	else
		if (--tries)
		{
			eDebug("-: %x", Status());
			timer->start(100, true);
		}
		else
		{
			eDebug("couldn't lock. (state: %x)", Status());
			state=stateIdle;
//			if ( transponder->satellite.inversion )
				/*emit*/ tunedIn(transponder, -ETIMEDOUT);
/*			else
			{
				transponder->satellite.inversion = 1;
				transponder->tune();
			}*/
		}
}

eFrontend::~eFrontend()
{
	if (fd>=0)
		::close(fd);
	frontend=0;
}

int eFrontend::Status()
{
	fe_status_t status;
	ioctl(fd, FE_READ_STATUS, &status);
	return status;
}
 
uint32_t eFrontend::BER()
{
	uint32_t ber;
	ioctl(fd, FE_READ_BER, &ber);
	return ber;
}

int eFrontend::SignalStrength()
{
	uint16_t strength;
	ioctl(fd, FE_READ_SIGNAL_STRENGTH, &strength);
	return strength;
}

int eFrontend::SNR()
{
	uint16_t snr;
	ioctl(fd, FE_READ_SNR, &snr);
	return snr;
}

uint32_t eFrontend::UncorrectedBlocks()
{
	uint32_t ublocks=0;
	ioctl(fd, FE_READ_UNCORRECTED_BLOCKS, &ublocks);
	return ublocks;
}

static fe_code_rate_t getFEC(int fec)		// etsi -> api
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

static fe_modulation_t getModulation(int mod)
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
		return QAM_AUTO;
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
	struct dvb_diseqc_master_cmd cmd;

	cmd.msg[0]=frame;
	cmd.msg[1]=addr;
	cmd.msg[2]=Cmd;
	cmd.msg_len=0;

	for (uint8_t i = 0; i < params.length() && i < 6; i += 2)
		cmd.msg[3 + cmd.msg_len++] = strtol( params.mid(i, 2).c_str(), 0, 16 );

	if (ioctl(fd, FE_SET_TONE, SEC_TONE_OFF) < 0) {
		perror("FE_SET_TONE");
		return -1;
	}

	if (ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &cmd) < 0) {
		perror("FE_DISEQC_SEND_MASTER_CMD");
		return -1;
	}
	
	lastcsw = -1;
	return 0;
}

#if 0
int eFrontend::RotorUseTimeout(secCmdSequence& seq, int newPosition )
{
	int TimePerDegree=1000; // msec
	int startDelay=800;//1000;

	// send DiSEqC Sequence ( normal diseqc switches )
	if ( ioctl(secfd, SEC_SEND_SEQUENCE, &seq) < 0 )
	{
		perror("SEC_SEND_SEQUENCE");
		return -1;
 	}

	/* emit */ rotorRunning();

	if ( curRotorPos != 1000 ) // uninitialized  
		usleep( (abs(newPosition - curRotorPos) * TimePerDegree * 100) + startDelay );

	/* emit */ rotorStopped();

	curRotorPos = newPosition;

	return 0;
}

int eFrontend::RotorUseInputPower(secCmdSequence& seq, void *cmds, int SeqRepeat )
{
	secCommand *commands = (secCommand*) cmds;
	int idlePowerInput=0;
	int runningPowerInput=0;
//	int cnt=0;

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

	// send DiSEqC Sequence ( normal diseqc switches )
	if ( ioctl(secfd, SEC_SEND_SEQUENCE, &seq) < 0 )
	{
		perror("SEC_SEND_SEQUENCE");
		return -1;
	}
	else if ( SeqRepeat )   // Sequence Repeat selected ?
	{
		usleep( 80000 ); // between seq repeats we wait 80ms
		ioctl(secfd, SEC_SEND_SEQUENCE, &seq);  // then repeat the cmd
	}
	usleep( 80000 ); // wait 80ms

	// get power input of Rotor on idle  not work on dbox yet .. only dreambox
	if (ioctl(fp, FP_IOCTL_GET_LNB_CURRENT, &idlePowerInput )<0)
	{
		eDebug("FP_IOCTL_GET_LNB_CURRENT sucks.\n");
		return -1;
	}
//	eDebug("idle power input = %dmA", idlePowerInput );

	// send DiSEqC Sequence (Rotor)
	seq.commands=&commands[seq.numCommands];  // last command is rotor cmd... see above...
	seq.numCommands=1;  // only rotor cmd

	if ( ioctl(secfd, SEC_SEND_SEQUENCE, &seq) < 0 )
	{
		perror("SEC_SEND_SEQUENCE");
		return -1;
	}
	else if ( SeqRepeat )  // Sequence repeat ?
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
//		eDebug("(%d) %d mA\n", cnt, runningPowerInput);
//		cnt++;

		if ( abs(runningPowerInput-idlePowerInput ) < 70 ) // rotor running ?
		{
			usleep(50000);  // not... then wait 50ms
		}
		else  // rotor is running
		{
			/* emit */ rotorRunning();          
			timeout=0;
			break;  // leave endless loop
		}

		if ( timeout <= time(0) )   // timeout
		{
			/* emit */ rotorTimeout();                                
			break;
		}
	}

	if ( !timeout )  // then the Rotor is Running... we wait if it stops..
	{
		// set rotor timeout to 30sec's...
		timeout = time(0) + 30;
//		cnt=0;
		while(true)
		{
			if (ioctl(fp, FP_IOCTL_GET_LNB_CURRENT, &runningPowerInput)<0)
			{
				printf("FP_IOCTL_GET_LNB_CURRENT sucks.\n");
				return 2;
			}
//				eDebug("(%d) %d mA", cnt, runningPowerInput);
//				cnt++;

			if ( abs( idlePowerInput-runningPowerInput ) > 70 ) // rotor stoped ?
			{
				usleep(50000);  // not... then wait 50ms
			}
			else  // rotor has stopped
			{
				/* emit */ rotorStopped();                      
				break;
			}

			if ( timeout <= time(0) ) // Rotor has timouted
			{
				/* emit */ rotorTimeout();                      
				break;
			}
		}

	}
	::close(fp);

	return 0;
}
#endif

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

					Rstation = r_eq / ( std::sqrt( 1.00 - f*(2.00-f)*std::sin(Radians(SiteLat))*std::sin(Radians(SiteLat)) ) ),

					Ra = (Rstation+Height_over_ocean)*std::cos(Radians(SiteLat)),
					Rz= Rstation*(1.00-f)*(1.00-f)*std::sin(Radians(SiteLat)),
//			alfa_r = r_sat - Rstation,

					alfa_rx=r_sat*std::cos(Radians(SatLon-SiteLon)) - Ra,
					alfa_ry=r_sat*std::sin(Radians(SatLon-SiteLon)),
					alfa_rz=-Rz,

					alfa_r_north=-alfa_rx*std::sin(Radians(SiteLat)) + alfa_rz*std::cos(Radians(SiteLat)),
					alfa_r_zenith=alfa_rx*std::cos(Radians(SiteLat)) + alfa_rz*std::sin(Radians(SiteLat)),

					El_geometric=Deg(std::atan2( alfa_r_zenith , std::sqrt(alfa_r_north*alfa_r_north+alfa_ry*alfa_ry))),


					x = std::fabs(El_geometric+0.589),
					refraction=std::fabs(a0+a1*x+a2*x*x+a3*x*x*x+a4*x*x*x*x),
          El_observed = 0.00;

	if (El_geometric > 10.2)
		El_observed = El_geometric+0.01617*(std::cos(Radians(std::abs(El_geometric)))/std::sin(Radians(std::abs(El_geometric))) );
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

					Rstation = r_eq / ( std::sqrt( 1 - f*(2-f)*std::sin(Radians(SiteLat))*std::sin(Radians(SiteLat)) ) ),
					Ra = (Rstation+Height_over_ocean)*std::cos(Radians(SiteLat)),
					Rz = Rstation*(1-f)*(1-f)*std::sin(Radians(SiteLat)),
//					alfa_r = r_sat-Rstation,

					alfa_rx = r_sat*std::cos(Radians(SatLon-SiteLon)) - Ra,
					alfa_ry = r_sat*std::sin(Radians(SatLon-SiteLon)),
					alfa_rz = -Rz,

					alfa_r_north = -alfa_rx*std::sin(Radians(SiteLat)) + alfa_rz*std::cos(Radians(SiteLat)),
//					alfa_r_zenith = alfa_rx*std::cos(Radians(SiteLat)) + alfa_rz*std::sin(Radians(SiteLat)),
					Azimuth = 0.00;

					if (alfa_r_north < 0)
						Azimuth = 180+Deg(std::atan(alfa_ry/alfa_r_north));
					else
						Azimuth = Rev(360+Deg(std::atan(alfa_ry/alfa_r_north)));

	return Azimuth;
}

double calcDeclination( double SiteLat, double Azimuth, double Elevation)
{
	return Deg( std::asin(std::sin(Radians(Elevation)) *
												std::sin(Radians(SiteLat)) +
												std::cos(Radians(Elevation)) *
												std::cos(Radians(SiteLat)) *
												std::cos(Radians(Azimuth))
												)
						);
}

double calcSatHourangle( double Azimuth, double Elevation, double Declination, double Lat )
{
	double a = - std::cos(Radians(Elevation)) *
							 std::sin(Radians(Azimuth)),

				 b = std::sin(Radians(Elevation)) *
						 std::cos(Radians(Lat)) -
						 std::cos(Radians(Elevation)) *
						 std::sin(Radians(Lat)) *
						 std::cos(Radians(Azimuth)),

// Works for all azimuths (northern & sourhern hemisphere)
						 returnvalue = 180 + Deg(std::atan2(a,b));

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
		fe_code_rate_t FEC_inner,			// FEC_inner api
		fe_spectral_inversion_t Inversion,	// spectral inversion, INVERSION_OFF / _ON / _AUTO (but please...)
		eSatellite* sat,
		fe_modulation_t QAM)				// Modulation, QAM_xx
{
	struct dvb_frontend_parameters front;
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
		struct dvb_diseqc_master_cmd* commands = NULL;
		eSwitchParameter &swParams = sat->getSwitchParams();
		eLNB *lnb = sat->getLNB();
    		// Variables to detect if DiSEqC must sent .. or not
		int csw = lnb->getDiSEqC().DiSEqCParam,
		    ToneBurst = lnb->getDiSEqC().MiniDiSEqCParam,
		    RotorCmd = -1;

		int cmdCount=0;
		
		if (csw <= eDiSEqC::BB)  // use AA/AB/BA/BB ?
		{
			eDebug("csw=%d, csw<<2=%d", csw, csw << 2);
			csw = 0xF0 | ( csw << 2 );
			if ( polarisation==polHor )
				csw |= 2;  // Horizontal

			if ( Frequency > lnb->getLOFThreshold() )
				csw |= 1;   // 22 Khz enabled
		}
		//else we sent directly the cmd 0xF0..0xFF

		eDebug("DiSEqC Switch cmd = %04x", csw);

		// Rotor Support
		if ( lnb->getDiSEqC().DiSEqCMode == eDiSEqC::V1_2 && !noRotorCmd )
		{
			if ( lnb->getDiSEqC().useGotoXX )
			{
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
			}
#if 0
			else  // we use builtin rotor sat table
			{
				std::map<int,int>::iterator it = lnb->getDiSEqC().RotorTable.find( sat->getOrbitalPosition() );

				if (it != lnb->getDiSEqC().RotorTable.end())  // position for selected sat found ?
					RotorCmd=it->second;
				else  // entry not in table found
					eDebug("Entry for %d,%d° not in Rotor Table found... please add", sat->getOrbitalPosition() / 10, sat->getOrbitalPosition() % 10 );
			}

			if ( RotorCmd != lastRotorCmd )  // rotorCmd must sent?
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
					eDebug("Rotor DiSEqC Param = %04x (useGotoXX)", RotorCmd);
					commands[cmdCount-1].u.diseqc.cmd=0x6E; // gotoXX Drive Motor to Angular Position
					commands[cmdCount-1].u.diseqc.numParams=2;
					commands[cmdCount-1].u.diseqc.params[0]=((RotorCmd & 0xFF00) / 0x100);
					commands[cmdCount-1].u.diseqc.params[1]=RotorCmd & 0xFF;
				}
				else
				{
					eDebug("Rotor DiSEqC Param = %02x (use stored position)", RotorCmd);
					commands[cmdCount-1].u.diseqc.cmd=0x6B;  // goto stored sat position
					commands[cmdCount-1].u.diseqc.numParams=1;
					commands[cmdCount-1].u.diseqc.params[0]=RotorCmd;
				}
			}
		}  
		if ( lnb->getDiSEqC().DiSEqCMode == eDiSEqC::SMATV )
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
#endif
		}

		if ( lnb->getDiSEqC().DiSEqCMode >= eDiSEqC::V1_0 )
		{
			if (csw != lastcsw || (ToneBurst && ToneBurst != lastToneBurst) )
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
					commands = new dvb_diseqc_master_cmd[cmdCount];
			}

			for ( int i = 0; i < loops;)  // fill commands...
			{
				commands[i].msg[0]= i ? 0xE1 : 0xE0; // repeated or not repeated transm.
				commands[i].msg[1]= 0x10;

				// when DiSEqC V1.0 is avail then loops is always 1 .. see above

				if ( loops > 1 && lnb->getDiSEqC().uncommitted_switch )
				{
					commands[i].msg[2] = 0x39;          // uncomitted switch
					eDebug("0x39");
				}
				else // DiSEqC < V1.1 do not support repeats
				{
					commands[i].msg[2] = 0x38;          // comitted switch
					eDebug("0x38");
				}

				commands[i].msg[3] = csw;
				commands[i].msg_len = 4;

				i++;

				if ( i < loops && lnb->getDiSEqC().uncommitted_gap )
				{
					memcpy( &commands[i], &commands[i-1], sizeof(struct dvb_diseqc_master_cmd) );
					commands[i].msg[0]=0xE1;
					commands[i].msg[2]=0x39;
					i++;
				}
			}
		}
	}
	else // no DiSEqC
	{
		eDebug("no DiSEqC");
		cmdCount = 0;
	}

	eDebug("Commands to send = %d", cmdCount);

	// calc Frequency
	int local = Frequency - ( Frequency > lnb->getLOFThreshold() ? lnb->getLOFHi() : lnb->getLOFLo() );
	front.frequency = local > 0 ? local : -local;

	// FIXME: set only once
	if (lnb->getIncreasedVoltage())
		ioctl(fd, FE_ENABLE_HIGH_LNB_VOLTAGE, 1);

	// disable tone before anything else
	if ( csw != lastcsw )
		ioctl(fd, FE_SET_TONE, SEC_TONE_OFF);
    
	// Voltage( 0/14/18V  vertical/horizontal )
	if ( swParams.VoltageMode == eSwitchParameter::_14V || ( polarisation == polVert && swParams.VoltageMode == eSwitchParameter::HV )  )
	{
		ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
		usleep(15 * 1000);
	}
	else if ( swParams.VoltageMode == eSwitchParameter::_18V || ( polarisation==polHor && swParams.VoltageMode == eSwitchParameter::HV)  )
	{
		ioctl(fd, FE_SET_VOLTAGE, SEC_VOLTAGE_18);
		usleep(15 * 1000);
	}

#if 0
	// handle DiSEqC Rotor
	if ( lnb->getDiSEqC().DiSEqCMode == eDiSEqC::V1_2 && (lastRotorCmd != RotorCmd) )
	{
		lastRotorCmd = RotorCmd;

		// drive rotor always with 18V ( is faster )
		seq.voltage = SEC_VOLTAGE_18;

		RotorUseInputPower(seq, (void*) commands, lnb->getDiSEqC().SeqRepeat );
		//RotorUseTimeout(seq, sat->getOrbitalPosition() );

		// set the right voltage
		if ( voltage != SEC_VOLTAGE_18 )
			ioctl(secfd, SEC_SET_VOLTAGE, &voltage);
	}
	else
#endif

	// DiSEqC commands
	if ( csw != lastcsw && lnb->getDiSEqC().DiSEqCMode >= eDiSEqC::V1_0 && commands && cmdCount)
	{
		ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &commands[0]);

		for (int i = 1; i < cmdCount; i++) 
		{
			usleep(80 * 1000);
			ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &commands[i]);
		}

		usleep(15 * 1000);
	}

        // Toneburst (MiniDiSEqC)
	if ( ToneBurst && ToneBurst != lastToneBurst )
	{
		if ( lnb->getDiSEqC().MiniDiSEqCParam == eDiSEqC::A)
		{
			ioctl(fd, FE_DISEQC_SEND_BURST, SEC_MINI_A);
			usleep(15 * 1000);
		}
		else if (lnb->getDiSEqC().MiniDiSEqCParam == eDiSEqC::B)
		{
			ioctl(fd, FE_DISEQC_SEND_BURST, SEC_MINI_B);
			usleep(15 * 1000);
		}
	}

	// set Continuous Tone ( 22 Khz... low - high band )
	if ( csw != lastcsw || ToneBurst != lastToneBurst )
	{
    		if ( (swParams.HiLoSignal == eSwitchParameter::ON) || ( (swParams.HiLoSignal == eSwitchParameter::HILO) && (Frequency > lnb->getLOFThreshold()) ) )
    		{
			ioctl(fd, FE_SET_TONE, SEC_TONE_ON);
		}
		else if ( (swParams.HiLoSignal == eSwitchParameter::OFF) || ( (swParams.HiLoSignal == eSwitchParameter::HILO) && (Frequency <= lnb->getLOFThreshold()) ) )
		{
			ioctl(fd, FE_SET_TONE, SEC_TONE_OFF);
		}
	}

	lastcsw = csw;
	lastToneBurst = ToneBurst;
	
	// delete allocated memory...
	if (cmdCount)
		delete [] commands;
	}
	else 
	{
		// we have only a cable box
		eDebug("no valid LNB... Cable Box ?");
	}


	front.inversion=Inversion;

	switch (type)
	{
		case feCable:
			eDebug("Cable Frontend detected");
			front.frequency = Frequency * 1000;
			front.u.qam.symbol_rate=SymbolRate;
			front.u.qam.fec_inner=FEC_inner;
			front.u.qam.modulation=QAM;
			break;
		case feSatellite:
			eDebug("Sat Frontend detected");
			front.u.qpsk.symbol_rate=SymbolRate;
			front.u.qpsk.fec_inner=FEC_inner;
			break;
	}
	if (ioctl(fd, FE_SET_FRONTEND, &front)<0)
	{
		perror("FE_SET_FRONTEND");
		return -1;
 	}
	eDebug("FE_SET_FRONTEND OK");
	eDebug("Symbolrate = %d", SymbolRate );
	state=stateTuning;
//	tries=30000000*2 / SymbolRate; // 1.0 second timeout
//	tries=tries<5?5:tries;
	tries=30;
	eDebug("tries=%d", tries);
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
