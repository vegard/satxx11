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

#ifndef SATXX11_PROPAGATE_BINARY_CLAUSE_HH
#define SATXX11_PROPAGATE_BINARY_CLAUSE_HH

#include <vector>

#include <satxx11/binary_clause.hh>
#include <satxx11/erase.hh>
#include <satxx11/literal.hh>

namespace satxx11 {

class propagate_binary_clause {
public:
	class binary_clause_share {
	public:
		std::vector<binary_clause> clauses;

		binary_clause_share()
		{
		}

		template<class Solver, class ClauseType>
		void share(Solver &s, ClauseType c)
		{
		}

		template<class Solver>
		void share(Solver &s, binary_clause c)
		{
			clauses.push_back(c);
		}

		template<class Solver, class ClauseType>
		void detach(Solver &s, ClauseType c)
		{
		}

		template<class Solver>
		void detach(Solver &s, binary_clause c)
		{
		}

		template<class Solver>
		bool restart(Solver &s)
		{
			for (binary_clause c: clauses) {
				if (!s.attach(c))
					return false;

				if (!s.stack.propagate(s))
					return false;
			}

			return true;
		}
	};

	typedef binary_clause_share share;

	/* XXX: Use std::unique_ptr<> */
	/* XXX: Share watchlists with other threads */
	std::vector<literal> *watchlists;

	propagate_binary_clause()
	{
	}

	template<class Solver>
	void start(Solver &s)
	{
		watchlists = new std::vector<literal>[2 * s.nr_variables];
	}

	~propagate_binary_clause()
	{
		delete[] watchlists;
	}

	template<class Solver, typename ClauseType>
	bool attach(Solver &s, ClauseType c)
	{
		return true;
	}

	template<class Solver>
	bool attach(Solver &s, binary_clause c)
	{
		watchlists[~c.a].push_back(c.b);
		watchlists[~c.b].push_back(c.a);
		return true;
	}

	/* NOTE: Only use this for clauses attached before starting the
	 * solver threads! */
	template<class Solver>
	bool attach(Solver &s, const std::vector<literal> &v, bool &ok)
	{
		if (v.size() != 2)
			return false;

		binary_clause c(v[0], v[1]);

		/* Attach clause in all threads */
		bool all_ok = true;
		for (unsigned int i = 0; i < s.nr_threads; ++i)
			all_ok = all_ok && s.solvers[i]->attach(c);

		ok = all_ok;
		return true;
	}

	template<class Solver>
	bool attach_learnt(Solver &s, const std::vector<literal> &v, bool &ok)
	{
		if (v.size() != 2)
			return false;

		binary_clause c(v[0], v[1]);

		/* Attach clause in our own thread, but share it with the other
		 * threads */
		ok = s.attach(c);
		s.share(c);
		return true;
	}

	template<class Solver, typename ClauseType>
	void detach(Solver &s, ClauseType c)
	{
	}

	template<class Solver>
	void detach(Solver &s, binary_clause c)
	{
		erase(watchlists[~c.a], c.b);
		erase(watchlists[~c.b], c.a);
	}

	/* Return false if and only if there was a conflict. */
	template<class Solver>
	bool propagate(Solver &s, literal lit)
	{
		assert_hotpath(s.defined(lit));
		assert_hotpath(s.value(lit));

		/* Go through the watchlist; for each clause in the
		 * watchlist, we need to assign the other literal to true. */
		for (literal other_lit: watchlists[lit]) {
			if (!s.implication(other_lit, binary_clause(~lit, other_lit)))
				return false;
		}

		return true;
	}
};

}

#endif
