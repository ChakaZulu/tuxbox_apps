/*
 * $Id: frontend.h,v 1.31 2007/05/13 20:14:40 houdini Exp $
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
		/* current 22kHz tone mode */
		fe_sec_tone_mode_t currentToneMode;
		/* current satellite position */
		int32_t currentSatellitePosition;
		/* how often to repeat DiSEqC 1.1 commands */
		uint8_t diseqcRepeats;
		/* DiSEqC type of attached hardware */
		diseqc_t diseqcType;
		/* low lnb offsets */
		int32_t lnbOffsetsLow[MAX_LNBS];
		/* high lnb offsets */
		int32_t lnbOffsetsHigh[MAX_LNBS];
		/* current Transponderdata */
		TP_params currentTransponder;
		/* speed up sat and cable search*/
		fe_spectral_inversion_t last_inversion;
		/* speed up cable seach */
		fe_modulation_t last_qam;
		/* selects different software control modes for uncommitted switch */
		int uncommitted_switch_mode;

		uint32_t			getDiseqcReply(const int timeout_ms) const;
		struct dvb_frontend_event	getEvent(void);
		struct dvb_frontend_parameters	getFrontend(void) const;

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
		void				sendUncommittedSwitchesCommand(uint8_t usCommand);
		void				setFrontend(const struct dvb_frontend_parameters *feparams);
		void				setSec(const uint8_t sat_no, const uint8_t pol, const bool high_band, const uint32_t frequency);
		void				reset(void);


	public:
		CFrontend(int _uncomitted_switch_mode = 0);
		~CFrontend(void);

		static fe_code_rate_t		getCodeRate(const uint8_t fec_inner);
		uint8_t				getDiseqcPosition(void) const		{ return currentTransponder.diseqc; }
		uint8_t				getDiseqcRepeats(void) const		{ return diseqcRepeats; }
		diseqc_t			getDiseqcType(void) const		{ return diseqcType; }
		uint32_t			getFrequency(void) const;
		static fe_modulation_t		getModulation(const uint8_t modulation);
		uint8_t				getPolarization(void) const;
		const struct dvb_frontend_info *getInfo(void) const			{ return &info; };

		uint32_t			getBitErrorRate(void) const;
		uint16_t			getSignalNoiseRatio(void) const;
		uint16_t			getSignalStrength(void) const;
		fe_status_t			getStatus(void) const;
		uint32_t			getUncorrectedBlocks(void) const;


		const int32_t 			getCurrentSatellitePosition() { return currentSatellitePosition; }

		void				setDiseqcRepeats(const uint8_t repeats)	{ diseqcRepeats = repeats; }
		void				setDiseqcType(const diseqc_t type);
		void				setLnbOffset(const bool high, const uint8_t index, const int offset);
		int					setParameters(TP_params *TP);
		int					setParameters(struct dvb_frontend_parameters *feparams, const uint8_t polarization = VERTICAL, const uint8_t diseqc = 0);
		const TP_params*	getParameters(void) const			{ return &currentTransponder; };
		struct dvb_frontend_event*	setParametersResponse(TP_params *TP);
		void 				setCurrentSatellitePosition(int32_t satellitePosition) {currentSatellitePosition = satellitePosition; }

		void 				positionMotor(uint8_t motorPosition);
		void				sendMotorCommand(uint8_t cmdtype, uint8_t address, uint8_t command, uint8_t num_parameters, uint8_t parameter1, uint8_t parameter2);
};

#endif /* __zapit_frontend_h__ */
