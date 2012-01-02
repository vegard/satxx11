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

#ifndef SATXX11_VALUATION_COMPACT_HH
#define SATXX11_VALUATION_COMPACT_HH

#include <vector>

#include <satxx11/literal.hh>

namespace satxx11 {

class valuation_compact {
public:
	std::vector<bool> data;

	template<class Solver>
	valuation_compact(Solver &s):
		data(std::vector<bool>(2 * s.nr_variables, false))
	{
	}

	template<class Solver>
	bool defined(Solver &s, unsigned int variable) const
	{
		assert_hotpath(variable < s.nr_variables);
		return data[2 * variable + 0];
	}

	template<class Solver>
	bool defined(Solver &s, literal lit) const
	{
		return defined(s, lit.variable());
	}

	template<class Solver>
	bool value(Solver &s, unsigned int variable) const
	{
		assert_hotpath(variable < s.nr_variables);
		assert_hotpath(defined(s, variable));
		return data[2 * variable + 1];
	}

	template<class Solver>
	bool value(Solver &s, literal lit) const
	{
		return value(s, lit.variable()) == lit.value();
	}

	template<class Solver>
	void assign(Solver &s, unsigned int variable, bool value)
	{
		debug_enter("variable = $, value = $", variable, value);

		assert_hotpath(variable < s.nr_variables);
		assert_hotpath(!defined(s, variable));
		data[2 * variable + 0] = true;
		data[2 * variable + 1] = value;
	}

	template<class Solver>
	void assign(Solver &s, literal lit, bool value)
	{
		assign(s, lit.variable(), lit.value() == value);
	}

	template<class Solver>
	void unassign(Solver &s, unsigned int variable)
	{
		debug_enter("variable = $", variable);

		assert_hotpath(variable < s.nr_variables);
		assert_hotpath(defined(s, variable));
		data[2 * variable + 0] = false;
	}
};

}

#endif
