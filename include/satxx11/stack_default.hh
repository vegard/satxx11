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

#ifndef SATXX11_STACK_DEFAULT_HH
#define SATXX11_STACK_DEFAULT_HH

namespace satxx11 {

class stack_default {
public:
	/* XXX: Use std::unique_ptr<>? */
	unsigned int *trail;
	unsigned int trail_index;
	unsigned int trail_size;

	unsigned int *decisions;
	unsigned int decision_index;

	/* Decision level at which the variable was set. */
	unsigned int *levels;

	template<class Solver>
	stack_default(Solver &s):
		trail(new unsigned int[s.nr_variables]),
		trail_index(0),
		trail_size(0),
		decisions(new unsigned int[s.nr_variables]),
		decision_index(0),
		levels(new unsigned int[s.nr_variables])
	{
	}

	~stack_default()
	{
		delete[] trail;
		delete[] decisions;
		delete[] levels;
	}

	template<class Solver>
	bool complete(Solver &s)
	{
		return trail_index == s.nr_variables;
	}

	template<class Solver>
	void decision(Solver &s, literal lit)
	{
		debug_enter("literal = $", lit);

		assert_hotpath(!s.defined(lit));
		assert_hotpath(trail_index == trail_size);

		unsigned int variable = lit.variable();
		s.assign(lit, true);
		trail[trail_size] = variable;
		decisions[decision_index] = trail_size;
		++trail_size;
		++decision_index;
		levels[variable] = decision_index;
	}

	/* Called whenever a variable is forced to a particular value. The
	 * variable may be defined already, in which case we have a conflict
	 * and this function will return false. */
	template<class Solver>
	void implication(Solver &s, literal lit)
	{
		debug_enter("literal = $", lit);

		assert_hotpath(!s.defined(lit));

		unsigned int variable = lit.variable();
		s.assign(lit, true);
		trail[trail_size] = variable;
		++trail_size;
		levels[variable] = decision_index;
	}

	/* Return false if and only if there was a conflict. */
	template<class Solver>
	bool propagate(Solver &s)
	{
		debug_enter("");

		while (trail_index < trail_size) {
			debug("trail_index = $, trail_size = $", trail_index, trail_size);
			unsigned int var = trail[trail_index++];

			/* If propagation caused a conflict, abort the rest
			 * of the conflicts as well. */
			if (!s.propagate.propagate(s, literal(var, s.value(var))))
				return debug_return(false, "$");
		}

		return debug_return(true, "$");
	}

	template<class Solver>
	void backtrack(Solver &s, unsigned int decision)
	{
		debug_enter("decision = $", decision);

		assert(decision < decision_index);

		/* Unassign the variables that we've assigned since the
		 * given index. */
		unsigned new_trail_size = decisions[decision];
		debug("new trail size = $", new_trail_size);

		/* XXX: It doesn't REALLY matter what order we do this in. */
		for (unsigned int i = trail_size; i-- > new_trail_size; )
			s.unassign(trail[i]);

		trail_index = new_trail_size;
		trail_size = new_trail_size;
		decision_index = decision;
	}
};

}

#endif
