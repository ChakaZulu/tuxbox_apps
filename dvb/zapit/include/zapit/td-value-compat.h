/*
 * compatibility stuff for conversion of Tripledragon API values to DVB API
 * and vice versa
 *
 * (C) 2009 Stefan Seyfried
 *
 * Released under the GPL V2.
 */

#ifndef _td_value_compat_
#define _td_value_compat_

static inline unsigned int dvbfec2tdfec(fe_code_rate_t fec)
{
	switch (fec) {
	case FEC_1_2: // FEC_1_2 ... FEC_3_4 are equal to TD_FEC_1_2 ... TD_FEC_3_4
	case FEC_2_3:
	case FEC_3_4:
		return (unsigned int)fec;
	case FEC_5_6:
		return TD_FEC_5_6;
	case FEC_7_8:
		return TD_FEC_7_8;
	default:
		break;
	}
	return TD_FEC_AUTO;
}

static inline fe_code_rate_t tdfec2dvbfec(unsigned int tdfec)
{
	switch (tdfec)
	{
	case TD_FEC_1_2:
	case TD_FEC_2_3:
	case TD_FEC_3_4:
		return (fe_code_rate_t)tdfec;
	case TD_FEC_5_6:
		return FEC_5_6;
	case TD_FEC_7_8:
		return FEC_7_8;
	default:
		break;
	}
	return FEC_AUTO;
}

#endif /* _td_value_compat_ */
