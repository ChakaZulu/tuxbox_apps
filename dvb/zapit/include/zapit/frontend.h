/*
 * $Id: frontend.h,v 1.25 2003/03/14 07:31:50 obi Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
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

#include <inttypes.h>
#include <linux/dvb/frontend.h>
#include <zapit/types.h>

#define MAX_LNBS	64	/* due to Diseqc 1.1  (2003-01-10 rasc) */

class CFrontend
{
	private:
		/* frontend file descriptor */
		int fd;
		/* tuning finished flag */
		bool tuned;
		/* information about the used frontend type */
		struct dvb_frontend_info info;
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

		uint32_t			getBitErrorRate(void) const;
		uint32_t			getDiseqcReply(const int timeout_ms) const;
		struct dvb_frontend_event	getEvent(void);
		struct dvb_frontend_parameters	getFrontend(void) const;
		uint16_t			getSignalNoiseRatio(void) const;
		uint16_t			getSignalStrength(void) const;
		fe_status_t			getStatus(void) const;
		uint32_t			getUncorrectedBlocks(void) const;

		void				secResetOverload(void);
		void				secSetTone(const fe_sec_tone_mode_t mode, const uint32_t ms);
		void				secSetVoltage(const fe_sec_voltage_t voltage, const uint32_t ms);
		void				sendDiseqcCommand(const struct dvb_diseqc_master_cmd *cmd, const uint32_t ms);
		void				sendDiseqcPowerOn(void);
		void				sendDiseqcReset(void);
		void				sendDiseqcSmatvRemoteTuningCommand(const uint32_t frequency);
		void				sendDiseqcStandby(void);
		void				sendDiseqcZeroByteCommand(const uint8_t frm, const uint8_t addr, const uint8_t cmd);
		void				sendToneBurst(const fe_sec_mini_cmd_t burst, const uint32_t ms);
		void				setFrontend(const struct dvb_frontend_parameters *feparams);
		void				setSec(const uint8_t sat_no, const uint8_t pol, const bool high_band, const uint32_t frequency);

	public:
		CFrontend(void);
		~CFrontend(void);

		static fe_code_rate_t		getCodeRate(const uint8_t fec_inner);
		uint8_t				getDiseqcPosition(void) const		{ return currentDiseqc; }
		uint8_t				getDiseqcRepeats(void) const		{ return diseqcRepeats; }
		diseqc_t			getDiseqcType(void) const		{ return diseqcType; }
		uint32_t			getFrequency(void) const;
		static fe_modulation_t		getModulation(const uint8_t modulation);
		uint8_t				getPolarization(void) const;
		const struct dvb_frontend_info *getInfo(void) const			{ return &info; };

		void				setDiseqcRepeats(const uint8_t repeats)	{ diseqcRepeats = repeats; }
		void				setDiseqcType(const diseqc_t type);
		void				setLnbOffset(const bool high, const uint8_t index, const int offset);
		int				setParameters(struct dvb_frontend_parameters *feparams, uint8_t polarization, uint8_t diseqc);
};

#endif /* __zapit_frontend_h__ */
