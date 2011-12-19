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

#ifndef REDUCE_SIZE_HH
#define REDUCE_SIZE_HH

#include <algorithm>
#include <vector>

#include "clause.hh"

/* Detach clauses based on their size (similar to minisat 2.2.0 heuristic);
 * all clauses smaller than a certain size are kept and approximately half
 * of all other learnt clauses are kept. */
template<unsigned int size>
class reduce_size {
public:
	std::vector<clause> clauses;

	template<class Solver>
	reduce_size(Solver &s)
	{
	}

	template<class Solver>
	void attach(Solver &s, clause c)
	{
		/* Never try to detach non-learnt clauses */
		if (!c.is_learnt())
			return;

		/* Never try to detach short clauses */
		if (c.size() <= size)
			return;

		clauses.push_back(c);
	}

	template<class Solver>
	void detach(Solver &s, clause c)
	{
	}

	template<class Solver>
	void resolve(Solver &s, clause c)
	{
	}

	struct clause_compare {
		bool operator()(clause a, clause b)
		{
			return a.size() < b.size();
		}
	};

	template<class Solver>
	void operator()(Solver &s)
	{
		std::sort(clauses.begin(), clauses.end(), clause_compare());

		auto begin = clauses.begin() + clauses.size() / 2;
		auto end = clauses.end();

		for (auto it = begin; it != end; ++it)
			s.detach(*it);

		clauses.erase(begin, end);
	}
};

#endif
