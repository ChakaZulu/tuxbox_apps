/*
 * $Id: frontend.h,v 1.23 2003/01/19 21:46:37 obi Exp $
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

#ifndef __zapit_frontend_h__
#define __zapit_frontend_h__

/* system */
#include <stdint.h>

/* linuxtv api */
#include <linux/dvb/frontend.h>

/* zapit */
#include <zapit/types.h>

#define MAX_LNBS	64	/* due to Diseqc 1.1  (2003-01-10 rasc) */

class CFrontend
{
	private:
		/* frontend file descriptor */
		int fd;
		/* last action failed flag */
		bool failed;
		/* tuning finished flag */
		bool tuned;
		/* all devices opened without error flag */
		bool initialized;
		/* information about the used frontend type */
		dvb_frontend_info *info;
		/* current tuned transport stream id / original network id */
		uint32_t currentTsidOnid;
		/* current tuned frequency */
		uint32_t currentFrequency;
		/* current 22kHz tone mode */
		fe_sec_tone_mode_t currentToneMode;
		/* current H/V voltage */
		fe_sec_voltage_t currentVoltage;
		/* current diseqc position */
		uint8_t currentDiseqc;
		/* how often to repeat DiSEqC 1.1 commands */
		uint8_t diseqcRepeats;
		/* DiSEqC type of attached hardware */
		diseqc_t diseqcType;
		/* low lnb offsets */
		int32_t lnbOffsetsLow[MAX_LNBS];
		/* high lnb offsets */
		int32_t lnbOffsetsHigh[MAX_LNBS];

		void setSec(uint8_t sat_no, uint8_t pol, bool high_band, uint32_t frequency);
		
		bool sendDiseqcZeroByteCommand(uint8_t frm, uint8_t addr, uint8_t cmd);
		bool sendDiseqcSmatvRemoteTuningCommand(uint32_t frequency);
		uint8_t receiveDiseqcReply(int timeout_ms);

	public:
		CFrontend(void);
		~CFrontend(void);

		/*
		 * linux dvb frontend api
		 */
		static fe_code_rate_t getCodeRate(uint8_t fec_inner);
		static fe_modulation_t getModulation(uint8_t modulation);

		struct dvb_frontend_info *getInfo(void)	{ return info; };

		bool secResetOverload(void);
		bool secSetTone(fe_sec_tone_mode_t mode);
		bool secSetVoltage(fe_sec_voltage_t voltage);

		bool sendDiseqcCommand(struct dvb_diseqc_master_cmd *cmd);
		bool sendToneBurst(fe_sec_mini_cmd_t burst);

		fe_status_t getStatus(void);
		uint32_t getBitErrorRate(void);
		uint16_t getSignalStrength(void);
		uint16_t getSignalNoiseRatio(void);
		uint32_t getUncorrectedBlocks(void);

		void setFrontend(struct dvb_frontend_parameters *feparams);
		struct dvb_frontend_parameters getFrontend(void);
		void discardEvents(void);
		struct dvb_frontend_event getEvent(void);
		struct dvb_frontend_event blockingTune(struct dvb_frontend_parameters *feparams);

		unsigned int getFrequency(void);
		unsigned char getPolarization(void);

		
		
		/*
		 * zapit tuner api
		 */
		bool tuneTsidOnid(uint32_t tsid_onid);
		bool tuneFrequency(struct dvb_frontend_parameters *feparams, uint8_t polarization, uint8_t diseqc);

		/*
		 * zapit diseqc api
		 */
#if 0
		bool sendDiseqcCommand(fe_sec_tone_mode_t mode, fe_sec_voltage_t voltage, uint8_t diseqc, uint32_t repeats);
#endif
		bool sendDiseqcPowerOn(void);
		bool sendDiseqcReset(void);
		bool sendDiseqcStandby(void);

		void setDiseqcRepeats(uint8_t repeats)		{ diseqcRepeats = repeats; }
		uint8_t getDiseqcRepeats(void)			{ return diseqcRepeats; }

		void setDiseqcType(diseqc_t type);
		diseqc_t getDiseqcType(void)			{ return diseqcType; }
		
		bool isInitialized(void)			{ return initialized; }
		uint32_t getTsidOnid(void)			{ return currentTsidOnid; }

		void setLnbOffset(bool high, uint8_t index, int32_t offset) {

			if (index < MAX_LNBS) {

				if (high)
					lnbOffsetsHigh[index] = offset;
				else
					lnbOffsetsLow[index] = offset;
			}
		}
};

#endif /* __zapit_frontend_h__ */
