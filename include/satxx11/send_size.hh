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

#ifndef SATXX11_SEND_SIZE_HH
#define SATXX11_SEND_SIZE_HH

#include <satxx11/clause.hh>

namespace satxx11 {

template<unsigned int max_size>
class send_size {
public:
	template<class Solver>
	send_size(Solver &s)
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
		/* Always share unit clauses (Duh.)*/
		return true;
	}

	template<class Solver>
	bool operator()(Solver &s, clause c)
	{
		return c.size() <= max_size;
	}
};

}

#endif
