#include <lib/base/ebase.h>
#include <lib/base/eerror.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/driver/eavswitch.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <math.h>

class eAudioDynamicCompression: public Object
{
	struct arg_s
	{
		int num;
		int clear;
		unsigned long long dst;
		int dst_n;
	};
	eTimer pollTimer;
	int fd;
	
	int read_rms(int i)
	{
		struct arg_s arg;
		arg.num = i;
		arg.clear = 1;
		if (ioctl(fd, 0, &arg))
			return -1;
		return (int)(sqrt(arg.dst / arg.dst_n) * 100000 / (17794890));
	}
	
	int num_low;
	int hyst_low, hyst_hi;
	
	void doPoll()
	{
		struct arg_s arg;
		
		int sum = read_rms(0) + read_rms(1) + read_rms(4) + read_rms(5);
		
		int result = 0;
		
		if (sum < hyst_low)
		{
			num_low++;
			if (num_low > 3)
				result = +1;
		} else 
			num_low = 0;

		if (sum > hyst_hi)
		{
			num_low = 0;
			result = -1;
		}
		
		eDebug("%d, %d, %d (%d %d)", sum, result, num_low, hyst_low, hyst_hi);
		
		if (result)
			eAVSwitch::getInstance()->changeVolume(0, -result*4);
	}
	static eAudioDynamicCompression *instance;
public:
	eAudioDynamicCompression *getInstance()
	{
		return instance;
	}
	eAudioDynamicCompression(): pollTimer(eApp)
	{
		fd = ::open("/dev/audio", O_RDWR);
		if (fd < 0)
		{
			eWarning("can't open /dev/audio (%m) - disabling audio dynamic compression support.");
			return;
		}
		
		CONNECT(pollTimer.timeout, eAudioDynamicCompression::doPoll);
		pollTimer.start(500, 0);
		
		num_low = 0;
		hyst_low = 8000;
		hyst_hi = 20000;

		instance = this;
	}
	~eAudioDynamicCompression()
	{
		if (fd >= 0)
			::close(fd);
		instance = 0;
	}
};

eAudioDynamicCompression *eAudioDynamicCompression::instance;

eAutoInitP0<eAudioDynamicCompression> init_eAudioDynamicCompression(eAutoInitNumbers::dvb, "eAudioDynamicCompression");
