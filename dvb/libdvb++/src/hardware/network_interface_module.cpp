/*
 * $Id: network_interface_module.cpp,v 1.1 2003/07/17 01:07:54 obi Exp $
 *
 * Copyright (C) 2002, 2003 Andreas Oberritter <obi@saftware.de>
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

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

#include <dvb/debug/debug.h>
#include <dvb/hardware/network_interface_module.h>

const nim_lnb_lof_t NetworkInterfaceModule::default_lof = {
	lof_low:     9750000UL,
	lof_high:   10600000UL,
	lof_offset: 11700000UL,
};

NetworkInterfaceModule::NetworkInterfaceModule(const unsigned char adapter, const unsigned char frontend)
{
	char filename[32];
	snprintf(filename, 32, "/dev/dvb/adapter%hu/frontend%hu", adapter, frontend);

	if ((fd = open(filename, O_RDWR|O_NONBLOCK)) == -1)
		DVB_ERROR(filename);

	else if (DVB_FOP(ioctl, FE_GET_INFO, &info) != -1)
		DVB_INFO("Using Frontend \"%s\"", info.name);

	if (info.type == FE_QPSK) {
		sec_type = (nim_sec_type_t) (NIM_SEC_SIMPLE | NIM_SEC_TONEBURST | NIM_SEC_DISEQC_1_0);
		band = NIM_BAND_LOW;
		pol = NIM_POLARIZATION_V;
		repeat_count = 0;

		DVB_FOP(ioctl, FE_SET_TONE, SEC_TONE_OFF);
		DVB_FOP(ioctl, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
	}
}

NetworkInterfaceModule::~NetworkInterfaceModule(void)
{
	if (fd != -1)
		close(fd);
}

uint32_t NetworkInterfaceModule::getBitErrorRate(void) const
{
	uint32_t ber;

	if (DVB_FOP(ioctl, FE_READ_BER, &ber) == -1)
		return 0xFFFFFFFFUL;

	return ber;
}

nim_dvb_type_t NetworkInterfaceModule::getType(void) const
{
	switch (info.type) {
	case FE_QAM:
		return NIM_DVB_C;
	case FE_QPSK:
		return NIM_DVB_S;
	case FE_OFDM:
		return NIM_DVB_T;
	default:
		return NIM_DVB_UNKNOWN;
	}
}

uint16_t NetworkInterfaceModule::getSignalNoiseRatio(void) const
{
	uint16_t snr;

	if (DVB_FOP(ioctl, FE_READ_SNR, &snr) == -1)
		return 0;

	return snr;
}

uint16_t NetworkInterfaceModule::getSignalStrength(void) const
{
	uint16_t ss;

	if (DVB_FOP(ioctl, FE_READ_SIGNAL_STRENGTH, &ss) == -1)
		return 0;

	return ss;
}

nim_status_t NetworkInterfaceModule::getStatus(void) const
{
	fe_status_t fe_status;
	nim_status_t nim_status = NIM_STATUS_UNKNOWN;

	if (DVB_FOP(ioctl, FE_READ_STATUS, &fe_status) == -1)
		return nim_status;

	if (fe_status & FE_HAS_SYNC)
		nim_status = (nim_status_t) (nim_status | NIM_STATUS_SYNC);
	if (fe_status & FE_HAS_LOCK)
		nim_status = (nim_status_t) (nim_status | NIM_STATUS_LOCK);

	return nim_status;
}

void NetworkInterfaceModule::setSec(nim_polarization_t pol, nim_diseqc_addr_t diseqc, nim_band_t band)
{
	struct dvb_diseqc_master_cmd cmd =
	{ {
		0xe0,	/* Command from Master, No reply required, First transmission */
		0x10,
		0x38,	/* Write Port N0 */
		0x00
	}, 4 };

	fe_sec_voltage_t voltage;
	fe_sec_mini_cmd_t mini_cmd;

	if ((diseqc % 2) == 0)
		mini_cmd = SEC_MINI_A;
	else
		mini_cmd = SEC_MINI_B;

	switch (band) {
	case NIM_BAND_LOW:
		cmd.msg[3] |= 0x10;
		break;
	case NIM_BAND_HIGH:
		cmd.msg[3] |= 0x11;
		break;
	default:
		break;
	}

	switch (pol) {
	case NIM_POLARIZATION_V:
		voltage = SEC_VOLTAGE_13;
		cmd.msg[3] |= 0x20;
		break;
	case NIM_POLARIZATION_H:
		voltage = SEC_VOLTAGE_18;
		cmd.msg[3] |= 0x22;
		break;
	default:
		DVB_INFO("invalid polarization");
		return;
	}

	switch (diseqc) {
	case NIM_DISEQC_ADDR1:
		cmd.msg[3] |= 0xC0;
		break;
	case NIM_DISEQC_ADDR2:
		cmd.msg[3] |= 0xC4;
		break;
	case NIM_DISEQC_ADDR3:
		cmd.msg[3] |= 0xC8;
		break;
	case NIM_DISEQC_ADDR4:
		cmd.msg[3] |= 0xCC;
		break;
	default:
		break;
	}

	if (sec_type & NIM_SEC_SIMPLE) {
		/* End of Continous Tone if present */
		if (this->band == NIM_BAND_HIGH)
			DVB_FOP(ioctl, FE_SET_TONE, SEC_TONE_OFF);

		/* Change of Voltage Signaling if required */
		if (this->pol != pol)
			DVB_FOP(ioctl, FE_SET_VOLTAGE, voltage);

		usleep(15000);
	}

	/* Full DiSEqC Message */
	if (sec_type & NIM_SEC_DISEQC_MASK) {
		DVB_FOP(ioctl, FE_DISEQC_SEND_MASTER_CMD, &cmd);
		cmd.msg[0] |= 0x01;	/* Command from Master, No reply required, Repeated transmission */
		for (unsigned int i = 0; i < repeat_count; ++i) {
			usleep(100000);
			DVB_FOP(ioctl, FE_DISEQC_SEND_MASTER_CMD, &cmd);
		}
		usleep(15000);
	}

	/* SA/SB Toneburst */
	if (sec_type & NIM_SEC_TONEBURST) {
		DVB_FOP(ioctl, FE_DISEQC_SEND_BURST, mini_cmd);
		usleep(15000);
	}

	/* Start of Continous Tone if required */
	if ((sec_type & NIM_SEC_SIMPLE) && (band == NIM_BAND_HIGH))
		DVB_FOP(ioctl, FE_SET_TONE, SEC_TONE_ON);

	this->band = band;
	this->pol = pol;
}

bool NetworkInterfaceModule::tune(struct dvb_frontend_parameters *dfp)
{
	struct dvb_frontend_event dfe;
	int i, ret;
	struct pollfd pfd;
	struct timeval tv, tv2;
	int msec;
	const int poll_timeout_ms = 5000;

	// keep valgrind happy
	memset(&dfe, 0, sizeof(struct dvb_frontend_event));

	if (errno != 0)
		errno = 0;

	if (fd != -1)
		while ((errno == 0) || (errno == EOVERFLOW))
			DVB_FOP(ioctl, FE_GET_EVENT, &dfe);

	for (i = 0; i < 2; ++i) {
		if (DVB_FOP(ioctl, FE_SET_FRONTEND, dfp) == -1)
			return false;

		pfd.fd = fd;
		pfd.events = POLLIN;

		msec = poll_timeout_ms;

		do {
			gettimeofday(&tv, NULL);

			if ((ret = poll(&pfd, 1, msec)) <= 0)
				break;

			gettimeofday(&tv2, NULL);

			msec -= ((tv2.tv_sec - tv.tv_sec) * 1000) + ((tv2.tv_usec - tv.tv_usec) / 1000);

			if (pfd.revents & POLLIN) {
				DVB_INFO("event after %d milliseconds", poll_timeout_ms - msec);
				DVB_FOP(ioctl, FE_GET_EVENT, &dfe);
				if (dfe.status & FE_HAS_LOCK) {
					DVB_INFO("FE_HAS_LOCK: freq %u", dfe.parameters.frequency);
					return true;
				}
				else if ((poll_timeout_ms - msec > 3000) && (dfe.status & FE_TIMEDOUT)) {
					DVB_INFO("FE_TIMEDOUT");
					break;
				}
				else {
					if (dfe.status & FE_HAS_SIGNAL)
						DVB_INFO("FE_HAS_SIGNAL");
					if (dfe.status & FE_HAS_CARRIER)
						DVB_INFO("FE_HAS_CARRIER");
					if (dfe.status & FE_HAS_VITERBI)
						DVB_INFO("FE_HAS_VITERBI");
					if (dfe.status & FE_HAS_SYNC)
						DVB_INFO("FE_HAS_SYNC");
					if (dfe.status & FE_TIMEDOUT)
						DVB_INFO("FE_TIMEDOUT");
					if (dfe.status & FE_REINIT)
						DVB_INFO("FE_REINIT");
				}
			}
			else {
				DVB_INFO("pfd[0].revents %d", pfd.revents);
			}

		} while (msec > 0);
	}

	return false;
}

bool NetworkInterfaceModule::tune(Transponder *ts)
{
	if (!ts)
		return false;

	switch (getType()) {
	case NIM_DVB_C:
		if (ts->getType() != Transponder::CABLE)
			return false;
		return tuneQam(ts->getFrequency(), ts->getSymbolRate(), ts->getFecInner(), ts->getModulation());
	case NIM_DVB_S:
		if (ts->getType() != Transponder::SATELLITE)
			return false;
		return tuneQpsk(ts->getFrequency(), ts->getSymbolRate(), ts->getPolarization(), ts->getFecInner());
	case NIM_DVB_T:
		if (ts->getType() != Transponder::TERRESTRIAL)
			return false;
		return false;
	default:
		return false;
	}
}

bool NetworkInterfaceModule::tuneQam(nim_frequency_t freq, nim_symbol_rate_t sr, nim_fec_inner_t fec, nim_modulation_cable_t m)
{
	struct dvb_frontend_parameters dfp = {
		frequency: freq,
		inversion: INVERSION_AUTO,
		u: {
			qam: {
				symbol_rate: sr,
				fec_inner: FEC_AUTO,
				modulation: QAM_AUTO
			}
		}
	};

	switch (fec) {
	case NIM_FEC_I_1_2:
		dfp.u.qam.fec_inner = FEC_1_2;
		break;
	case NIM_FEC_I_2_3:
		dfp.u.qam.fec_inner = FEC_2_3;
		break;
	case NIM_FEC_I_3_4:
		dfp.u.qam.fec_inner = FEC_3_4;
		break;
	case NIM_FEC_I_5_6:
		dfp.u.qam.fec_inner = FEC_5_6;
		break;
	case NIM_FEC_I_7_8:
		dfp.u.qam.fec_inner = FEC_7_8;
		break;
	case NIM_FEC_I_NONE:
		dfp.u.qam.fec_inner = FEC_NONE;
		break;
	default:
		break;
	}

	switch (m) {
	case NIM_MODULATION_C_QAM16:
		dfp.u.qam.modulation = QAM_16;
		break;
	case NIM_MODULATION_C_QAM32:
		dfp.u.qam.modulation = QAM_32;
		break;
	case NIM_MODULATION_C_QAM64:
		dfp.u.qam.modulation = QAM_64;
		break;
	case NIM_MODULATION_C_QAM128:
		dfp.u.qam.modulation = QAM_128;
		break;
	case NIM_MODULATION_C_QAM256:
		dfp.u.qam.modulation = QAM_256;
		break;
	default:
		break;
	}

	return tune(&dfp);
}


bool NetworkInterfaceModule::tuneQpsk(nim_frequency_t freq, nim_symbol_rate_t sr, nim_polarization_t p, nim_fec_inner_t fec, nim_diseqc_addr_t d)
{
	struct dvb_frontend_parameters dfp = {
		frequency: freq,
		inversion: INVERSION_AUTO,
		u: {
			qpsk: {
				symbol_rate: sr,
				fec_inner: FEC_AUTO
			}
		}
	};

	switch (fec) {
	case NIM_FEC_I_1_2:
		dfp.u.qpsk.fec_inner = FEC_1_2;
		break;
	case NIM_FEC_I_2_3:
		dfp.u.qpsk.fec_inner = FEC_2_3;
		break;
	case NIM_FEC_I_3_4:
		dfp.u.qpsk.fec_inner = FEC_3_4;
		break;
	case NIM_FEC_I_5_6:
		dfp.u.qpsk.fec_inner = FEC_5_6;
		break;
	case NIM_FEC_I_7_8:
		dfp.u.qpsk.fec_inner = FEC_7_8;
		break;
	case NIM_FEC_I_NONE:
		dfp.u.qpsk.fec_inner = FEC_NONE;
		break;
	default:
		break;
	}

	if (freq < default_lof.lof_offset) {
		dfp.frequency -= default_lof.lof_low;
		setSec(p, d, NIM_BAND_LOW);
	}
	else {
		dfp.frequency -= default_lof.lof_high;
		setSec(p, d, NIM_BAND_HIGH);
	}

	return tune(&dfp);
}

void NetworkInterfaceModule::setSecType(nim_sec_type_t sec_type, uint8_t repeat_count)
{
	if ((this->sec_type >= NIM_SEC_DISEQC_1_0) && (sec_type < NIM_SEC_DISEQC_1_0));
		/* TODO: diseqc standby */

	if (sec_type < NIM_SEC_DISEQC_1_0)
		repeat_count = 0;

	else if ((sec_type & NIM_SEC_DISEQC_1_MASK) == NIM_SEC_DISEQC_1_0)
		repeat_count = 0;

	else if ((sec_type & NIM_SEC_DISEQC_2_MASK) == NIM_SEC_DISEQC_2_0)
		repeat_count = 0;

	this->repeat_count = repeat_count;

	if ((this->sec_type < NIM_SEC_DISEQC_1_0) && (sec_type >= NIM_SEC_DISEQC_1_0));
		/* TODO: diseqc power on */

	this->sec_type = sec_type;
}

