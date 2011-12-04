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

/*
 * VSIDS heap implementation
 * Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 * Copyright (c) 2007-2010, Niklas Sorensson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 * OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef DECIDE_VSIDS_HH
#define DECIDE_VSIDS_HH

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <vector>

#include "clause.hh"
#include "literal.hh"

class decide_vsids {
public:
	std::vector<literal> heap;

	/* Indexed by literal */
	std::vector<unsigned int> activities;
	std::vector<unsigned int> positions;

	template<class Solver>
	decide_vsids(Solver &s):
		heap(2 * s.nr_variables),
		activities(2 * s.nr_variables),
		positions(2 * s.nr_variables)
	{
		for (unsigned int i = 0; i < s.nr_variables; ++i) {
			for (bool v: {false, true}) {
				literal l(i, v);
				heap[l] = l;
				activities[l] = 0;
				positions[l] = l;
			}
		}
	}

	~decide_vsids()
	{
	}

	unsigned int parent(unsigned int i)
	{
		return (i - 1) >> 1;
	}

	unsigned int left(unsigned int i)
	{
		return (i << 1) + 1;
	}

	unsigned int right(unsigned int i)
	{
		return (i << 1) + 2;
	}

	void percolate_up(unsigned int i)
	{
		debug_enter("i = $", i);

		literal x = heap[i];
		unsigned int p = parent(i);

		while (i > 0 && activities[x] < activities[heap[p]]) {
			heap[i] = heap[p];
			positions[heap[p]] = i;
			i = p;
			p = parent(i);
		}

		heap[i] = x;
		positions[x] = i;
	}

	void percolate_down(unsigned int i)
	{
		debug_enter("i = $", i);

		unsigned int n = heap.size();

		literal x = heap[i];

		while (true) {
			unsigned int l = left(i);
			if (l >= n)
				break;

			unsigned int child;

			unsigned int r = right(i);
			if (r < n)
				child = activities[heap[l]] < activities[heap[r]] ? r : l;
			else
				child = l;

			if (activities[heap[child]] <= activities[x])
				break;

			heap[i] = heap[child];
			positions[heap[i]] = i;
			i = child;
		}

		heap[i] = x;
		positions[x] = i;
	}

	void bump(literal l)
	{
		debug_enter("literal = $", l);

		++activities[l];
		percolate_up(positions[l]);
	}

	void attach(clause c)
	{
		debug_enter("clause = $", c);

		for (unsigned int i = 0, n = c.size(); i < n; ++i)
			bump(c[i]);
	}

	void detach(clause c)
	{
	}

	template<class Solver, class Propagate>
	literal operator()(Solver &s, Propagate &p)
	{
		debug_enter("");

		literal l;

		/* Since we don't actually remove elements when they get
		 * defined, we need some kind of guarantee that we don't
		 * just try to pick the same element all the time (if all
		 * the elements have the same activity, for example) */
		for (unsigned int i = 0; i < heap.size(); ++i) {
			l = heap[i];
			if (p.defined(l))
				continue;

			activities[l] = 0;
			percolate_down(i);

			debug("picked $", l);
			return l;
		}

		assert(false);
	}
};

#endif
