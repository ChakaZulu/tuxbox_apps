/*
 * $Id: frontend.h,v 1.17 2002/11/02 17:21:15 obi Exp $
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

/* system */
#include <stdint.h>

/* nokia api */
#include <linux/dvb/frontend.h>

/* zapit */
#include "channel.h"

#define MAX_LNBS	4

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

		void setSec (uint8_t sat_no, uint8_t pol, bool high_band, uint32_t frequency);
		
		const bool sendDiseqcZeroByteCommand (uint8_t frm, uint8_t addr, uint8_t cmd);
		const bool sendDiseqcSmatvRemoteTuningCommand (uint32_t frequency);
		const uint8_t receiveDiseqcReply (int timeout_ms);

	public:
		CFrontend ();
		~CFrontend ();

		/*
		 * linux dvb frontend api
		 */
		static fe_code_rate_t CFrontend::getCodeRate (uint8_t fec_inner);
		static fe_modulation_t CFrontend::getModulation (uint8_t modulation);

		const dvb_frontend_info * getInfo ()	{ return info; };

		const bool secResetOverload ();
		const bool secSetTone (fe_sec_tone_mode_t mode);
		const bool secSetVoltage (fe_sec_voltage_t voltage);

		const bool sendDiseqcCommand (struct dvb_diseqc_master_cmd * cmd);
		const bool sendToneBurst (fe_sec_mini_cmd_t burst);

		const fe_status_t getStatus ();
		const uint32_t getBitErrorRate ();
		const uint16_t getSignalStrength ();
		const uint16_t getSignalNoiseRatio ();
		const uint32_t getUncorrectedBlocks ();

		void setFrontend (dvb_frontend_parameters *feparams);
		const dvb_frontend_parameters * getFrontend ();
		void discardEvents ();
		const bool getEvent ();

		unsigned int getFrequency ();
		unsigned char getPolarization ();

		
		
		/*
		 * zapit tuner api
		 */
		const bool tuneChannel (CZapitChannel *channel);
		const bool tuneFrequency (dvb_frontend_parameters *feparams, uint8_t polarization, uint8_t diseqc);

		/*
		 * zapit diseqc api
		 */
#if 0
		const bool sendDiseqcCommand (fe_sec_tone_mode_t mode, fe_sec_voltage_t voltage, uint8_t diseqc, uint32_t repeats);
#endif
		const bool sendDiseqcPowerOn ();
		const bool sendDiseqcReset ();
		const bool sendDiseqcStandby ();

		void setDiseqcRepeats (uint8_t repeats)		{ diseqcRepeats = repeats; }
		const uint8_t getDiseqcRepeats ()		{ return diseqcRepeats; }

		void setDiseqcType (diseqc_t type)		{ diseqcType = type; }
		const diseqc_t getDiseqcType ()			{ return diseqcType; }
		
		const bool isInitialized ()			{ return initialized; }
		const uint32_t getTsidOnid ()			{ return currentTsidOnid; }

		void setLnbOffset (bool high, uint8_t index, int32_t offset) {

			if (index < MAX_LNBS) {

				if (high)
					lnbOffsetsHigh[index] = offset;
				else
					lnbOffsetsLow[index] = offset;
			}
		}
};

#endif /* __frontend_h__ */
