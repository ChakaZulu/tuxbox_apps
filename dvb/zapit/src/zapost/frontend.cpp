/*
 * $Id: frontend.cpp,v 1.34 2002/11/18 00:27:57 obi Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* system c++ */
#include <iostream>

/* zapit */
#include <zapit/debug.h>
#include <zapit/frontend.h>
#include <zapit/getservices.h>
#include <zapit/nit.h>
#include <zapit/sdt.h>
#include <zapit/settings.h>


#define DVB_IOCTL(cmd, arg)				\
	do { 						\
		if (ioctl(fd, cmd, arg) < 0) {		\
			ERROR("ioctl");			\
			failed = true;			\
		}					\
		else {					\
			failed = false;			\
		}					\
							\
	} while(0)


extern std::map <uint32_t, transponder> transponders;


/*
 * constructor
 */

CFrontend::CFrontend ()
{
	failed = false;
	tuned = false;
	currentToneMode = SEC_TONE_OFF;
	currentVoltage = SEC_VOLTAGE_13;
	currentFrequency = 0;
	currentTsidOnid = 0;
	diseqcRepeats = 0;
	diseqcType = NO_DISEQC;

	info = new dvb_frontend_info();

	if ((fd = open(FRONTEND_DEVICE, O_RDWR|O_NONBLOCK)) < 0) {
		ERROR(FRONTEND_DEVICE);
		initialized = false;
	}
	else if (ioctl(fd, FE_GET_INFO, info) < 0) {
		ERROR("FE_GET_INFO");
		initialized = false;
	}
	else {
		initialized = true;
		sendDiseqcReset();
		sendDiseqcPowerOn();
	}
}


/*
 * destructor
 */

CFrontend::~CFrontend ()
{
	sendDiseqcStandby();
	
	delete info;
	close(fd);
}



/*
 * dvb frontend api
 */

fe_code_rate_t CFrontend::getCodeRate (uint8_t fec_inner)
{
	switch (fec_inner & 0x0F) {
	case 0x01:
		return FEC_1_2;
	case 0x02:
		return FEC_2_3;
	case 0x03:
		return FEC_3_4;
	case 0x04:
		return FEC_5_6;
	case 0x05:
		return FEC_7_8;
	case 0x0F:
		return FEC_NONE;
	default:
		return FEC_AUTO;
	}
}


fe_modulation_t CFrontend::getModulation (uint8_t modulation)
{
	switch (modulation) {
	case 0x01:
		return QAM_16;
	case 0x02:
		return QAM_32;
	case 0x03:
		return QAM_64;
	case 0x04:
		return QAM_128;
	case 0x05:
		return QAM_256;
	default:
		return QAM_AUTO;
	}
}


unsigned int CFrontend::getFrequency ()
{
	switch (info->type) {
	case FE_QPSK:
		if (currentToneMode == SEC_TONE_OFF)
			return currentFrequency + lnbOffsetsLow[currentDiseqc];
		else
			return currentFrequency + lnbOffsetsHigh[currentDiseqc];

	case FE_QAM:
	default:
		return currentFrequency;
	}
}


unsigned char CFrontend::getPolarization ()
{
	if (currentVoltage == SEC_VOLTAGE_13)
		return 1;
	else
		return 0;
}


const fe_status_t CFrontend::getStatus ()
{
	fe_status_t status;

	DVB_IOCTL(FE_READ_STATUS, &status);

	return status;
}


const uint32_t CFrontend::getBitErrorRate ()
{
	uint32_t ber;

	DVB_IOCTL(FE_READ_BER, &ber);

	return ber;
}


const uint16_t CFrontend::getSignalStrength ()
{
	uint16_t strength;

	DVB_IOCTL(FE_READ_SIGNAL_STRENGTH, &strength);

	return strength;
}


const uint16_t CFrontend::getSignalNoiseRatio ()
{
	uint16_t snr;

	DVB_IOCTL(FE_READ_SNR, &snr);

	return snr;
}


const uint32_t CFrontend::getUncorrectedBlocks ()
{
	uint32_t blocks;

	DVB_IOCTL(FE_READ_UNCORRECTED_BLOCKS, &blocks);

	return blocks;
}


void CFrontend::discardEvents ()
{
	struct dvb_frontend_event event;

	/* clear old events */
	while (1) {
		if (ioctl(fd, FE_GET_EVENT, &event) == -1)
			break;
	}
}


void CFrontend::setFrontend (dvb_frontend_parameters *feparams)
{
	tuned = false;

	INFO("freq %u", feparams->frequency);

	discardEvents();

	DVB_IOCTL(FE_SET_FRONTEND, feparams);
}


const dvb_frontend_parameters *CFrontend::getFrontend ()
{
	dvb_frontend_parameters *feparams = new dvb_frontend_parameters();

	DVB_IOCTL(FE_GET_FRONTEND, feparams);

	return feparams;
}

#define TIMEOUT_MAX_MS 5000

struct dvb_frontend_event CFrontend::getEvent ()
{
	struct dvb_frontend_event event;
	struct pollfd pfd;
	struct timeval tv, tv2;

	int msec = TIMEOUT_MAX_MS;

	pfd.fd = fd;
	pfd.events = POLLIN;

	gettimeofday(&tv, NULL);

	while (msec > 0) {

		int res = poll(&pfd, 1, msec);

		if (res <= 0) {
			failed = true;
			break;
		}

		gettimeofday(&tv2, NULL);
		msec -= ((tv2.tv_sec - tv.tv_sec) * 1000) + ((tv2.tv_usec - tv.tv_usec) / 1000);

		if (pfd.revents & POLLIN) {

			INFO("event after %d milliseconds", TIMEOUT_MAX_MS - msec);

			DVB_IOCTL(FE_GET_EVENT, &event);

			if (event.status & FE_HAS_LOCK) {
				currentFrequency = event.parameters.frequency;
				INFO("FE_HAS_LOCK: freq %u", currentFrequency);
				tuned = true;
				break;
			}

			else {
				WARN("NO LOCK: %x", event.status);
			}
		}

		else {
			WARN("pfd[0].revents %d", pfd.revents);
		}

	}

	if (!tuned) {
		currentFrequency = 0;
		currentTsidOnid = 0;
	}

	return event;
}




/*
 * linuxtv SEC / DiSEqC api
 */


const bool
CFrontend::secSetTone (fe_sec_tone_mode_t toneMode)
{
	DVB_IOCTL(FE_SET_TONE, toneMode);

	if (!failed)
		currentToneMode = toneMode;

	return !failed;
}


const bool
CFrontend::secSetVoltage (fe_sec_voltage_t voltage)
{
	DVB_IOCTL(FE_SET_VOLTAGE, voltage);
	usleep(15 * 1000);

	if (!failed)
		currentVoltage = voltage;

	return !failed;
}


const bool
CFrontend::secResetOverload ()
{
	DVB_IOCTL(FE_DISEQC_RESET_OVERLOAD, 0);
	return !failed;
}


const bool
CFrontend::sendDiseqcCommand (struct dvb_diseqc_master_cmd * cmd)
{
	DVB_IOCTL(FE_DISEQC_SEND_MASTER_CMD, cmd);
	usleep(15 * 1000);
	return !failed;
}


const uint8_t
CFrontend::receiveDiseqcReply (int timeout_ms)
{
	int ret;
	std::string status;
	struct dvb_diseqc_slave_reply reply;

	reply.timeout = timeout_ms;

	DVB_IOCTL(FE_DISEQC_RECV_SLAVE_REPLY, &reply);

	if (failed) {
		status = "ioctl failed";
		ret = 0;
	}

	else {

		if (reply.msg_len == 0) {
			status = "timeout";
			ret = 1;
		}

		else {

			switch (reply.msg[0]) {
			case 0xe4:
				status = "ok";
				ret = 0;
				break;

			case 0xe5:
				status = "invalid address or unknown command";
				ret = 1;
				break;

			case 0xe6:
				status = "parity error";
				ret = 1;
				break;

			case 0xe7:
				status = "contention flag mismatch";
				ret = 1;
				break;

			default:
				status = "unexpected reply";
				ret = 0;
				break;
			}

		}

	}

	INFO("status: %s", status.c_str());

	return ret;
}


const bool
CFrontend::sendToneBurst (fe_sec_mini_cmd_t burst)
{
	DVB_IOCTL(FE_DISEQC_SEND_BURST, burst);
	usleep(15 * 1000);
	return !failed;
}




/*
 * zapit frontend api
 */

const bool
CFrontend::tuneChannel (CZapitChannel *channel)
{
	std::map <uint32_t, transponder>::iterator transponder;

	if (!channel)
		return false;

	transponder = transponders.find(channel->getTsidOnid());

	if (transponder == transponders.end())
		return false;

	currentTsidOnid = channel->getTsidOnid();

	return tuneFrequency(&(transponder->second.feparams), transponder->second.polarization, transponder->second.DiSEqC);
}


struct dvb_frontend_event
CFrontend::blockingTune (dvb_frontend_parameters *feparams)
{
	setFrontend(feparams);
	return getEvent();
}
	

const bool
CFrontend::tuneFrequency (dvb_frontend_parameters *feparams, uint8_t polarization, uint8_t diseqc)
{
	int freq_offset = 0;

	/* sec */
	if (info->type == FE_QPSK) {

		bool high_band;

		/* tone */
		if (feparams->frequency < 11700000) {
			high_band = false;
			freq_offset = lnbOffsetsLow[diseqc];
		}
		else {
			high_band = true;
			freq_offset = lnbOffsetsHigh[diseqc];
		}

		feparams->frequency -= freq_offset;
		setSec (diseqc, polarization, high_band, feparams->frequency);
	}


	/*
	 * frontends which can not handle auto inversion but
	 * are set to auto inversion will try without first
	 */

	if ((!(info->caps & FE_CAN_INVERSION_AUTO)) && (feparams->inversion == INVERSION_AUTO))
		feparams->inversion = INVERSION_OFF;


	bool retry = false;
	uint32_t real_tsid_onid = 0;
	struct dvb_frontend_event event;
	bool wrong_ts;

	do {
		wrong_ts = false;

		event = blockingTune(feparams);

		/*
		 * maybe the frontend got lock at a different frequency
		 * than requested, so we need to look up the tsid/onid.
		 *
		 * FIXME: be somewhat tolerant about this since fe_bend_frequency
		 * prevents exact tuning
		 */

		if (event.parameters.frequency != feparams->frequency) {
			real_tsid_onid = get_sdt_TsidOnid();
			if (currentTsidOnid != real_tsid_onid)
				wrong_ts = true;
		}

		/*
		 * software auto inversion for stupid frontends
		 * (retry tuning only once)
		 */

		if ((!(info->caps & FE_CAN_INVERSION_AUTO)) && (!retry))
			if ((!tuned) || (wrong_ts)) {
				feparams->inversion = (fe_spectral_inversion_t) ((~feparams->inversion) & 0x01);
				retry = true;
				continue;
			}

	} while (0);


	if (tuned) {
		
		if (wrong_ts) {
			
			/*
			 * zapit shall know about the frontend's mistake.
			 * of course it is not its own fault ;)
			 */

			currentTsidOnid = real_tsid_onid;
		}

		else {

			/*
			 * if everything went ok, then it is a good idea to copy the real
			 * frontend parameters, so we can update the service list, if it differs.
			 *
			 * TODO: set a flag to indicate a change in the service list
			 */

			if (memcmp(feparams, &event.parameters, sizeof(struct dvb_frontend_parameters)))
				memcpy(feparams, &event.parameters, sizeof(struct dvb_frontend_parameters));
		}

	}


	/*
	 * add the frequency offset to the frontend parameters again
	 * because they are used for the channel list and were given
	 * to this method as a pointer
	 */

	if (info->type == FE_QPSK)
		feparams->frequency += freq_offset;

	return tuned;
}




/*
 * zapit SEC / DiSEqC api
 */


void
CFrontend::setSec (uint8_t sat_no, uint8_t pol, bool high_band, uint32_t frequency)
{
	uint8_t repeats = diseqcRepeats;

	fe_sec_voltage_t v = pol ? SEC_VOLTAGE_13 : SEC_VOLTAGE_18;
	fe_sec_tone_mode_t t = high_band ? SEC_TONE_ON : SEC_TONE_OFF;
	fe_sec_mini_cmd_t b = ((sat_no / 4) % 2) ? SEC_MINI_B : SEC_MINI_A;

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

	cmd.msg[3] = 0xf0 | (((sat_no * 4) & 0x0f) | (high_band ? 1 : 0) | (pol ? 0 : 2));

	/*
	 * set all SEC / DiSEqC parameters
	 */

	secSetTone(SEC_TONE_OFF);
	secSetVoltage(v);

	if (diseqcType >= SMATV_REMOTE_TUNING) {

		if (diseqcType >= DISEQC_2_0)
			cmd.msg[0] |= 0x02;	/* reply required */

		sendDiseqcCommand(&cmd);

		if (diseqcType >= DISEQC_2_0)
			repeats += receiveDiseqcReply(50);

	}

	if ((diseqcType >= DISEQC_1_1) && (repeats)) {

		for (uint16_t i = 0; i < repeats; i++) {

			uint8_t again = 0;

			usleep(100 * 1000);	/* wait at least 100ms before retransmission */

			cmd.msg[2] |= 0x01;	/* uncommitted switches */
			sendDiseqcCommand(&cmd);

			if (diseqcType >= DISEQC_2_0)
				again += receiveDiseqcReply(50);

			cmd.msg[0] |= 0x01;	/* repeated transmission */
			cmd.msg[2] &= 0xFE;	/* committed switches */
			sendDiseqcCommand(&cmd);

			if (diseqcType >= DISEQC_2_0)
				again += receiveDiseqcReply(50);

			if (again == 2)
				repeats++;

		}

	}

	if (diseqcType == SMATV_REMOTE_TUNING)
		sendDiseqcSmatvRemoteTuningCommand(frequency);

	if (diseqcType == MINI_DISEQC)
		sendToneBurst(b);

	secSetTone(t);

	currentDiseqc = sat_no;
}


const bool
CFrontend::sendDiseqcPowerOn ()
{
	return sendDiseqcZeroByteCommand(0xe0, 0x10, 0x03);
}


const bool
CFrontend::sendDiseqcReset ()
{
	/* Reset && Clear Reset */
	return (sendDiseqcZeroByteCommand(0xe0, 0x10, 0x00) && sendDiseqcZeroByteCommand(0xe0, 0x10, 0x01));
}


const bool
CFrontend::sendDiseqcStandby ()
{
	return sendDiseqcZeroByteCommand(0xe0, 0x10, 0x02);
}


const bool
CFrontend::sendDiseqcZeroByteCommand (uint8_t framing_byte, uint8_t address, uint8_t command)
{
	struct dvb_diseqc_master_cmd diseqc_cmd = {
		{ framing_byte, address, command, 0x00, 0x00, 0x00 }, 3
	};

	return sendDiseqcCommand(&diseqc_cmd);
}


const bool
CFrontend::sendDiseqcSmatvRemoteTuningCommand (uint32_t frequency)
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

	return sendDiseqcCommand(&cmd);
}

