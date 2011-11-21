#ifndef DECIDE_RANDOM_HH
#define DECIDE_RANDOM_HH

#include "clause.hh"
#include "literal.hh"

class decide_random {
public:
	template<class Solver>
	decide_random(Solver &s)
	{
	}

	void attach(clause c)
	{
	}

	void detatch(clause c)
	{
	}

	template<class Solver, class Propagate>
	literal operator()(Solver &s, Propagate &p)
	{
		/* Find unassigned literal */
		unsigned int variable;

		/* Pick a variable at random */
		do {
			variable = s.rand() % s.nr_variables;
		} while (p.defined(variable));

		return literal(variable, s.rand() % 2);
	}
};

#endif
