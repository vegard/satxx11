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

#ifndef ANALYZE_1UIP_HH
#define ANALYZE_1UIP_HH

#include <vector>

#include "debug.hh"
#include "clause.hh"
#include "literal.hh"
#include "watch_indices.hh"

class analyze_1uip {
public:
	template<class Solver>
	analyze_1uip(Solver &s)
	{
	}

	template<class Solver, class Propagate>
	void operator()(Solver &s, Propagate &p)
	{
		/* This algorithm comes from the MiniSat paper:
		 * http://minisat.se/downloads/MiniSat.pdf */

		std::vector<bool> seen(p.nr_variables, false);

		assert(p.decision_index > 0);
		unsigned int trail_index = p.trail_index;

		unsigned int counter = 0;
		std::vector<literal> conflict_clause;
		unsigned int new_decision_index = 0;

		unsigned int variable;
		clause reason = p.conflict_reason;

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

					unsigned int level = p.levels[variable];
					if (level == p.decision_index) {
						++counter;
					} else if (level > 0) {
						/* Exclude variables from decision level 0 */
						conflict_clause.push_back(lit);
						if (level > new_decision_index)
							new_decision_index = level;
					}
				}
			}

			do {
				assert_hotpath(trail_index > 0);
				variable = p.trail[--trail_index];
			} while (!seen[variable]);

			reason = p.reasons[variable];

			assert_hotpath(counter > 0);
			--counter;
		} while (counter > 0);

		literal asserting_literal = ~literal(variable, p.value(variable));

		/* XXX: According to "A case for simple SAT solvers", we
		 * should backtrack to the "next highest decision level" of
		 * the learnt clause's literals. */
		s.backtrack(new_decision_index);

		if (conflict_clause.size() == 0) {
			assert(new_decision_index == 0);

			debug("learnt = $", asserting_literal);

			/* XXX: This can never fail, so we should not check
			 * whether it does in the hotpath. */
			bool ret = p.implication(asserting_literal, clause());
			assert(ret);

			s.share(asserting_literal);
		} else {
			conflict_clause.push_back(asserting_literal);

			clause learnt_clause = s.allocate.allocate(s.nr_threads, s.id, true, conflict_clause);
			debug("learnt = $", learnt_clause);

			/* Automatically force the opposite polarity for the last
			 * variable (after backtracking its consequences). */
			bool ret = p.implication(asserting_literal, learnt_clause);
			assert(ret);

			/* Attach the newly learnt clause. It will be satisfied by
			 * the implication above. */
			s.attach_with_watches(learnt_clause, learnt_clause.size() - 1, learnt_clause.size() - 2);
			s.share(learnt_clause);
		}
	}
};

#endif
