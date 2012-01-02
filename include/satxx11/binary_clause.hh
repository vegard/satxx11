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

#ifndef SATXX11_BINARY_CLAUSE_HH
#define SATXX11_BINARY_CLAUSE_HH

#include <sstream>

#include <satxx11/literal.hh>

namespace satxx11 {

class binary_clause {
public:
	literal a;
	literal b;

	binary_clause(literal a, literal b):
		a(a),
		b(b)
	{
	}

	void get_literals(std::vector<literal> &v) const
	{
		v.push_back(a);
		v.push_back(b);
	}

	std::string string() const
	{
		std::ostringstream ss;

		ss << a << ", " << b;

		return ss.str();
	}
};

std::ostream &operator<<(std::ostream &os, const binary_clause &c)
{
	return os << c.string();
}

}

#endif
