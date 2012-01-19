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

#ifndef SATXX11_PROPAGATE_UNARY_CLAUSE_HH
#define SATXX11_PROPAGATE_UNARY_CLAUSE_HH

namespace satxx11 {

class propagate_unary_clause {
public:
	class unary_clause_share {
	public:
		std::vector<literal> literals;

		unary_clause_share()
		{
		}

		template<class Solver, class ClauseType>
		void share(Solver &s, ClauseType c)
		{
		}

		template<class Solver>
		void share(Solver &s, literal lit)
		{
			literals.push_back(lit);
		}

		template<class Solver, class ClauseType>
		void detach(Solver &s, ClauseType c)
		{
		}

		template<class Solver>
		void detach(Solver &s, literal lit)
		{
		}

		template<class Solver>
		bool restart(Solver &s)
		{
			for (literal lit: literals) {
				if (!s.attach(lit))
					return false;
			}

			/* XXX: Do we need to call propagate() after _each_
			 * attached literal, or is this enough? */
			return s.stack.propagate(s);
		}
	};

	typedef unary_clause_share share;

	propagate_unary_clause()
	{
	}

	template<class Solver>
	void start(Solver &s)
	{
	}

	template<class Solver>
	bool attach(Solver &s, literal lit)
	{
		return s.implication(lit);
	}

	template<class Solver, typename ClauseType>
	bool attach(Solver &s, ClauseType c)
	{
		return true;
	}

	/* NOTE: Only use this for clauses attached before starting the
	 * solver threads! */
	template<class Solver>
	bool attach(Solver &s, const std::vector<literal> &v, bool &ok)
	{
		if (v.size() != 1)
			return false;

		literal lit = v[0];

		/* Attach literal in all threads */
		bool all_ok = true;
		for (unsigned int i = 0; i < s.nr_threads; ++i)
			all_ok = all_ok && s.solvers[i]->attach(lit);

		ok = all_ok;
		return true;
	}

	template<class Solver>
	bool attach_learnt(Solver &s, const std::vector<literal> &v, bool &ok)
	{
		if (v.size() != 1)
			return false;

		literal lit = v[0];

		ok = s.attach(lit);
		s.share(lit);
		return true;
	}

	template<class Solver, typename ClauseType>
	void detach(Solver &s, ClauseType c)
	{
	}

	/* Return false if and only if there was a conflict. */
	template<class Solver>
	bool propagate(Solver &s, literal lit)
	{
		return true;
	}
};

}

#endif
