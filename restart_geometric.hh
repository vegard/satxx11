#ifndef RESTART_GEOMETRIC_HH
#define RESTART_GEOMETRIC_HH

template<unsigned int initial, unsigned int per, unsigned int cent = 100>
class restart_geometric {
public:
	unsigned int counter;
	double value;

	restart_geometric():
		counter(0),
		value(initial)
	{
	}

	bool operator()()
	{
		if (++counter < value)
			return false;

		counter = 0;
		value *= (1 + 1. * per / cent);
		return true;
	}
};

#endif
