/*
 * $Id: frontend.h,v 1.1 2002/04/14 06:06:31 obi Exp $
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

#ifndef __frontend_h__
#define __frontend_h__

#include <ost/frontend.h>
#include <ost/sec.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include <iostream>

#include "nit.h"

enum diseqc_t
{
	NO_DISEQC,
	MINI_DISEQC,
	DISEQC_1_0,
	DISEQC_1_1,
	SMATV_REMOTE_TUNING
};

class CFrontend
{
	private:
		/* frontend file descriptor */
		int frontend_fd;
		/* sec file descriptor */
		int sec_fd;
		/* last action failed flag */
		bool failed;
		/* tuning finished flag */
		bool tuned;
		/* all devices opened without error flag */
		bool initialized;
		/* information about the used frontend type */
		FrontendInfo *info;
		/* current tuned transport stream id / original network id */
		uint32_t currentTsidOnid;
		/* current tuned frequency */
		uint32_t currentFrequency;
		/* current 22kHz tone mode */
		secToneMode currentToneMode;
		/* current H/V voltage */
		secVoltage currentVoltage;
		/* current diseqc position */
		uint8_t currentDiseqc;
		/* how often to repeat DiSEqC 1.1 commands */
		uint32_t diseqcRepeats;
		/* DiSEqC type of attached hardware */
		diseqc_t diseqcType;

	public:
		CFrontend ();
		~CFrontend ();

		/* ost tuner api */
		void selfTest ();
		void setPowerState (FrontendPowerState state);
		void setFrontend (FrontendParameters *feparams);
		const FrontendPowerState getPowerState ();
		const FrontendStatus getStatus ();
		const uint32_t getBitErrorRate ();
		const int32_t getSignalStrength ();
		const int32_t getSignalNoiseRatio ();
		const uint32_t getUncorrectedBlocks ();
		const uint32_t getNextFrequency (uint32_t frequency);
		const uint32_t getNextSymbolRate (uint32_t rate);
		const FrontendParameters *getFrontend ();
		const FrontendEvent *getEvent ();
		const FrontendInfo *getInfo ()	{ return info; };

		/* ost sec api */
		void secSetTone (secToneMode mode);
		void secSetVoltage (secVoltage voltage);
		void secSendSequence (secCmdSequence *sequence);
		//void secResetOverload ();
		const secStatus *secGetStatus ();

		/* zapit api */
		const bool tuneChannel (CZapitChannel *channel);
		const bool tuneFrequency (FrontendParameters feparams, uint8_t polarization, uint8_t diseqc);
		const bool sendMiniDiseqcCommand (secToneMode mode, secVoltage voltage, uint8_t diseqc);
		const bool sendDiseqcCommand (secToneMode mode, secVoltage voltage, uint8_t diseqc, uint32_t repeats);
		const bool sendSmatvRemoteTuningCommand (secToneMode toneMode, secVoltage voltage, uint8_t diseqc, uint32_t frequency);

		void setDiseqcRepeats(uint32_t repeats)	{ diseqcRepeats = repeats; }
		void setDiseqcType(diseqc_t type)	{ diseqcType = type; }
		const uint32_t getDiseqcRepeats()	{ return diseqcRepeats; }
		const diseqc_t getDiseqcType()		{ return diseqcType; }

		const bool isInitialized()		{ return initialized; }
		const uint32_t getTsidOnid()		{ return currentTsidOnid; }
};

#endif /* __frontend_h__ */
