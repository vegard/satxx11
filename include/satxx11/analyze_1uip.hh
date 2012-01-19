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

#ifndef SATXX11_ANALYZE_1UIP_HH
#define SATXX11_ANALYZE_1UIP_HH

#include <vector>

#include <satxx11/debug.hh>
#include <satxx11/clause.hh>
#include <satxx11/literal.hh>
#include <satxx11/watch_indices.hh>

namespace satxx11 {

template<class Minimise>
class analyze_1uip {
public:
	Minimise minimise;

	template<class Solver>
	analyze_1uip(Solver &s)
	{
	}

	template<class Solver>
	void operator()(Solver &s)
	{
		/* This algorithm comes from the MiniSat paper:
		 * http://minisat.se/downloads/MiniSat.pdf */

		std::vector<bool> seen(s.nr_variables, false);

		assert(s.stack.decision_index > 0);
		unsigned int trail_index = s.stack.trail_size;

		unsigned int counter = 0;
		std::vector<literal> conflict_clause;

		unsigned int variable;
		std::vector<literal> reason;
		s.conflict_reason.get_literals(reason);

		while (true) {
			/* Generic hook -- but our main intention is to let
			 * the VSIDS heuristic bump the clause activity */
			/* XXX: Ideally, we would also pass some information
			 * like what it is resolved _with_... */
			s.resolve(reason);

			for (literal lit: reason) {
				unsigned int variable = lit.variable();

				if (seen[variable])
					continue;

				seen[variable] = true;
				s.resolve(lit);

				unsigned int level = s.stack.levels[variable];
				if (level == s.stack.decision_index) {
					++counter;
				} else if (level > 0) {
					/* Exclude variables from decision level 0 */
					conflict_clause.push_back(lit);
				}
			}

			do {
				assert_hotpath(trail_index > 0);
				variable = s.stack.trail[--trail_index];
			} while (!seen[variable]);

			assert_hotpath(counter > 0);
			--counter;

			if (counter == 0)
				break;

			/* Get next clause */
			s.reasons[variable].get_literals(reason);
		}

		if (conflict_clause.size() > 0)
			minimise(s, seen, conflict_clause);

		unsigned int new_decision_index = 0;
		for (literal l: conflict_clause) {
			unsigned int level = s.stack.levels[l.variable()];

			if (level > new_decision_index)
				new_decision_index = level;
		}

		literal asserting_literal = ~literal(variable, s.value(variable));
		conflict_clause.push_back(asserting_literal);

		s.backtrack(new_decision_index);

		/* XXX: The old code is faster because it doesn't need to
		 * look for new watches. */
		bool ok = s.attach_learnt(conflict_clause);
		assert(ok);
	}
};

}

#endif
