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

#ifndef PLUGIN_BASE_HH
#define PLUGIN_BASE_HH

#include "clause.hh"
#include "literal.hh"

class plugin_base {
public:
	template<class Solver>
	void start(Solver &s)
	{
	}

	template<class Solver>
	void assign(Solver &s, unsigned int variable, bool value)
	{
	}

	template<class Solver>
	void unassign(Solver &s, unsigned int variable)
	{
	}

	template<class Solver>
	void attach(Solver &s, literal lit)
	{
	}

	template<class Solver>
	void attach(Solver &s, clause c)
	{
	}

	template<class Solver>
	void detach(Solver &s, clause c)
	{
	}

	template<class Solver>
	void decision(Solver &s, literal lit)
	{
	}

	template<class Solver>
	void conflict(Solver &s)
	{
	}

	template<class Solver>
	void backtrack(Solver &s, unsigned int decision)
	{
	}

	template<class Solver>
	void sat(Solver &s)
	{
	}

	template<class Solver>
	void unsat(Solver &s)
	{
	}
};

#endif
