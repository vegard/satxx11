#ifndef RESTART_AND_HH
#define RESTART_AND_HH

template<class RestartA, class RestartB>
class restart_and {
public:
	RestartA a;
	RestartB b;

	restart_and()
	{
	}

	bool operator()()
	{
		return a() && b();
	}
};

#endif
