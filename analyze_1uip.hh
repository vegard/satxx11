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
		std::vector<bool> seen(p.nr_variables, false);

		unsigned int trail_index = p.trail_index;

		unsigned int counter = 0;
		std::vector<literal> conflict_clause;
		unsigned int new_decision_index = 0;

		unsigned int variable;
		clause reason = p.conflict_reason;

		do {
			assert(reason);
			debug("reason = $", reason);

			for (unsigned int i = 0; i < reason.size(); ++i) {
				literal lit = reason[i];
				unsigned int variable = lit.variable();

				if (!seen[variable]) {
					seen[variable] = true;

					if (p.levels[variable] == p.decision_index) {
						++counter;
					} else {
						/* XXX: Exclude variables from decision level 0 */
						conflict_clause.push_back(lit);
						if (p.levels[variable] > new_decision_index)
							new_decision_index = p.levels[variable];
					}
				}
			}

			do {
				variable = p.trail[--trail_index];
			} while (!seen[variable]);

			reason = p.reasons[variable];

			--counter;
		} while (counter > 0);

		literal asserting_literal = ~literal(variable, p.value(variable));
		conflict_clause.push_back(asserting_literal);

		/* XXX: Deal with unit facts (1 literal) */
		unsigned int clause_id = *s.clause_counter++;
		clause learnt_clause(clause_id, conflict_clause);

		while (p.watches.size() <= clause_id)
			p.watches.push_back(watch_indices());

		debug("learnt = $", learnt_clause);

		/* XXX: According to "A case for simple SAT solvers", we
		 * should backtrack to the "next highest decision level" of
		 * the learnt clause's literals. */
		s.backtrack(new_decision_index);

		/* Automatically force the opposite polarity for the last
		 * variable (after backtracking its consequences). */
		s.decision(asserting_literal);

		/* Attach the newly learnt clause. It will be satisfied by
		 * the decision above. */
		if (learnt_clause.size() >= 2) {
			s.attach(learnt_clause,
				watch_indices(learnt_clause.size() - 1, learnt_clause.size() - 2));
		}
	}
};

#endif