#include <lib/dvb/si.h>
#include <lib/dvb/cainfo.h>

static t_CASystems CASystems[] =
{
	/*  CaID */
	{0, 0x17}, // betacrypt
	{0, 0x06}, // irdeto
	{0, 0x01}, // irdeto
	{0, 0x05}, // via
	{0, 0x18}, // nagra
	{0, 0x09}, // nds
	{0, 0x0d}, // cryptoworks
	{0, 0x0b}  // conax
};

unsigned int eCAInfo::NumberOfCASystems(void)
{
	return sizeof(CASystems) / sizeof(t_CASystems);
}

t_CASystems *eCAInfo::getPtr2CASystems(void)
{
	return &CASystems[0];
}

void eCAInfo::saveCAIDsInfo(__u8 *data)
{
	unsigned short ia;
	unsigned char descriptor_length = 0;
	unsigned short pmtlen;
	int dpmtlen = 0;
	int pos = 10;
	for (unsigned int i = 0; i < sizeof(CASystems) / sizeof(t_CASystems); i++)
		CASystems[i].status = 0;
	pmtlen = ((data[1] & 0xf) << 8) + data[2] + 3;
	while (pos < pmtlen)
	{
		dpmtlen = ((data[pos] & 0x0f) << 8) | data[pos + 1];
		for (ia = pos + 2; ia < (dpmtlen + pos + 2); ia += descriptor_length + 2)
		{
			descriptor_length = data[ia + 1];
			if (ia < pmtlen - 4)
			if (data[ia] == 0x09 && data[ia + 1] > 0)
			{
				for (unsigned int i = 0; i < sizeof(CASystems) / sizeof(t_CASystems); i++)
				{
					if (data[ia+2] == CASystems[i].caID)
					{
						printf("[CAINFO] ca system %x is active\n", CASystems[i].caID);
						CASystems[i].status = 1;
						break;
					}
				}
			}
		}
		pos += dpmtlen + 5;
	}
}

