/*
 * $Id: frontend.cpp,v 1.1 2002/04/14 06:06:31 obi Exp $
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

#include "frontend.h"

#define FRONTEND_DEVICE	"/dev/ost/frontend0"
#define SEC_DEVICE	"/dev/ost/sec0"

extern std::map <uint32_t, transponder> transponders;

/* constructor */
CFrontend::CFrontend ()
{
	failed = false;
	tuned = false;
	diseqcRepeats = 0;
	diseqcType = NO_DISEQC;

	info = new FrontendInfo();

	if ((frontend_fd = open(FRONTEND_DEVICE, O_RDWR)) < 0)
	{
		perror(FRONTEND_DEVICE);
		initialized = false;
	}
	else if (ioctl(frontend_fd, FE_GET_INFO, info) < 0)
	{
		perror("FE_GET_INFO");
		initialized = false;
	}
	else if ((sec_fd = open(SEC_DEVICE, O_RDWR)) < 0)
	{
		perror(SEC_DEVICE);
		initialized = false;
	}
	else
	{
		initialized = true;
	}
}

/* destructor */
CFrontend::~CFrontend ()
{
	delete info;
	close(sec_fd);
	close(frontend_fd);
}

/*
 * ost frontend api
 */
void CFrontend::selfTest ()
{
	if (ioctl(frontend_fd, FE_SELFTEST) < 0)
	{
		perror("FE_SELFTEST");
		failed = true;
	}
	else
	{
		failed = false;
	}
}

void CFrontend::setPowerState (FrontendPowerState state)
{
	if (ioctl(frontend_fd, FE_SET_POWER_STATE, state) < 0)
	{
		perror("FE_SET_POWER_STATE");
		failed = true;
	}
	else
	{
		failed = false;
	}
}

const FrontendPowerState CFrontend::getPowerState ()
{
	FrontendPowerState state;

	if (ioctl(frontend_fd, FE_GET_POWER_STATE, &state) < 0)
	{
		perror("FE_GET_POWER_STATE");
		failed = true;
	}

	return state;
}

const FrontendStatus CFrontend::getStatus ()
{
	FrontendStatus status;

	if (ioctl(frontend_fd, FE_READ_STATUS, &status) < 0)
	{
		perror("FE_READ_STATUS");
		failed = true;
	}
	else
	{
		failed = false;
	}

	return status;
}

const uint32_t CFrontend::getBitErrorRate ()
{
	uint32_t ber;

	if (ioctl(frontend_fd, FE_READ_BER, &ber) < 0)
	{
		perror("FE_READ_BER");
		failed = true;
	}
	else
	{
		failed = false;
	}

	return ber;
}

const int32_t CFrontend::getSignalStrength ()
{
	int32_t strength;

	if (ioctl(frontend_fd, FE_READ_SIGNAL_STRENGTH, &strength) < 0)
	{
		perror("FE_READ_SIGNAL_STRENGTH");
		failed = true;
	}
	else
	{
		failed = false;
	}

	return strength;
}

const int32_t CFrontend::getSignalNoiseRatio ()
{
	int32_t snr;
	
	if (ioctl(frontend_fd, FE_READ_SNR, &snr) < 0)
	{
		perror("FE_READ_SNR");
		failed = true;
	}
	else
	{
		failed = false;
	}

	return snr;
}

const uint32_t CFrontend::getUncorrectedBlocks ()
{
	uint32_t blocks;

	if (ioctl(frontend_fd, FE_READ_UNCORRECTED_BLOCKS, &blocks) < 0)
	{
		perror("FE_READ_UNCORRECTED_BLOCKS");
		failed = true;
	}
	else
	{
		failed = false;
	}

	return blocks;
}

const uint32_t CFrontend::getNextFrequency (uint32_t frequency)
{
	if (ioctl(frontend_fd, FE_GET_NEXT_FREQUENCY, &frequency) < 0)
	{
		perror("FE_GET_NEXT_FREQUENCY");
		failed = true;
	}
	else
	{
		failed = false;
	}

	return frequency;
}

const uint32_t CFrontend::getNextSymbolRate (uint32_t rate)
{
	if (ioctl(frontend_fd, FE_GET_NEXT_SYMBOL_RATE, rate) < 0)
	{
		perror("FE_GET_NEXT_SYMBOL_RATE");
		failed = true;
	}
	else
	{
		failed = false;
	}

	return rate;
}

void CFrontend::setFrontend (FrontendParameters *feparams)
{
	tuned = false;

	if (ioctl(frontend_fd, FE_SET_FRONTEND, feparams) < 0)
	{
		perror("FE_SET_FRONTEND");
		failed = true;
	}
	else
	{
		currentFrequency = feparams->Frequency;
		failed = false;
	}
}

const FrontendParameters *CFrontend::getFrontend ()
{
	FrontendParameters *feparams = new FrontendParameters();

	if (ioctl(frontend_fd, FE_GET_FRONTEND, feparams) < 0)
	{
		perror("FE_GET_FRONTEND");
		failed = true;
	}
	else
	{
		failed = false;
	}

	return feparams;
}

#if 0
const FrontendInfo *CFrontend::getInfo ()
{
	FrontendInfo *info = new FrontendInfo();

	if (ioctl(frontend_fd, FE_GET_INFO, info) < 0)
	{
		perror("FE_GET_INFO");
		failed = true;
	}
	else
	{
		failed = false;
	}

	return info;
}
#endif

const FrontendEvent *CFrontend::getEvent ()
{
	FrontendEvent *event = new FrontendEvent();

	if (ioctl(frontend_fd, FE_GET_EVENT, event) < 0)
	{
		perror("FE_GET_EVENT");
		failed = true;
	}
	else
	{
		failed = false;
	}

	if (event->type == FE_COMPLETION_EV)
	{
		tuned = true;
	}

	return event;
}

/*
 * ost sec api
 */
void CFrontend::secSetTone (secToneMode toneMode)
{
	if (ioctl(sec_fd, SEC_SET_TONE, toneMode) < 0)
	{
		perror("SEC_SET_TONE");
		failed = true;
	}
	else
	{
		currentToneMode = toneMode;
		failed = false;
	}
}

void CFrontend::secSetVoltage (secVoltage voltage)
{
	if (ioctl(sec_fd, SEC_SET_VOLTAGE, voltage) < 0)
	{
		perror("SEC_SET_VOLTAGE");
		failed = true;
	}
	else
	{
		currentVoltage = voltage;
		failed = false;
	}
}

const secStatus *CFrontend::secGetStatus ()
{
	secStatus *status = new secStatus();

	if (ioctl(sec_fd, SEC_GET_STATUS, status) < 0)
	{
		perror("SEC_GET_STATUS");
		failed = true;
	}
	else
	{
		failed = false;
	}

	return status;
}

void CFrontend::secSendSequence (secCmdSequence *sequence)
{
	if (ioctl(sec_fd, SEC_SEND_SEQUENCE, sequence) < 0)
	{
		perror("SEC_SEND_SEQUENCE");
		failed = true;
	}
	else
	{
		currentToneMode = sequence->continuousTone;
		currentVoltage = sequence->voltage;
		failed = false;
	}
}

#if 0
void CFrontend::secResetOverload ()
{
	if (ioctl(sec_fd, SEC_RESET_OVERLOAD) < 0)
	{
		perror("SEC_RESET_OVERLOAD");
		failed = true;
	}
	else
	{
		failed = false;
	}
}
#endif

/*
 * zapit frontend api
 */
const bool CFrontend::tuneChannel (CZapitChannel *channel)
{
	if (transponders.count(channel->getTsidOnid()) == 0)
	{
		/* if not found, lookup in nit */
		parse_nit(channel->getDiSEqC());

		if (transponders.count(channel->getTsidOnid()) == 0)
		{
			return false;
		}
	}

	currentTsidOnid = channel->getTsidOnid();

	std::map <uint32_t, transponder>::iterator transponder = transponders.find(currentTsidOnid);

	return tuneFrequency
	(
		transponder->second.feparams,
		transponder->second.polarization,
		transponder->second.DiSEqC
	);
}

const bool CFrontend::tuneFrequency (FrontendParameters feparams, uint8_t polarization, uint8_t diseqc)
{
	bool secChanged = false;

	/* sec */
	if (info->type == FE_QPSK)
	{
		secToneMode toneMode;
		secVoltage voltage;

		/* tone */
		if (feparams.Frequency < 11700000)
		{
			/* low band */
			feparams.Frequency -= 9750000;
			toneMode = SEC_TONE_OFF;
		}
		else
		{
			/* high band */
			feparams.Frequency -= 10600000;
			toneMode = SEC_TONE_ON;
		}

		/* voltage */
		if (polarization == 1)
		{
			/* vertical */
			voltage = SEC_VOLTAGE_13;
		}
		else
		{
			/* horizontal */
			voltage = SEC_VOLTAGE_18;
		}

		/* do diseqc stuff */
		switch (diseqcType)
		{
		case NO_DISEQC:
			if (currentToneMode != toneMode)
			{
				secSetTone(toneMode);
				secChanged = true;
			}
			if (currentVoltage != voltage)
			{
				secSetVoltage(voltage);
				secChanged = true;
			}
			break;

		case MINI_DISEQC:
			if ((currentVoltage != voltage) || (currentToneMode != toneMode) || (currentDiseqc != diseqc))
			{
				sendMiniDiseqcCommand(toneMode, voltage, diseqc);
				secChanged = true;
			}
			break;

		case DISEQC_1_0:
			if ((currentVoltage != voltage) || (currentToneMode != toneMode) || (currentDiseqc != diseqc))
			{
				sendDiseqcCommand(toneMode, voltage, diseqc, 0);
				secChanged = true;
			}
			break;

		case DISEQC_1_1:
			if ((currentVoltage != voltage) || (currentToneMode != toneMode) || (currentDiseqc != diseqc))
			{
				sendDiseqcCommand(toneMode, voltage, diseqc, diseqcRepeats);
				secChanged = true;
			}
			break;

		case SMATV_REMOTE_TUNING:
			if ((currentFrequency != feparams.Frequency) || (currentVoltage != voltage) || (currentToneMode != toneMode) || (currentDiseqc != diseqc))
			{
				sendSmatvRemoteTuningCommand(toneMode, voltage, diseqc, feparams.Frequency);
				secChanged = true;
			}
			break;
		}
	}

	if ((currentFrequency != feparams.Frequency) || (secChanged == true))
	{
		/* tune */
		setFrontend(&feparams);

		/* wait for completion */
		getEvent();
	}

	return tuned;
}

/*
 * zapit sec api
 */
const bool CFrontend::sendMiniDiseqcCommand (secToneMode toneMode, secVoltage voltage, uint8_t diseqc)
{
	secCmdSequence *sequence = new secCmdSequence();

	switch (diseqc)
	{
	case 0:
		sequence->miniCommand = SEC_MINI_A;
		break;
	case 1:
		sequence->miniCommand = SEC_MINI_B;
		break;
	default:
		failed = true;
		return false;
	}

	sequence->numCommands = 0;
	sequence->continuousTone = toneMode;
	sequence->voltage = voltage;
	sequence->commands = NULL;

	secSendSequence(sequence);

	delete sequence;

	if (failed == false)
	{
		currentDiseqc = diseqc;
		return true;
	}
	else
	{
		return false;
	}
}

const bool CFrontend::sendDiseqcCommand (secToneMode toneMode, secVoltage voltage, uint8_t diseqc, uint32_t repeats)
{
	/*
	 * note: current api and drivers always set the framing byte 
	 * to 0xE0. repeated diseqc commands should set this to 0xE1.
	 */
	secCmdSequence *sequence = new secCmdSequence();
	sequence->miniCommand = SEC_MINI_NONE;
	sequence->continuousTone = toneMode;
	sequence->voltage = voltage;

	sequence->commands[0].type = SEC_CMDTYPE_DISEQC;
	//sequence->commands[0].u.diseqc.frame = 0xE0;	/* from master, no reply, 1st transmission */
	sequence->commands[0].u.diseqc.addr = 0x10;	/* any lnb switcher or smatv */
	sequence->commands[0].u.diseqc.cmd = 0x38;	/* write to port group 0 (committed switches) */
	sequence->commands[0].u.diseqc.numParams = 1;
	sequence->commands[0].u.diseqc.params[0] = 0xF0 | ((diseqc << 2) & 0x0F) | (toneMode == SEC_TONE_ON ? 1 : 0) | (voltage == SEC_VOLTAGE_18 ? 2 : 0);

	for (sequence->numCommands = 1; sequence->numCommands < (repeats << 1) + 1; sequence->numCommands += 2)
	{
		sequence->commands[sequence->numCommands].type = SEC_CMDTYPE_DISEQC;
		//sequence->commands[sequence->numCommands].u.diseqc.frame = (sequence->numCommands - 1 ? 0xE1 : 0xE0);
		sequence->commands[sequence->numCommands].u.diseqc.addr = 0x10;		/* any lnb switcher or smatv */
		sequence->commands[sequence->numCommands].u.diseqc.cmd = 0x39;		/* write to port group 1 (uncommitted switches) */
		sequence->commands[sequence->numCommands].u.diseqc.numParams = 1;
		sequence->commands[sequence->numCommands].u.diseqc.params[0] = 0xF0 | ((diseqc << 2) & 0x0F) | (toneMode == SEC_TONE_ON ? 1 : 0) | (voltage == SEC_VOLTAGE_18 ? 2 : 0);
		sequence->commands[sequence->numCommands + 1].type = SEC_CMDTYPE_DISEQC;
		//sequence->commands[sequence->numCommands + 1].u.diseqc.frame = 0xE1;	/* from master, no reply, repeated transmission */
		sequence->commands[sequence->numCommands + 1].u.diseqc.addr = 0x10;	/* any lnb switcher or smatv */
		sequence->commands[sequence->numCommands + 1].u.diseqc.cmd = 0x38;	/* write to port group 0 (committed switches) */
		sequence->commands[sequence->numCommands + 1].u.diseqc.numParams = 1;
		sequence->commands[sequence->numCommands + 1].u.diseqc.params[0] = 0xF0 | ((diseqc << 2) & 0x0F) | (toneMode == SEC_TONE_ON ? 1 : 0) | (voltage == SEC_VOLTAGE_18 ? 2 : 0);
	}

	secSendSequence(sequence);

	delete sequence;

	if (failed == false)
	{
		currentDiseqc = diseqc;
		return true;
	}
	else
	{
		return false;
	}
}

const bool CFrontend::sendSmatvRemoteTuningCommand (secToneMode toneMode, secVoltage voltage, uint8_t diseqc, uint32_t frequency)
{
	secCmdSequence *sequence = new secCmdSequence();
	sequence->miniCommand = SEC_MINI_NONE;
	sequence->continuousTone = toneMode;
	sequence->voltage = voltage;
	sequence->numCommands = 2;

	sequence->commands[0].type = SEC_CMDTYPE_DISEQC;
	sequence->commands[0].u.diseqc.addr = 0x10;	/* any interface */
	sequence->commands[0].u.diseqc.cmd = 0x38;	/* write n0 */
	sequence->commands[0].u.diseqc.numParams = 1;
	sequence->commands[0].u.diseqc.params[0] = 0xF0 | ((diseqc << 2) & 0x0F) | (toneMode == SEC_TONE_ON ? 1 : 0) | (voltage == SEC_VOLTAGE_18 ? 2 : 0);
	sequence->commands[1].type = SEC_CMDTYPE_DISEQC;
	sequence->commands[1].u.diseqc.addr = 0x71;	/* intelligent slave interface for multi-master bus */
	sequence->commands[1].u.diseqc.cmd = 0x58;	/* write channel frequency */
	sequence->commands[1].u.diseqc.numParams = 3;
	sequence->commands[1].u.diseqc.params[0] = (((frequency / 10000000) << 4) & 0xF0) | ((frequency / 1000000) & 0x0F);
	sequence->commands[1].u.diseqc.params[1] = (((frequency / 100000) << 4) & 0xF0) | ((frequency / 10000) & 0x0F);
	sequence->commands[1].u.diseqc.params[2] = (((frequency / 1000) << 4) & 0xF0) | ((frequency / 100) & 0x0F);

	secSendSequence(sequence);

	delete sequence;

	if (failed == false)
	{
		currentDiseqc = diseqc;
		return true;
	}
	else
	{
		return false;
	}
}

