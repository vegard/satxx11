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

#ifndef SATXX11_SIMPLIFY_FAILED_LITERAL_PROBING_HH
#define SATXX11_SIMPLIFY_FAILED_LITERAL_PROBING_HH

#include <cstdio>

#include <satxx11/clause.hh>
#include <satxx11/literal.hh>

namespace satxx11 {

/* A very simple version of Failed Literal Probing. In short, it's a bit like
 * restarting VERY frequently (after just 1 decision), but without using the
 * usual decision variable heuristic. */
class simplify_failed_literal_probing {
public:
	unsigned int nr_rounds;

	simplify_failed_literal_probing():
		nr_rounds(0)
	{
	}

	template<class Solver>
	unsigned int probe(Solver &s, unsigned int var)
	{
		unsigned int nr_literals = 0;

		for (bool value: {false, true}) {
			if (s.defined(var))
				continue;

			s.stack.decision(s, literal(var, value));
			while (!s.stack.propagate(s)) {
				s.analyze(s);
				++nr_literals;
			}

			if (s.stack.decision_index > 0)
				s.stack.backtrack(s, 0);
		}

		return nr_literals;
	}

	template<class Solver>
	void operator()(Solver &s)
	{
		assert(s.stack.decision_index == 0);

		unsigned int nr_literals = 0;

		if (nr_rounds % 100 == 0) {
			/* Try a random share of the variables. */
			for (unsigned int i = 0, n = s.nr_variables / s.nr_threads; i < n; ++i)
				nr_literals += probe(s, s.random() % s.nr_variables);
		} else {
			/* Try a random one percent of the variables. */
			for (unsigned int i = 0, n = s.nr_variables / 100; i < n; ++i)
				nr_literals += probe(s, s.random() % s.nr_variables);
		}

		/* XXX: Don't abuse printf like this. The other plugins might want to
		 * know about this too. */
		if (nr_literals > 0)
			printf("c Failed literal probing learned %u literals\n", nr_literals);

		++nr_rounds;
	}
};

}

#endif
