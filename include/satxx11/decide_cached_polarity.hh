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

#ifndef SATXX11_DECIDE_CACHED_POLARITY_HH
#define SATXX11_DECIDE_CACHED_POLARITY_HH

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <vector>

#include <satxx11/clause.hh>
#include <satxx11/literal.hh>

namespace satxx11 {

template<class Decide>
class decide_cached_polarity {
public:
	Decide x;
	std::vector<bool> polarities;

	template<class Solver>
	decide_cached_polarity(Solver &s):
		x(s),
		polarities(s.nr_variables)
	{
	}

	~decide_cached_polarity()
	{
	}

	template<class Solver>
	void assign(Solver &s, unsigned int variable, bool value)
	{
		x.assign(s, variable, value);
	}

	template<class Solver>
	void unassign(Solver &s, unsigned int variable)
	{
		x.unassign(s, variable);
	}

	template<class Solver>
	void resolve(Solver &s, clause c)
	{
		x.resolve(s, c);
	}

	void attach(clause c)
	{
		x.attach(c);
	}

	void detach(clause c)
	{
		x.detach(c);
	}

	template<class Solver>
	void conflict(Solver &s)
	{
		x.conflict(s);
	}

	template<class Solver, class Propagate>
	literal operator()(Solver &s, Propagate &p)
	{
		unsigned int var = x(s, p);
		literal lit(var, polarities[var]);
		polarities[var] = !polarities[var];
		return lit;
	}
};

}

#endif
