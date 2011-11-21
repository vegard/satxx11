#ifndef RESTART_OR_HH
#define RESTART_OR_HH

template<class RestartA, class RestartB>
class restart_or {
public:
	RestartA a;
	RestartB b;

	restart_or()
	{
	}

	bool operator()()
	{
		return a() || b();
	}
};

#endif

