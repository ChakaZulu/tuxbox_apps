#include <lib/base/eerror.h>
#include <lib/system/econfig.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <sys/stat.h>

eConfig *eConfig::instance;

eConfig::eConfig()
{
	if (!instance)
		instance=this;

	FILE *f = fopen(CONFIGDIR "/enigma/config", "r");
	if (f)
	{
		char buffer[1024];
		while (1)
		{
			if (!fgets(buffer, 1024, f))
				break;
			if (strlen(buffer) < 4)
				break;
			buffer[strlen(buffer)-1]=0;
			char *key = buffer + 2;
			char *opt = strchr(key, '=');
			if (!opt)
				continue;
			*opt++ = 0;
			
			switch(*buffer)
			{
			case 's': keys_string[key] = opt; break;
			case 'u': keys_uint[key] = strtoul(opt, 0, 0x10); break;
			case 'd':
			{
				char *endptr=0;
				keys_double[key] = strtod(opt, &endptr);
				if ( endptr && *endptr )
				{
					if ( *endptr == ',' )
						*endptr = '.';
					else if (*endptr == '.' )
						*endptr = ',';
					endptr=0;
					keys_double[key] = strtod(opt, &endptr);
					if ( endptr && *endptr )
						eDebug("failed to parse %s %s", key, opt);
				}
				break;
			}
			case 'i':
			{
				if ( sscanf(opt, "%x", &keys_int[key] ) != 1 )
				{
					if (sscanf(opt, "%x", &keys_int[key] ) != 1 )
						eDebug("couldn't parse %s", opt);
				}
				break;
			}              
			}
		}
		fclose(f);
	}

	locked=1;
	ppin=0;
	getKey("/elitedvb/pins/parentallock", ppin );
}

eConfig::~eConfig()
{
	flush();
	if (instance==this)
		instance=0;
}

int eConfig::getKey(const char *key, int &i)
{
	std::map<std::string, int>::iterator it = keys_int.find(key);
	if (it == keys_int.end())
		return -1;
	i = it->second;
	return 0;
}

int eConfig::getKey(const char *key, unsigned int &ui)
{
	std::map<std::string, unsigned int>::iterator it = keys_uint.find(key);
	if (it == keys_uint.end())
		return -1;
	ui = it->second;
	return 0;
}

int eConfig::getKey(const char *key, double &d)
{
	std::map<std::string, double>::iterator it = keys_double.find(key);
	if (it == keys_double.end())
		return -1;
	d = it->second;
	return 0;
}

int eConfig::getKey(const char *key, char * &string)
{
	std::map<std::string, std::string>::iterator it = keys_string.find(key);
	if (it == keys_string.end())
		return -1;
	string = strdup(it->second.c_str());
	return 0;
}

int eConfig::setKey(const char *key, const int &i)
{
	keys_int[key] = i;
	return 0;
}

int eConfig::setKey(const char *key, const unsigned int &ui)
{
	keys_uint[key] = ui;
	return 0;
}

int eConfig::setKey(const char *key, const double &d)
{
	keys_double[key] = d;
	return 0;
}

int eConfig::setKey(const char *key, const char *s)
{
	keys_string[key] = s;
	return 0;
}


void eConfig::delKey(const char *key)
{
	keys_int.erase(key);
	keys_string.erase(key);
	keys_uint.erase(key);
	keys_double.erase(key);
}


void eConfig::flush()
{
	FILE *f = fopen(CONFIGDIR "/enigma/config", "w");
	if (!f)
	{
		eWarning("couldn't write config!");
		return;
	}

	for (std::map<std::string, int>::iterator i(keys_int.begin()); i != keys_int.end(); ++i)
		fprintf(f, "i:%s=%08x\n", i->first.c_str(), i->second);
	for (std::map<std::string, unsigned int>::iterator i(keys_uint.begin()); i != keys_uint.end(); ++i)
		fprintf(f, "u:%s=%08x\n", i->first.c_str(), i->second);
	for (std::map<std::string, double>::iterator i(keys_double.begin()); i != keys_double.end(); ++i)
		fprintf(f, "d:%s=%lf\n", i->first.c_str(), i->second);
	for (std::map<std::string, std::string>::iterator i(keys_string.begin()); i != keys_string.end(); ++i)
		fprintf(f, "s:%s=%s\n", i->first.c_str(), i->second.c_str());

	fclose(f);
}

eAutoInitP0<eConfig> init_eRCConfig(eAutoInitNumbers::configuration, "Configuration");
