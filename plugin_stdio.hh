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

#ifndef PLUGIN_STDIO_HH
#define PLUGIN_STDIO_HH

#include <limits>

#include "clause.hh"
#include "literal.hh"
#include "plugin_base.hh"

class plugin_stdio:
	public plugin_base
{
public:
	unsigned int nr_restarts;

	unsigned int nr_learnt_clauses_attached;
	unsigned int nr_learnt_clauses_detached;
	unsigned int nr_decisions;

	double avg_backtrack_level;
	unsigned int min_backtrack_level;
	unsigned int max_backtrack_level;

	double avg_clause_length;
	unsigned int min_clause_length;
	unsigned int max_clause_length;

	unsigned int nr_clause_1;
	unsigned int nr_clause_2;
	unsigned int nr_clause_3;

	void header()
	{
		/* XXX: Dynamically adjust column widths. */
		printf("c  Thread number\n");
		printf("c  |    Number of restarts\n");
		printf("c  |    |   Number of decisions\n");
		printf("c  |    |      |           Number of learnt clauses (attached/detached)\n");
		printf("c  |    |      |           |              Backtrack level (min/avg/max)\n");
		printf("c  |    |      |           |              |            Clause length (min/avg/max)\n");
		printf("c  |    |      |           |              |            |         Number of learnt clauses (size 1/2/3)\n");
		printf("c  |    |      |           |              |            |         |\n");
	}

	plugin_stdio():
		nr_restarts(0),
		nr_clause_1(0),
		nr_clause_2(0),
		nr_clause_3(0)
	{
		init();
	}

	template<class Solver>
	void start(Solver &s)
	{
		if (s.id == 0)
			header();
	}

	void init()
	{
		nr_learnt_clauses_attached = 0;
		nr_learnt_clauses_detached = 0;
		nr_decisions = 0,

		avg_backtrack_level = 0;
		min_backtrack_level = std::numeric_limits<unsigned int>::max();
		max_backtrack_level = std::numeric_limits<unsigned int>::min();

		avg_clause_length = 0;
		min_clause_length = std::numeric_limits<unsigned int>::max();
		max_clause_length = std::numeric_limits<unsigned int>::min();
	}

	void attach(unsigned int n)
	{
		++nr_learnt_clauses_attached;

		static const double alpha = 0.995;
		avg_clause_length = alpha * avg_clause_length + (1 - alpha) * n;

		if (n < min_clause_length)
			min_clause_length = n;
		if (n > max_clause_length)
			max_clause_length = n;
	}

	template<class Solver>
	void attach(Solver &s, literal l)
	{
		attach(1);

		/* XXX: We don't strictly know if it was learnt or not... */
		++nr_clause_1;
	}

	template<class Solver>
	void attach(Solver &s, clause c)
	{
		if (!c.is_learnt())
			return;

		unsigned int n = c.size();
		attach(n);

		if (n == 2)
			++nr_clause_2;
		if (n == 3)
			++nr_clause_3;
	}

	template<class Solver>
	void detach(Solver &s, clause c)
	{
		if (!c.is_learnt())
			return;

		++nr_learnt_clauses_detached;
	}

	template<class Solver>
	void decision(Solver &s, literal lit)
	{
		nr_decisions += 1;
	}

	template<class Solver>
	void backtrack(Solver &s, unsigned int decision)
	{
		static const double alpha = 0.995;
		avg_backtrack_level = alpha * avg_backtrack_level + (1 - alpha) * decision;

		/* A restart, basically. */
		if (decision == 0) {
			nr_restarts += 1;

			if (s.id == 0 && nr_restarts % std::max<unsigned int>(1, 20 / s.nr_threads) == 0) {
				printf("c\n");
				header();
			}

			printf("c %2u: %3u %6u %6u/%-6u %3u/%06.2f/%-3u %2u/%06.2f/%-3u %2u/%2u/%2u\n",
				s.id,
				nr_restarts, nr_decisions,
				nr_learnt_clauses_attached, nr_learnt_clauses_detached,
				min_backtrack_level, avg_backtrack_level, max_backtrack_level,
				min_clause_length, avg_clause_length, max_clause_length,
				nr_clause_1, nr_clause_2, nr_clause_3);

			init();
		} else {
			if (decision < min_backtrack_level)
				min_backtrack_level = decision;
			if (decision > max_backtrack_level)
				max_backtrack_level = decision;
		}
	}
};

#endif
