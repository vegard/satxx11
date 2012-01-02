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

#ifndef SATXX11_RECEIVE_ALL_HH
#define SATXX11_RECEIVE_ALL_HH

#include <satxx11/clause.hh>

namespace satxx11 {

class receive_all {
public:
	template<class Solver>
	receive_all(Solver &s)
	{
	}

	template<class Solver, class ClauseType>
	void attach(Solver &s, ClauseType c)
	{
	}

	template<class Solver, class ClauseType>
	void detach(Solver &s, ClauseType c)
	{
	}

	template<class Solver>
	bool operator()(Solver &s, literal l)
	{
		/* Always receive unit clauses (Duh.)*/
		return true;
	}

	template<class Solver>
	bool operator()(Solver &s, clause c)
	{
		/* Always receive clauses shared by other threads. */
		return true;
	}
};

}

#endif
