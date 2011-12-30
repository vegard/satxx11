/*
 * SAT solver
 * Copyright (C) 2011  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SATXX11_DECIDE_RANDOM_HH
#define SATXX11_DECIDE_RANDOM_HH

#include <satxx11/clause.hh>
#include <satxx11/literal.hh>

namespace satxx11 {

class decide_random {
public:
	template<class Solver>
	decide_random(Solver &s)
	{
	}

	template<class Solver>
	void assign(Solver &s, unsigned int variable, bool value)
	{
	}

	template<class Solver>
	void unassign(Solver &s, unsigned int variable)
	{
	}

	template<class Solver>
	void resolve(Solver &s, literal l)
	{
	}

	template<class Solver>
	void resolve(Solver &s, clause c)
	{
	}

	void attach(clause c)
	{
	}

	void detach(clause c)
	{
	}

	template<class Solver>
	void conflict(Solver &s)
	{
	}

	template<class Solver>
	unsigned int operator()(Solver &s)
	{
		assert(s.propagate.trail_index < s.nr_variables);

		/* Find unassigned literal */
		unsigned int variable;

		/* Pick a variable at random */
		do {
			variable = s.random() % s.nr_variables;
		} while (s.defined(variable));

		return variable;
	}
};

}

#endif
