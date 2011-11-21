#ifndef RESTART_FIXED_HH
#define RESTART_FIXED_HH

template<unsigned int n>
class restart_fixed {
public:
	unsigned int counter;

	restart_fixed():
		counter(0)
	{
	}

	bool operator()()
	{
		if (++counter < n)
			return false;

		counter = 0;
		return true;
	}
};

#endif
