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

	void attach(clause c)
	{
	}

	void detach(clause c)
	{
	}

	template<class Solver, class Propagate>
	literal operator()(Solver &s, Propagate &p)
	{
		assert(p.trail_index < s.nr_variables);

		/* Find unassigned literal */
		unsigned int variable;

		/* Pick a variable at random */
		do {
			variable = s.random() % s.nr_variables;
		} while (p.defined(variable));

		return literal(variable, s.random() % 2);
	}
};

}

#endif
