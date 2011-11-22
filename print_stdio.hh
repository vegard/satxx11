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

#ifndef PRINT_STDIO_HH
#define PRINT_STDIO_HH

#include "clause.hh"
#include "literal.hh"

class print_stdio {
public:
	unsigned int nr_learnt_clauses;
	unsigned int nr_learnt_literals;
	unsigned int nr_decisions;
	double avg_backtrack_level;
	double avg_clause_length;
	unsigned int nr_restarts;

	void header()
	{
		/* XXX: Dynamically adjust column widths. */
		printf("c  Thread number\n");
		printf("c  |    Number of restarts\n");
		printf("c  |    |   Number of decisions\n");
		printf("c  |    |      |      Number of learnt clauses\n");
		printf("c  |    |      |      |        Number of literals in learnt clauses\n");
		printf("c  |    |      |      |        |      Average backtrack level\n");
		printf("c  |    |      |      |        |      |      Average clause length\n");
		printf("c  |    |      |      |        |      |      |\n");
	}

	template<class Solver>
	print_stdio(Solver &s):
		nr_learnt_clauses(0),
		nr_learnt_literals(0),
		nr_decisions(0),
		avg_backtrack_level(0),
		avg_clause_length(0),
		nr_restarts(0)
	{
		if (s.id == 0)
			header();
	}

	template<class Solver>
	void attach(Solver &s, clause c)
	{
		nr_learnt_clauses += 1;
		nr_learnt_literals += c.size();

		static const double alpha = 0.995;
		avg_clause_length = alpha * avg_clause_length + (1 - alpha) * c.size();
	}

	template<class Solver>
	void detach(Solver &s, clause c)
	{
		nr_learnt_clauses -= 1;
		nr_learnt_literals -= c.size();
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

			if (s.id == 0 && nr_restarts % 10 == 0) {
				printf("c\n");
				header();
			}

			printf("c %2u: %3u %6u %6u %8u %6.2f %6.2f\n",
				s.id,
				nr_restarts, nr_decisions,
				nr_learnt_clauses, nr_learnt_literals,
				avg_backtrack_level, avg_clause_length);
		}
	}
};

#endif
