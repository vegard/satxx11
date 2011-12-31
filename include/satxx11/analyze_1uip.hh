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

		assert(s.propagate.decision_index > 0);
		unsigned int trail_index = s.propagate.trail_size;

		unsigned int counter = 0;
		std::vector<literal> conflict_clause;

		unsigned int variable;
		clause reason = s.propagate.conflict_reason;

		do {
			assert(reason);
			debug("reason = $", reason);

			/* Generic hook -- but our main intention is to let
			 * the VSIDS heuristic bump the clause activity */
			/* XXX: Ideally, we would also pass some information
			 * like what it is resolved _with_... */
			s.resolve(reason);

			for (unsigned int i = 0; i < reason.size(); ++i) {
				literal lit = reason[i];
				unsigned int variable = lit.variable();

				if (!seen[variable]) {
					seen[variable] = true;
					s.resolve(lit);

					unsigned int level = s.propagate.levels[variable];
					if (level == s.propagate.decision_index) {
						++counter;
					} else if (level > 0) {
						/* Exclude variables from decision level 0 */
						conflict_clause.push_back(lit);
					}
				}
			}

			do {
				assert_hotpath(trail_index > 0);
				variable = s.propagate.trail[--trail_index];
			} while (!seen[variable]);

			reason = s.propagate.reasons[variable];

			assert_hotpath(counter > 0);
			--counter;
		} while (counter > 0);

		literal asserting_literal = ~literal(variable, s.value(variable));

		if (conflict_clause.size() > 0)
			minimise(s, seen, conflict_clause);

		if (conflict_clause.size() == 0) {
			debug("learnt = $", asserting_literal);

			s.backtrack(0);

			/* XXX: This can never fail, so we should not check
			 * whether it does in the hotpath. */
			bool ret = s.implication(asserting_literal, clause());
			assert(ret);

			s.attach(asserting_literal);
			s.share(asserting_literal);
		} else {
			unsigned int new_decision_index = 0;
			for (literal l: conflict_clause) {
				unsigned int level = s.propagate.levels[l.variable()];

				if (level > new_decision_index)
					new_decision_index = level;
			}

			conflict_clause.push_back(asserting_literal);

			clause learnt_clause = s.allocate.allocate(s.nr_threads, s.id, true, conflict_clause);
			debug("learnt = $", learnt_clause);

			s.backtrack(new_decision_index);

			/* Automatically force the opposite polarity for the last
			 * variable (after backtracking its consequences). */
			bool ret = s.implication(asserting_literal, learnt_clause);
			assert(ret);

			/* Attach the newly learnt clause. It will be satisfied by
			 * the implication above. */
			s.attach_with_watches(learnt_clause, learnt_clause.size() - 1, learnt_clause.size() - 2);
			s.share(learnt_clause);
		}
	}
};

}

#endif
