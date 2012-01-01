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

#ifndef SATXX11_MINIMISE_MINISAT_HH
#define SATXX11_MINIMISE_MINISAT_HH

#include <stack>
#include <vector>

#include <satxx11/debug.hh>
#include <satxx11/clause.hh>
#include <satxx11/literal.hh>

namespace satxx11 {

/* Recursive minimisation as done in Minisat 2.2.0. */
class minimise_minisat {
public:
	minimise_minisat()
	{
	}

	static uint64_t level_to_abstract_level(unsigned int level)
	{
		return 1UL << (level & 63);
	}

	template<class Solver>
	bool redundant(Solver &s, std::vector<bool> &seen, literal lit, uint64_t abstract_levels)
	{
		debug_enter("");

		std::stack<unsigned int> agenda;
		agenda.push(lit.variable());

		while (!agenda.empty()) {
			unsigned int var = agenda.top();
			agenda.pop();

			std::vector<literal> reason;

			assert_hotpath(s.reasons[var]);
			s.reasons[var].get_literals(reason);

			for (literal lit: reason) {
				unsigned int var = lit.variable();

				if (seen[var])
					continue;

				seen[var] = 1;

				if (s.propagate.levels[var] == 0)
					continue;

				if (!s.reasons[var])
					return false;

				if (!(abstract_levels & level_to_abstract_level(s.propagate.levels[var])))
					return false;

				agenda.push(var);
			}
		}

		/* “If search always ends at marked literals then the candidate
		 * can be removed.” */
		return true;
	}

	template<class Solver>
	void operator()(Solver &s, std::vector<bool> &seen, std::vector<literal> &learnt_clause)
	{
		debug_enter("");

		uint64_t abstract_levels = 0;
		for (literal lit: learnt_clause)
			abstract_levels |= level_to_abstract_level(s.propagate.levels[lit.variable()]);

		std::vector<literal> result;
		for (literal lit: learnt_clause) {
			if (s.reasons[lit.variable()]) {
				/* XXX: This copy may be expensive. */
				std::vector<bool> seen_copy = seen;
				if (redundant(s, seen_copy, lit, abstract_levels))
					continue;
			}

			result.push_back(lit);
		}

		learnt_clause = result;
	}
};

}

#endif
