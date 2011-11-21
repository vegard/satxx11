#ifndef RESTART_LUBY_HH
#define RESTART_LUBY_HH

class restart_luby {
private:
	static unsigned int luby(unsigned int i)
	{
		for (unsigned int k = 1; k < 32; ++k) {
			if (i == (1U << k) - 1)
				return 1 << (k - 1);
		}

		for (unsigned int k = 1; ; ++k) {
			if ((1U << (k - 1)) <= i && i < (1U << k) - 1)
				return luby(i - (1 << (k - 1)) + 1);
		}
	}

public:
	unsigned int counter;

	unsigned int value;
	unsigned int max;

	restart_luby():
		counter(0),
		value(1),
		max(luby(1))
	{
	}

	bool operator()()
	{
		if (++counter < max)
			return false;

		counter = 0;
		max = luby(++value);
		return true;
	}
};

#endif

