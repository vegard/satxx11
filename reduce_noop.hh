#ifndef REDUCE_NOOP
#define REDUCE_NOOP

#include "clause.hh"

class reduce_noop {
public:
	template<class Solver>
	reduce_noop(Solver &s)
	{
	}

	template<class Solver>
	void attach(Solver &s, clause c)
	{
	}

	template<class Solver>
	void detach(Solver &s, clause c)
	{
	}

	template<class Solver>
	void operator()(Solver &s)
	{
	}
};

#endif
