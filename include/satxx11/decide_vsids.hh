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

#ifndef SATXX11_DECIDE_VSIDS_HH
#define SATXX11_DECIDE_VSIDS_HH

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <vector>

#include <satxx11/clause.hh>
#include <satxx11/literal.hh>

namespace satxx11 {

template<unsigned int var_decay>
class decide_vsids {
public:
	unsigned int size;
	std::vector<unsigned int> heap;

	/* Indexed by variable */
	std::vector<bool> contained;
	std::vector<double> activities;
	std::vector<unsigned int> positions;

	double var_inc;

	template<class Solver>
	decide_vsids(Solver &s):
		size(s.nr_variables),
		heap(s.nr_variables),
		contained(s.nr_variables),
		activities(s.nr_variables),
		positions(s.nr_variables),
		var_inc(1)
	{
		for (unsigned int i = 0; i < s.nr_variables; ++i) {
			heap[i] = i;
			contained[i] = true;
			activities[i] = 0;
			positions[i] = i;
		}

		/* Just a way to break ties between multiple threads */
		for (unsigned int i = 0; i < 100; ++i)
			bump(s.random() % s.nr_variables);
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

		unsigned int x = heap[i];
		unsigned int p = parent(i);

		while (i > 0 && activities[x] > activities[heap[p]]) {
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

		unsigned int n = size;

		unsigned int x = heap[i];

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

			if (activities[x] >= activities[heap[child]])
				break;

			heap[i] = heap[child];
			positions[heap[i]] = i;
			i = child;
		}

		heap[i] = x;
		positions[x] = i;
	}

	void bump(unsigned int var)
	{
		debug_enter("variable = $", var);

		activities[var] += var_inc;
		if (activities[var] > 1e100) {
			for (unsigned int i = 0, n = activities.size(); i < n; ++i)
				activities[i] *= 1e-100;

			var_inc *= 1e-100;
		}

		if (contained[var]) {
			percolate_up(positions[var]);
		} else {
			unsigned int i = size++;
			heap[i] = var;

			contained[var] = true;
			positions[var] = i;

			percolate_up(i);
		}
	}

	void bump(literal lit)
	{
		bump(lit.variable());
	}

	template<class Solver>
	void assign(Solver &s, unsigned int variable, bool value)
	{
	}

	template<class Solver>
	void unassign(Solver &s, unsigned int variable)
	{
		if (contained[variable])
			return;

		unsigned int i = size++;
		heap[i] = variable;

		contained[variable] = true;
		positions[variable] = i;

		percolate_up(i);
	}

	template<class Solver>
	void resolve(Solver &s, literal l)
	{
		bump(l);
	}

	template<class Solver>
	void resolve(Solver &s, const std::vector<literal> &v)
	{
		for (literal lit: v)
			bump(lit);
	}

	void attach(clause c)
	{
	}

	void detach(clause c)
	{
	}

	template<class Solver>
	void conflict(Solver &s)
	{
		var_inc *= 1 / (var_decay / 100.);
	}

	template<class Solver>
	unsigned int operator()(Solver &s)
	{
		debug_enter("");

		unsigned int var;

		do {
			assert(size > 0);

			var = heap[0];
			contained[var] = false;
			heap[0] = heap[--size];
			positions[heap[0]] = 0;
			percolate_down(0);
		} while (s.defined(var));

		return var;
	}
};

}

#endif
