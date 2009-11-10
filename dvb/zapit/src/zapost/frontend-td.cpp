/*
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
 *
 * (C) 2007-2009 Stefan Seyfried
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* system c */
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

/* zapit */
#include <zapit/debug.h>
#include <zapit/frontend.h>
#include <zapit/settings.h>

#define diff(x,y)	(max(x,y) - min(x,y))
#define min(x,y)	((x < y) ? x : y)
#define max(x,y)	((x > y) ? x : y)

CFrontend::CFrontend(int _uncommitted_switch_mode, int extra_flags)
{
	tuned = false;
	currentToneMode = SEC_TONE_OFF;
	currentTransponder.polarization = VERTICAL;
	currentTransponder.feparams.frequency = 0;
	diseqcRepeats = 0;
	diseqcType = NO_DISEQC;
	secfd = -1;

	info.type = FE_QPSK; // only SAT tuner in TD

	uncommitted_switch_mode = _uncommitted_switch_mode;
	auto_fec = extra_flags & 1;
	finetune = extra_flags & 2;
	if ((uncommitted_switch_mode<0) || (uncommitted_switch_mode>2)) uncommitted_switch_mode = 0;
	printf("[frontend] uncommitted_switch_mode %d auto_fec %d finetune %d\n", uncommitted_switch_mode, auto_fec, finetune);

	if ((fd = open(FRONTEND_DEVICE, O_RDWR|O_NONBLOCK)) < 0)
		ERROR(FRONTEND_DEVICE);

	/* the order of those events was empirically determined (and by strace'ing the original app)
	   I have no idea if this is the best or the most efficient way to set up the frontend, but
	   it seems to work */
	fop(ioctl, IOC_TUNER_DISEQC_MODE, DISEQC_MODE_CMD_ONLY);
	fop(ioctl, IOC_TUNER_LNBLOOPTHROUGH, 0);
	fop(ioctl, IOC_TUNER_SET_STANDBY, 0);
	fop(ioctl, IOC_TUNER_HARDRESET);
	fop(ioctl, IOC_TUNER_INIT);
	fop(ioctl, IOC_TUNER_LNB_ENABLE, 1);
	fop(ioctl, IOC_TUNER_LNB_SET_HORIZONTAL);
	secfd = fd;
}

CFrontend::~CFrontend(void)
{
	if (diseqcType > MINI_DISEQC)
		sendDiseqcStandby();

	fop(ioctl, IOC_TUNER_LOCKLED_ENABLE, 0);
	fop(ioctl, IOC_TUNER_LNB_ENABLE, 0);
	fop(ioctl, IOC_TUNER_22KHZ_ENABLE, 0);
	fop(ioctl, IOC_TUNER_SET_STANDBY, 1);
	fop(ioctl, IOC_TUNER_LNBLOOPTHROUGH, 1);

	close(fd);
}

void CFrontend::reset(void)
{
WARN("======================================== CFrontend::reset =================================");
	close(fd);

	if ((fd = open(FRONTEND_DEVICE, O_RDWR|O_NONBLOCK|O_SYNC)) < 0)
		ERROR(FRONTEND_DEVICE);
}

fe_code_rate_t CFrontend::getCodeRate(const uint8_t fec_inner)
{
	switch (fec_inner & 0x0F) {
	case 0x01:
		return (fe_code_rate_t)TD_FEC_1_2;
	case 0x02:
		return (fe_code_rate_t)TD_FEC_2_3;
	case 0x03:
		return (fe_code_rate_t)TD_FEC_3_4;
	case 0x04:
		return (fe_code_rate_t)TD_FEC_5_6;
	case 0x05:
		return (fe_code_rate_t)TD_FEC_7_8;
	case 0x0F:
//		return (fe_code_rate_t)TD_FEC_NONE;
	default:
		return (fe_code_rate_t)TD_FEC_AUTO;
	}
}

unsigned int CFrontend::xml2FEC(const uint8_t fec)
{
	switch (fec) {
	case 0x01:
		return (fe_code_rate_t)TD_FEC_1_2;
	case 0x02:
		return (fe_code_rate_t)TD_FEC_2_3;
	case 0x03:
		return (fe_code_rate_t)TD_FEC_3_4;
	case 0x05:
		return (fe_code_rate_t)TD_FEC_5_6;
	case 0x07:
		return (fe_code_rate_t)TD_FEC_7_8;
	default:
		break;
	}
	return (fe_code_rate_t)TD_FEC_AUTO;
}

uint8_t CFrontend::FEC2xml(const unsigned int fec)
{
	switch (fec)
	{
	case TD_FEC_1_2:
		return 1;
	case TD_FEC_2_3:
		return 2;
	case TD_FEC_3_4:
		return 3;
	case TD_FEC_5_6:
		return 5;
	case TD_FEC_7_8:
		return 7;
	default:
		return 9;
	}
}

uint32_t CFrontend::getFrequency(void) const
{
	if (currentToneMode == SEC_TONE_OFF)
		return currentTransponder.feparams.frequency + lnbOffsetsLow[currentTransponder.diseqc];
	else
		return currentTransponder.feparams.frequency + lnbOffsetsHigh[currentTransponder.diseqc];
}

void CFrontend::setUncommittedSwitchMode(const int mode)
{
	uncommitted_switch_mode = mode;
	if ((uncommitted_switch_mode<0) || (uncommitted_switch_mode>2))
		uncommitted_switch_mode = 0;
	printf("[frontend] uncommitted_switch_mode %d\n", uncommitted_switch_mode);
}

int CFrontend::getUncommittedSwitchMode(void) const
{
	return uncommitted_switch_mode;
}

uint8_t CFrontend::getPolarization(void) const
{
	return currentTransponder.polarization;
}

fe_status_t CFrontend::getStatus(void) const
{
	fe_status_t status;

	fop(ioctl, FE_READ_STATUS, &status);

	return status;
}

uint32_t CFrontend::getBitErrorRate(void) const
{
	uint32_t ber;
	fop(ioctl, IOC_TUNER_SET_ERROR_SOURCE, QPSKERRORSOURCE_QPSK_BIT_ERRORS);
	fop(ioctl, IOC_TUNER_GET_ERRORS, &ber);

	return ber;
}

uint16_t CFrontend::getSignalStrength(void) const
{
	uint16_t strength;

	fop(ioctl, FE_READ_SIGNAL_STRENGTH, &strength);

	return strength;
}

uint16_t CFrontend::getSignalNoiseRatio(void) const
{
	uint16_t snr;

	fop(ioctl, FE_READ_SNR, &snr);

	return snr;
}

uint32_t CFrontend::getUncorrectedBlocks(void) const
{
	uint32_t blocks = 0;

	fop(ioctl, IOC_TUNER_SET_ERROR_SOURCE, QPSKERRORSOURCE_PACKET_ERRORS);
	fop(ioctl, IOC_TUNER_GET_ERRORS, &blocks);

	return blocks;
}

void CFrontend::setFrontend(const tunersetup *tuner)
{
	tunersetup t2;
	memcpy(&t2, tuner, sizeof(t2));
	/* again, the order of events as strace'd in the original app */

	quiet_fop(ioctl, IOC_TUNER_LOCKLED_ENABLE, 0);
	fop(ioctl, IOC_TUNER_STOPTHAT);

	int freq_range = tuner->symbolrate / 500;	/* empirically: SR 22000, FRANGE = 44 */
	fop(ioctl, IOC_TUNER_SET_FRANGE, freq_range);	/* SR 27500, FRANGE = 55, 29900 == 59 */

	if (auto_fec)
		t2.fec = 0;

	WARN("freq: %u sym: %hu fec: %hu(%hu) pol: %hu high: %hu frange: %d",
		tuner->frequency, tuner->symbolrate, tuner->fec, t2.fec, tuner->polarity, tuner->highband, freq_range);
	if (finetune)
		fop(ioctl, IOC_TUNER_SETUP_NBF, &t2);
	else
		fop(ioctl, IOC_TUNER_SETUP_NB, &t2);
//	fop(ioctl, IOC_TUNER_SETUP_NB, tuner);
}

/* the timeout is large, but we get a POLLERR from the driver if it decides
   that there is nothing to do anymore, so this should be ok */
#define TIME_STEP 1000
#define TIMEOUT_MAX_MS (25 * TIME_STEP)

tunersetup CFrontend::getFrontend(void) const
{
	return currentTransponder.feparams;
}

dvb_frontend_event CFrontend::getEvent(void)
{
	dvb_frontend_event event;
	struct pollfd pfd;
	struct timeval tv;
	long long start;
	long long now;

	gettimeofday(&tv, NULL);
	start = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	pfd.fd = fd;
	pfd.events = POLLERR;
	pfd.revents = 0;

	memset(&event, 0, sizeof(dvb_frontend_event));

	if (finetune)
		pfd.events |= POLLPRI;
	else
		pfd.events |= POLLIN;

	int ret;

	now = start;
	while (now < start + TIMEOUT_MAX_MS)
	{
		ret = poll(&pfd, 1, TIME_STEP); /* maybe we should just do TIMEOUT_MAX_MS ? */

		gettimeofday(&tv, NULL);
		now = tv.tv_sec * 1000 + tv.tv_usec / 1000;

		if (ret == 0)
		{
			WARN("poll timeout - total %lld ms", now - start);
			continue;
		}
		if (ret < 0)
		{
			WARN("poll error: %m");
			break;
		}

		quiet_fop(ioctl, FE_READ_STATUS, &event.status);
		WARN("FE status - 0x%02x %s%s%s%s%s%s%s %s%s%s", event.status, \
			event.status & FE_HAS_SIGNAL	? "SIGNAL ":"", \
			event.status & FE_HAS_CARRIER	? "CARRIER ":"", \
			event.status & FE_HAS_VITERBI	? "VITERBI ":"", \
			event.status & FE_HAS_SYNC	? "SYNC ":"", \
			event.status & FE_HAS_LOCK	? "LOCK ":"", \
			event.status & FE_TIMEDOUT	? "TIMEDOUT ":"", \
			event.status & FE_REINIT	? "REINIT ":"", \
			pfd.revents & POLLIN	? "POLLIN ":"", \
			pfd.revents & POLLPRI	? "POLLPRI ":"", \
			pfd.revents & POLLERR	? "POLLERR ":"");

		if (pfd.revents & POLLERR)
			break;

		if (event.status == 0x1f) // FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC|FE_HAS_LOCK
			tuned = true;

		if (event.status & FE_TIMEDOUT) /* does this ever happen? */
			break;

		/* when finetune is used (SETUP_NBF), we get a POLLPRI when tuning is finished,
		   otherwise we get a POLLIN */
		if (finetune && tuned && pfd.revents & POLLPRI)
			break;
		if (!finetune && tuned && pfd.revents & POLLIN)
			break;
	}

	if (ioctl(fd, IOC_TUNER_SETUP_GET, &event.parameters) < 0)
		WARN("IOC_TUNER_SETUP_GET failed: %m");

	if (!tuned)
		currentTransponder.feparams.frequency = 0;
	else
		currentTransponder.feparams.frequency = event.parameters.realfrq;

	WARN("%stuned after %lld ms realfrq: %d", tuned?"":"not ", now - start, currentTransponder.feparams.frequency);
	quiet_fop(ioctl, IOC_TUNER_LOCKLED_ENABLE, (unsigned int)tuned);
	return event;
}

#ifdef DEBUG_SEC_TIMING

#define TIMER_START()									\
	struct timeval tv, tv2;								\
	unsigned int msec;								\
	gettimeofday(&tv, NULL)

#define TIMER_STOP()									\
	gettimeofday(&tv2, NULL);							\
	msec = ((tv2.tv_sec - tv.tv_sec) * 1000) + ((tv2.tv_usec - tv.tv_usec) / 1000); \
	DBG("%u msec", msec)

#else /* DEBUG_SEC_TIMING */

#define TIMER_START() \
	do {} while (0)

#define TIMER_STOP() \
	do {} while (0)

#endif /* DEBUG_SEC_TIMING */

void CFrontend::secSetTone(const fe_sec_tone_mode_t toneMode, const uint32_t ms)
{
	TIMER_START();
	unsigned int tm;
	if (toneMode == SEC_TONE_ON)
		tm = 1;
	else
		tm = 0;

	DBG("IOC_TUNER_22KHZ_ENABLE %d", tm);
	if (quiet_fop(ioctl, IOC_TUNER_22KHZ_ENABLE, tm) == 0) {
		currentToneMode = toneMode;
		usleep(1000 * ms);
	}

	TIMER_STOP();
}

void CFrontend::secSetVoltage(const fe_sec_voltage_t voltage, const uint32_t ms)
{
	TIMER_START();

	unsigned int enable, pol;
	enable = (voltage != SEC_VOLTAGE_OFF);
	pol = (voltage == SEC_VOLTAGE_13);

//	WARN("enable: %d pol: %d", enable, pol);
//	fop(ioctl, IOC_TUNER_LNB_ENABLE, enable);
//	if (!enable)
//		return;

	if (fop(ioctl, IOC_TUNER_LNB_SET_POLARISATION, pol) == 0) {
		currentTransponder.polarization = voltage;
		usleep(1000 * ms);
	}

	TIMER_STOP();
}

void CFrontend::secResetOverload(void)
{
	TIMER_START();
	printf("CFrontend::secResetOverload() not implemented on Tripledragon\n");
	TIMER_STOP();
}

void CFrontend::sendDiseqcCommand(const struct dvb_diseqc_master_cmd *cmd, const uint32_t ms)
{
	TIMER_START();
	char raw[1 + 3 + 3]; // length_byte + frame + addr + cmd + params[3]
	memset(&raw[0], 0, 7);
	raw[0] = cmd->msg_len;
	memcpy(&raw[1], cmd->msg, cmd->msg_len);
	WARN("ms: %d raw: len:%02x frame:%02x addr:%02x cmd:%02x para:%02x:%02x:%02x",
		ms, raw[0],raw[1],raw[2],raw[3],raw[4],raw[5],raw[6]);
#if 0
	/* set voltage if not set... */
	if (curVoltage == -1) {
		if (ioctl(fd, IOC_TUNER_LNB_ENABLE) < 0)
			eDebug("IOC_TUNER_LNB_ENABLE failed (%m)");
		if (ioctl(fd, IOC_TUNER_LNB_SET_HORIZONTAL) < 0)
			eDebug("IOC_TUNER_LNB_SET_HORIZONTAL failed (%m)");
	}
#endif

	if (fop_sec(ioctl, IOC_TUNER_DISEQC_SEND, &raw) == 0)
		usleep(1000 * ms);

	TIMER_STOP();
}

void CFrontend::sendToneBurst(const fe_sec_mini_cmd_t burst, const uint32_t ms)
{
	TIMER_START();
	int ret;
	/* even though it looks useful, this ioctl seems not to be used in the original soft */
	// fop(ioctl, IOC_TUNER_DISEQC_MODE, DISEQC_MODE_BURST_ONLY);
	switch (burst)
	{
	case SEC_MINI_A:
		ret = fop(ioctl, IOC_TUNER_DISEQC_SEND_BURST, 0);
//		ret = ioctl(fd, IOC_TUNER_DISEQC_SEND, "\x04\xe0\x10\x38\xf0");
		break;
	case SEC_MINI_B:
		ret = fop(ioctl, IOC_TUNER_DISEQC_SEND_BURST, 1);
//		ret = ioctl(fd, IOC_TUNER_DISEQC_SEND, "\x04\xe0\x10\x38\xf4");
		break;
	default:
		WARN("not MINI_A(%d)/MINI_B(%d): %d!", SEC_MINI_A, SEC_MINI_B, burst);
		return;
	}
	if (ret < 0)
	{
		WARN("IOC_TUNER_DISEQC_SEND_BURST failed (%m)");
		return;
	}
#if 0
	secCmdSequence sequence;
	secCommand command;
	memset(&sequence, 0, sizeof(sequence));
	memset(&command, 0, sizeof(command));

	command.type = SEC_CMDTYPE_DISEQC_RAW;
	sequence.miniCommand = burst;
	sequence.continuousTone = currentToneMode;
	sequence.voltage = currentTransponder.polarization;
	sequence.commands = &command;
	sequence.numCommands = 0;

	if (fop_sec(ioctl, SEC_SEND_SEQUENCE, sequence) == 0)
#endif
	usleep(ms * 1000);
	TIMER_STOP();
}

void CFrontend::setDiseqcType(const diseqc_t newDiseqcType)
{
	switch (newDiseqcType) {
	case NO_DISEQC:
		INFO("NO_DISEQC");
		break;
	case MINI_DISEQC:
		INFO("MINI_DISEQC");
		break;
	case SMATV_REMOTE_TUNING:
		INFO("SMATV_REMOTE_TUNING");
		break;
	case DISEQC_1_0:
		INFO("DISEQC_1_0");
		break;
	case DISEQC_1_1:
		INFO("DISEQC_1_1");
		break;
	case DISEQC_1_2:
		INFO("DISEQC_1_2");
		break;
	default:
		WARN("Invalid DiSEqC type");
		return;
	}

	if (diseqcType != newDiseqcType)
	{
		/* make sure that the switch gets reset, so that he will
		   accept the new mode */
		sendDiseqcReset();
		sendDiseqcPowerOn();
	}

	diseqcType = newDiseqcType;
}

void CFrontend::setLnbOffset(const bool high, const uint8_t index, int offset)
{
	if (index < MAX_LNBS) {
		if (high)
			lnbOffsetsHigh[index] = offset;
		else
			lnbOffsetsLow[index] = offset;
	}
}

void CFrontend::sendMotorCommand(uint8_t cmdtype, uint8_t address, uint8_t command, uint8_t num_parameters, uint8_t parameter1, uint8_t parameter2)
{
	struct dvb_diseqc_master_cmd cmd;

	printf("[frontend] sendMotorCommand: cmdtype   = %x, address = %x, cmd   = %x\n", cmdtype, address, command);
	printf("[frontend] sendMotorCommand: num_parms = %d, parm1   = %x, parm2 = %x\n", num_parameters, parameter1, parameter2);

	cmd.msg[0] = cmdtype; //command type
	cmd.msg[1] = address; //address
	cmd.msg[2] = command; //command
	cmd.msg[3] = parameter1;
	cmd.msg[4] = parameter2;
	cmd.msg_len = 3 + num_parameters;

	sendDiseqcCommand(&cmd, 15);
	printf("[frontend] motor positioning command sent.\n");
}

void CFrontend::positionMotor(uint8_t motorPosition)
{
	struct dvb_diseqc_master_cmd cmd;

	if (motorPosition != 0)
	{
		cmd.msg[0] = 0xE0; //command type
		cmd.msg[1] = 0x31; //address
		cmd.msg[2] = 0x6B; //command: goto stored motor position
		cmd.msg[3] = motorPosition;
		cmd.msg_len = 4;
		sendDiseqcCommand(&cmd, 15);
		printf("[frontend] motor positioning command sent.\n");
	}
}

int CFrontend::setParameters(dvb_frontend_parameters *feparams, const uint8_t polarization, const uint8_t diseqc )
{
	TP_params TP;
	memcpy(&TP.feparams, feparams, sizeof(dvb_frontend_parameters));
	TP.polarization = polarization;
	TP.diseqc = diseqc;
	return setParameters(&TP);

}

int CFrontend::setParameters(TP_params *TP)
{
	int ret, freq_offset = 0;
	bool high_band = false;

	/* on TD, this is always true... */
	if (info.type == FE_QPSK)
	{
		if (TP->feparams.frequency < 11700000)
		{
			high_band = false;
			freq_offset = lnbOffsetsLow[TP->diseqc];
		}
		else
		{
			high_band = true;
			freq_offset = lnbOffsetsHigh[TP->diseqc];
		}

		TP->feparams.frequency -= freq_offset;
		TP->feparams.highband = (unsigned short)high_band;
		setSec(TP->diseqc, TP->polarization, high_band, TP->feparams.frequency);
	}

	dvb_frontend_event event;
//	int tryagain = 0; //flame on derget for this

	tuned = false;
	TP->feparams.polarity = TP->polarization;
	setFrontend (&TP->feparams);
	event = getEvent();	/* check if tuned */

	if (tuned)
	{
//		if (tryagain)
//			WARN("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!TUNED ON 2nd TRY!!!!!!!!!!!!!!!!!!!!!");
		ret = 0;
#if 0
		ret = diff(event.parameters.frequency, TP->tuner.frequency);
		/*
		 * if everything went ok, then it is a good idea to copy the real
		 * frontend parameters, so we can update the service list, if it differs.
		 *
		 * TODO: set a flag to indicate a change in the service list
		 */
#endif
		WARN("tuned. freq diff: %d", currentTransponder.feparams.frequency - TP->feparams.frequency);
		memcpy(&currentTransponder.feparams, &TP->feparams, sizeof(tunersetup));
	}
	else
	{
		WARN("TUNING FAILED freq: %u!", TP->feparams.frequency + freq_offset);
		ret = -1;
	}

	/*
	 * add the frequency offset to the frontend parameters again
	 * because they are used for the channel list and were given
	 * to this method as a pointer
	 */

	TP->feparams.frequency += freq_offset;

	return ret;
}

void CFrontend::setSec(const uint8_t sat_no, const uint8_t pol, const bool high_band, const uint32_t frequency)
{
	uint8_t repeats = diseqcRepeats;

	fe_sec_voltage_t v = (pol & 1) ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
	fe_sec_tone_mode_t t = high_band ? SEC_TONE_ON : SEC_TONE_OFF;
	fe_sec_mini_cmd_t b = (sat_no & 1) ? SEC_MINI_B : SEC_MINI_A;

	/*
	 * [0] from master, no reply, 1st transmission
	 * [1] any lnb switcher or smatv
	 * [2] write to port group 0 (committed switches)
	 * [3] high nibble: reset bits, low nibble set bits
	 *     bits are: option, position, polarizaion, band
	 */

	struct dvb_diseqc_master_cmd cmd = {
		{ 0xe0, 0x10, 0x38, 0x00, 0x00, 0x00 }, 4
	};

	cmd.msg[3] = 0xf0 | (((sat_no * 4) & 0x0f) | (high_band ? 1 : 0) | ((pol & 1) ? 0 : 2));

	/*
	 * set all SEC / DiSEqC parameters
	 */

//	secSetTone(SEC_TONE_OFF, 15);
	secSetTone(t, 15);
	secSetVoltage(v, 15);

	if ((diseqcType == DISEQC_1_1) && (uncommitted_switch_mode > 0))
	{
		static uint8_t prevSatNo = 255; // initialised with greater than max Satellites (64)
		// because we only want to send uncommitted switch
		// command if necessary to save zap time
		DBG("new Sat %d previous Sat %d", sat_no, prevSatNo);
		//DBG("new Sat/4 %d previous Sat/4 %d", sat_no/4, prevSatNo/4);

		if ((prevSatNo/4 != sat_no/4) && (1 == uncommitted_switch_mode))
		{
			sendUncommittedSwitchesCommand(0xF0 + sat_no/4);
		}
		else if ((prevSatNo != sat_no) && (2 == uncommitted_switch_mode))
		{
			sendUncommittedSwitchesCommand(0xF0 + sat_no);
		}
		prevSatNo = sat_no;
	}

	if (diseqcType >= SMATV_REMOTE_TUNING) {
		sendDiseqcCommand(&cmd, 15);
	}

	if ((diseqcType >= DISEQC_1_1) && (repeats)) {
		for (uint16_t i = 0; i < repeats; i++) {

			usleep(1000 * 100);	/* wait at least 100ms before retransmission */

			if (0 == uncommitted_switch_mode) {
				cmd.msg[2] |= 0x01;	/* uncommitted switches */
				sendDiseqcCommand(&cmd, 15);
			}

			cmd.msg[0] |= 0x01;	/* repeated transmission */
			cmd.msg[2] &= 0xFE;	/* committed switches */
			sendDiseqcCommand(&cmd, 15);
		}
	}

	if (diseqcType == SMATV_REMOTE_TUNING)
		sendDiseqcSmatvRemoteTuningCommand(frequency);

	if (diseqcType == MINI_DISEQC || diseqcType == DISEQC_1_0)
		sendToneBurst(b, 15);

	currentTransponder.diseqc = sat_no;
}

void CFrontend::sendDiseqcPowerOn(void)
{
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x03);
}

void CFrontend::sendDiseqcReset(void)
{
	/* Reset && Clear Reset */
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x00);
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x01);
}

void CFrontend::sendDiseqcStandby(void)
{
	sendDiseqcZeroByteCommand(0xe0, 0x10, 0x02);
}

void CFrontend::sendDiseqcZeroByteCommand(uint8_t framing_byte, uint8_t address, uint8_t command)
{
	struct dvb_diseqc_master_cmd diseqc_cmd = {
		{ framing_byte, address, command, 0x00, 0x00, 0x00 }, 3
	};

	sendDiseqcCommand(&diseqc_cmd, 15);
}

void CFrontend::sendDiseqcSmatvRemoteTuningCommand(const uint32_t frequency)
{
	/*
	 * [0] from master, no reply, 1st transmission
	 * [1] intelligent slave interface for multi-master bus
	 * [2] write channel frequency
	 * [3] frequency
	 * [4] frequency
	 * [5] frequency
	 */

	struct dvb_diseqc_master_cmd cmd = {
		{ 0xe0, 0x71, 0x58, 0x00, 0x00, 0x00 }, 6
	};

	cmd.msg[3] = (((frequency / 10000000) << 4) & 0xF0) | ((frequency / 1000000) & 0x0F);
	cmd.msg[4] = (((frequency / 100000) << 4) & 0xF0) | ((frequency / 10000) & 0x0F);
	cmd.msg[5] = (((frequency / 1000) << 4) & 0xF0) | ((frequency / 100) & 0x0F);

	sendDiseqcCommand(&cmd, 15);
}

void CFrontend::sendUncommittedSwitchesCommand(uint8_t usCommand)
{
	struct dvb_diseqc_master_cmd cmd = {{ 0xe0, 0x10, 0x39, 0x00, 0x00, 0x00 }, 4};

	//command from master, no reply required, first transmission
	//all families (don't care adress)
	//write to port group 1 (uncommitted switches)
	cmd.msg[3] = usCommand;
	sendDiseqcCommand(&cmd, 15);

	if (diseqcRepeats)
	{
		cmd.msg[0] = 0xe1; /* repeated transmission */
		for (uint16_t i = 0; i < diseqcRepeats; i++)
		{
			usleep(1000 * 100); /* wait at least 100ms before retransmission */
			sendDiseqcCommand(&cmd, 15);
		}
	}
	//DBG"[frontend] uncommitted switches command (0x%x) sent", cmd.msg[3]);
}
