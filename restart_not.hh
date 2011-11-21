#ifndef RESTART_NOT_HH
#define RESTART_NOT_HH

template<class Restart>
class restart_not {
public:
	Restart x;

	restart_not()
	{
	}

	bool operator()()
	{
		return !x();
	}
};

#endif
